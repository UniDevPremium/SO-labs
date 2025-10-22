#include <unistd.h>
#include <errno.h>
#include "common.h"


int readOneByOne(int fd, char* buf, char separator) {

    int ret;

    /** [TO DO] READ THE MESSAGE THROUGH THE FIFO DESCRIPTOR
     *
     * Suggestions:
     * - you can read from a FIFO as from a regular file descriptor
     * - since you don't know the length of the message, just
     *   read one byte at a time from the socket
     * - leave the cycle when 'separator' ('\n') is encountered 
     * - repeat the read() when interrupted before reading any data
     * - return the number of bytes read
     * - reading 0 bytes means that the other process has closed
     *   the FIFO unexpectedly: this is an error that should be
     *   dealt with!
     **/

    if(fd < 0) handle_error("fifo not opened");

    int read_bytes = 0, found = 0;

    while(found == 0){
        ret = read(fd, buf + read_bytes, 1);

        if(ret == 0) break;

        if(ret == -1){
            if(errno == EINTR) continue;
            handle_error("read");
        }

        if(buf[read_bytes] == separator) found = 1;
        
        read_bytes++;
    }

    return read_bytes;

}

void writeMsg(int fd, char* buf, int size) {

    int ret;
    /** [TO DO] SEND THE MESSAGE THROUGH THE FIFO DESCRIPTOR
     *
     * Suggestions:
     * - you can write on the FIFO as on a regular file descriptor
     * - make sure that all the bytes have been written: use a while
     *   cycle in the implementation as we did for file descriptors!
     **/

    if(fd == 0) handle_error("fifo not opened");

    int written_bytes = 0, bytes_left = size;
    while(bytes_left > 0){
        ret = write(fd, buf + written_bytes, bytes_left);

        if(ret == -1){
            if(errno == EINTR) continue;
            handle_error("write");
        }

        bytes_left -= ret;
        written_bytes += ret;
    }

}
