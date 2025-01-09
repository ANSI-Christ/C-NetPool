#include <string.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>

#define NETPOOL_IMPL
#include "NetPool.h"

static void tcp_server_proc(NetUnit * const u,const enum NET_EVENT e){
    switch(e){
        case NET_CONNECT:{
            u->timeout=2;
            NetUnitWrite(u,"!@#$%^&*()",11,NULL);
            puts("tcp proc send message");
            break;
        }
        case NET_TIMEOUT:
            puts("tcp proc timeout");
            NetUnitDisconnect(u);
            break;
        case NET_DISCONNECT:
            puts("tcp proc disconnect");
            break;
        case NET_ERROR:
            printf("tcp proc error (%s)\n",strerror(errno));
            sleep(1);
            break;
    }
}

static void tcp_server(NetUnit * const u,const enum NET_EVENT e){
    switch(e){
        case NET_ACCEPT:
            puts("tcp server accept new client");
            u->handler=tcp_server_proc;
            break;
        case NET_TIMEOUT:
            puts("tcp server timeout");
            break;
        case NET_DISCONNECT:
            puts("tcp server down");
            break;
        case NET_ERROR:
            printf("tcp server error (%s)\n",strerror(errno));
            sleep(1);
            break;
    }
}

static void tcp_client(NetUnit * const u,const enum NET_EVENT e){
    switch(e){
        case NET_CONNECT:
            puts("tcp client connect");
            u->timeout=4;
            break;
        case NET_CANREAD:{
            char s[1024];
            unsigned int l=NetUnitRead(u,s,sizeof(s),NULL);
            printf("tcp client get message [%s]\n",s);
            break;
        }
        case NET_TIMEOUT:
            puts("tcp client timeout");
            NetUnitDisconnect(u);
            break;
        case NET_DISCONNECT:
            NetPoolEmit(NetUnitPool(u),100);
            puts("tcp client disconnect");
            break;
        case NET_ERROR:
            printf("tcp client error (%s)\n",strerror(errno));
            sleep(1);
            break;
    }
}

static void tcp_client_delay(NetUnit * const u,const enum NET_EVENT e){
    NetAddress a[1];
    if( e==NET_TIMEOUT && !NetUnitConnect(u,NetAddressTranslate(NET_LOCAL4,12345,a)) ){
        u->timeout=20; /* time for connecting */
        u->handler=tcp_client;
    }
}

int main(){
    int emit;
    NetAddress a[1];
    NetPool *p=NetPoolCreate();
    NetUnit *u;

    if( (u=NetPoolUnit(p,NET_TCP)) && !NetUnitListen(u,NetAddressTranslate(NET_ANY4,12345,a)) ){
        u->handler=tcp_server;
        u->timeout=5;
        u->data.cptr="tcp server";
    }
    NetUnitAutoRemove(u);

    if( (u=NetPoolUnit(p,NET_TCP)) ){
        u->handler=tcp_client_delay;
        u->timeout=6; /* client try to connect later */
    }
    NetUnitAutoRemove(u);

_mark:
    switch(NetPoolDispatch(p,&emit)){
        case 0: printf("emit:%d\n",emit); break;
        case EINVAL: break;
        default: goto _mark;
    }

    NetPoolDestroy(p);
    return 0;
}
