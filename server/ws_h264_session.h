#ifndef _WS_H264_SESSION_H_
#define _WS_H264_SESSION_H_

#include "libwebsockets.h"
#include <stdint.h>
#include "SDL2/SDL_thread.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

struct ws_connection_s 
{
    struct lws *wsi;
    uint64_t    iCurrsequence;

    ws_connection_s();
    ~ws_connection_s();

    void resetToUse();
};

typedef struct  ws_connection_s ws_connection_t,*lpws_connection_t;


class CWsH264Session {
public:
    CWsH264Session(lpws_connection_t conn,unsigned iCurr);
    virtual ~CWsH264Session();

    lpws_connection_t GetWSConnection() const {
        return pWsConn;
    }

    bool InitSession();   // 启动线程从文件中读取数据并开始传输
    //void Process();
    bool ReadAndSend();

private:
    unsigned            iCurrSequence;
    lpws_connection_t   pWsConn;
    SDL_Thread          *thread;

    AVPacket *packet ;
    AVFormatContext *ifmt_ctx;
};

#endif  // _WS_H264_SESSION_H_