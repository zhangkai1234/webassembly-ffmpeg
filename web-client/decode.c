#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <emscripten/emscripten.h>


// ffmpeg请直接参考 example里面的各种范例:

typedef struct 
{
    AVCodecContext  *c;             // 用于解码
    AVPacket        *raw_pkt;       // 存储js传递进来的h264数据.
    AVFrame         *decode_frame;  // 存储解码成功后的YUV数据.

    struct SwsContext* swsCtx;      // 格式转换,有些解码后的数据不一定是YUV格式的数据
    uint8_t         *sws_data;      // 

    AVFrame         *yuv_frame;     // 存储解码成功后的YUV数据,ffmpeg解码成功后的数据不一定是 YUV420P

    uint8_t         *js_h264_buf;   // 存储JS层传递的h264 buf
    unsigned         js_h264_len;   // h264 len

    uint8_t         *yuv_buffer;    // 最大只支持到1920 * 1080

} JSDecodeHandle,*LPJSDecodeHandle;

/**
 * 初始化H264解码器
 * @return 成功返回句柄,失败返回NULL
 */
LPJSDecodeHandle EMSCRIPTEN_KEEPALIVE initDecoder();

/**
* 分配len大小的内存,并把内存的句柄
*/
uint8_t* EMSCRIPTEN_KEEPALIVE GetBuffer(LPJSDecodeHandle handle,int len);

/**
* 解码一帧数据
* @return >0 成功 <0:失败
*/
int EMSCRIPTEN_KEEPALIVE Decode(LPJSDecodeHandle handle,int len);

/**
* 获取解码成功后的宽
*/
int EMSCRIPTEN_KEEPALIVE GetWidth(LPJSDecodeHandle handle);

/**
* 获取解码成功后的高
*/
int EMSCRIPTEN_KEEPALIVE GetHeight(LPJSDecodeHandle handle);

/**
* 获取渲染后的数据
*/
uint8_t* EMSCRIPTEN_KEEPALIVE GetRenderData(LPJSDecodeHandle handle);


LPJSDecodeHandle EMSCRIPTEN_KEEPALIVE initDecoder()
{
    LPJSDecodeHandle handle = (LPJSDecodeHandle)malloc( sizeof(JSDecodeHandle) );
    memset( handle, 0 , sizeof(JSDecodeHandle) );

    do 
    {
        handle->raw_pkt = av_packet_alloc();
        if(!handle->raw_pkt) break;

        handle->decode_frame = av_frame_alloc();
        if(!handle->decode_frame) break;

        const AVCodec *codec = avcodec_find_decoder( AV_CODEC_ID_H264 );
        if(!codec) 
        {
            fprintf(stderr, "Codec not found\n");
            break;
        }
        handle->c = avcodec_alloc_context3(codec);
        if(!handle->c)
        {
            fprintf(stderr, "Could not allocate video codec context\n");
            break;
        } 

        //handle->c->thread_count = 5;

        if(avcodec_open2(handle->c,codec,NULL) < 0) 
        {
            fprintf(stderr, "Could not open codec\n");
            break;
        }

        // 我们最大支持到1920 * 1080,保存解码后的YUV数据，然后返回给前端!
        int max_width = 1920;
        int max_height = 1080;
        handle->yuv_buffer = malloc( max_width * max_height * 3 / 2);

        fprintf(stderr,"ffmpeg h264 decode init success.\n");
        return handle;
    } while(0);

    fprintf(stderr,"ffmpeg h264 decode init fail.\n");
    return NULL;    
}

uint8_t* EMSCRIPTEN_KEEPALIVE GetBuffer(LPJSDecodeHandle handle,int len)
{
    if(handle->js_h264_len < len)
    {
        if(handle->js_h264_buf) free(handle->js_h264_buf);
        handle->js_h264_buf = malloc( len * 2);
        memset(handle->js_h264_buf,0,len * 2) ; // 这句很重要!
        handle->js_h264_len = len * 2;
    }
    return handle->js_h264_buf;
}


