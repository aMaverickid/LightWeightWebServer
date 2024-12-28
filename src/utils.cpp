#include "utils.h"
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "fastcgi/fastcgi.h"

int fcgi_sock; // fastcgi socket
int fcgi_connected; // fastcgi connection status
extern u_short fcgi_reqID; // fastcgi request ID
NV_PAIR params[100];
int num; // number of parameters

// typedef struct 
// {
//     /* data */
//     int stat;
//     int cs;
//     time_t start;
//     int timeout;
// } TIMER;
// TIMER Timers[100];


static char sock_buff[BUFFER_SIZE];
static int sock_idx, sock_max;
int readChar(int sock, char *ch)
{
    if (sock_idx >= sock_max) {  // buff is empty, reload
        int rc = recv(sock, sock_buff, sizeof(sock_buff), 0);
         if (rc <= 0) return -1;
        sock_max = rc;
        sock_idx = 0;
    }

    *ch = sock_buff[sock_idx++];
    return 0;
}


int getBodyLenFromHeader(char *head)
{    
    char *p = strstr(head, "Content-Length: ");
    if (p == NULL) {
        Log(YELLOW "No Content-Length in header, it's a GET request");
        return 0;
    }
    p += strlen("Content-Length: ");
    int len = 0;
    while (*p != '\r' && *p >= '0' && *p <= '9') {
        len = len * 10 + (*p - '0');
        p++;
    }
    return len;
}
/**
 * 
 */
void handleReq(int sock, char *head, char *body, int bodyLen)
{
    // 读取请求方法（method），如果不是GET或POST，返回405
    char method[16], uri[256], query[256];
    memset(method, 0, sizeof(method));
    memset(uri, 0, sizeof(uri));
    memset(query, 0, sizeof(query));

    sscanf(head, "%15s %255s", method, uri);

    // 检查 URI是否包含查询参数
    char *qmark = strchr(uri, '?');
    if (qmark != NULL) {
        *qmark = '\0';
        strcpy(query, qmark+1);
    }

    // 如果该请求是静态资源，按照路径读取文件，然后发回HTTP响应
    Log(YELLOW"Request: %s %s", method, uri);
    // 如果请求路径为根路径，映射到默认首页
    if (strcmp(uri, "/") == 0) {
        strcpy(uri, "/index.html"); // 将根路径映射到默认首页
    }
    if (!strstr(uri, ".php")) {
        // 处理静态资源请求
        char filePath[512];
        Log(YELLOW"original uri: %s", uri);
        transFileName(uri);
        Log(YELLOW"now uri: %s", uri);
        snprintf(filePath, sizeof(filePath), "%s", uri); // 文件路径，假设当前目录为根目录

        Log(YELLOW"Request for static file: %s", filePath);
        int fd = open(filePath, O_RDONLY);
        if (fd < 0) {
            // 文件未找到
            Log(RED"File not found: %s", filePath);
            sendHttpResp(sock, 404, "Not Found", strlen("Not Found"), "text/plain");
            return;
        }

        struct stat fileStat;
        if (fstat(fd, &fileStat) < 0) {
            close(fd);
            sendHttpResp(sock, 500, "Internal Server Error", strlen("Internal Server Error"), "text/plain");
            return;
        }

        char *fileContent = (char *) malloc(fileStat.st_size);
        if (!fileContent) {
            close(fd);
            sendHttpResp(sock, 500, "Internal Server Error", strlen("Internal Server Error"), "text/plain");
            return;
        }

        read(fd, fileContent, fileStat.st_size);
        close(fd);

        // 根据文件类型设置 Content-Type
        char *contentType = "text/plain";
        if (strstr(uri, ".html")) contentType = "text/html";
        else if (strstr(uri, ".css")) contentType = "text/css";
        else if (strstr(uri, ".js")) contentType = "application/javascript";
        else if (strstr(uri, ".png")) contentType = "image/png";
        else if (strstr(uri, ".jpg")) contentType = "image/jpeg";
        else if (strstr(uri, ".txt")) contentType = "text/plain";
        else if (strstr(uri, ".pdf")) contentType = "application/pdf";

        Log("Send file: %s, size: %ld, content type: %s", filePath, fileStat.st_size, contentType);
        sendHttpResp(sock, 200, fileContent, fileStat.st_size, contentType);
        free(fileContent);
    } 
    else {
        // 如果请求路径不是静态资源，转发到FastCGI服务器
        char *buf = (char *)malloc(1024);
        char *contextType = (char *)malloc(1024);
        // set as 'text/html' by default
        char filePath[512];
        Log(YELLOW"original uri: %s", uri);
        transFileName(uri);        
        strcpy(contextType, "text/html");
        int len = 0;
        int rc = sendFastCGIRequest(method, uri, query, (u_char *)body, bodyLen);
        if (rc == 504) {
            sendHttpResp(sock, rc, "Gateway Timeout", strlen("Gateway Timeout"), "text/plain");
        }
        else if (rc == 0) {
            rc = getFastCGIResponse(&buf, &len, contextType);
            if (rc == 504) {
                sendHttpResp(sock, rc, "Gateway Timeout", strlen("Gateway Timeout"), "text/plain");
            }
            else if (rc == 502) {
                sendHttpResp(sock, rc, "Bad Gateway", strlen("Bad Gateway"), "text/plain");
            }
            else if (rc == 500) {
                sendHttpResp(sock, rc, "Internal Server Error", strlen("Internal Server Error"), "text/plain");
            }
            else if (rc == 404) {
                sendHttpResp(sock, rc, "File Not Found", strlen("File Not Found"), "text/plain");
            }
            else {
                sendHttpResp(sock, rc, buf, len, contextType);
            }
        }
        free(buf);
        free(contextType);
    }
}

