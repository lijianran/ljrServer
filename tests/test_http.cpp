
#include "../ljrServer/http/http.h"
#include "../ljrServer/log.h"

void test_request()
{
    ljrserver::http::HttpRequest::ptr req(new ljrserver::http::HttpRequest);
    req->setHeader("host", "www.baidu.com");
    req->setBody("hello baidu");

    req->dump(std::cout) << std::endl;
}

void test_response()
{
    ljrserver::http::HttpResponse::ptr rsp(new ljrserver::http::HttpResponse);
    rsp->setHeader("X-X", "lijianran");
    rsp->setBody("hello lijianran");
    rsp->setStatus((ljrserver::http::HttpStatus)400);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, const char *argv[])
{
    test_request();

    test_response();

    return 0;
}