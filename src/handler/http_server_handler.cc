#include <src/handler/http_server_handler.h>
namespace my_net{

HttpKeepAliveTimeItem::HttpKeepAliveTimeItem(HttpServerHandler& handler)
    :_Handler(handler)
{}

HttpKeepAliveTimeItem::~HttpKeepAliveTimeItem()
{}

void HttpKeepAliveTimeItem::Process() {
    _Handler.OnKeepAliveTimeout();
}

HttpIdleTimeItem::HttpIdleTimeItem(HttpServerHandler& handler)
    :_Handler(handler)
{}

HttpIdleTimeItem::~HttpIdleTimeItem()
{}

void HttpIdleTimeItem::Process() {
    _Handler.OnIdleTimeout();
}

HttpServerHandler::HttpServerHandler(MyNetUserHandlerPtr httpPacketHandler)
    :_MyNetUserHandlerPtr(httpPacketHandler),
     _KeepAliveTimeout(0)
{
    _HttpKeepAliveTimeItemPtr.reset(new HttpKeepAliveTimeItem(*this));
    _HttpIdleTimeItemPtr.reset(new HttpIdleTimeItem(*this));
}

HttpServerHandler::~HttpServerHandler()
{}

bool HttpServerHandler::Reply(HttpResponsePacketPtr httpResponsePacket) {
    std::unique_lock<std::mutex> lock(_BaseIOHandler->GetMutex());
    if(!_KeepAlive) {
        _BaseIOHandler->SetCloseAfterAllPacketSent(true);
    }
  
    size_t sn = httpResponsePacket->GetSerialNumber();
    // 还没有轮到发送这个packet
    if(_SendNo != sn) {
        _ResponsePacketPtrMap[sn] = httpResponsePacket;
        return true;
    }

    // 调用Connection发送这个包
    if(!_BaseIOHandler->SendPacketWithoutLock(httpResponsePacket)) {
        MY_LOG(ERROR, "%s", "send packet failed");
        return false;
    }

    // 发送这个包之后 要把后续可以发送的包都发出去
    _SendNo++;
    bool ok = true;
    while(!_ResponsePacketPtrMap.empty()) {
        auto it = _ResponsePacketPtrMap.begin();
        // 已经没有可以顺序发送的包了
        if(it->first != _SendNo) {
            break;
        }

        HttpResponsePacketPtr packet = it->second;
        _ResponsePacketPtrMap.erase(it);
        MY_LOG(DEBUG, "[%d]", _ResponsePacketPtrMap.size());

        ok = _BaseIOHandler->SendPacketWithoutLock(packet);
        if(!ok) {
            MY_LOG(ERROR, "send packet [%d] failed", _SendNo);
            break;
        }
        _SendNo++;
        MY_LOG(DEBUG, "[%d]", _SendNo);
    }
    return ok;
}

void HttpServerHandler::OnKeepAliveTimeout() {
    HttpPacketPtr tmp(NULL);
    MY_LOG(DEBUG, "%s", "TIME OUT");
    _MyNetUserHandlerPtr->Process(*this,
                                tmp,
                                BaseIoHandler::PROCESS_ERROR::PE_HTTP_KEEPALIVE_TIMEOUT);
    _BaseIOHandler->Close();
}

void HttpServerHandler::OnIdleTimeout() {
    HttpPacketPtr tmp(NULL);
    MY_LOG(DEBUG, "%s", "IDLE TIME OUT");
    _MyNetUserHandlerPtr->Process(*this,
                                tmp,
                                BaseIoHandler::PROCESS_ERROR::PE_HTTP_IDLE_TIMEOUT);
    _BaseIOHandler->Close();
}

bool HttpServerHandler::Process(HttpPacketPtr packet, int error) {
    auto& mutex = _BaseIOHandler->GetMutex();
    auto eventLoopPtr = _BaseIOHandler->GetEventLoop();

    mutex.lock();
    if(_HttpKeepAliveTimeItemPtr->GetTime() > 0) {
        eventLoopPtr->DeleteTimeEvent(_HttpKeepAliveTimeItemPtr);
    }

    if(_HttpIdleTimeItemPtr->GetTime() > 0) {
        eventLoopPtr->DeleteTimeEvent(_HttpIdleTimeItemPtr);
    }

    if(error != BaseIoHandler::PROCESS_ERROR::PE_NONE) {
        mutex.unlock();
        _MyNetUserHandlerPtr->Process(*this, packet, error);
        return false;
    }
    
    HttpRequestPacket* requestPacket = dynamic_cast<HttpRequestPacket*>(packet.get());
    requestPacket->SetSerialNumber(_RecvNo);
    _RecvNo++;
    _KeepAlive = requestPacket->GetIsKeepAlive();
    mutex.unlock();

    bool ok = _MyNetUserHandlerPtr->Process(*this, packet, error);
    if(ok) {
        if(_KeepAlive && _KeepAliveTimeout != 0) {
            eventLoopPtr->AddTimeEvent(_HttpKeepAliveTimeItemPtr, _KeepAliveTimeout);
        }
    }
    return ok;
}

bool HttpServerHandler::AddIdleTimeout(size_t time) {
    auto eventLoopPtr = _BaseIOHandler->GetEventLoop();
    return eventLoopPtr->AddTimeEvent(_HttpIdleTimeItemPtr, time);
}

}
