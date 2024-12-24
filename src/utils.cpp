#include "utils.h"
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

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
    if (strstr(uri, ".html") || strstr(uri, ".css") || strstr(uri, ".js") || strstr(uri, ".png") || strstr(uri, ".jpg")) {
        // 处理静态资源请求
        char filePath[512];
        transFileName(uri);
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

        Log("Send file: %s, size: %ld, content type: %s", filePath, fileStat.st_size, contentType);
        sendHttpResp(sock, 200, fileContent, fileStat.st_size, contentType);
        free(fileContent);
    }    
}

void transFileName(char *uri) {
    char buff[256] = {};
    strcpy(buff, "../2682");
    strcat(buff, uri);
    strcpy(uri, buff);
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

}

int getFastCGIResponse(char **buf, int *len, char *content_type) {

}