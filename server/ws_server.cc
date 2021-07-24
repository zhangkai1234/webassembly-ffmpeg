#include <assert.h>

#include "ws_server.h"
#include "ws_c_memory.h"
#include "config.h"


CWebSockServer::CWebSockServer()
{

}

CWebSockServer::~CWebSockServer()
{

}

bool CWebSockServer::InitConnections()
{
    m_connectionMutex = SDL_CreateMutex();
    if(m_connectionMutex == NULL) 
    {
        return false;
    }

    lpws_connection_t p_Conn;
    CMemory *p_Memory = CMemory::GetInstance();

    int ilenconn = sizeof(ws_connection_t);

    // 连接池中的元素创建后便不会被释放，所以我们刚开始时候可以创建少一点的元素，如果不够了，后续我们再进行添加.
    for(int i = 0 ; i < CONNECTIONS ; i++)
    {
        p_Conn = static_cast<lpws_connection_t>(p_Memory->AllocMemory( ilenconn , true));
        p_Conn = new(p_Conn) ws_connection_t();     // 定位new
        p_Conn->resetToUse();                       // 初始化
        m_connctionList.push_back(p_Conn);          // 所有的连接池item不论是否空闲都存放在list中.
        m_freeconnectionList.push_back(p_Conn);
    }
    m_free_connection_n = m_total_connection_n = m_connctionList.size();
    return true;
}

lpws_connection_t CWebSockServer::GetOneConnection(struct lws *ws)
{
    CLock lock(m_connectionMutex);

    if( !m_freeconnectionList.empty() )
    {
        lpws_connection_t p_Conn = m_freeconnectionList.front();
        m_freeconnectionList.pop_front();
        p_Conn->resetToUse();
        p_Conn->wsi = ws;
        --m_free_connection_n;
        return p_Conn;
    }

    //  走到这里，表示没有空闲的连接了，这个时候我们重新建立一个.
    CMemory *p_Memory = CMemory::GetInstance();
    lpws_connection_t p_Conn = (lpws_connection_t)p_Memory->AllocMemory(sizeof(ws_connection_t),true);
    p_Conn = new(p_Conn) ws_connection_t ();
    p_Conn->resetToUse();
    m_connctionList.push_back(p_Conn); // 注意这里: 入总表，不要入freeconnectionList.
    ++m_total_connection_n;
    p_Conn->wsi = ws;
    return p_Conn;
}

void CWebSockServer::FreeConnection(lpws_connection_t pConn)
{
    CLock lock(m_connectionMutex);
    pConn->iCurrsequence--;
    m_freeconnectionList.push_back(pConn); 
    ++m_free_connection_n;
}


#define MAX_PAYLOAD_SIZE  10 * 1024
#define LWSWS_CONFIG_STRING_SIZE (64 * 1024)

struct session_data {
    unsigned char buf[LWS_PRE + MAX_PAYLOAD_SIZE];
    int len;
};

static int 
protocol_ws_control_callback( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len ) ;


void CWebSockServer::RunWebServer()
{
    int wsPort = LISTEN_PORT;
	int debug_level = LLL_USER | 7;
    int uid = -1, gid = -1;
	int use_ssl = 0;
	int opts = 0;
	int n = 0;

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port  = wsPort;               // 要绑定的端口
    info.iface = NULL;

	struct lws_vhost *vhost;
    struct lws_context *context;

    struct lws_protocols protocols[] = {
        /* first protocol must always be HTTP handler */
        {"ws",protocol_ws_control_callback,sizeof(struct session_data) ,MAX_PAYLOAD_SIZE,0,this,},
        { NULL, NULL, 0, 0 } /* terminator */
    };

    info.protocols = protocols;
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;

    info.gid = gid;
	info.uid = uid;
	info.options = opts | LWS_SERVER_OPTION_VALIDATE_UTF8 | LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
    info.user = this;

    info.timeout_secs = 5;
    // info.ip_limit_ah = 8; /* for testing */
	// info.ip_limit_wsi = 8; /* for testing */


    context = lws_create_context(&info);
    if(context == NULL)
    {
        lwsl_err("libwebsockets init failed.\n");
        return ;
    }

    vhost = lws_create_vhost(context,&info);
    if(!vhost)
    {
        lwsl_err("vhost creation failed.\n");
        return ;
    }

    while( n >= 0 )
    {
        n = lws_service(context, 20);
    }

    lws_context_destroy(context);
}

static int 
protocol_ws_control_callback( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len ) 
{
    auto protocol = lws_get_protocol(wsi);
    CWebSockServer *server = NULL;
    if(protocol != NULL)
    {
        server = static_cast<CWebSockServer*> (protocol->user);
    }


    switch(reason)
    {
        case LWS_CALLBACK_ESTABLISHED:
        {
            lwsl_err("连接建立.\n");
            server->NewConnection(wsi);
        }
        break;
        case LWS_CALLBACK_RECEIVE:
        {
            // lwsl_err("数据接收中.\n");
        }
        break;
        case LWS_CALLBACK_SERVER_WRITEABLE:
        {
            // lwsl_err("可写.\n");
            server->NotifyWritable(wsi);//可以发数据了，并且不会出现粘包情况.
        }
        break;
        case LWS_CALLBACK_CLOSED:
        {
            lwsl_err("已关闭.\n");
            server->CloseConnection(wsi);
        }
        break;
        default:
        {
            // 尚未处理的状态码
        }   
        break;
    }
    return 0;
}

void CWebSockServer::NewConnection(struct lws *wsi)
{
    CLock lock(m_connectionMutex);
    lpws_connection_t pConn = GetOneConnection(wsi);
    CWsH264Session* session =  new CWsH264Session(pConn,pConn->iCurrsequence);
    if(!session->InitSession())
    {
        delete session;
        return;
    }
    lws_callback_on_writable(wsi);  // 避免网络阻塞,模型决定了决定数据的发送方式!
    m_session_map.insert( std::make_pair(wsi, session) );
}

void CWebSockServer::NotifyWritable(struct lws *wsi)
{
    auto iter = m_session_map.find(wsi);
    if(iter == m_session_map.end() ) return ;

    CWsH264Session* session = iter->second;
    if ( !session->ReadAndSend() )
    {
        fprintf(stderr,"read the end of file. \n");
        m_session_map.erase(iter);
        delete session;
    }
}

void CWebSockServer::CloseConnection(struct lws *wsi)
{
    CLock lock(m_connectionMutex);

    auto iter = m_session_map.find(wsi);
    if(iter == m_session_map.end() ) return ;

    CWsH264Session *session  = iter->second;
    lpws_connection_t pConn = session->GetWSConnection();
    FreeConnection(pConn);

    m_session_map.erase(iter);
    delete session;
}