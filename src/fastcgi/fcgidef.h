/*
  fcgidef.h 
*/

typedef struct {
    u_char version;              // �汾 
    u_char type;                 // ��Ϣ���� 
    u_char requestIdB1;          // ����ID��2�ֽڣ���8λ�� 
    u_char requestIdB0;          // ����ID����8λ��
    u_char contentLengthB1;      // ���ݳ����ֽ�����2�ֽڣ���8λ�� 
    u_char contentLengthB0;      // ���ݳ��ȵĵ�8λ
    u_char paddingLength;        // ��䳤��
    u_char reserved;             // ����
} FCGI_HEADER;

typedef enum {
    FCGI_BEGIN_REQUEST      =  1, // ��ʼ����                              
    FCGI_ABORT_REQUEST      =  2, // �ж�����     
    FCGI_END_REQUEST        =  3, // ��������                             
    FCGI_PARAMS             =  4, // ����������������
    FCGI_STDIN              =  5, // ��׼���루POST���ݣ�
    FCGI_STDOUT             =  6, // ��׼�������Ӧ�� 
    FCGI_STDERR             =  7, // ��׼���󣨴�����Ϣ��
    FCGI_DATA               =  8, // ����
    FCGI_GET_VALUES         =  9, // GETֵ             
    FCGI_GET_VALUES_RESULT  = 10  // GETֵ�Ľ��
} FCGI_REQ_TYPE;

typedef struct {
    u_char roleB1;         // ��ɫ��2�ֽڣ���8λ��
    u_char roleB0;         // ��ɫ����8λ��
    u_char flags;          // ��־�����λΪ1ʱ��ʾ�������ӣ�����λ��δʹ�ã�
    u_char reserved[5];    // ����
} FCGI_BEGIN_REQ;

typedef enum {
    FCGI_RESPONDER  = 1,         // ��Ӧ�ߣ���ǰʹ�ã�
    FCGI_AUTHORIZER = 2,         // ��֤��
    FCGI_FILTER = 3              // ������
} FCGI_ROLE;

typedef struct {
    unsigned char appStatusB3;      // Ӧ�ü���״̬�루HTTP״̬�룬4�ֽڣ���8λ) 
    unsigned char appStatusB2;      // �θ�8λ
    unsigned char appStatusB1;      // �ε�8λ
    unsigned char appStatusB0;      // ��8λ
    unsigned char protocolStatus;   // Э�鼶��״̬�� �ο���FCGI_STATUS 
    unsigned char reserved[3];      // ����
} FCGI_END_REQ;

typedef enum {
    FCGI_REQUEST_COMPLETE   = 0,        // ������������� 
    FCGI_CANT_MPX_CONN      = 1,        // �ܾ������󡣳������Ӹ�������
    FCGI_OVERLOADED         = 2,        // �ܾ������󡣳�������
    FCGI_UNKNOWN_ROLE       = 3         // �ܾ������󡣲���ʶ��Ľ�ɫ
} FCGI_STATUS;

