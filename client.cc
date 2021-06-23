#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <thread>
#include <vector>
#include <iostream>
#include <cstring>
#include <atomic>
#include "util.h"
#include "util_msg.h"
#include "client_message.pb.h"
#include "server_message.pb.h"

bool debugging = false;
int nb_clients  = -1;
int port        = -1;
int nb_messages = -1;
using namespace sockets;

void send_termination_msg(int sockfd, int64_t bytes_sent) {
    // TODO:
    //construct
    client_msg cMsg;
    client_msg_OperationData* opd;
    opd = cMsg.add_ops();
    opd->set_type(client_msg::TERMINATION);
    //send
    
    char buf[128];
    int size = cMsg.ByteSize();
    cMsg.SerializeToArray(buf,size);
    secure_send(sockfd,buf,size);
    //wait for reply

    std::unique_ptr<char[]> smsgbuf;
    int len = secure_recv(sockfd,smsgbuf);
    server_msg sMsg;
    if(sMsg.ParseFromArray(smsgbuf.get(),len)){
       if(sMsg.IsInitialized()){
            int result = sMsg.result();
            //mutex
            fprintf(stdout,"%d\n",result);
        };
    }
}

void client(void* args) {
    // TODO:
    // create socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1){
        perror("socket");
        exit(0);
    }

    // connect to server
    if(debugging)printf("connecting\n");
    struct hostent *he = (struct hostent*)args;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
    int ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret == -1){
        perror("connect");
        exit(0);
    }

    // sending request
    int counter = 0;
    std::unique_ptr<char[]> buf;
    size_t opsize;
    while(counter < nb_messages){
        buf = get_operation(opsize);
        secure_send(fd, buf.get(), opsize);
        ++counter;
    }
    if(debugging)printf("all msg sent out!\n");
    send_termination_msg(fd,0);
    close(fd);
}


int main (int args, char* argv[]) {

    if (args < 5) {
        std::cerr << "usage: ./client <nb_threads> <hostname> <port> <nb_messages>\n";
    }

    nb_clients  = std::atoi(argv[1]);
    port        = std::atoi(argv[3]);
    nb_messages = std::atoi(argv[4]);

    struct hostent *he;
    if ((he = gethostbyname(argv[2])) == NULL) {
        exit(1);
    }

    // creating the client threads
    std::vector<std::thread> threads;

    for (int i = 0; i < nb_clients; i++) {
        threads.emplace_back(std::thread(client, he));
    }

    for (auto& thread : threads) {
        thread.join();
    }
}
