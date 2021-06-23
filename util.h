#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <memory>


/**
 * It takes as arguments one char[] array of 4 or bigger size and an integer.
 * It converts the integer into a byte array.
 */
void convertIntToByteArray(char* dst, int sz) {
    auto tmp = dst;
    tmp[0] = (sz >> 24) & 0xFF;
    tmp[1] = (sz >> 16) & 0xFF;
    tmp[2] = (sz >> 8) & 0xFF;
    tmp[3] = sz & 0xFF;
}

/**
 * It takes as an argument a ptr to an array of size 4 or bigger and 
 * converts the char array into an integer.
 */
int convertByteArrayToInt(char* b) {
    return (b[0] << 24)
        + ((b[1] & 0xFF) << 16)
        + ((b[2] & 0xFF) << 8)
        + (b[3] & 0xFF);
}


/**
 * It constructs the message to be sent. 
 * It takes as arguments a destination char ptr, the payload (data to be sent)
 * and the payload size.
 * It returns the expected message format at dst ptr;
 *
 *  |<---msg size (4 bytes)--->|<---payload (msg size bytes)--->|
 *
 *
 */
void construct_message(char* dst, char* payload, size_t payload_size) {
    // TODO:
    int psize=int(payload_size);
    convertIntToByteArray(dst,psize);
    //how to control the size of dst?
    for(int i=0;i<psize;i++){
        dst[i+4] = payload[i];
    }
}

/**
 * It returns the actual size of msg.
 * Not that msg might not contain all payload data. 
 * The function expects at least that the msg contains the first 4 bytes that
 * indicate the actual size of the payload.
 */
int get_payload_size(char* msg, size_t bytes) {
    // TODO:
    return convertByteArrayToInt(msg);
}


/**
 * Sends to the connection defined by the fd, a message with a payload (data) of size len bytes.
 * The fd should be non-blocking socket.
 */
int secure_send(int fd, char* data, size_t len) {
    // TODO:
    int sentbyte;
    len = len+4;
    auto dst = std::make_unique<char[]>(len);
    construct_message(dst.get(),data,len-4);

    sentbyte = send(fd,dst.get(),len,0);
    int offset = sentbyte;

    while(len-offset){
        sentbyte = send(fd, dst.get()+offset, len-offset,0);
        if(sentbyte==-1)return -1;
        offset += sentbyte;
    }
    return offset-4;
}

/**
 * Receives a message from the fd (non-blocking) and stores it in buf.
 */
int secure_recv(int fd, std::unique_ptr<char[]>& buf) {
    // TODO
    static int cnt;
    ssize_t recvedbyte;
    char header[4];
    while(recv(fd, header, 4,0) == -1)continue;
    int len = convertByteArrayToInt(header);

    buf = std::make_unique<char[]>(len);
    char* ptr = buf.get();
    int count = len;
    while(count>0){
        recvedbyte = recv(fd, ptr, count, 0);
        if(recvedbyte==-1){continue;} ///> for nothing received but still under connection
        if(recvedbyte==0){return 0;}  ///> for disconnection
        ptr += recvedbyte;
        count -= recvedbyte;
    }
    ++cnt;
    return len;
}
