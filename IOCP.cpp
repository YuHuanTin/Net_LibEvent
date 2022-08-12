#include <iostream>
#include <event.h>
//IOCP��Ҫ����
#include <event2/thread.h>

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
            evutil_make_listen_socket_reuseable(listener);
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
class cEvent{
public:
    //��ӡ֧�ֵ�����ģ��
    static void printf_supported_methods(){
        const char **methods = event_get_supported_methods();
        for (int i = 0; methods[i] != nullptr; ++i) {
            printf("[+]methods:%s\n", methods[i]);
        }
    }
    //��ӡevent_base���е�����
    static void printf_base_features(event_base *base){
        int getFeatures = event_base_get_features(base);
        if (getFeatures & EV_FEATURE_ET){
            printf("[+]EV_FEATURE_ET\n");
        }
        if (getFeatures & EV_FEATURE_EARLY_CLOSE){
            printf("[+]EV_FEATURE_EARLY_CLOSE\n");
        }
        if (getFeatures & EV_FEATURE_FDS){//������֧��epoll
            printf("[+]EV_FEATURE_FDS\n");
        }
        if (getFeatures & EV_FEATURE_O1){
            printf("[+]EV_FEATURE_O1\n");
        }
    }
    //��ӡ��ǰevent_base������ģ��
    static void printf_base_method(event_base *base){
        printf("[+]current method is:%s\n",event_base_get_method(base));
    }
    //����event_config������ģ��
    static bool set_config_method(event_config *config,const char *method){
        if (event_config_avoid_method(config,method) == 0){
            return true;
        }
        return false;
    }
    //����event_config��flag
    static void set_config_flags(event_config *config,event_base_config_flag flag){
        event_config_set_flag(config,flag);
    }
    //����event_configΪIOCP
    static void set_config_IOCP(event_config *config){
        event_config_set_flag(config,EVENT_BASE_FLAG_STARTUP_IOCP);
        //��ʼ��IOCP�߳�����
        evthread_use_windows_threads();
        //����cpu����
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        event_config_set_num_cpus_hint(config,(int)(info.dwNumberOfProcessors * 2));
    }

};
void data_cb(evutil_socket_t socket, short ev_, void *userArgs){
    if (ev_ & EV_TIMEOUT){
        printf("time out\n");
        event_free((event *)userArgs);
        evutil_closesocket(socket);
        return;
    }
    char buf [4096];
    int len = recv(socket,buf, sizeof(buf),0);
//    if (len > 0) {
//        buf[len] = '\0';
//        printf("[+]%s\n", buf);
//    }
    char response[] = "HTTP/1.1 200 OK\r\n"
                      "Content-Length: 56\r\n"
                      "Content-Type: text/html\r\n\r\n"
                      "<html><head></head><body><p>Hello LibEvent</p></body></html>";
    send(socket,response, sizeof(response),0);
    event_free((event *)userArgs);
    evutil_closesocket(socket);
}
void client_cb(evutil_socket_t socket, short i, void *userArgs){
    sockaddr_in cliAddr{};
    int lenOfSin = sizeof(cliAddr);
    auto client = (evutil_socket_t)accept(socket, (sockaddr *)&cliAddr, &lenOfSin);
    //printf("[+]client_cb ip:%s,port:%d\n",inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));

    //�ͻ��˶�ȡ&&����
    event * mEvent_Read = event_new((event_base *) userArgs, client, EV_READ | EV_PERSIST, read_cb, event_self_cbarg());
    //�趨�ͻ��˳�ʱ
    timeval t = {3,0};//10s��ʱ
    event_add(mEvent_Read,&t);
}
int main() {
    cSocket socket_;
    socket_.InitWSA();

    //��������������
    event_config *mEventConfig = event_config_new();
    cEvent::set_config_IOCP(mEventConfig);
    //��ʼ������libevent������
    event_base *mEventBase = event_base_new_with_config(mEventConfig);
    //�ͷ�����������
    event_config_free(mEventConfig);
    if (mEventBase == nullptr){
        fn_ErrorOutput("event_base_new_with_config",0);
        return -1;
    }
    //����socket
    evutil_socket_t sock = socket_.CreateListenSocket();
    event *ev = event_new(mEventBase, sock, EV_READ | EV_PERSIST, client_cb, mEventBase);
    event_add(ev,nullptr);

    event_base_dispatch(mEventBase);
    event_free(ev);
    event_base_free(mEventBase);
    return 0;
}

