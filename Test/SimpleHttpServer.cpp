#include "SimpleHttpServer.h"



bool turnLogs = false;
void fn_OutputErr(const std::string &msg){
    printf("[-]%s\n",msg.c_str());
}
void fn_OutputLogs(const std::string &msg){
    printf("[Log]%s\n",msg.c_str());
}
bool fn_get_suffix(std::string &str){
    size_t pos = str.rfind('.');
    if (pos == std::string::npos){
        return false;
    }
    str = str.substr(pos + 1,str.length() - (pos + 1));
    return true;
}
void server_cb(evhttp_request *request,void *){
    std::string suffix {evhttp_request_get_uri(request)};
    if (turnLogs){
        fn_OutputLogs(suffix);
    }
    if (suffix.length() > 0){
        //获取Headers
        evkeyvalq *inputHeaders = evhttp_request_get_input_headers(request);
        if (turnLogs){
            for (evkeyval *pInEvkeyval = inputHeaders->tqh_first; pInEvkeyval != nullptr; pInEvkeyval = pInEvkeyval->next.tqe_next) {
                std::string out = pInEvkeyval->key;
                out.append(": ");
                out.append(pInEvkeyval->value);
                fn_OutputLogs(out);
            }
        }
        //获取后缀判断Context-Type并设置
        if (fn_get_suffix(suffix)){
            evkeyvalq *pOutEvkeyval = evhttp_request_get_output_headers(request);
            if (suffix == "jpg" || suffix == "jpeg"){
                evhttp_add_header(pOutEvkeyval, "Content-Type", "image/jpeg");
            }
            if (suffix == "png" || suffix == "gif"){
                evhttp_add_header(pOutEvkeyval, "Content-Type", ("image/" + suffix).c_str());
            }
            if (suffix == "html" || suffix == "xml"){
                evhttp_add_header(pOutEvkeyval, "Content-Type", ("text/" + suffix).c_str());
            }
        }

        evbuffer *outputBuffer = evhttp_request_get_output_buffer(request);
        evbuffer_add(outputBuffer, "hi world", 8);

        evhttp_send_reply(request,HTTP_OK,"", nullptr);
    }
}
int main(){
    //启动服务器
    cSimpleHttpServer simpleHttpServer{};
    simpleHttpServer.startServer("0.0.0.0",233,server_cb);
    simpleHttpServer.stopServer();
    return 0;
}