void transFileName(char *uri) {
    char buff[256] = {};
    // 动态资源使用绝对路径
    if (strstr(uri, ".php")) {
        // C:\Users\Wan Zhenjie\OneDrive\24Fall\计网\lab\lab8\轻量级WEB服务器\2682\server
        strcpy(buff, "/home/wzj/Coding/LightWeightWebServer/2682/server");
        strcat(buff, uri);
        strcpy(uri, buff);
    } else {
        strcpy(buff, "../2682");
        strcat(buff, uri);
        strcpy(uri, buff);
    }
}

int sendHttpResp(int sock, int statCode, char *resp, int len, char * content_type)
{
    int rc;
    char * buff = (char *)malloc(len + 1000), *p;
    const char * statMesg;

    if (statCode == 200) statMesg = "OK";
    else if (statCode == 403) statMesg = "Forbidden";
    else if (statCode == 404) statMesg = "Not Found";
    else if (statCode == 405) statMesg = "Not Support";
    else if (statCode == 500) statMesg = "Internal Server Error";
    else if (statCode == 502) statMesg = "Bad Gateway";
    else if (statCode == 504) statMesg = "Gateway Timeout";
    else statMesg = "Unknown Error";

    memset(buff, 0, len + 1000);
    p = buff;

    // 添加状态栏
    sprintf(p, "HTTP/1.1 %d %s\r\n", statCode, statMesg);
    p += strlen(p);
    
    // 添加头部：服务器信息
    sprintf(p, "Server: MyWebServer2682\r\n");
    p += strlen(p);

    // 添加头部：体部长度
    sprintf(p, "Content-Length: %d\r\n", len);
    p += strlen(p);

    if (content_type != NULL) {
        sprintf(p, "Content-Type: %s\r\n", content_type);
        p += strlen(p);
    }

    // 添加头部：是否保持连接
    sprintf(p, "Connection: keep-alive\r\n");
    p += strlen(p);


    int head_len = strlen(buff);

    // 添加头部结束符
    sprintf(p, "\r\n");
    p += strlen(p);
    
    // Log("Response: %s", buff);
    // 添加体部（可能是非ASCII码）
    if (len > 0) memcpy(p, resp, len);
    
    rc = send(sock, buff, head_len+len+2, 0);
    free(buff);
    return rc;
}

int sendFastCGIRequest(char *method, char *uri, char *query, u_char *post, int postLen) {

    // 检查网关连接
    if (fcgi_connected != 1) {
        Log(YELLOW"Connecting to FastCGI gateway\n");        
        if (connect_to_fcgi_gateway("127.0.0.1", 9000) < 0) {
            Log(RED"Failed to connect to FastCGI gateway");
            return 504;
        }        
    }
    
    num = 0;
    addParam("REQUEST_METHOD", method); // 请求方法
    addParam("SCRIPT_FILENAME", uri); // 脚本文件名
    addParam("CONTENT_TYPE","application/x-www-form-urlencoded"); // 内容类型
    if (strcmp(method, "GET") == 0) {
        addParam("CONTENT_LENGTH", "0"); // 内容长度        
    } else {
        char dataLen[10];
        sprintf(dataLen, "%d", postLen);
        Log("Post data length: %s", dataLen);
        addParam("CONTENT_LENGTH", dataLen); // 内容长度
    }
    addParam("QUERY_STRING", query); // 查询字符串
    addParam("REMOTE_ADDR", "127.0.0.1"); // 客户端地址
    addParam("REMOTE_PORT", "2682"); // 客户端端口

    Log("Sending FastCGI request: %s %s", method, uri);
    Log(YELLOW"Post data: %s", post);
    fcgi_reqID++;
    if (fcgi_begin_request(fcgi_sock, fcgi_reqID) < 0) return 504;
    if (fcgi_params(fcgi_sock, fcgi_reqID, params, num) < 0) return 504;
    if (fcgi_param_end(fcgi_sock, fcgi_reqID) < 0) return 504;
    if (fcgi_stdin(fcgi_sock, fcgi_reqID, post, postLen) < 0) return 504;

    return 0;
}

