#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include "util.h"

int32_t
main(int32_t argc, char *argv[])
{
    int32_t     sock_server;    /* server's socket          */
    int32_t     sock_client;    /* accepted client's socket */
    sockaddr_in addr_server;    /* bind address             */
    sockaddr_in addr_client;    /* client's address         */
    socklen_t   addrlen;        /* client's address length  */
    char        buff[1024];     /* data buffer              */
    int32_t     ans;            /* answer                   */
    ssize_t     rb, wb;         /* read / written bytes     */

    /* create tcp socket */
    sock_server = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sock_server == -1, "unable to create tcp socket (%s)", strerror(errno));

    /* bind tcp socket */
    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sin_family      = AF_INET;
    addr_server.sin_port        = htons(9999);
    addr_server.sin_addr.s_addr = INADDR_ANY;

    ans = bind(sock_server, (struct sockaddr *) &addr_server,
               sizeof(addr_server));
    DIE(ans == -1, "unable to bind tcp socket (%s)", strerror(errno));

    /* place socket in listen state */
    ans = listen(sock_server, 10);
    DIE(ans == -1, "unable to listen on tcp socket (%s)", strerror(errno));

    /* main program loop */
    while (1) {
        /* accept new connection */
        addrlen = sizeof(addr_client);
        sock_client = accept(sock_server, (struct sockaddr *) &addr_client,
                             &addrlen);
        DIE(sock_client == -1,
            "unable to accept connectionv (%s)", strerror(errno));

        /* echo read message back */
        rb = recv(sock_client, buff, sizeof(buff), 0);
        DIE(rb == -1, "unable to receive data (%s)", strerror(errno));

        wb = send(sock_client, buff, rb, 0);
        DIE(wb == -1, "unable to send data (%s)", strerror(errno));

        /* terminate client connection before accepting next connection */
        close(sock_client);
    }

    /* close server socket */
    close(sock_server);

    return 0;
}

