#include <iostream>
#include <event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>

#pragma comment(lib,"../lib/event.lib")
#pragma comment(lib,"ws2_32.lib")

void http_cb(evhttp_request *request,void *userArgs){
    //��ȡ��������
    const char *url = evhttp_request_get_uri(request);
    printf("[+]url:%s\n",url);
    //��ȡ��������
    std::string requestType;
    switch (evhttp_request_get_command(request)) {
        case EVHTTP_REQ_GET:{
            requestType = "GET";
            break;
        }
        case EVHTTP_REQ_POST:{
            requestType = "POST";
            break;
        }
        default:
            break;
    }

    printf("[+]request method:%s\n",requestType.c_str());
    //��ȡ��Ϣ��ͷ
    evkeyvalq *headers = evhttp_request_get_input_headers(request);
    for (evkeyval *p = headers->tqh_first; p != nullptr; p = p->next.tqe_next) {
        printf("[+]%s:%s\n",p->key,p->value);
    }
    //��ȡ��������,GETΪ��,POST�б���Ϣ
    evbuffer *inputBuffer = evhttp_request_get_input_buffer(request);
    char buf[4096] {0};
    while (evbuffer_get_length(inputBuffer) > 0){
        int n = evbuffer_remove(inputBuffer, buf, sizeof(buf) - 1);
        if (n > 0){
            printf("[+]payload:%s\n",buf);
        }
    }

    //��Ӧ����
    evkeyvalq *outHeader = evhttp_request_get_output_headers(request);
    std::string path = url;

    size_t pos = path.rfind('.');
    if (pos != std::string::npos){
        std::string prefix = path.substr(pos +1,path.length() - (pos + 1));
        if (prefix == "jpg" || prefix == "gif" || prefix == "png"){
            std::string tmp = "image/" + prefix;
            //���Э��ͷ
            evhttp_add_header(outHeader,"Content-Type",tmp.c_str());
        } else if (prefix == "zip"){
            evhttp_add_header(outHeader,"Content-Type","application/zip");
        } else if (prefix == "html"){
            evhttp_add_header(outHeader,"Content-Type","text/html;charset=UTF8");
        }
    }

    //��Ӧ����
    evbuffer *outputBuffer = evhttp_request_get_output_buffer(request);
    FILE *fp;
    fopen_s(&fp,"D:\\�ҵ��ļ�\\IDM\\�����ļ�-IDM\\helloworld.txt","rb");
    size_t bytesRead;
    while ((bytesRead = fread(buf, 1,sizeof(buf),fp)) > 0){
        evbuffer_add(outputBuffer,buf,4096);
    }
    fclose(fp);
    evhttp_send_reply(request,HTTP_OK,"",outputBuffer);
}

int main(){
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2),&wsaData);

    event_base *eventBase = event_base_new();
    //����evhttp������
    evhttp *evHttp = evhttp_new(eventBase);
    //�󶨶˿ں�ip,0.0.0.0��ʾ����������
    if(evhttp_bind_socket(evHttp,"0.0.0.0",233) != 0){
        //ʧ��
    }
    //�趨�ص�����
    evhttp_set_gencb(evHttp,http_cb,nullptr);

    event_base_dispatch(eventBase);
    evhttp_free(evHttp);
    event_base_free(eventBase);
    return 0;
}