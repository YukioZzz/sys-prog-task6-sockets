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


int nb_clients  = -1;
int port        = -1;
int nb_messages = -1;


void send_termination_msg(int sockfd, int64_t bytes_sent) {
    // TODO:
}

void client(void* args) {
    // TODO:
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
