#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 9999

#define TASK_COMM_LEN 16
#define MAX_ARG_LEN 64
#define MAX_ANON_NAME 80

struct mapping_info {
  char name[TASK_COMM_LEN];
  char args[MAX_ARG_LEN];
  char anon_name[MAX_ANON_NAME];
  unsigned long init_addr;
  unsigned long mapped_addr; /* when sending via tcp, will use this as offset */
  unsigned long mapping_len;
  unsigned long len;
  unsigned long prot;
  unsigned long is_end_of_mapping;
};

// Convert prot number to letters (R/W/X)
void prot_to_letters(unsigned long prot, char out[4]) {
  out[0] = (prot & 0x1) ? 'R' : '-';
  out[1] = (prot & 0x2) ? 'W' : '-';
  out[2] = (prot & 0x4) ? 'X' : '-';
  out[3] = '\0';
}

// Sanitize strings to safely use them as directory or file names
// Replaces '/' with '_' and strips out '@' entirely
void sanitize_filename(char *str, size_t max_len) {
  size_t j = 0;
  for (size_t i = 0; i < max_len && str[i] != '\0'; ++i) {
    if (str[i] == '/') {
      str[j++] = '_';
    } else if (str[i] != '@') {
      str[j++] = str[i];
    }
  }
  str[j] = '\0';
}

uint64_t counter = 0;

void dump_mapping(struct mapping_info &info, char *buf) {
  char prot_letters[4];
  prot_to_letters(info.prot, prot_letters);

  // Safely extract and sanitize name (Level 1 Directory)
  char safe_name[TASK_COMM_LEN];
  strncpy(safe_name, info.name, sizeof(safe_name));
  safe_name[sizeof(safe_name) - 1] = '\0';
  sanitize_filename(safe_name, sizeof(safe_name));
  if (strlen(safe_name) == 0) {
    strcpy(safe_name, "unknown_name");
  }

  // Safely extract and sanitize args (Level 2 Directory)
  char safe_args[MAX_ARG_LEN];
  strncpy(safe_args, info.args, sizeof(safe_args));
  safe_args[sizeof(safe_args) - 1] = '\0';
  sanitize_filename(safe_args, sizeof(safe_args));
  if (strlen(safe_args) == 0) {
    strcpy(safe_args, "unknown_args");
  }

  // Safely extract and sanitize anon_name
  char safe_anon[MAX_ANON_NAME];
  strncpy(safe_anon, info.anon_name, sizeof(safe_anon));
  safe_anon[sizeof(safe_anon) - 1] = '\0';
  sanitize_filename(safe_anon, sizeof(safe_anon));
  if (strlen(safe_anon) == 0) {
    strcpy(safe_anon, "noname");
  }

  // 0. Create the root dumps directory
  mkdir("dumps", 0777);

  // 1. Create top-level directory for the name inside dumps
  char level1_dir[256];
  snprintf(level1_dir, sizeof(level1_dir), "dumps/%s", safe_name);
  mkdir(level1_dir, 0777);

  // 2. Build nested directory path and create it for the args
  char nested_dir[512];
  snprintf(nested_dir, sizeof(nested_dir), "dumps/%s/%s", safe_name, safe_args);
  mkdir(nested_dir, 0777);

  // 3. Build filename with .gz extension inside the nested directory
  char filepath[1024];
  snprintf(filepath, sizeof(filepath),
           "dumps/%s/%s/%llu_%lx_%s_%lx_%s_%s.dump.gz", safe_name, safe_args,
           counter++, info.mapping_len, safe_name, info.init_addr, prot_letters,
           safe_anon);

  // Construct shell command to pipe data into gzip
  char command[2048];
  snprintf(command, sizeof(command), "gzip -9 > '%s'", filepath);

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
    printf("Dumped to file: %s\n", filepath);
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
  int ret = send_tcp(sock, buf, 4, 0);
  if (ret < 0) {
    fprintf(stderr, "Error sending bytes: %d (%s)", ret, strerror(errno));
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
    unsigned long current_mapping_size = 0;

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
      if (root_buf == NULL) {
        if (info.mapping_len == 0) {
          fprintf(stderr, "Received mapping_len of 0\n");
          goto client_disconnected;
        }

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
        if (root_buf && (info.mapped_addr + info.len <= current_mapping_size)) {
          buf = root_buf + info.mapped_addr;
          while (total_read < info.len) {
            rb = recv(sock_client, buf + total_read, info.len - total_read, 0);
            if (rb <= 0)
              goto client_disconnected;
            total_read += rb;
          }
        } else {
          fprintf(stderr,
                  "Critical: OOB Write attempted. Offset: %lu, Len: %lu, "
                  "MapSize: %lu\n",
                  info.mapped_addr, info.len, current_mapping_size);
          goto client_disconnected;
        }
      }

      if (info.is_end_of_mapping == 1) {
        dump_mapping(info, root_buf);
        if (analyze_nop(sock_client) < 0) {
          goto client_disconnected;
        }
      }

      munmap(root_buf, current_mapping_size);
      root_buf = NULL;
      current_mapping_size = 0;
    }

  client_disconnected:
    if (root_buf != NULL) {
      munmap(root_buf, current_mapping_size);
    }
    printf("Client disconnected\n");
    close(sock_client);
  }

  close(sock_server);
  return 0;
}
