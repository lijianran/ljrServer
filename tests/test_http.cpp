
#include "../ljrServer/http/http.h"
#include "../ljrServer/log.h"

/**
 * @brief 测试输出 http 请求
 * 
 */
void test_request() {
    ljrserver::http::HttpRequest::ptr req(new ljrserver::http::HttpRequest);
    req->setHeader("host", "www.baidu.com");
    req->setBody("hello baidu");

    req->dump(std::cout) << std::endl;
}

/**
 * @brief 测试输出 http 响应
 * 
 */
void test_response() {
    ljrserver::http::HttpResponse::ptr rsp(new ljrserver::http::HttpResponse);
    rsp->setHeader("X-X", "lijianran");
    rsp->setBody("hello lijianran");
    rsp->setStatus((ljrserver::http::HttpStatus)400);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

/**
 * @brief 测试
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, const char *argv[]) {
    // 测试输出 http 请求
    test_request();

    // 测试输出 http 响应
    test_response();

    return 0;
}