#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fastcgi.h"

NV_PAIR params[100];
int num; 

void addParam(const char *name, const char *value)
{
	strcpy(params[num].name, name);
	strcpy(params[num].value, value);
	num ++;
}

int main(int argc, char *argv[]) {
	int sock = fcgi_init_socket();
	
	if (fcgi_connect(sock, "127.0.0.1", 9000) < 0) 
		fcgi_exitWithErr("Cannot connect to FastCGI Gateway");
    	
	printf("FastCGI Gateway Connected\n");
	
	char post_data[2][100] = {"login=abcd&pass=1234", "login=1234&pass=abcd"};
	char query[2][100] = {"action=login", "action=logout"};
	u_short reqID = 0;

	int i;
    for (i=0; i<2; i++)  {
		char dataLen[10];
    	printf("\n--- No.%d request ---\n", i);

    	num = 0;
		addParam("REQUEST_METHOD","POST");    		// �ύ��ʽ 
		addParam("SCRIPT_FILENAME", "c:/web/server/do.php");        // ִ�еĽű��ļ�·��������·���� 
		addParam("CONTENT_TYPE","application/x-www-form-urlencoded");  // �������� 
		
		sprintf(dataLen, "%lu", strlen(post_data[i])); 
		addParam("CONTENT_LENGTH", dataLen);		// ���ݳ��ȣ�POST���ݵĳ��ȣ��ַ�����ʽ��
		 
		addParam("QUERY_STRING", query[i]);			// �ͻ�����GET��ʽ�ύ�Ĳ����� 
		addParam("REMOTE_ADDR", "127.0.0.1");		// �ͻ��˵�ַ��Web����������socket������Ϣ��ȡ��
		addParam("REMOTE_PORT", "1111");			// �ͻ��˶˿ڣ�Web����������socket������Ϣ��ȡ�� 
	
		// ����Ϊ��ѡ���� 
		addParam("DOCUMENT_ROOT","c:/web/server");  // ��վ�ĸ�Ŀ¼ 
		addParam("SCRIPT_NAME","/do.php");          // ִ�еĽű����� 
		addParam("HTTP_CONTENT_LENGTH", dataLen);    // HTTP������Ϣ���岿���ȣ�����CONTENT_LENGTH�� 
		addParam("REQUEST_URI","/do.php");			// ����URI 
		addParam("SERVER_PORT", "80");				// �����������˿� 
		addParam("SERVER_ADDR", "127.0.0.1");		// ��������ַ 
		addParam("REDIRECT_STATUS","200");			// �ض���״̬ 
		
		reqID ++;
		if (fcgi_begin_request(sock, reqID) < 0) 
			fcgi_exitWithErr("Send begin Request failed");
		
		if (fcgi_params(sock, reqID, params, num) < 0)
			fcgi_exitWithErr("Send params failed");
			
		if (fcgi_param_end(sock, reqID) < 0)
			fcgi_exitWithErr("Send param end failed");
			
		if (fcgi_stdin(sock, reqID, (u_char *)post_data[i], strlen(post_data[i])) < 0)
			fcgi_exitWithErr("Send stdin failed");
		
		u_char *std_out, *std_err;
		size_t out_len, err_len; 
		int app_stat, proto_stat;
		fcgi_getResp(sock, &std_out, &out_len, &std_err, &err_len, &app_stat, &proto_stat);
	
		if (proto_stat != 0) printf("Request deny by Gateway, stat=%d\n", proto_stat);
		if (app_stat != 0) printf("App not return zero: stat=%d\n", app_stat);
		
	    if (std_out != NULL) {
			printf("\n***std_out(len=%lu):\n%s\n", out_len, std_out);
		}
		if (std_err != NULL) {
			printf("\n\n***std_err(len=%lu):\n%s\n", err_len, std_err);
		}
		
		sleep(1);
	}
	
	close(sock);
	return 0;
}