// 解码流程参考: decodec_video.c文件
int EMSCRIPTEN_KEEPALIVE Decode(LPJSDecodeHandle handle,int len)
{
    handle->raw_pkt->data = handle->js_h264_buf;
    handle->raw_pkt->size = len;

    // fprintf(stderr, "[0]:%d [3]:%d [4]:%d [5]:%d [6]:%d\n",handle->js_h264_buf[0],handle->js_h264_buf[3], handle->js_h264_buf[4],
    //     handle->js_h264_buf[5],handle->js_h264_buf[6]);

    int ret = avcodec_send_packet(handle->c,handle->raw_pkt);
    if(ret < 0)
    {
        fprintf(stderr, "Error sending a packet for decoding\n"); // 0x00 00 00 01
        return -1;
    }

    ret = avcodec_receive_frame(handle->c, handle->decode_frame); // 这句话不是每次都成功的.
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
        fprintf(stderr, "EAGAIN -- ret:%d -%d -%d -%s\n" , ret ,AVERROR(EAGAIN) , AVERROR_EOF, av_err2str(ret) );
        return -1;
    }
    else if(ret < 0)
    {
        fprintf(stderr, "Error during decoding\n");
        return -1;
    }   
    // fprintf(stderr, "decode success: %d\n" , ret);
    return ret;
}


int EMSCRIPTEN_KEEPALIVE GetWidth(LPJSDecodeHandle handle)
{
    return handle->decode_frame->width;
}

int EMSCRIPTEN_KEEPALIVE GetHeight(LPJSDecodeHandle handle)
{
    return handle->decode_frame->height;
}

#ifndef bool 
#define bool    int
#define true    1
#define false   0
#endif 

uint8_t* EMSCRIPTEN_KEEPALIVE GetRenderData(LPJSDecodeHandle handle)
{
    int width = handle->decode_frame->width;
    int height = handle->decode_frame->height;

    bool sws_trans = false; // 我们确保得到的数据格式是YUV.
    if( handle->decode_frame->format != AV_PIX_FMT_YUV420P )
    {
        sws_trans = true;
        fprintf(stderr,"需要进行格式转换:%d\n" , handle->decode_frame->format );
    }

    AVFrame* newframe = handle->decode_frame;
    if(sws_trans)
    {
        if( handle->swsCtx == NULL )
        {
            handle->swsCtx = sws_getContext(width, height, (enum AVPixelFormat)handle->decode_frame->format, // in
					width, height, AV_PIX_FMT_YUV420P, // out
					SWS_BICUBIC, NULL, NULL, NULL);
            
            handle->yuv_frame = av_frame_alloc();
            handle->yuv_frame->width  = width;
            handle->yuv_frame->height = height;
            handle->yuv_frame->format = AV_PIX_FMT_YUV420P;

            int numbytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1);
            handle->sws_data = (uint8_t*)av_malloc(numbytes * sizeof(uint8_t));
            av_image_fill_arrays(handle->yuv_frame->data, handle->yuv_frame->linesize, handle->sws_data, 
                                AV_PIX_FMT_YUV420P,
                                width, height, 1);
        }

        if( sws_scale(handle->swsCtx,
                        handle->decode_frame->data, handle->decode_frame->linesize, 0, height,	// in
                        handle->yuv_frame->data, handle->yuv_frame->linesize // out
            ) == 0 )
        {
            fprintf(stderr,"Error in SWS Scale to YUV420P.");
            return NULL;
        }
        newframe = handle->yuv_frame;
    }

    // copy Y data
    memcpy(handle->yuv_buffer,newframe->data[0] , width * height );

    // U
    memcpy(handle->yuv_buffer + width * height , newframe->data[1], width * height  / 4) ;

    // V
    memcpy(handle->yuv_buffer + width * height + width * height  / 4 , newframe->data[2], width * height  / 4 );
    
    return handle->yuv_buffer;
}