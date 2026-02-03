#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>

#include "common.h"

int main(int argc, char* argv[]) {
    int ret;

    // variables for handling a socket
    int sock;
    struct sockaddr_in server = {0}; // some fields are required to be filled with 0
    size_t len = sizeof(struct sockaddr_in);

    /** [SOLUTION]
     *  TODO: create a socket for contacting the server
     *
     * Suggestions:
     * - protocollo AF_INET
     * - tipo SOCK_STREAM
     */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        handle_error("Could not create socket");

    if (DEBUG) fprintf(stderr, "Socket created...\n");

    /** [SOLUTION]
     *  TODO: set up parameters for the connection and initiate a connection to the server
     *
     * Suggestions:
     * - set the 3 fields of server:
     * - - server.sin_addr.s_addr: we must specify the server address
     * - - server.sin_family,
     * - - server.sin_port (using htons() method)
     * - initiate a connection to the server
     * - - attention to the bind method:
     * - - it requires as second field struct sockaddr* addr, but our address is a struct sockaddr_in, hence we must cast it (struct sockaddr*) &server
     */
    //
    server.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server.sin_family      = AF_INET;
    server.sin_port        = htons(SERVER_PORT); // don't forget about network byte order!

    char buf[1024] = {0};
    size_t buf_len = sizeof(buf);
    int msg_len;
    char* quit_command = SERVER_COMMAND;
    size_t quit_command_len = strlen(quit_command);

    // main loop
    while (1) {
        printf("Insert your message: ");

        /* Read a line from stdin
         *
         * fgets() reads up to sizeof(buf)-1 bytes and on success
         * returns the first argument passed to it. */
        if (fgets(buf, sizeof(buf), stdin) != (char*)buf) {
            fprintf(stderr, "Error while reading from stdin, exiting...\n");
            exit(EXIT_FAILURE);
        }

        msg_len = strlen(buf);
        // buf[--msg_len] = '\0'; // remove '\n' from the end of the message

		/** [SOLUTION]
         *  TODO: send message to server
         *
         * Suggestions:
         * - send() with flags = 0 is equivalent to write() to a descriptor
         * - the number of bytes to send is msg_len as we have discarded '\n'
         * - deal with partially sent messages
         * - message size IS NOT buf size
         */
        ret = sendto(
            sock,
            buf,
            msg_len,
            0,
            (struct sockaddr*)&server,
            len
        );
        if (ret == -1) handle_error("Cannot write to the socket");

        if (DEBUG) fprintf(stderr, "Sent message of %d bytes...\n", ret);


        /** [SOLUTION]
         *  TODO: After a quit command we won't receive any more data from
         *  the server, thus we must exit the main loop.
         *
         * Suggestions:
         * - compare the number of bytes sent with the length of the
         *   quit command that tells the server to close the connection
         *   (see SERVER_COMMAND macro in common.h)
         * - perform a byte-to-byte comparsion when required using
         *   memcmp(const void *ptr1, const void *ptr2, size_t num)
         * - exit from the cycle when there is nothing left to receive
         */
		if (msg_len == quit_command_len && !memcmp(buf, quit_command, quit_command_len)){

            if (DEBUG) fprintf(stderr, "Sent QUIT command ...\n");
            break;
        }

        /** [SOLUTION]
         *  TODO: read message from server
         * Suggestions:
         * - recv() with flags = 0 is equivalent to read() from a descriptor
         * - for sockets, we get a 0 return value only when the peer closes
         *   the connection: if there are no bytes to read and we invoke
         *   recv() we will get stuck, because the call is blocking!
         * - deal with partially sent messages (we do not know the message size)
         */

        ret = recvfrom(
            sock,
            buf,
            buf_len,
            0,
            (struct sockaddr*)&server,
            (socklen_t*)&len
        );
        if (ret == -1) handle_error("[@client] unable to read");

        if (DEBUG) fprintf(stderr, "Received answer of %d bytes...\n",ret);

        printf("Server response: %s\n", buf); // no need to insert '\0'
    }


    /** [SOLUTION]
     *  TODO: close socket and release unused resources
     */
    ret = close(sock);
    if (ret < 0) handle_error("Cannot close the socket");

    if (DEBUG) fprintf(stderr, "Socket closed...\n");

    if (DEBUG) fprintf(stderr, "Exiting...\n");

    exit(EXIT_SUCCESS);
}