int getFastCGIResponse(char **buf, int *len, char *content_type) {
    int rc = 200;

    u_char *std_out, *std_err;
    size_t out_len, err_len;
    int app_stat, proto_stat;
    if (fcgi_getResp(fcgi_sock, &std_out, &out_len, &std_err, &err_len, &app_stat, &proto_stat) < 0) {
        Log(RED"Failed to get FastCGI response");
        disconnectFCGI();
        rc = 504;
    }

    if (proto_stat != 0 || app_stat != 0) {
        Log(RED"FastCGI response error: app_stat=%d, proto_stat=%d", app_stat, proto_stat);
        rc = 502;
    }

    if (err_len > 0) {
        Log(RED"\n***std_err(len=%lu):\n\n%s", err_len, std_err);
    }
    if (out_len > 0) {
        Log(YELLOW"\n***std_out(len=%lu):\n\n%s", out_len, std_out);

        char head[1024];
        char *p = (char *)std_out;
        int i = 0;
        // Log("FastCGI response content type: %s", content_type);
        memset(head, 0, sizeof(head));
        while (*p != '\0')
        {
            if (i >= 3 && *p == '\n' && *(p-1) == '\r' && *(p-2) == '\n' && *(p-3) == '\r')
                break;
            head[i++] = *p++;
        }
        if (*p == '\n') p++;        
        getContentTypeFromHeader(head, content_type);
        Log(PURPLE"FastCGI response header: %s", head);
        if (content_type[0] == '\0') {            
            strcmp(content_type, "text/html");
        }
        
        getStatusFromHeader(head, &rc);
        Log("FastCGI response status: %d", rc);
        if (rc != 404 && rc!=504 && rc!=502) {
            *len = strlen((char *)std_out) -i;
            *buf = (char *)realloc(*buf, *len);
            if (*buf == NULL) rc = 500;
            else memcpy(*buf, p, *len);
        }

    }
    Log("FastCGI response status: %d", rc);
    return rc;
}


int connect_to_fcgi_gateway(char ip[], u_short port) {
    if (fcgi_sock <= 0) {
        fcgi_sock = fcgi_init_socket();
    }

    if (fcgi_connect(fcgi_sock, ip, port) < 0) {
        Log(RED"Failed to connect to FastCGI gateway");
        return -1;
    }

    fcgi_connected = 1;
    Log("Connected to FastCGI gateway");
    return 0;
}

void disconnectFCGI() {
    if (fcgi_sock > 0) {
        close(fcgi_sock);
        fcgi_sock = 0;
        fcgi_connected = 0;
    }
}

void addParam(const char *name, const char *value)
{
	strcpy(params[num].name, name);
	strcpy(params[num].value, value);
	num ++;
}

void getContentTypeFromHeader(char *head, char *content_type)
{
    char *p = strstr(head, "Content-type: ");
    if (p == NULL) {   
        Log(YELLOW "No Content-type in header");
        return;             
    }
    p += strlen("Content-type: ");
    int i = 0;
    while (*p != '\r' && *p != '\n') {
        content_type[i++] = *p++;
    }    
    content_type[i] = '\0';
}

void getStatusFromHeader(char *head, int *status)
{
    char *p = strstr(head, "Status: ");
    if (p == NULL) {
        Log(YELLOW "No status in header");  
        return;     
    }
    p += strlen("Status: ");
    *status = 0;
    while (*p != ' ') {
        *status = *status * 10 + (*p - '0');
        p++;
    }
}

// // TIMER
// int setTimer(int cs, int timeout)
// {
//     int i;
//     for (int i=0; i < 100; i++) {
//         if (Timers[i].stat == 0) {
//             Timers[i].stat = 1;
//             Timers[i].cs = cs;
//             Timers[i].start = time(NULL);
//             Timers[i].timeout = timeout;
//             return 0;
//         }
//     }
// }

// void clrTimer(int i)
// {
//     if (i < 0 || i >= 100) return;

//     Timers[i].stat = 0;
//     Timers[i].cs = 0;
// }

// void resetTimer(int i)
// {
//     if (i < 0 || i >= 100) return;
//     Timers[i].start = time(NULL);
// }

// int running;
// void timer_proc()
// {
//     int i;
//     running = 1;
//     while (running)
//     {
//         /* code */
//         time_t now = time(NULL);
//         for (i = 0; i < 100; i++)
//         {
//             /* code */
//             if (Timers[i].stat == 0) continue;
//             if (now - Timers[i].start >= Timers[i].timeout) {
//                 shutdown(Timers[i].cs, SHUT_RDWR);
//                 close(Timers[i].cs);
//                 Timers[i].stat = 0;
//                 Timers[i].cs = 0;
//             }
//         }
//         sleep(1);
//     }
    
// }

// void *Thread_Client(void *arg)
// {
//     timer_proc();
// }