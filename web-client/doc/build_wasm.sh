# build_wasm.sh不能放在windows/linux共享文件夹下.

rm -rf ffmpeg.wasm ffmpeg.js


export TOTAL_MEMORY=67108864
export FFMPEG_PATH=./ffmpeg-emcc
export SOURCE=./decode

echo "Running Emscripten..."
emcc decode.c ${FFMPEG_PATH}/lib/libavformat.a \
    ${FFMPEG_PATH}/lib/libavcodec.a  \
    ${FFMPEG_PATH}/lib/libswresample.a \
    ${FFMPEG_PATH}/lib/libswscale.a \
    ${FFMPEG_PATH}/lib/libavutil.a \
    -O3 \
    -I "${FFMPEG_PATH}/include" \
    -s WASM=1 \
    -s NO_EXIT_RUNTIME=1 \
    -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall']" \
    -s ASSERTIONS=0 \
    -s TOTAL_MEMORY=167772160 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -o ${PWD}/ffmpeg.js

echo "Finished Build"