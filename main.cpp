#include <iostream>
#include <event.h>

#pragma comment(lib,"../lib/event.lib")
#pragma comment(lib,"ws2_32.lib")


void fn_ErrorOutput(const std::string &msg,unsigned long e){
    printf("[-]%s ,%lu\n",msg.c_str(),e);
}
class cSocket{
private:
    SOCKET CreateSocket(){
        SOCKET newSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        if (newSocket != SOCKET_ERROR){
            return newSocket;
        }
        fn_ErrorOutput("cSocket::CreateSocket::socket",WSAGetLastError());
        return INVALID_SOCKET;
    }
public:
    void InitWSA(){
        WSADATA wsadata;
        if (WSAStartup(MAKEWORD(2,2), &wsadata) != 0)
            fn_ErrorOutput("cSocket::InitWSA::WSAStartup",WSAGetLastError());
    }
    evutil_socket_t CreateListenSocket(){
        auto listener = (evutil_socket_t)CreateSocket();
        evutil_make_socket_nonblocking(listener);
        sockaddr_in serverAddr{
            AF_INET,
            htons(233),
            INADDR_ANY
        };
        if (bind(listener,(sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR){
            fn_ErrorOutput("cSocket::CreateListenSocket::bind",WSAGetLastError());
        }
        if (listen(listener,SOMAXCONN) == SOCKET_ERROR){
            fn_ErrorOutput("cSocket::CreateListenSocket::listen",WSAGetLastError());
        }
        return listener;
    }

};
char
rot13_char(char c)
{
    /* We don't want to use isalpha here; setting the locale would change
     * which characters are considered alphabetical. */
    if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
        return c + 13;
    else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
        return c - 13;
    else
        return c;
}

void fn_read(struct bufferevent *bev, void *ctx)
{
    struct evbuffer *input, *output;
    char *line;
    size_t n;
    int i;
    input = bufferevent_get_input(bev);
    output = bufferevent_get_output(bev);

    while ((line = evbuffer_readln(input, &n, EVBUFFER_EOL_LF))) {
        for (i = 0; i < n; ++i)
            line[i] = rot13_char(line[i]);
        evbuffer_add(output, line, n);
        evbuffer_add(output, "\n", 1);
        free(line);
    }

    if (evbuffer_get_length(input) >= 0x4000) {
        /* Too long; just process what there is and go on so that the buffer
         * doesn't grow infinitely long. */
        char buf[1024];
        while (evbuffer_get_length(input)) {
            int n = evbuffer_remove(input, buf, sizeof(buf));
            printf_s("%s",buf,n);
            for (i = 0; i < n; ++i)
                buf[i] = rot13_char(buf[i]);
            evbuffer_add(output, buf, n);
        }
        evbuffer_add(output, "\n", 1);
    }
}

void fn_write(bufferevent *bev, void *userArgs){
    /*
    evbuffer *input, *output;
    input = bufferevent_get_input(bev);
    output = bufferevent_get_output(bev);
*/
    //closesocket(*(SOCKET *)&userArgs);
}
void fn_accept(evutil_socket_t listener, short event, void *userArgs){
    auto mEvent_base= (event_base *)userArgs;

    sockaddr client{};
    int socketaddrLen = (int) sizeof(client);
    auto fd = (evutil_socket_t)accept(listener, &client, &socketaddrLen);
    if (fd == SOCKET_ERROR){
        fn_ErrorOutput("fn_accept::accept",WSAGetLastError());
    }
    evutil_make_socket_nonblocking(fd);
    bufferevent *mBufferEvent;
    mBufferEvent = bufferevent_socket_new(mEvent_base,fd,BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(mBufferEvent,fn_read,fn_write, nullptr, (void *)fd);
    bufferevent_setwatermark(mBufferEvent,EV_READ,0,0x4000);
    bufferevent_setwatermark(mBufferEvent,EV_WRITE,0,0x4000);
    bufferevent_enable(mBufferEvent,EV_READ | EV_WRITE);
}
int main() {
    cSocket socket_;
    socket_.InitWSA();
    evutil_socket_t listener = socket_.CreateListenSocket();
    event_base *event_base;
    event *listen_event;

    event_base = event_base_new();
    if (event_base == nullptr){
        fn_ErrorOutput("event_base_new",0);
    }
    //发送大数据再加上EV_WRITE
    listen_event = event_new(event_base, listener, EV_READ | EV_PERSIST, fn_accept, (void *) event_base);

    event_add(listen_event, nullptr);
    event_base_dispatch(event_base);
    return 0;
}

