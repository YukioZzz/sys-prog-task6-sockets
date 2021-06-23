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
#include <atomic>

#include <sys/wait.h>
#include <signal.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>
#include <map>

#include <mutex>
#include <condition_variable>
#include <algorithm>
#include "util.h"
#include "client_message.pb.h"
#include "server_message.pb.h"


int nb_server_threads   = 2;
int port                = 1025;
constexpr int backlog   = 1024; // how many pending connections the queue will hold


// the atomic number that is increased/decreased according to the received requests
std::atomic<int64_t>    number{0};


// helper function to pass argument threads
struct thread_args {
    std::vector<int> listening_socket;
    std::mutex mtx;
    std::condition_variable cv;
    std::map<int, uint64_t> total_bytes;
};


bool process_request(std::unique_ptr<char[]>& buffer, struct thread_args* ptr, int csock, int buf_size) {
    // TODO:
    // Note: buffer should be serialized into a protobuf object. It might contain more than one (repeated) requests.
    return false;

}

void server(void* args) {
    // TODO:
    // wait until there are connections
    //

    // receive and process requests
}



int main(int args, char* argv[]) {
    if (args < 3) {
        std::cerr << "usage: ./server <nb_server_threads> <port>\n";
    }

    nb_server_threads = std::atoi(argv[1]);
    port = std::atoi(argv[2]);

    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<struct thread_args>> threads_args;

    for (int i = 0; i < nb_server_threads; i++) {
        auto ptr = std::make_unique<struct thread_args>();
        threads_args.emplace_back(std::move(ptr));
        threads.emplace_back(std::thread(server, threads_args.back().get()));
    }


    // create the socket on the listening address

    // wait for incomming connections and assign them the threads

    return 0;
}








