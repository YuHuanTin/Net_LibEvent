#include <iostream>
#include <event.h>

#pragma comment(lib,"../lib/event.lib")
#pragma comment(lib,"ws2_32.lib")
void fn_ErrorOutput(const std::string &msg,unsigned long e){
    printf("[-]%s ,%lu\n",msg.c_str(),e);
}
char response[] = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n\r\n"
                   "Hello LibEvent";
void read_cb(evutil_socket_t socket, short ev_, void *userArgs){
    char buf [4096]{0};
    int bytesRead = recv(socket,buf, sizeof(buf),0);
    if (bytesRead > 0){
        buf[bytesRead] = '\0';
        printf("[+]%s\n",buf);
        //·¢ËÍÊý¾Ý
        send(socket, response, sizeof(response), 0);
    }
    event_free((event *)userArgs);
    evutil_closesocket(socket);
}
void client_cb(evutil_socket_t socket,short ev_,void *userArgs){
    sockaddr_in sin{};
    int len = sizeof(sin);
    auto client = (evutil_socket_t)accept(socket,(sockaddr *)&sin,&len);
    printf("[+]connect ip:%s,port:%d\n",inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

    event *ev_r = event_new((event_base *) userArgs, client, EV_READ | EV_PERSIST, read_cb, event_self_cbarg());
    event_add(ev_r,nullptr);
}
int main(){
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2),&wsaData);
    auto listen_s = (evutil_socket_t)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in addr{
        AF_INET,
        htons(233),
        INADDR_ANY
    };
    if (bind(listen_s, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR){
        fn_ErrorOutput("bind",WSAGetLastError());
    }
    if (listen(listen_s, SOMAXCONN) == SOCKET_ERROR){
        fn_ErrorOutput("listen",WSAGetLastError());
    }

    event_base *eventBase = event_base_new();
    if (eventBase == nullptr){
        fn_ErrorOutput("event_base_new",0);
        return -1;
    }
    event *ev = event_new(eventBase, listen_s, EV_READ | EV_PERSIST, client_cb, eventBase);
    if (ev == nullptr){
        fn_ErrorOutput("event_new",0);
        event_base_free(eventBase);
        return -1;
    }
    event_add(ev,nullptr);
    event_base_dispatch(eventBase);
    return 0;
}