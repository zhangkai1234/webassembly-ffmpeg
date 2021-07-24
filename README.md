webassembly ffmpeg测试项目:

网页播放效果图:
![image](https://user-images.githubusercontent.com/18042866/126866328-39936e79-032c-4305-a1a2-7c39c8e5531c.png)

目录介绍:

nginx:
web服务器

server: 
websocket server服务器程序

moudle： 
项目中使用的第三方开源库.

web-client:
webassembly + ffmpeg h264 decode + webgl

播放流程:

1.启动server
2.启动nginx server.
3.http://ip:port/index.html

