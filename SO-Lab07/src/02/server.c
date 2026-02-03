#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>

#include "common.h"

// Method for processing incoming requests. The method takes as argument
// the socket descriptor for the incoming connection.
void* connection_handler(int sock) {

    int ret, n;

    struct sockaddr_in client = {0};
    socklen_t len = sizeof(client);

    char buf[1024] = {0};
    size_t buf_len = sizeof(buf);

    char* quit_command = SERVER_COMMAND;
    size_t quit_command_len = strlen(quit_command);

    while(1){

        ret = recvfrom(
            sock,
            (void*)&buf,
            buf_len,
            0,
            (struct sockaddr*)&client,
            &len
        );
        if (ret == -1) handle_error("[@server] unable to recvfrom");
        
        n = ret;
        if (DEBUG) fprintf(stderr, "Received command of %d bytes...\n",n);

        if (n == quit_command_len && (memcmp((const void*)buf, (const void*)quit_command, n) == 0)){
            if (DEBUG) fprintf(stderr, "Received QUIT command...\n");
            break;
        }

        ret = sendto(
            sock,
            (const void*)&buf,
            n,
            0,
            (const struct sockaddr*)&client,
            len
        );
        if (ret == -1) handle_error("[@server] unable to sendto");

        if (DEBUG) fprintf(stderr, "Sent message of %d bytes back...\n", ret);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    int sock, ret;
    // some fields are required to be filled with 0
    struct sockaddr_in server = {0};


    /** [SOLUTION]
     *  TODO: Create a socket for listening
     *
     * Suggestions:
     * - protocollo AF_INET
     * - tipo SOCK_STREAM
     */
    sock = socket(AF_INET , SOCK_DGRAM , 0);
    if (sock < 0)
        handle_error("Could not create socket");

    if (DEBUG) fprintf(stderr, "Socket created...\n");

    /* We enable SO_REUSEADDR to quickly restart our server after a crash:
     * for more details, read about the TIME_WAIT state in the TCP protocol */
    int reuseaddr_opt = 1;
    ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    if (ret < 0)
        handle_error("Cannot set SO_REUSEADDR option");

    /** [SOLUTION]
     *  TODO: set server address and bind it to the socket
     *
     * Suggestions:
     * - set the 3 fields of server:
     * - - server_addr.sin_addr.s_addr: we want to accept connections from any interface
     * - - server_addr.sin_family,
     * - - server_addr.sin_port (using htons() method)
     * - bind address to socket
     * - - attention to the bind method:
     * - - it requires as second field struct sockaddr* addr, but our address is a struct sockaddr_in, hence we must cast it (struct sockaddr*) &server_addr
     */
    server.sin_addr.s_addr = INADDR_ANY; // we want to accept connections from any interface
    server.sin_family      = AF_INET;
    server.sin_port        = htons(SERVER_PORT); // don't forget about network byte order!

    ret = bind(
        sock,
        (struct sockaddr*)&server,
        sizeof(server)
    );
    if (ret < 0)
        handle_error("Cannot bind address to socket");

    if (DEBUG) fprintf(stderr, "Binded address to socket...\n");

    // loop to handle incoming connections (sequentially)
    while(1){
        connection_handler(sock);
    }
    
    if (DEBUG) fprintf(stderr, "Done!\n");

    exit(EXIT_SUCCESS); // this will never be executed
}
