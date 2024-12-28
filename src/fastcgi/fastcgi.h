/*
  fastcgi.h 
*/

typedef unsigned char u_char;
typedef unsigned short u_short;

// ��ֵ��(Name Value Pair) 
typedef struct {
    char name[255];
    char value[255];
} NV_PAIR;

#ifdef __cplusplus
extern "C" {
#endif
// �쳣�˳�
void fcgi_exitWithErr(const char err[]);

// ��ʼ��
int fcgi_init_socket();

// �������� 
int fcgi_connect(int s, char ip[], int port);

// ���Ϳ�ʼ������Ϣ 
int fcgi_begin_request(int sock, u_short reqID);

// ���Ͳ�����Ϣ 
int fcgi_params(int sock, u_short reqID, NV_PAIR params[], int num);

// ���Ͳ���������Ϣ 
int fcgi_param_end(int sock, u_short reqID);

// ���ͱ�׼������Ϣ 
int fcgi_stdin(int sock, u_short reqID, u_char *data, size_t dataLen);

// ��ȡ��������Ӧ��Ϣ 
// ���ر�׼������ݺ��ֽ�������׼�������ݺ��ֽ�����Ӧ�õ�״̬��Э���״̬
// app_stat=0����ʾӦ��û�з������󣬷������˴��� 
// proto_stat=0����ʾFCGI����ɹ��������ʾ�ܾ� 
// ��������0��socketʧ�ܷ���-1 
int fcgi_getResp(int sock, u_char **std_out, size_t *out_len, 
                            u_char **std_err, size_t *err_len,
							int *app_stat, int *proto_stat); 
 
 #ifdef __cplusplus
}
#endif
