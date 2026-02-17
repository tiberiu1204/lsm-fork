#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 9999

struct mapping_info {
  char name[16];
  unsigned long pid;
  unsigned long long init_addr;
  unsigned long offset;
  unsigned long mapping_len;
  unsigned long len;
  unsigned long prot;
  unsigned long is_file_backed;
  unsigned long is_end_of_mapping;
};

// Convert prot number to letters (R/W/X)
void prot_to_letters(unsigned long prot, char out[4]) {
  out[0] = (prot & 0x1) ? 'R' : '-';
  out[1] = (prot & 0x2) ? 'W' : '-';
  out[2] = (prot & 0x4) ? 'X' : '-';
  out[3] = '\0';
}

uint64_t counter = 0;

void dump_mapping(mapping_info &info, char *buf) {
  // Convert prot to letters
  char prot_letters[4];
  prot_to_letters(info.prot, prot_letters);

  // Build filename with .gz extension
  char filename[256];
  // CHANGED: Moved counter to front, added mapping_len (size) in hex
  snprintf(filename, sizeof(filename),
           "%llu_%lx_%s_pid_%lu_%llx_%s_%lu.dump.gz", counter++,
           info.mapping_len, info.name, info.pid, info.init_addr, prot_letters,
           info.is_file_backed);

  // Construct shell command to pipe data into gzip
  // -9 specifies maximum compression (slowest)
  // We use redirection > to write the compressed stdout to the file
  char command[512];
  snprintf(command, sizeof(command), "gzip -9 > '%s'", filename);

  // Open a pipe to the gzip command
  FILE *f = popen(command, "w");
  if (f == NULL) {
    perror("popen failed");
    return;
  }

  // Write the raw buffer to gzip's stdin
  size_t written = fwrite(buf, 1, info.mapping_len, f);
  if (written != info.mapping_len) {
    fprintf(stderr, "Warning: Only wrote %lu of %lu bytes to compressor\n",
            written, info.mapping_len);
  }

  // Close the pipe (this waits for gzip to finish compression)
  int ret = pclose(f);
  if (ret != 0) {
    fprintf(stderr, "Compression command failed with exit code %d\n", ret);
  } else {
    printf("Dumped to file: %s (Max Compression)\n", filename);
  }
}

int send_tcp(int sock, const void *buf, size_t size, int flags) {
  ssize_t total_sent = 0, sb = 0;
  while (total_sent < size) {
    sb = send(sock, buf, size, 0);
    if (sb <= 0)
      return sb;
    total_sent += sb;
  }
  printf("Sent %d bytes to client\n", size);
  return 0;
}

int analyze_nop(int sock) {
  char buf[4] = {0};
  if (int ret = send_tcp(sock, buf, 4, 0) < 0) {
    fprintf(stderr, "Error sending bytes: %d (%s)", ret, strerror(ret));
    return -1;
  }
  return 0;
}

int main() {
  int sock_server, sock_client;
  struct sockaddr_in addr_server, addr_client;
  socklen_t addrlen;
  ssize_t rb, total_read;

  sock_server = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_server < 0) {
    perror("socket");
    return 1;
  }

  int opt = 1;
  setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  memset(&addr_server, 0, sizeof(addr_server));
  addr_server.sin_family = AF_INET;
  addr_server.sin_port = htons(SERVER_PORT);
  addr_server.sin_addr.s_addr = INADDR_ANY;

  if (bind(sock_server, (struct sockaddr *)&addr_server, sizeof(addr_server)) <
      0) {
    perror("bind");
    return 1;
  }

  if (listen(sock_server, 10) < 0) {
    perror("listen");
    return 1;
  }

  printf("Server listening on port %d...\n", SERVER_PORT);

  while (1) {
    addrlen = sizeof(addr_client);
    sock_client =
        accept(sock_server, (struct sockaddr *)&addr_client, &addrlen);
    if (sock_client < 0) {
      perror("accept");
      continue;
    }

    printf("Client connected\n");

    char *buf = NULL;
    char *root_buf = NULL;
    unsigned long current_mapping_size = 0; // <--- ADDED: Track current size

    while (1) {
      struct mapping_info info;
      total_read = 0;
      while (total_read < sizeof(info)) {
        rb = recv(sock_client, ((char *)&info) + total_read,
                  sizeof(info) - total_read, 0);
        if (rb <= 0)
          goto client_disconnected;
        total_read += rb;
      }

      // Allocate buffer for new mapping
      if (info.offset == 0) {
        if (root_buf != NULL) {
          // FIX: Use the stored size of the OLD buffer, not the info of the NEW
          // one
          munmap(root_buf, current_mapping_size);
        }

        // Update the tracker to the NEW size
        current_mapping_size = info.mapping_len;

        buf = (char *)mmap(NULL, info.mapping_len, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (buf == MAP_FAILED) {
          fprintf(stderr, "mmap failed: %d (%s)\n", errno, strerror(errno));
          goto client_disconnected;
        }
        root_buf = buf;
      }

      if (info.len > 0) {
        total_read = 0;
        // Safety check to ensure we don't write out of bounds
        if (root_buf && (info.offset + info.len <= current_mapping_size)) {
          buf = root_buf + info.offset;
          while (total_read < info.len) {
            rb = recv(sock_client, buf + total_read, info.len - total_read, 0);
            if (rb <= 0)
              goto client_disconnected;
            total_read += rb;
          }
        } else {
          // If we get here, the client sent bad offsets or logic is out of sync
          fprintf(stderr,
                  "Critical: OOB Write attempted. Offset: %lu, Len: %lu, "
                  "MapSize: %lu\n",
                  info.offset, info.len, current_mapping_size);
          goto client_disconnected;
        }
      }

      if (info.is_end_of_mapping == 1) {
        dump_mapping(info, root_buf);
        if (analyze_nop(sock_client) < 0) {
          goto client_disconnected;
        }
      }
    }

  client_disconnected:
    // Cleanup if disconnected mid-processing
    if (root_buf != NULL) {
      munmap(root_buf, current_mapping_size);
    }
    printf("Client disconnected\n");
    close(sock_client);
  }

  close(sock_server);
  return 0;
}
