#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <emscripten/emscripten.h>

void EMSCRIPTEN_KEEPALIVE initDecoder();


void EMSCRIPTEN_KEEPALIVE initDecoder()
{
    const AVCodec *codec;
    codec = avcodec_find_decoder(AV_CODEC_ID_MPEG1VIDEO);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }
    fprintf(stderr, "init Decoder success\n");
}

