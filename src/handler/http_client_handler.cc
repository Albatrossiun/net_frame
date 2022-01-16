#include <src/handler/http_client_handler.h>
namespace my_net{

HttpClientRequestTimeItem::HttpClientRequestTimeItem(HttpClientHandler& handler)
    :_handler(handler)
{}

HttpClientRequestTimeItem::~HttpClientRequestTimeItem()
{}

void HttpClientRequestTimeItem::Process() {
    _handler.OnTimeOut();
}

HttpClientHandler::OneRequest::OneRequest(HttpClientHandler& h1, MyNetUserHandlerPtr h2)
    :_Handler(h2)
{
    _Timer.reset(new HttpClientRequestTimeItem(h1));
}

HttpClientHandler::OneRequest::~OneRequest()
{}

// 同步请求使用的handler
namespace {
class RequestPacketHandler : public MyNetUserHandlerPtr {
public:
    RequestPacketHandler()
        :_Error(0)
    {}
    ~RequestPacketHandler() {
        _PacketPtr.reset(); 
    }

public:
    bool Wait(HttpPacketPtr& packet, int& error) {
        std::unique_lock<std::mutex> lck(_Mutex);
        while(_PacketPtr == NULL && _Error == 0) {
            _ConditionVariable.wait(lck);
        }
        packet = _PacketPtr;
        error = _Error;
        _PacketPtr.reset();
        _Error = 0;
        return true;
    }

    virtual bool Process(HttpHandler* handler, HttpPacketPtr& packet, int error) {
        std::unique_lock<std::mutex> lck(_Mutex);
        _PacketPtr = packet;
        _Error = error;
        _ConditionVariable.notify_all();
        return true;
    }
private:
    std::mutex _Mutex;
    std::condition_variable _ConditionVariable;
    HttpPacketPtr _PacketPtr;
    int _Error;
};
};

HttpClientHandler::HttpClientHandler()
{}

HttpClientHandler::~HttpClientHandler()
{}

bool HttpClientHandler::Process(HttpPacketPtr packet, int error) {
    auto& mutex = _BaseIOHandler->GetMutex();
    auto eventLoopPtr = _BaseIOHandler->GetEventLoop();
    std::unique_lock<std::mutex> lock(mutex);

    // 处理错误
    if(error != BaseIoHandler::PROCESS_ERROR::PE_NONE) {
        if(_RequestLists.empty()) {
            return false;
        } 
        while(!_RequestLists.empty()) {
            auto oneRequest = _RequestLists.front();
            _RequestLists.pop_front();
            if(oneRequest->_Timer->GetTime() > 0) {
                eventLoopPtr->DeleteTimeEvent(oneRequest->_Timer);
            }
            lock.unlock();
            oneRequest->_Handler->Process(*this, packet, error);
            lock.lock();
        }
        return false;
    }

    if(_RequestLists.empty()) {
        MY_LOG(ERROR, "%s", "get response but no oneRequest, close connection");
        _BaseIOHandler->CloseWithoutLock();
        return false;
    }

    HttpPacket* httpPacket = dynamic_cast<HttpPacket*>(packet.get());
    SetKeepAlive(httpPacket->GetIsKeepAlive()); 

    OneRequestPtr oneRequest = _RequestLists.front();
    _RequestLists.pop_front();
    if(oneRequest->_Timer->GetTime() > 0) {
        eventLoopPtr->DeleteTimeEvent(oneRequest->_Timer);
    }

    lock.unlock();
    return oneRequest->_Handler->Process(*this, packet, error);
}

bool HttpClientHandler::Request(HttpRequestPacketPtr requestPacket,
                              int timeout,
                              HttpPacketPtr& responsePacketPtr,
                              int& error)
{
    auto& mutex = _BaseIOHandler->GetMutex();
    auto eventLoopPtr = _BaseIOHandler->GetEventLoop();

    RequestPacketHandler handler;
    {
        std::unique_lock<std::mutex> lock(mutex);
        OneRequestPtr oneRequest(new OneRequest(*this, handler));
        if(timeout > 0 && !eventLoopPtr->AddTimeEvent(oneRequest->_Timer, timeout)) {
            MY_LOG(ERROR, "request failed, set [%d] timeout failed", timeout);
            return false;
        }
        if(!_BaseIOHandler->SendPacketWithoutLock(requestPacket)) {
            MY_LOG(ERROR, "%s", "send packet faild");
            if(timeout > 0) {
                eventLoopPtr->DeleteTimeEvent(oneRequest->_Timer);
            }
            return false;
        }

        _RequestLists.push_back(oneRequest);
    }
    if(!handler.Wait(responsePacketPtr, error)) {
        return false;
    }
    return true;
}


bool HttpClientHandler::RequestAsync(HttpRequestPacketPtr requestPacket,
                 int timeout,
                 MyNetUserHandlerPtr& handler)
{
    auto& mutex = _BaseIOHandler->GetMutex();
    auto eventLoopPtr = _BaseIOHandler->GetEventLoop();
    {
        std::unique_lock<std::mutex> lock(mutex);
        OneRequestPtr oneRequest(new OneRequest(*this, handler));
        if(timeout > 0 && !eventLoopPtr->AddTimeEvent(oneRequest->_Timer, timeout)) {
            MY_LOG(ERROR, "request failed, set [%d] timeout failed", timeout);
            return false;
        }
        if(!_BaseIOHandler->SendPacketWithoutLock(requestPacket)) {
            MY_LOG(ERROR, "%s", "send packet faild");
            if(timeout > 0) {
                eventLoopPtr->DeleteTimeEvent(oneRequest->_Timer);
            }
            return false;
        }
        _RequestLists.push_back(oneRequest);

    }
    return true;
}

void HttpClientHandler::OnTimeOut() {
    auto& mutex = _BaseIOHandler->GetMutex();
    auto eventLoopPtr = _BaseIOHandler->GetEventLoop();
    mutex.lock();
    HttpPacketPtr NULLp;
    while(!_RequestLists.empty()) {
        auto oneRequest = _RequestLists.front();
        _RequestLists.pop_front();
        if(oneRequest->_Timer->GetTime() > 0) {
            eventLoopPtr->DeleteTimeEvent(oneRequest->_Timer);
        }
        mutex.unlock();
        oneRequest->_Handler->Process(*this, NULLp, BaseIoHandler::PROCESS_ERROR::PE_PACKET_TIMEOUT);
        mutex.lock();
    }
}

}
