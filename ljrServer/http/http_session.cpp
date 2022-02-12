
#include "http_session.h"
// http 报文解析
#include "http_parser.h"

namespace ljrserver {

namespace http {

/**
 * @brief 构造 http session 会话
 *
 * @param sock socket 句柄对象
 * @param owner 是否持有 [= true]
 */
HttpSession::HttpSession(Socket::ptr sock, bool owner)
    : SocketStream(sock, owner) {
    // 初始化构造 SocketStream 对象
}

/**
 * @brief 服务端接收请求
 *
 * @return HttpRequest::ptr
 */
HttpRequest::ptr HttpSession::recvRequest() {
    // http 请求解析器
    HttpRequestParser::ptr parser(new HttpRequestParser);
    // 获取约定好的配置属性 http 请求缓存大小
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();

    // uint64_t buff_size = 100;
    // 创建缓存
    std::shared_ptr<char> buffer(new char[buff_size], [](char *ptr) {
        // 自定义析构方法
        delete[] ptr;
    });

    // 裸指针
    char *data = buffer.get();
    // 缓存偏移
    int offset = 0;

    do {
        // 通过 SocketStream::read 读取
        int len = read(data + offset, buff_size - offset);
        if (len <= 0) {
            // 读取失败
            close();
            return nullptr;
        }

        len += offset;
        // 解析客户端发送来的 http 请求报文
        size_t nparse = parser->execute(data, len);
        if (parser->hasError()) {
            // 解析有错误
            close();
            return nullptr;
        }

        // 请求体存入缓存
        offset = len - nparse;
        if (offset == (int)buff_size) {
            // 缓存已经满了
            close();
            return nullptr;
        }

        if (parser->isFinished()) {
            // 解析已经完成
            break;
        }

    } while (true);

    // 获取头部的 content-length
    int64_t length = parser->getContentLength();
    if (length > 0) {
        // 请求体缓存
        std::string body;
        body.resize(length);

        int len = 0;
        if (length >= offset) {
            // body.append(data, offset);
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            // body.append(data, length);
            memcpy(&body[0], data, length);
            len = length;
        }

        length -= offset;
        if (length > 0) {
            // 还没读取完毕 读取完剩下的内容到 body 缓存
            if (readFixSize(&body[len], length) <= 0) {
                // 读取失败
                close();
                return nullptr;
            }
        }

        // 设置 http 请求对象的内容体
        parser->getData()->setBody(body);
    }

    // 返回 http 请求对象
    return parser->getData();
}

/**
 * @brief 服务端通过 socket stream 发送响应
 *
 * @param rsp http 响应对象
 * @return int
 */
int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss;
    // 输出 http 响应报文
    ss << *rsp;
    std::string data = ss.str();
    //  通过继承方法 Stream::writeFixSize 传输 http 响应报文
    return writeFixSize(data.c_str(), data.size());
}

}  // namespace http

}  // namespace ljrserver
