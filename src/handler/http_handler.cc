#include <src/handler/http_handler.h>

namespace my_net {

HttpHandler::HttpHandler()
    :_BaseIOHandler(NULL),
     _KeepAlive(false),
     _RecvNo(0),
     _SendNo(0)
{}

HttpHandler::~HttpHandler()
{
}

void HttpHandler::Close() {
    _BaseIOHandler->Close();
}

bool HttpHandler::IsClosed() const {
    if(_BaseIOHandler != NULL) {
        return _BaseIOHandler->IsClosed();
    }
    return true;
}

void HttpHandler::Destroy() {
    if(_BaseIOHandler != NULL) {
        _BaseIOHandler->Destroy();
        _BaseIOHandler = NULL;
    }
    delete this;
}

}
