#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
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

bool debugging = false;
int nb_server_threads   = 2;
int port                = 1025;
constexpr int backlog   = 1024; // how many pending connections the queue will hold
int epfd;
int lfd;

// the atomic number that is increased/decreased according to the received requests
std::atomic<int64_t>    number{0};

using namespace sockets;
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
    client_msg cMsg;

    if(!cMsg.ParseFromArray(buffer.get(),buf_size)){
        if(debugging)printf("parsing failed!\n");
        return false;    
    }else{
        if(debugging)printf("parsing success!\n");
    }
    if(cMsg.IsInitialized()){
        for(int i=0; i<cMsg.ops_size(); i++){
            if(cMsg.ops(i).type()==client_msg::ADD){
                if(debugging)printf("getting cmd add!\n");
                if(cMsg.ops(i).has_argument())
                    number += cMsg.ops(i).argument();
	        }
            else if(cMsg.ops(i).type()==client_msg::SUB){
                if(debugging)printf("getting cmd sub!\n");
                if(cMsg.ops(i).has_argument())
                    number -= cMsg.ops(i).argument();
            }
	        else if(cMsg.ops(i).type()==client_msg::TERMINATION){
                if(debugging)printf("getting cmd termination!\n");
                server_msg smsg;
                int val = number.load(); ///> store the current value;
                smsg.set_result(val);
                printf("%d\n",val);
                std::cout.flush();
                //smsg.set_total_bytes();
                char smsgbuf[128];
                smsg.SerializeToArray(smsgbuf,smsg.ByteSize());
                if(debugging)printf("going to sending back\n");
                secure_send(ptr->listening_socket.back(),smsgbuf,smsg.ByteSize());
                if(debugging)printf("cur state sent!\n");
                //return true;
            }
        }
    }
    return false;
}

void server(void* args) {
    // TODO:
    // wait until there are connections
    //
    if(debugging)printf("server thread initing\n");
    thread_args* m_args = (thread_args*)args;
    while(1){                                               ///> the server threads can't be terminated, so make it in a loop
        std::unique_lock<std::mutex> locker(m_args->mtx);   ///> strange though, it will do lock every loop(RAII)
        while (m_args->listening_socket.empty()){
            m_args->cv.wait(locker);                        ///> wait for productor
        }
        if(debugging)printf("server get notify!\n");
        int curfd = m_args->listening_socket.back();       ///> serve the last one
        
        // receive and process requests
        std::unique_ptr<char[]> buf;
        while(1){
            int len = secure_recv(curfd, buf);
            if(len==0){   ///> disconnected
                if(debugging)printf("disconnecting\n");
                epoll_ctl(epfd, EPOLL_CTL_DEL, curfd, NULL);
                close(curfd);
                break;
            }else if(len>0){
                if(debugging)printf("doing processing upcomming request with len:%d!\n",len);
                process_request(buf,m_args,0,len);
            }else{
                if(debugging)printf("pending!");
                if(errno == EAGAIN){
                    continue;
                }else{
                }
            }
        }
        m_args->listening_socket.pop_back();
        if(debugging)printf(" processing done and remove fd!\n");
        locker.unlock(); ///> release lock
    }
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
    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if(lfd == -1){
        perror("socket");
        exit(0);
    }
    
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(&(addr.sin_zero), 0, 8);

    int yes = 1;
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
        perror("setsockopt");
        exit(1);
    }

    if(bind(lfd, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        perror("bind");
        exit(1);
    }

    // wait for incomming connections and assign them the threads
    if(listen(lfd,backlog) == -1){
        perror("listen");
        exit(1);
    }

    int connection_counter = 0;
    epfd = epoll_create(1);
    struct epoll_event ev;
    ev.events = EPOLLIN; 
    ev.data.fd = lfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);

    struct epoll_event evs[1024];
    int size = sizeof(evs) / sizeof(struct epoll_event);
    while(1){
        int num = epoll_wait(epfd, evs, size, -1);
        for(int i=0;i<num;i++){
            if(debugging)printf("cur num is : %d\n",num);
            int curfd = evs[i].data.fd;
            if(curfd==lfd){
                int cfd = accept(curfd, NULL, NULL);
                if(debugging)printf("accept connection at cfd：%d\n",cfd);
                int flag = fcntl(cfd, F_GETFL);
                flag |= O_NONBLOCK;
                fcntl(cfd, F_SETFL, flag);

                ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT; ///> use EPOLLONESHOT to guarantee only one thread will handle this fd
                ev.data.fd = cfd;
                if(epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev) == -1)
                {
                    perror("epoll_ctl");
                    exit(0);
                }
            }else{
                ++connection_counter;
                if(debugging)printf("hello with curfd: %d\n",curfd);
                int elected_id = connection_counter % nb_server_threads;
                if(debugging)printf("counter:%d & elected_id is %d\n",connection_counter,elected_id);
                thread_args* tptr = threads_args[elected_id].get();
                if(1){
                    tptr->mtx.lock();
                    if(debugging)printf("inserting curfd %d\n",curfd);
                    tptr->listening_socket.push_back(curfd);
                    tptr->cv.notify_one();
                    tptr->mtx.unlock();
                }///> it will be just gone if I use try_lock here with EPOLLONESHOT
            }
        }
    }
    return 0;
}
