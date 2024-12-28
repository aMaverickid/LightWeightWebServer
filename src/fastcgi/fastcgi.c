//
//  fastcgi.c
//  Copyright by QiuJS@zju. All rights reserved.
//
//  usage for web server:
//     s = fcgi_init();
//     fcgi_connect(s, gatewayIP, gatewayPort);
//     fcgi_begin_request(s, reqID);
//     fcgi_params(s, reqID, params, paramNum);
//     fcgi_param_end(s, reqID); 
//     fcgi_stdin(s, reqID, postData, dataLen);
//     fcgi_getResp(s, &std_out, &out_len, &std_err, &err_len, &app_stat, &proto_stat);
//     close(s);
//							 
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#endif
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "fastcgi.h"
#include "fcgidef.h" 

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

u_short fcgi_reqID;

// �쳣�˳�
void fcgi_exitWithErr(const char err[])
{
    printf("Fatal Error: %s\n", err);
    exit(-1);
}

// �����׽���ѡ��
void set_socket_options(int s)
{
    int on = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
    
    struct timeval t;
    t.tv_sec = 20;   // ��
    t.tv_usec = 0;   // ����
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&t, sizeof(t));
}

// ��ʼ��
int fcgi_init_socket()
{
#ifdef _WIN32
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData)!=0) 
    	fcgi_exitWithErr("Start Winsocket Failed");
#endif
    
    int s;
    s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (s < 0) fcgi_exitWithErr("unable to init socket");
    set_socket_options(s);
    return s;
}

int fcgi_connect(int s, char ip[], int port)
{
    struct sockaddr_in channel;
    memset(&channel, 0, sizeof(channel));
    channel.sin_family = AF_INET;
    channel.sin_addr.s_addr = inet_addr(ip);
    channel.sin_port = htons(port);
    int rc = connect(s, (struct sockaddr *)&channel, sizeof(channel));
    if (rc < 0) perror("connect failed.");
    return rc;
}

// ��buff�й�����Ϣͷ�� 
void fcgi_make_head(u_char *buff, u_char type, u_short reqID, u_short contentLen)
{
	FCGI_HEADER *header = (FCGI_HEADER *)buff;
	
	header->version = 1;
	header->type = type;
	header->requestIdB0 = reqID & 0xff;
	header->requestIdB1 = (reqID >> 8) & 0xff;
	header->contentLengthB0 = contentLen & 0xff;
	header->contentLengthB1 = (contentLen >> 8) & 0xff;
}

// ���Ϳ�ʼ������Ϣ 
int fcgi_begin_request(int sock, u_short reqID)
{
	size_t mesg_len = sizeof(FCGI_HEADER)+sizeof(FCGI_BEGIN_REQ);
	u_char *buff = malloc(mesg_len);
	memset(buff, 0, mesg_len);
	
	fcgi_make_head(buff, FCGI_BEGIN_REQUEST, reqID, sizeof(FCGI_BEGIN_REQ));
	
	FCGI_BEGIN_REQ *req = (FCGI_BEGIN_REQ *)(buff + sizeof(FCGI_HEADER));
	req->roleB0 = FCGI_RESPONDER;
	req->roleB1 = 0;
	req->flags = 1;   // keep connection
	
	int rc = send(sock, buff, mesg_len, 0);
	free(buff);
	return rc;
}

// ���Ͳ�����Ϣ 
int fcgi_params(int sock, u_short reqID, NV_PAIR params[], int num) 
{
	size_t paraLen = 0; 
	int i;
	
	// �������в�������Ҫ���ֽ��� 
	for (i=0; i<num; i++) {
		int k;
		// ÿ��nameռ���ֽ��� = �����ֶγ��� + �����ֶγ��� 
		k = strlen(params[i].name);
		if (k < 128) paraLen += 1 + k;   // �������<128�������ֶ�ռ1���ֽ� 
		else paraLen += 4 + k;             // ���򳤶��ֶ�ռ4���ֽ� 
		
		// ÿ��valueռ���ֽ��� = �����ֶγ��� + �����ֶγ��� 
		k = strlen(params[i].value);
		if (k < 128) paraLen += 1 + k;   // �������<128�������ֶ�ռ1���ֽ� 
		else paraLen += 4 + k;             // ���򳤶��ֶ�ռ4���ֽ� 
	}
	
	size_t mesg_len = sizeof(FCGI_HEADER) + paraLen;  // �ܳ���=�����Գ��� + �̶���8�ֽ�ͷ��
	 
	u_char *buff = malloc(mesg_len);     // ����ռ� 
	memset(buff, 0, mesg_len);           // ���� 
	
	fcgi_make_head(buff, FCGI_PARAMS, reqID, paraLen);
	
	// �������� 
	u_char *p = buff + sizeof(FCGI_HEADER);
	for (i=0; i<num; i++) {
		int k, m;
		// ���name���� 
		k = strlen(params[i].name);
		if (k < 128) *p++ = k;
		else {
			*p++ = k >> 24;           // ���λ 
			*p++ = (k >> 16) & 0xff;
			*p++ = (k >> 8) & 0xff;
			*p++ = k & 0xff;          // ���λ 
		}
		
		// ���value���� 
		m = strlen(params[i].value);
		if (m < 128) *p++ = m;
		else {
			*p++ = m >> 24;           // ���λ 
			*p++ = (m >> 16) & 0xff;
			*p++ = (m >> 8) & 0xff;
			*p++ = m & 0xff;          // ���λ 
		}
		
		
		memcpy(p, params[i].name, k);
		p += k;
		
		memcpy(p, params[i].value, m);
		p += m;
	}
	
	int rc = send(sock, buff, mesg_len, 0);
	free(buff);
	return rc;
}

