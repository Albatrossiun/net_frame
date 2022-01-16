#include <src/handler/base_io_handler.h>
namespace my_net {

BaseIoHandler::BaseIoHandler(EventLoopPtr eventLoopPtr)
    :_State(CONNECT_STATE::CS_NONE),
     _EventLoopPtr(eventLoopPtr),
     _DoCloseAfterSend(false)
{
    _ConnectionTimerPtr.reset(new ConnectionTimer(this));
}

BaseIoHandler::~BaseIoHandler()
{}

bool BaseIoHandler::SendPacket(HttpPacketPtr& packet) {
    std::unique_lock<std::mutex> lock(_Mutex);
    _Mutex.lock();
    bool ok = SendPacketWithoutLock(packet);
    _Mutex.unlock();
    if(ok) {
        packet.reset();
    }
    return ok;
}

bool BaseIoHandler::SendPacketWithoutLock(HttpPacketPtr packet) {
    // 判断状态
    if(_State != CONNECT_STATE::CS_CONNECTING && 
        _State != CONNECT_STATE::CS_CONNECTED) {
        MY_LOG(ERROR, "send packet failed, connectino status is [%u]", _State);
        return false;    
    }

    // 编码packet
    DataBlobPtr dataBlob(new DataBlob);
    if(!_PacketEncoderPtr->Encode(packet, dataBlob)) {
        MY_LOG(ERROR, "%s", "send packet failed, encode failed");
        return false;
    }

    // 如果现在还在等连接建立 就把要发送的数据暂存起来 一会再发
    if(_State == CONNECT_STATE::CS_CONNECTING) {
        _DataBlobPtrList.push_back(dataBlob);
        return true;
    }

    // 如果当前连接已经成功建立了 但是要发送的数据太多 堆积在list里了
    // 这个时候也需要把这个数据放在list里排队
    if(!_DataBlobPtrList.empty()) {
        _DataBlobPtrList.push_back(dataBlob);
        return true;
    }

    // 开始发送数据
    bool ok = true;
    // 一次性可能发不完 循环发
    while(true) {
        size_t rest = dataBlob->RestSize();
        if(rest == 0) {
            // 发送完成了
            break;
        }
        // 发送数据
        int sendNum = _SocketPtr->Write(dataBlob->GetRestData(), rest);
        // 发送时出错了
        if(sendNum < 0) {
            if(errno != EAGAIN) {
                char BUFF[1024] = {0};
                strerror_r(errno, BUFF, 1024);
                MY_LOG(ERROR, "send failed, errno : [%s]", BUFF);
                ok = false;
            }
            break;
        }
        // 发送成功了
        dataBlob->AddOffset(sendNum);
    }

    if(ok) {
        if(dataBlob->RestSize() > 0) {
            _DataBlobPtrList.push_back(dataBlob);
        } else if(_DoCloseAfterSend) { 
            //auto t = std::bind(&BaseIoHandler::ReportErrorAndClose, this, PROCESS_ERROR::PE_CONNECT_CLOSE);
            //PostTask(t);
        }
    }

    return ok;
}

bool BaseIoHandler::Connect(size_t timeout) {
    // 加锁
    std::unique_lock<std::mutex> lock(_Mutex);
    return ConnectWithoutLock(timeout);
}

bool BaseIoHandler::ConnectWithoutLock(size_t timeout) {
    // 检查状态 
    if(_State != CONNECT_STATE::CS_NONE) {
        MY_LOG(ERROR, "connect failed, state is [%u]", _State);
        return false;
    }
    // 检查有没有设置socket
    if(_SocketPtr == NULL) {
        MY_LOG(ERROR, "%s", "connect failed, socketPtr is NULL");
        return false;
    }

    // 开始创建连接
    // 默认创建tcp连接
    TcpClientSocket* socket = dynamic_cast<TcpClientSocket*>(_SocketPtr.get());
    if(socket == NULL) {
        MY_LOG(ERROR, "%s", "convert socket to tcp client socket failed");
        return false;
    }

    // 注册监听事件
    this->_EopllEventType = EPOLLIN | EPOLLOUT;
    if(!_EventLoopPtr->AddIOEvent(this)) {
        MY_LOG(ERROR, "%s", "enable io event failed");
        return false;
    }
   
    TcpClientSocket::TcpSocketConnectType BaseIoHandlerType = 
        socket->Connect();
    MY_LOG(DEBUG, "socket connect ret [%d]", BaseIoHandlerType);
    if(BaseIoHandlerType == TcpClientSocket::TcpSocketConnectType::TSCT_ERROR) {
        socket->Close();
        return false;
    }

    if(BaseIoHandlerType == TcpClientSocket::TcpSocketConnectType::TSCT_CONNECTED) {
        // 成功建立了连接
        _State = CONNECT_STATE::CS_CONNECTED;
    } else {
        // 连接还在进行中
        _State = CONNECT_STATE::CS_CONNECTING;
        // 如果设置了超时时间
        if(timeout != 0) {
            bool ok = _EventLoopPtr->AddTimeEvent(_ConnectionTimerPtr, timeout);
            if(!ok) {
                MY_LOG(ERROR, "set timeItem failed, [%d]", timeout);
                _State = CONNECT_STATE::CS_CLOSED;
                socket->Close();
                return false;
            }
        }
    }
    return true;
}

bool BaseIoHandler::DoRead() 
{
    // 上锁
    _Mutex.lock();
    
    PROCESS_ERROR pe = PROCESS_ERROR::PE_NONE;
    bool userBreak = false;
    // 如果连接还在建立中
    // 检查一下连接是否建立成功
    if(_State == CONNECT_STATE::CS_CONNECTING && !CheckConnectingState()) {
        pe = PROCESS_ERROR::PE_CONNECT_FAILED; 
    }

    char buff[4096];
    while(_State == CONNECT_STATE::CS_CONNECTED) {
        int n = _SocketPtr->Read(buff, sizeof(buff));
        if(n < 0) {
            if(errno != EAGAIN) {
                char BUFF[1024] = {0};
                strerror_r(errno, BUFF, 1024);
                MY_LOG(ERROR, "read failed, errno : [%s]", BUFF);
                pe = PROCESS_ERROR::PE_READ_FAILED;
            }
            break;
        }

        if(!_PacketParserPtr->Decode(buff, n)) {
            pe = PROCESS_ERROR::PE_DECODE_FAILED;
            break;
        }
        while(!userBreak) {
            HttpPacketPtr packet = _PacketParserPtr->GetPacket();
            if(packet == NULL) {
                break;
            }
            _Mutex.unlock();
            if(!_HttpHandler->Process(packet, pe)) {
                userBreak = true;
            } 
            _Mutex.lock();
        }
        if(userBreak) {
            break;
        }
        if(n == 0) {
            pe = PROCESS_ERROR::PE_CONNECT_CLOSE;
            break;
        }
    }

    if(userBreak) {
        CloseWithoutLock(); 
    }
  
    // 解锁
    _Mutex.unlock();
    if(pe != PROCESS_ERROR::PE_NONE) {
        ReportErrorAndClose(pe);
        return false;
    }
    return !userBreak;
}

bool BaseIoHandler::DoWrite()
{
    // 上锁
    _Mutex.lock();
    if(_State == CONNECT_STATE::CS_CONNECTING && !CheckConnectingState()) {
        _Mutex.unlock();
        ReportErrorAndClose(PROCESS_ERROR::PE_CONNECT_FAILED);
        return false;
    }
    if(_State != CONNECT_STATE::CS_CONNECTED) {
        _Mutex.unlock();
        return false;
    }
    // 没有需要发送的数据
    if(_DataBlobPtrList.empty()) {
        _Mutex.unlock();
        return false;
    }

    // 逐个发送
    bool error = false;
    bool writeable = true;
    auto it = _DataBlobPtrList.begin();
    while(it!=_DataBlobPtrList.end()) {
        DataBlobPtr p = *it;
        while(true) {
            size_t len = p->RestSize();
            if(len == 0) {
                it = _DataBlobPtrList.erase(it);        
                break;
            }
            int n = _SocketPtr->Write(p->GetRestData(), len);
            // 写失败
            if(n < 0) {
                if(errno != EAGAIN) {
                    // 发生错误
                    error = true;
                } else {
                    // 缓存区写满了
                    writeable = false;
                }
                break;
            }
            p->AddOffset(n);
        }
        if(!writeable || error) {
            break;
        }
    }

    PROCESS_ERROR pe = PROCESS_ERROR::PE_NONE;
    if(error) {
        pe = PROCESS_ERROR::PE_WRITE_FAILED;
    } else if(writeable) {
        if(_DoCloseAfterSend) {
            pe = PROCESS_ERROR::PE_DECODE_FAILED;
        }
    }

    _Mutex.unlock();
    if(pe != PROCESS_ERROR::PE_NONE) {
        ReportErrorAndClose(pe);
    }
    return true;
}

bool BaseIoHandler::IsConnected()
{
    std::unique_lock<std::mutex> lock(_Mutex);
    return _State == CONNECT_STATE::CS_CONNECTED;
}

bool BaseIoHandler::IsClosed()
{
    std::unique_lock<std::mutex> lock(_Mutex);
    return _State != CONNECT_STATE::CS_CONNECTING && _State != CONNECT_STATE::CS_CONNECTED;
}

void BaseIoHandler::ReportErrorAndClose(PROCESS_ERROR pe) {
    _Mutex.lock();
    _State = CONNECT_STATE::CS_CONNECTING;
    _Mutex.unlock();
    _HttpHandler->Process(NULL, pe);
    Close();
}

void BaseIoHandler::Close()
{
    _Mutex.lock();
    CloseWithoutLock();
    _Mutex.unlock();
}

void BaseIoHandler::CloseWithoutLock() {
    if (_State != CONNECT_STATE::CS_CLOSED) {
        _State = CONNECT_STATE::CS_CLOSED;
        if(_SocketPtr != NULL) {
            _SocketPtr->Close();
        }
        if(_ConnectionTimerPtr->GetTime() > 0) {
            _EventLoopPtr->DeleteTimeEvent(_ConnectionTimerPtr);
        }
    }
}

bool BaseIoHandler::AddTimeItem(TimeItemPtr timeItemPtr, int timeout) {
    return _EventLoopPtr->AddTimeEvent(timeItemPtr, timeout);
}

bool BaseIoHandler::DeleteTimeItem(TimeItemPtr timeItemPtr) {
    return _EventLoopPtr->DeleteTimeEvent(timeItemPtr);
}

void BaseIoHandler::TimeOutProcess()
{
    ReportErrorAndClose(PROCESS_ERROR::PE_CONNECT_TIMEOUT);
}

bool BaseIoHandler::CheckConnectingState()
{
    if(_State != CONNECT_STATE::CS_CONNECTING) {
        return false;
    }
    int code = 0;
    if(!_SocketPtr->GetSoError(&code) || code != 0) {
        MY_LOG(DEBUG, "error is [%d]", code);
        return false;
    }
    _State = CONNECT_STATE::CS_CONNECTED;
    // 删除超时时间
    if(_ConnectionTimerPtr->GetTime() > 0) {
        _EventLoopPtr->DeleteTimeEvent(_ConnectionTimerPtr);
    }
    return true;
}



void BaseIoHandler::Destroy() {
    Close();
    //delete this;
    // TODO 增加引用计数
}


}