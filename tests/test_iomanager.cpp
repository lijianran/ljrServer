
#include "../ljrServer/ljrserver.h"
#include "../ljrServer/iomanager.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <iostream>

ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

int sock = 0;

void test_fiber()
{
    LJRSERVER_LOG_INFO(g_logger) << "test_fiber";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "182.61.200.7", &addr.sin_addr.s_addr);

    if (!connect(sock, (const sockaddr *)&addr, sizeof(addr)))
    {
    }
    else if (errno == EINPROGRESS)
    {
        LJRSERVER_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);

        ljrserver::IOManager::GetThis()->addEvent(sock, ljrserver::IOManager::READ, []() {
            LJRSERVER_LOG_INFO(g_logger) << "read callback";
        });
        ljrserver::IOManager::GetThis()->addEvent(sock, ljrserver::IOManager::WRITE, []() {
            LJRSERVER_LOG_INFO(g_logger) << "write callback";
            // close(sock);

            ljrserver::IOManager::GetThis()->cancelEvent(sock, ljrserver::IOManager::READ);
            close(sock);
        });
    }
    else
    {
        LJRSERVER_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test1()
{
    std::cout << "EPOLLIN = " << EPOLLIN << " EPOLLOUT = " << EPOLLOUT << std::endl;
    ljrserver::IOManager iom(2, false, "test");
    iom.schedule(&test_fiber);
}

ljrserver::Timer::ptr s_timer;
void test_timer()
{
    ljrserver::IOManager iom(2, false, "test");
    // iom.addTimer(500, [](){
    //     LJRSERVER_LOG_INFO(g_logger) << "hello timer!";
    // }, true);
    s_timer = iom.addTimer(
        1000, []() {
            static int i = 0;
            LJRSERVER_LOG_INFO(g_logger) << "hello timer! i = " << i;
            if (++i == 3)
            {
                s_timer->reset(2000, true);
                // s_timer->cancel();
            }
        },
        true);
}

int main(int argc, char const *argv[])
{
    // test1();
    test_timer();
    return 0;
}
