cmake_minimum_required(VERSION 2.8)
set (CMAKE_CXX_STANDARD 14)
project(web-ffmpeg)

set(APP_NAME                "simple_web_server")         #生成的可执行文件的名字
set(CMAKE_CXX_FLAGS         "-std=c++11 -g")             #c++编译支持选项
set(CMAKE_C_FLAGS           "-fpermissive")              #c编译支持选项
set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/build) #生成可执行文件的目录

#头文件搜索位置
include_directories(${PROJECT_SOURCE_DIR}/libwebsockets/include)    
include_directories(${PROJECT_SOURCE_DIR}/libwebsockets/include/libwebsockets)
include_directories(${PROJECT_SOURCE_DIR}/SDL/include)
include_directories(${PROJECT_SOURCE_DIR}/ffmpeg/include)

#链接库搜索位置
Link_directories(${PROJECT_SOURCE_DIR}/libwebsockets/lib)       
Link_directories(${PROJECT_SOURCE_DIR}/SDL/lib) 
Link_directories(${PROJECT_SOURCE_DIR}/ffmpeg/lib) 
link_libraries(
    -lwebsockets 
    -lSDL2 -lSDL2main
    -lavdevice -lavfilter -lavformat -lavcodec -lswresample -lswscale -lavutil
    -lpthread -lssl -lcrypto -ldl -lz
)


aux_source_directory( ${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST ) 
add_executable(${APP_NAME} ${SRC_LIST})