// ���Ͳ���������Ϣ 
int fcgi_param_end(int sock, u_short reqID) 
{
	size_t mesg_len = sizeof(FCGI_HEADER);
	u_char *buff = malloc(mesg_len);      
	memset(buff, 0, mesg_len);            
	
	fcgi_make_head(buff, FCGI_PARAMS, reqID, 0);
	
	int rc = send(sock, buff, mesg_len, 0);
	free(buff);
	return rc;
}

// ���ͱ�׼������Ϣ 
int fcgi_stdin(int sock, u_short reqID, u_char *data, size_t dataLen)
{
	size_t mesg_len = sizeof(FCGI_HEADER)+dataLen;
	u_char *buff = malloc(mesg_len);
	memset(buff, 0, mesg_len);
	
	fcgi_make_head(buff, FCGI_STDIN, reqID, dataLen);
	
	u_char *p = buff + sizeof(FCGI_HEADER);
	memcpy(p, data, dataLen);
	
	int rc = send(sock, buff, mesg_len, 0);
	free(buff);
	return rc;
}

// ��ȡsocket���ݣ�ֱ���ֽ����ﵽ��Ҫ��ģ���������socket�ر�
// ����-1��ʾsocket�ѹرգ����򷵻��Ѷ������ֽ�����������Ҫ��ģ� 
int fcgi_readTCP(int sock, u_char *buff, size_t need)
{
	size_t have = 0;
	int len;
	do {
		len = recv(sock, buff+have, need-have, 0);
		if (len <= 0) return -1;
		
		have += len;
	} while (have < need);
	
	return have;
}

static u_char *out_buf = NULL, *err_buf = NULL;  
//static u_char *body = NULL;
// ��ȡ��������Ӧ��Ϣ 
// ���ر�׼������ݺ��ֽ�������׼�������ݺ��ֽ�����Ӧ�õ�״̬��Э���״̬
// proto_stat=0����ʾ֮ǰ������ɹ��������ʾ�ܾ� 
int fcgi_getResp(int sock, u_char **std_out, size_t *out_len, 
                            u_char **std_err, size_t *err_len,
							int *app_stat, int *proto_stat)
{
	FCGI_HEADER head;
	u_short content_len; 
	//u_char *out_buf = NULL, *err_buf = NULL;  
	u_char *body = NULL;
	u_char *p;
	int stat = 0;   // 0-read header, 1-read content
	size_t olen = 0, elen = 0;
	u_char padding[8];
	int rc = 0;
	
	if (out_buf == NULL) out_buf = malloc(1000);
	out_buf[0] = '\0';
	if (err_buf == NULL) err_buf = malloc(1000);
	err_buf[0] = '\0';
	while (1) {
		if (stat == 0) {
			memset(&head, 0, sizeof(head));
			int len = fcgi_readTCP(sock, (u_char *)&head, sizeof(FCGI_HEADER));
			if (len <= 0) {
				printf("Error: failed when recv header\n");
				rc = -1;
				break;  // socket failed
			}
			content_len = (head.contentLengthB1 << 8)+ head.contentLengthB0;
			//printf("Get header: type=%d, content_len=%d\n", head.type, content_len);
			if (content_len > 0) {
				stat = 1;  // ׼�������ݲ��� 
				body = realloc(body, content_len) ;  // body�Ǳ䳤�ģ���Ҫ��̬����ռ� 
				if (body == NULL) fcgi_exitWithErr("malloc failed");
				
				memset(body, 0, content_len);
			}
				
			// ����������ݲ���Ϊ�գ�����һ��ͷ��
		}
		else if (stat == 1) {  // body is not NULL and content_len > 0 
			int len = fcgi_readTCP(sock, body, content_len);
			if (len <= 0) {
				printf("Error: failed when recv content\n");
				rc = -1;
				break;
			}
			// handle body
			if (head.type == FCGI_STDOUT) {
				//printf("Read a stdout mesg. content_len = %d\n", content_len); 
				// �����µ����ݳ�����չ�������ռ� 
				out_buf = realloc(out_buf, olen + content_len+1);  // ������1���ֽ�'\0' 
				if (out_buf == NULL) fcgi_exitWithErr("realloc failed for stdout");
				memset(out_buf + olen, 0, content_len+1);
				
				// �µ�������ӵ�֮ǰ�Ļ��������� 
				memcpy(out_buf + olen, body, content_len);
				olen += content_len;
			}
			else if (head.type == FCGI_STDERR) {  
				//printf("Read a stderr mesg. content_len = %d\n", content_len); 
				err_buf = realloc(err_buf, elen + content_len+1); // ������1���ֽ�'\0'
				if (err_buf == NULL) fcgi_exitWithErr("realloc failed for stderr");
				memset(err_buf + elen, 0, content_len+1);
					
				memcpy(err_buf + elen, body, content_len);
				elen += content_len;
			}
			else if (head.type == FCGI_END_REQUEST) {  // ��Ӧ������Ϣ 
				FCGI_END_REQ *p = (FCGI_END_REQ *)body;
				*proto_stat = p->protocolStatus;
				*app_stat = (p->appStatusB3 << 24) + (p->appStatusB2 << 16) +
							(p->appStatusB1 << 8) + p->appStatusB0;
							
				//printf("Read a end request mesg. app_stat = %d, proto_stat = %d\n", *app_stat, *proto_stat);
				break; 
			}
				
			if (head.paddingLength > 0) {  // ��������ֽ� 
				fcgi_readTCP(sock, padding, head.paddingLength);
			}
				
			// ׼������һ��ͷ��
			stat = 0;
		}
	}
	
	free(body);
	 
	*std_out = out_buf;
	*out_len = olen;
	
	*std_err = err_buf; 
	*err_len = elen; 
	
	return rc;
}
