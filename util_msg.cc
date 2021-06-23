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
#include <atomic>
#include "util_msg.h"
#include "client_message.pb.h"
#include "server_message.pb.h"


#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
using namespace sockets;

/**
 * It takes as arguments one char[] array of 4 or bigger size and an integer.
 * It converts the integer into a byte array.
 */
void _convertIntToByteArray(char* dst, int sz) {
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
int _convertByteArrayToInt(char* b) {
    return (b[0] << 24)
        + ((b[1] & 0xFF) << 16)
        + ((b[2] & 0xFF) << 8)
        + (b[3] & 0xFF);
}

std::unique_ptr<char[]> get_operation(size_t& size) {
    static std::atomic<int> i{0};
    
    client_msg cMsg;
    client_msg_OperationData* opd;
    opd = cMsg.add_ops();

    if (i.fetch_add(1) % 2 == 0) {
        // create an ADD request that adds +5
        // that can be either 5 repeated add requests
        // or one add request with 5 as an argument
        //
        opd->set_type(client_msg::ADD);
        opd->set_argument(5);
    }
    else {
        // create a SUB request with 2 as an argument
        // that can be either 2 repeated sub requests
        // or one add request with 2 as an argument
        opd->set_type(client_msg::SUB);
        opd->set_argument(2);    
    }
    
    // Serialize the protobuf object into a char buf and return it.
    size = cMsg.ByteSize();
    std::unique_ptr<char[]> ch = std::make_unique<char[]>(size);
    if(!cMsg.SerializeToArray(ch.get(), size)){
        printf("serializing failed!");
    };
    return ch;
}
