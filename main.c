#include <string.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <pthread.h>

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

static void simple_test(void){
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
}




void *inputer(NetUnit *u){
    pthread_detach(pthread_self());
    if(u){
        char s[1024];
        int n,l=sprintf(s,"<%s enter chat>",u->data.cptr)+1;
        NetUnitWrite(u,s,l,0);
        n=sprintf(s,"%s: ",u->data.cptr);
        while(1){
            gets(s+n);
            if(!strcmp(s+n,"/exit")){
                l=sprintf(s,"<%s leave chat>",u->data.cptr)+1;
                NetUnitWrite(u,s,l,0);
                NetPoolEmit(NetUnitPool(u),2);
                break;
            }
            l=strlen(s)+1;
            NetUnitWrite(u,s,l,0);
        };
    }
    return NULL;
}

static void chat_server_proc(NetUnit * const u,const enum NET_EVENT e){
    switch(e){
        case NET_CANREAD:{
            char s[1024];
            int l=NetUnitRead(u,s,1024,0);
            if(l>0){
                const NetUnit * const server=NetUnitNodeServer(u);
                NetUnit *i=NetUnitNodeNext(u);
                for(;i && NetUnitNodeServer(i)==server;i=NetUnitNodeNext(i))
                    NetUnitWrite(i,s,l,0);
            }
            break;
        }
    }
}

static void chat_server(NetUnit * const u,const enum NET_EVENT e){
    switch(e){
        case NET_ACCEPT:{
            u->handler=chat_server_proc;
            u->data.ptr=u;
            break;
        }
    }
}

static void chat_client(NetUnit * const u,const enum NET_EVENT e){
    NetPool *p=NetUnitPool(u);
    switch(e){
        case NET_CONNECT:{
            pthread_t t[1];
            pthread_create(t,0,inputer,u);
            break;
        }
        case NET_CANREAD:{
            char s[1024];
            int l=NetUnitRead(u,s,1024,0);
            if(l>0) puts(s);
            break;
        }
        case NET_ERROR: printf("error: %s\n",strerror(errno));
            /*
                if server isnt created errno can be [ETIMEDOUT,EHOSTUNREACH,ENETDOWN,...]
                in this localhost chat we ignore error, thinking errno is [ETIMEDOUT,EHOSTUNREACH]
            */
        {
            NetAddress a[1];
            NetUnit *server=NetPoolUnit(p,NET_TCP);
            NetUnitDisconnect(u);
            NetUnitAutoRemove(server);
            if(NetUnitListen(server,NetAddressTranslate(NET_LOCAL4,12345,a))){
                NetPoolEmit(p,1);
                return;
            }
            server->handler=chat_server;
            NetUnitConnect(u,NetUnitAddress(u));
            u->timeout=10;
            break;
        }
    }
}

static void localhost_chat(int argc, char **argv){
    NetAddress a[1];
    NetPool *p=NetPoolCreate();
    NetUnit *u=NetPoolUnit(p,NET_TCP);
    int emit;

    if(u){
        NetUnitConnect(u,NetAddressTranslate(NET_LOCAL4,12345,a));
        u->timeout=10;
        u->handler=chat_client;
        u->data.cptr=(argc>1?argv[1]:"anonymous");
    }

_mark:
    switch(NetPoolDispatch(p,&emit)){
        case 0: printf("emit:%d\n",emit); break;
        case EINVAL: break;
        default: goto _mark;
    }

    NetPoolDestroy(p);
    puts("\nexit");
    return 0;
}

void main(int argc,char **argv){
    simple_test();
//    localhost_chat(argc,argv);
}