#ifndef _WS_SERVER_H_
#define _WS_SERVER_H_

#include <list>
#include <atomic>
#include <map>


#include "ws_c_lockmutex.h"
#include "ws_h264_session.h"




/**
 * 当用户连接上来后,就不断的传递h264文件的帧给前端用于解码!
 */
class CWebSockServer {
public:
    CWebSockServer();
    virtual ~CWebSockServer();

public:
    bool    InitConnections();  // 初始化连接池
    void    RunWebServer();     // 启动WebSocket Server


    // 回调, 有没有办法弄成私有函数.
    void    NewConnection(struct lws *);
    void    CloseConnection(struct lws *);
    void    NotifyWritable(struct lws *);

private:
    lpws_connection_t GetOneConnection(struct lws *ws);
    void FreeConnection(lpws_connection_t pConn);

private:
    // 互斥相关
    SDL_mutex                       *m_connectionMutex;

    std::list<lpws_connection_t>    m_connctionList;        // 连接池(存放所有的元素)
    std::list<lpws_connection_t>    m_freeconnectionList;   // 存放所有可以适用的连接池!

    std::atomic<int>                m_total_connection_n;   // 连接池中总的元素数量
    std::atomic<int>                m_free_connection_n;    // 当前可用的连接池元素数量

    std::map<struct lws *,CWsH264Session*> m_session_map;
};

#endif // _WS_SERVER_H_