
#include "../ljrServer/http/http_parser.h"
#include "../ljrServer/log.h"

// 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

// 请求数据
const char test_request_data[] =
    "POST / HTTP/1.1\r\n"
    "Host: www.baidu.com\r\n"
    "Content-length: 10\r\n\r\n"
    "123456789011";

/**
 * @brief 测试解析 http 请求
 *
 */
void test_request() {
    // http 请求解析器
    ljrserver::http::HttpRequestParser parser;
    // 请求数据
    std::string tmp = test_request_data;
    // 解析数据
    size_t s = parser.execute(&tmp[0], tmp.size());

    LJRSERVER_LOG_INFO(g_logger)
        << "execute rt=" << s << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished() << " total=" << tmp.size()
        << " content-length=" << parser.getContentLength();

    tmp.resize(tmp.size() - s);

    // 解析结果
    LJRSERVER_LOG_INFO(g_logger)
        << "http 请求头部解析结果: " << std::endl
        << parser.getData()->toString() << "头部解析完成...";
    LJRSERVER_LOG_INFO(g_logger) << "请求体如下:" << std::endl << tmp;
}

// const char test_response_data[] = "HTTP/1.0 200 OK\r\n"
//                                   "Accept-Ranges: bytes\r\n"
//                                   "Cache-Control: no-cache\r\n"
//                                   "Content-Length: 14615\r\n"
//                                   "Content-Type: text/html\r\n"
//                                   "Date: Mon, 03 May 2021 14:43:11 GMT\r\n"
//                                   "Pragma: no-cache\r\n"
//                                   "Server: BWS/1.1\r\n";

// 响应数据
const char test_response_data[] =
    "HTTP/1.1 200 OK\r\n"
    "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
    "Server: Apache\r\n"
    "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
    "ETag: \"51-47cf7e6ee8400\"\r\n"
    "Accept-Ranges: bytes\r\n"
    "Content-Length: 81\r\n"
    "Cache-Control: max-age=86400\r\n"
    "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
    "Connection: Close\r\n"
    "Content-Type: text/html\r\n\r\n"
    "<html>\r\n"
    "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
    "</html>\r\n";

/**
 * @brief 测试解析 http 响应
 *
 */
void test_response() {
    // http 响应解析器
    ljrserver::http::HttpResponseParser parser;
    // 响应数据
    std::string tmp = test_response_data;
    // 解析数据
    size_t s = parser.execute(&tmp[0], tmp.size(), false);

    LJRSERVER_LOG_INFO(g_logger)
        << "execute rt=" << s << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished() << " total=" << tmp.size()
        << " content-length=" << parser.getContentLength()
        << " tmp[s]=" << tmp[s];

    tmp.resize(tmp.size() - s);

    // 解析结果
    LJRSERVER_LOG_INFO(g_logger)
        << "http 响应头部解析结果: " << std::endl
        << parser.getData()->toString() << "头部解析完成...";
    LJRSERVER_LOG_INFO(g_logger) << "响应体如下:" << std::endl
                                 << tmp << "解析完成";
}

/**
 * @brief 测试
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char const *argv[]) {
    // 测试解析 http 请求
    test_request();

    // 测试解析 http 响应
    test_response();

    return 0;
}
