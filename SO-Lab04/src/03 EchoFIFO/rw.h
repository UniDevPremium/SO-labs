#ifndef FIFO_UTILS_H
#define FIFO_UTILS_H

/**
 * Reads from a FIFO descriptor one byte at a time until the
 * separator character is found.
 * 
 * @param fd        FIFO file descriptor
 * @param buf       buffer to store the message
 * @param separator character that terminates the message
 * @return number of bytes read
 */
int readOneByOne(int fd, char* buf, char separator);

/**
 * Writes a message through the FIFO descriptor ensuring that
 * all bytes are written.
 * 
 * @param fd   FIFO file descriptor
 * @param buf  buffer containing the message
 * @param size number of bytes to write
 */
void writeMsg(int fd, char* buf, int size);

#endif // FIFO_UTILS_H
