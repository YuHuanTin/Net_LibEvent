#include <iostream>
#include <event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>

#pragma comment(lib,"../lib/event.lib")
#pragma comment(lib,"ws2_32.lib")

void http_client_cb(evhttp_request *request, void *userArgs){
    auto bev = (bufferevent *)userArgs;
    //�������Ӧ����
    if (request == nullptr){
        int errorCode = EVUTIL_SOCKET_ERROR();
        printf("[-]%s\n",evutil_socket_error_to_string(errorCode));
        return;
    }

    //��ȡpath
    const char *path = evhttp_request_get_uri(request);
    printf("[+]%s\n",path);
    //��ȡ����״̬��Ϣ
    printf("[+]%d\n",evhttp_request_get_response_code(request));//200
    printf("[+]%s\n",evhttp_request_get_response_code_line(request));//OK
    //��ȡ����
    evbuffer *evbuffer = evhttp_request_get_input_buffer(request);
    int len;
    do {
        char buf[4096] = {0};
        len = evbuffer_remove(evbuffer,buf, sizeof(buf) - 1);
        //���
        printf("[+]%s\n",buf);
    } while (len > 0);
}

int main(){
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2),&wsaData);

    event_base *eventBase = event_base_new();

    //����������Ϣ-GET
    std::string url = "http://127.0.0.1:233/1.html";
    evhttp_uri *evhttpUri = evhttp_uri_parse(url.c_str());
    //��ȡЭ��,�п�����http/https/null
    const char *scheme = evhttp_uri_get_scheme(evhttpUri);
    //��ȡhost,�п���www.baidu.com/null
    const char *host = evhttp_uri_get_host(evhttpUri);
    //��ȡport,��дΪ-1
    int port = evhttp_uri_get_port(evhttpUri);
    if (port < 0){
        if (strcmp(scheme,"http") == 0){
            port = 80;
        }
        if (strcmp(scheme,"https") == 0){
            port = 443;
        }
    }
    //��ȡ����·��,�п���Ϊ��
    const char *path = evhttp_uri_get_path(evhttpUri);
    if (path == nullptr || strlen(path) == 0){
        path = "/";
    }
    //��ȡquery,?���������,�п���Ϊnull
    const char *query = evhttp_uri_get_query(evhttpUri);
    printf("[+]%s ,%s ,%s ,%s ,%d \n",scheme,host,path,query,port);


    //bufferevent����http������,�ر�bufferevnetʱͬ���ر�socket
    bufferevent *bev = bufferevent_socket_new(eventBase,-1,BEV_OPT_CLOSE_ON_FREE);
    //����
    evhttp_connection *evhttpConnection = evhttp_connection_base_bufferevent_new(eventBase,
                                                                                 nullptr,//DNS����
                                                                                 bev,
                                                                                 host,
                                                                                 port);
    //��������
    evhttp_request *evhttpRequest = evhttp_request_new(http_client_cb,bev);
    //����������Ϣ,headers
    evkeyvalq *out_Headers = evhttp_request_get_output_headers(evhttpRequest);
    evhttp_add_header(out_Headers,"Host",host);
    //����post����
    evbuffer *evbuffer = evhttp_request_get_output_buffer(evhttpRequest);
    //evbuffer_add(evbuffer,"nihao",5);
    evbuffer_add_printf(evbuffer,"name=%d,psd=%s",32,"nihao");

    //��������,path����Ϊpath+query/path
    evhttp_make_request(evhttpConnection,evhttpRequest,EVHTTP_REQ_POST,path);


    event_base_dispatch(eventBase);
    event_base_free(eventBase);
    return 0;
}