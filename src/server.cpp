#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

#include "def.h"
#include "server.h"
#include "utils.h"
using namespace std;

int server_running = 1;
pthread_mutex_t lock;
pthread_cond_t cond;

class Server {
    public:
        Server();
        ~Server();

        // socket 主线程
        pthread_t main_id;
        // 多线程服务
        int thread_count = 0;
        pthread_t thread_id[MAX_CONNENCTION];

        static void *connectWithClient(void *arg);
        static void *service(void *arg);
        void stop_server();
    
    private:        
        int sockid = -1;
        struct sockaddr_in serverAddr;
};
Server::Server() {
    // Constructor
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);
    
    // create socket
    sockid = socket(AF_INET, SOCK_STREAM, 0);

    int on = 1;
    setsockopt(sockid, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    // set timeout for 1 second
    struct timeval t;
    t.tv_sec = 10;
    t.tv_usec = 0;
    setsockopt(sockid, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));

    Log("Server created with socket id: %d", sockid);
    if (sockid < 0) {
        Err("Socket creation failed");
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SOCKPORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // bind socket to the localhost:port
    if (bind(sockid, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        Err("Binding failed");        
    }
    Log("Server binded to port: %d", SOCKPORT);

    // listen to the port
    if (listen(sockid, MAX_CONNENCTION) < 0) {
        Err("Listening failed");
    }

    Log("Server listening on port: %d", SOCKPORT);


}

Server::~Server() {
    // Destructor  
}

void *Server::connectWithClient(void* arg) {
    
    Server *server = (Server *)arg;
    int clnt_sockid = -1;
    struct sockaddr_in clntAddr;
    socklen_t clntAddrLen = sizeof(clntAddr);
    while (server_running) {
        clnt_sockid = accept(server->sockid, (struct sockaddr *)&clntAddr, &clntAddrLen);
        if (clnt_sockid < 0) {
            if (!server_running) break;
            Log(RED"Accept failed");
            continue;
        }
        Log("Client connected with socket id: %d, client ip: %s, client port: %d", clnt_sockid, inet_ntoa(clntAddr.sin_addr), ntohs(clntAddr.sin_port));

        // 建立服务线程
        
        pthread_t tid;
        int *pclnt_sockid = new int(clnt_sockid);
        pthread_create(&tid, NULL, &service, (void *)pclnt_sockid);
        
        if (server->thread_count < MAX_CONNENCTION) {
            pthread_mutex_lock(&lock);
            Log("Thread count: %d, tid: %u", server->thread_count, tid);
            server->thread_id[server->thread_count++] = tid;        
            pthread_mutex_unlock(&lock);
        } else {
            Err("Thread count exceeds the limit");
        }
    }

    Log("Service thread with client socket id: %d is waiting", clnt_sockid);
    if (server_running) {
        pthread_mutex_lock(&lock);
        pthread_cond_wait(&cond, &lock);
        pthread_mutex_unlock(&lock);
    }

    pthread_exit(nullptr);  
}

void *Server::service(void *arg) {
    int clnt_sockid = *(int *)arg;
    // Log("Service thread created with client socket id: %d", clnt_sockid);

    char head[MAX_BUFFER], *body, ch;
    int i, bodyLen, rc;

    body = (char *)malloc(100);

    while (server_running) {
        Log(CLEAR"Service thread with client socket id: %d is running", clnt_sockid);        
        memset(head, 0, sizeof(head));
        body[0] = '\0';
        i = 0;

        // read Head
        while (server_running) {
            // Log(CLEAR"Reading heading, waiting Readchar");
            rc = readChar(clnt_sockid, &ch);
            if (rc == -1) break; // socket closed

            if (i < sizeof(head) - 1) head[i++] = ch;
            else {
                Err("Header too long");
            }

            if (i > 3 && head[i - 1] == '\n' && head[i - 2] == '\r' && head[i - 3] == '\n' && head[i - 4] == '\r') {
                break;
            }
        }

        Log(CLEAR"rc: %d", rc);
        if (rc == -1) break; // socket closed


        // read Body
        bodyLen = getBodyLenFromHeader(head);

        if (bodyLen > 0) {
            body = (char *)realloc(body, bodyLen+1);
            if (body == NULL) {
                Err("Memory allocation for body failed");
            }

            memset(body, 0, bodyLen+1);
            i = 0;
            while (i < bodyLen) {
                rc = readChar(clnt_sockid, &ch);
                if (rc == -1) break; // socket closed
                body[i++] = ch;
            }

            if (rc == -1) break; // socket closed
        }

        // 处理请求
        handleReq(clnt_sockid, head, body, bodyLen);
        Log(PURPLE"Request handled by client socket id: %d", clnt_sockid);
    }

    // wait until server stops 
    Log(YELLOW"[Timeout] Service thread with client socket id: %d is closed, waiting to exit", clnt_sockid);
    close(clnt_sockid);
    if (server_running) {
        pthread_mutex_lock(&lock);
        pthread_cond_wait(&cond, &lock);
        pthread_mutex_unlock(&lock);
    }

    if (body != NULL) free(body);
    pthread_exit(nullptr);
}

void Server::stop_server() {
    pthread_mutex_lock(&lock);
    server_running = 0;
    pthread_mutex_unlock(&lock);
    pthread_cond_broadcast(&cond);  // 广播通知所有线程退出

    close(sockid);
    Log("Server closed with socket id: %d", sockid);
    for (int i = 0; i < thread_count; i++) {        
        pthread_join(thread_id[i], NULL);
        Log(YELLOW"Joining thread: %d of %d, id: %u", i, thread_count, thread_id[i]);
    }  
}

int main() {
    Server *server = new Server();
    // 创建socket主线程，该线程等待客户端连接
    // server->connectWithClient();
    pthread_create(&server->main_id, NULL, &Server::connectWithClient, (void *)server);
    Log("Main thread created with id: %u", server->main_id);
    server->thread_id[server->thread_count++] = server->main_id;
    // 等待用户输入命令，输入exit时关闭服务器
    while (1) {
        cout << BLUE"Input 'exit' to close the server" << endl;
        string cmd;
        cin >> cmd;
        if (cmd == "exit") {
            Log(CLEAR"Waiting 10 sec for server to stop...");
            server->stop_server();
            break;
        }
    }
    
    
    delete server;
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

    return 0;
}