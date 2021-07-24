#include "ws_h264_session.h"

ws_connection_s::ws_connection_s()
{
    iCurrsequence = 0;
}

ws_connection_s::~ws_connection_s()
{

}

void ws_connection_s::resetToUse()
{
    iCurrsequence++;
}


CWsH264Session::CWsH264Session(lpws_connection_t conn,unsigned iCurr)
    :pWsConn(conn),iCurrSequence(iCurr),packet(NULL),ifmt_ctx(NULL)
{

}

CWsH264Session::~CWsH264Session()
{
    //SDL_WaitThread(thread,NULL);
    lws_close_free_wsi(pWsConn->wsi,LWS_CLOSE_STATUS_NORMAL,"end of session."); // 触发一此可写和LWS_CALLBACK_CLOSED
    if(packet) av_packet_free(&packet);
    if(ifmt_ctx) avformat_close_input(&ifmt_ctx);
}

// int (SDLCALL * SDL_ThreadFunction) (void *data);

// int ReadH264FromFile(void *data)
// {
//     CWsH264Session *session = static_cast<CWsH264Session*>( data );
//     session->Process();
//     return 0;
// }

bool CWsH264Session::InitSession()
{
    // thread = SDL_CreateThread(&ReadH264FromFile,"Thread Read File",this);

    const char *filename = "./test.264";
    int ret;

    if ( (ret = avformat_open_input(&ifmt_ctx,filename,NULL,NULL) ) < 0 )  // 编译的时候，必须支持avformat h264
    {
        av_log(NULL,AV_LOG_ERROR,"Cannot open input file:%s -ret:%d\n" , filename , ret);
        return false;
    }

    if( !(packet = av_packet_alloc()) )
    {
        av_log(NULL,AV_LOG_ERROR," av packet alloc fail.");
        return false;
    }         

    return true;
}


/**
 * @return true:读取到一帧数据并发送成功 false:失败
 */
bool CWsH264Session::ReadAndSend()
{
    int ret;
    if ( (ret = av_read_frame(ifmt_ctx,packet)) < 0 )
    {
        return false;
    }

    unsigned char PostBuf [LWS_PRE + packet->size];
    memset(PostBuf,0,sizeof(PostBuf));
    memcpy(PostBuf + LWS_PRE, packet->data,packet->size);
   

    int n = lws_write(pWsConn->wsi,&PostBuf[LWS_PRE],packet->size,LWS_WRITE_BINARY);
    if( n != packet->size)
    {
         fprintf(stderr,"fail ,read packet size:%d send:%d\n" , packet->size, n);
        return false;
    }
    else
    {
        fprintf(stderr,"success,read packet size:%d send:%d\n" , packet->size, n);
        lws_callback_on_writable(pWsConn->wsi);  // 继续可写通知!

        // FILE *fs = fopen("five_frame.264","ab+");
        // fwrite(packet->data,1,packet->size,fs);
        // fclose(fs);

        // static int index = 0;
        // index++;
        // if(index > 5)
        // {
        //     exit(2);
        // }
        


        // 测试解码到底能否干活! --是能干活的.
        // const AVCodec *codec = avcodec_find_decoder( AV_CODEC_ID_H264 );
        // AVCodecContext  *c = avcodec_alloc_context3(codec);
        // avcodec_open2(c,codec,NULL);

        // int ret = avcodec_send_packet(c,packet);

        // AVFrame *decode_frame = av_frame_alloc();
        // ret = avcodec_receive_frame(c, decode_frame);
        // fprintf(stderr,"receive frame res:%d\n" , ret);

    }
    
    av_packet_unref(packet);

    return true;
}
