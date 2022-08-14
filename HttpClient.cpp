#include <iostream>
#include <event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>

#pragma comment(lib,"../lib/event.lib")
#pragma comment(lib,"ws2_32.lib")

void http_client_cb(evhttp_request *request, void *userArgs){
    auto bev = (bufferevent *)userArgs;
    //服务端响应错误
    if (request == nullptr){
        int errorCode = EVUTIL_SOCKET_ERROR();
        printf("[-]%s\n",evutil_socket_error_to_string(errorCode));
        return;
    }

    //获取path
    const char *path = evhttp_request_get_uri(request);
    printf("[+]%s\n",path);
    //获取返回状态信息
    printf("[+]%d\n",evhttp_request_get_response_code(request));//200
    printf("[+]%s\n",evhttp_request_get_response_code_line(request));//OK
    //获取内容
    evbuffer *evbuffer = evhttp_request_get_input_buffer(request);
    int len;
    do {
        char buf[4096] = {0};
        len = evbuffer_remove(evbuffer,buf, sizeof(buf) - 1);
        //输出
        printf("[+]%s\n",buf);
    } while (len > 0);
}

int main(){
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2),&wsaData);

    event_base *eventBase = event_base_new();

    //生成请求信息-GET
    std::string url = "http://127.0.0.1:233/1.html";
    evhttp_uri *evhttpUri = evhttp_uri_parse(url.c_str());
    //获取协议,有可能是http/https/null
    const char *scheme = evhttp_uri_get_scheme(evhttpUri);
    //获取host,有可能www.baidu.com/null
    const char *host = evhttp_uri_get_host(evhttpUri);
    //获取port,不写为-1
    int port = evhttp_uri_get_port(evhttpUri);
    if (port < 0){
        if (strcmp(scheme,"http") == 0){
            port = 80;
        }
        if (strcmp(scheme,"https") == 0){
            port = 443;
        }
    }
    //获取请求路径,有可能为空
    const char *path = evhttp_uri_get_path(evhttpUri);
    if (path == nullptr || strlen(path) == 0){
        path = "/";
    }
    //获取query,?后面的内容,有可能为null
    const char *query = evhttp_uri_get_query(evhttpUri);
    printf("[+]%s ,%s ,%s ,%s ,%d \n",scheme,host,path,query,port);


    //bufferevent连接http服务器,关闭bufferevnet时同步关闭socket
    bufferevent *bev = bufferevent_socket_new(eventBase,-1,BEV_OPT_CLOSE_ON_FREE);
    //连接
    evhttp_connection *evhttpConnection = evhttp_connection_base_bufferevent_new(eventBase,
                                                                                 nullptr,//DNS解析
                                                                                 bev,
                                                                                 host,
                                                                                 port);
    //创建请求
    evhttp_request *evhttpRequest = evhttp_request_new(http_client_cb,bev);
    //设置请求信息,headers
    evkeyvalq *out_Headers = evhttp_request_get_output_headers(evhttpRequest);
    evhttp_add_header(out_Headers,"Host",host);
    //发送post数据
    evbuffer *evbuffer = evhttp_request_get_output_buffer(evhttpRequest);
    //evbuffer_add(evbuffer,"nihao",5);
    evbuffer_add_printf(evbuffer,"name=%d,psd=%s",32,"nihao");

    //发起请求,path可以为path+query/path
    evhttp_make_request(evhttpConnection,evhttpRequest,EVHTTP_REQ_POST,path);


    event_base_dispatch(eventBase);
    event_base_free(eventBase);
    return 0;
}