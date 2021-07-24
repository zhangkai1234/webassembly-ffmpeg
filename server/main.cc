#include "ws_server.h"

int main(int argc,char **argv)
{   
    CWebSockServer webserver;
    webserver.RunWebServer(); // do not return
    return 0;
}
