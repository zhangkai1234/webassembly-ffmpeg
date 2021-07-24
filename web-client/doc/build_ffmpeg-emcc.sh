# 生成ffmpeg-emcc版本的静态库,可以放在windows/linux共享文件夹下.
# build_wasm.sh不能放在windows/linux共享文件夹下.

cd ffmpeg
make clean
emconfigure ./configure --cc="emcc"\
        --cxx="em++" \
        --ar="emar" \
        --ranlib=emranlib --prefix=../ffmpeg-emcc/ \
        --enable-cross-compile --target-os=none \
        --arch=x86_32 \
        --cpu=generic \
        --enable-gpl \
        --disable-asm \
        --disable-programs 

make
make install

# make
# make install
# make clean
# emconfigure ./configure --cc="emcc" --cxx="em++" --ar="emar" --ranlib=emranlib \
#         --prefix=../ffmpeg-emcc --enable-cross-compile --target-os=none --arch=x86_64 \
#         --cpu=generic --disable-ffplay --disable-ffprobe \
#         --disable-asm --disable-doc --disable-devices \
#         --disable-indevs --disable-outdevs --disable-network \
#         --disable-w32threads --disable-pthreads --enable-ffmpeg \
#         --enable-static --disable-shared --enable-decoder=pcm_mulaw \
#         --enable-decoder=pcm_alaw --enable-decoder=adpcm_ima_smjpeg \
#         --enable-decoder=aac --enable-decoder=hevc \
#         --enable-decoder=h264 --enable-protocol=file --disable-stripping