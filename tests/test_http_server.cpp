
#include "../ljrServer/http/http_server.h"
#include "../ljrServer/log.h"

static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_ROOT();

void run()
{
    ljrserver::http::HttpServer::ptr server(new ljrserver::http::HttpServer);

    ljrserver::Address::ptr addr = ljrserver::Address::LookupAnyIPAdress("0.0.0.0:8023");

    while (!server->bind(addr))
    {
        sleep(2);
    }

    auto sd = server->getServletDispatch();
    sd->addServlet("/lijianran/xx", [](ljrserver::http::HttpRequest::ptr req,
                                       ljrserver::http::HttpResponse::ptr rsp,
                                       ljrserver::http::HttpSession::ptr session) {
        rsp->setBody(req->toString());
        return 0;
    });

    sd->addGlobServlet("/lijianran/*", [](ljrserver::http::HttpRequest::ptr req,
                                          ljrserver::http::HttpResponse::ptr rsp,
                                          ljrserver::http::HttpSession::ptr session) {
        rsp->setBody("Glob:\r\n"+req->toString());
        return 0;
    });

    server->start();
}

int main(int argc, char const *argv[])
{
    ljrserver::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
