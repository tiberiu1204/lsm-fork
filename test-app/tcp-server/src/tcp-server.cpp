#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERVER_PORT 9999

struct mapping_info {
  char name[16];
  unsigned long pid;
  unsigned long long init_addr;
  unsigned long mapped_addr;
  unsigned long len;
  unsigned long prot;
  unsigned long is_file_backed;
};

// Convert prot number to letters (R/W/X)
void prot_to_letters(unsigned long prot, char out[4]) {
  out[0] = (prot & 0x1) ? 'R' : '-';
  out[1] = (prot & 0x2) ? 'W' : '-';
  out[2] = (prot & 0x4) ? 'X' : '-';
  out[3] = '\0';
}

int main() {
  int sock_server, sock_client;
  struct sockaddr_in addr_server, addr_client;
  socklen_t addrlen;
  ssize_t rb, total_read;

  // Create TCP socket
  sock_server = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_server < 0) {
    perror("socket");
    return 1;
  }

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

    while (1) {
      struct mapping_info info;
      // Read mapping_info struct first
      total_read = 0;
      while (total_read < sizeof(info)) {
        rb = recv(sock_client, ((char *)&info) + total_read,
                  sizeof(info) - total_read, 0);
        if (rb <= 0)
          goto client_disconnected;
        total_read += rb;
      }

      // Allocate buffer for page
      if (info.len == 0)
        continue; // skip empty pages
      char *buf = (char *)malloc(info.len);
      if (!buf) {
        fprintf(stderr, "malloc failed for page of size %lu\n", info.len);
        goto client_disconnected;
      }

      // Read page content
      total_read = 0;
      while (total_read < info.len) {
        rb = recv(sock_client, buf + total_read, info.len - total_read, 0);
        if (rb <= 0) {
          free(buf);
          goto client_disconnected;
        }
        total_read += rb;
      }

      // Convert prot to letters
      char prot_letters[4];
      prot_to_letters(info.prot, prot_letters);

      // Build filename
      char filename[256];
      snprintf(filename, sizeof(filename), "%s_pid_%lu_%llx_%s_%lu.dump",
               info.name, info.pid, info.init_addr, prot_letters,
               info.is_file_backed);

      // Write to file
      FILE *f = fopen(filename, "wb");
      if (!f) {
        perror("fopen");
        free(buf);
        continue;
      }
      fwrite(buf, 1, info.len, f);
      fclose(f);

      printf("Saved page to %s\n", filename);
      free(buf);
    }

  client_disconnected:
    printf("Client disconnected\n");
    close(sock_client);
  }

  close(sock_server);
  return 0;
}
