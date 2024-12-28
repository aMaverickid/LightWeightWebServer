/*
  fcgidef.h 
*/

typedef struct {
    u_char version;              // 版本 
    u_char type;                 // 消息类型 
    u_char requestIdB1;          // 请求ID（2字节，高8位） 
    u_char requestIdB0;          // 请求ID（低8位）
    u_char contentLengthB1;      // 内容长度字节数（2字节，高8位） 
    u_char contentLengthB0;      // 内容长度的低8位
    u_char paddingLength;        // 填充长度
    u_char reserved;             // 保留
} FCGI_HEADER;

typedef enum {
    FCGI_BEGIN_REQUEST      =  1, // 开始请求                              
    FCGI_ABORT_REQUEST      =  2, // 中断请求     
    FCGI_END_REQUEST        =  3, // 结束请求                             
    FCGI_PARAMS             =  4, // 参数（环境变量）
    FCGI_STDIN              =  5, // 标准输入（POST数据）
    FCGI_STDOUT             =  6, // 标准输出（响应） 
    FCGI_STDERR             =  7, // 标准错误（错误信息）
    FCGI_DATA               =  8, // 数据
    FCGI_GET_VALUES         =  9, // GET值             
    FCGI_GET_VALUES_RESULT  = 10  // GET值的结果
} FCGI_REQ_TYPE;

typedef struct {
    u_char roleB1;         // 角色（2字节，高8位）
    u_char roleB0;         // 角色（低8位）
    u_char flags;          // 标志（最低位为1时表示保持连接，其他位暂未使用）
    u_char reserved[5];    // 保留
} FCGI_BEGIN_REQ;

typedef enum {
    FCGI_RESPONDER  = 1,         // 响应者（当前使用）
    FCGI_AUTHORIZER = 2,         // 认证者
    FCGI_FILTER = 3              // 过滤者
} FCGI_ROLE;

typedef struct {
    unsigned char appStatusB3;      // 应用级别状态码（HTTP状态码，4字节，高8位) 
    unsigned char appStatusB2;      // 次高8位
    unsigned char appStatusB1;      // 次低8位
    unsigned char appStatusB0;      // 低8位
    unsigned char protocolStatus;   // 协议级别状态码 参考：FCGI_STATUS 
    unsigned char reserved[3];      // 保留
} FCGI_END_REQ;

typedef enum {
    FCGI_REQUEST_COMPLETE   = 0,        // 请求的正常结束 
    FCGI_CANT_MPX_CONN      = 1,        // 拒绝新请求。超出连接个数限制
    FCGI_OVERLOADED         = 2,        // 拒绝新请求。超出负载
    FCGI_UNKNOWN_ROLE       = 3         // 拒绝新请求。不能识别的角色
} FCGI_STATUS;

