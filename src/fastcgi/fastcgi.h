/*
  fastcgi.h 
*/

typedef unsigned char u_char;
typedef unsigned short u_short;

// 名值对(Name Value Pair) 
typedef struct {
    char name[255];
    char value[255];
} NV_PAIR;

#ifdef __cplusplus
extern "C" {
#endif
// 异常退出
void fcgi_exitWithErr(const char err[]);

// 初始化
int fcgi_init_socket();

// 连接网关 
int fcgi_connect(int s, char ip[], int port);

// 发送开始请求消息 
int fcgi_begin_request(int sock, u_short reqID);

// 发送参数消息 
int fcgi_params(int sock, u_short reqID, NV_PAIR params[], int num);

// 发送参数结束消息 
int fcgi_param_end(int sock, u_short reqID);

// 发送标准输入消息 
int fcgi_stdin(int sock, u_short reqID, u_char *data, size_t dataLen);

// 读取并解析响应消息 
// 返回标准输出内容和字节数，标准错误内容和字节数，应用的状态，协议的状态
// app_stat=0，表示应用没有发生错误，否则发生了错误 
// proto_stat=0，表示FCGI请求成功，否则表示拒绝 
// 正常返回0，socket失败返回-1 
int fcgi_getResp(int sock, u_char **std_out, size_t *out_len, 
                            u_char **std_err, size_t *err_len,
							int *app_stat, int *proto_stat); 
 
 #ifdef __cplusplus
}
#endif
