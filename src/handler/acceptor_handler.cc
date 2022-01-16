#include <src/handler/acceptor_handler.h>
#include <src/socket/tcp_socket.h>

namespace my_net {


AcceptorHandler::AcceptorHandler(EventLoopPtr eventLoop, 
    const std::vector<EventLoopPtr>& eventLoopVec)
    :_Stop(true),
     _AcceptEventLoop(eventLoop),
     _EventLoopVec(eventLoopVec),
     _NextIndex(0)
{}

AcceptorHandler::~AcceptorHandler()
{}

bool AcceptorHandler::Start(const AcceptorConfig& config)
{
    std::unique_lock<std::mutex> lock(_Mutex);
    if(config._PacketEncoderFactoryPtr == NULL or
        config._PacketParserFactorPtr == NULL or
        _Socket == NULL) {
        MY_LOG(ERROR, "start failed, get null ptr, "
        "encoder_factory : [%x]"
        " ,parser_factory : [%x]"
        " ,socket : [%s]",
        config._PacketEncoderFactoryPtr.get(),
        config._PacketParserFactorPtr.get(),
        _Socket.get());    
        return false;
    }

    if(!_Stop) {
        MY_LOG(ERROR, "%s", "start faild, already started");
        return false;
    }
    
    _Config = config;
    // 往EventLoop里注册listenSocket 开始等待连接
    // MY_LOG(DEBUG, "将监听套接字[%d]注册到epoll里", _Socket->GetFd());
    _EopllEventType = EPOLLIN | EPOLLOUT;
    bool ok = _AcceptEventLoop->AddIOEvent(this);
    if(!ok) {
        MY_LOG(ERROR, "start failed, add [%d] to event_loop failed",
                _Socket->GetFd());
        _Config._PacketParserFactorPtr.reset();
        _Config._PacketEncoderFactoryPtr.reset();
        return false;
    }
    
    _Stop = false;
    return true;
}

bool AcceptorHandler::Stop() {
    std::unique_lock<std::mutex> lock(_Mutex);
    if(_Stop) {
        MY_LOG(ERROR, "%s", "stop failed, already stoped");
        return false;
    }

    if(_Socket == NULL) {
        MY_LOG(ERROR, "%s", "stop failed, socket is NULL");
        return false;
    }
    _EopllEventType = 0;
    bool ok = _AcceptEventLoop->ModifyIOEvent(this);
    if(!ok) {
        MY_LOG(ERROR, "stop fialed, remove io event of [%d] failed", _Socket->GetFd());
        return false;
    }
    _Stop = true;
    return true;
}

bool AcceptorHandler::SetListenSocket(SocketPtr socket) {
    std::unique_lock<std::mutex> lock(_Mutex);
    if(_Socket != NULL) {
        MY_LOG(ERROR, "%s", "set listen socket failed, socket already seted");
        return false;
    }
    _Socket = socket;
    return true;
}

SocketPtr AcceptorHandler::GetListenSocket()  const {
    std::unique_lock<std::mutex> lock(_Mutex);
    return _Socket;
}

EventLoopPtr AcceptorHandler::GetNextEventLoop() const {
    std::unique_lock<std::mutex> lock(_Mutex);
    return GetNextEventLoop();
}


EventLoopPtr AcceptorHandler::GetNextEventLoopWithOutLock() const {
    return _EventLoopVec[_NextIndex++ % _EventLoopVec.size()];
}

bool AcceptorHandler::DoRead() {
    // MY_LOG(DEBUG, "%s", "acceptor on read");
    std::unique_lock<std::mutex> lock(_Mutex);
    TcpListenSocket* tcpListenSocket = dynamic_cast<TcpListenSocket*>(_Socket.get());
    if(tcpListenSocket == NULL) {
        MY_LOG(ERROR, "%s", "convert socket to tcp listen socket failed, get NULL ptr");
        return false;
    }
    // 我们使用的是epoll边缘触发模式
    // 因此, 当listen socket可读时, 需要进行循环accept
    while(!_Stop) {
        TcpServerSocketPtr newSocket(NULL);
        // 全部都accept完了 可以退出循环了
        if(!tcpListenSocket->Accept(newSocket)) {
            break;
        }
        if(!newSocket->SetNonBlocking(true)) {
            MY_LOG(ERROR, "%s", "accept one connection failed, set no block failed");
            newSocket.reset();
            continue;
        }
        if(_Config._NoDelay) {
            newSocket->SetTcpNoDelay(true);
        }
        if(_Config._SoLinger) {
            newSocket->SetSoLinger(true, 1);
        }
        if(_Config._RecvBuffSize != 0) {
            newSocket->SetSoRcvBuf(_Config._RecvBuffSize);
        }
        if(_Config._SendBuffSize != 0) {
            newSocket->SetSoSndBuf(_Config._SendBuffSize);
        }


        HttpServerHandlerPtr httpServerHandler(new HttpServerHandler(_Config._PacketHandler));
        if(_Config._KeepAliveTimeout > 0) {
            httpServerHandler->SetKeepAliveTimeout(_Config._KeepAliveTimeout);
        }


        EventLoopPtr nextEventLoop = GetNextEventLoopWithOutLock();
        BaseIoHandler* baseIoHandler = new BaseIoHandler(nextEventLoop);
        
        httpServerHandler->SetBaseIOHandler(baseIoHandler);
        
        baseIoHandler->SetSocket(newSocket);

        baseIoHandler->_State = BaseIoHandler::CONNECT_STATE::CS_CONNECTED;
        baseIoHandler->SetPacketHandler(httpServerHandler);
        baseIoHandler->SetPacketParser(_Config._PacketParserFactorPtr->CreatePacketParser());
        baseIoHandler->SetPacketEncoder(_Config._PacketEncoderFactoryPtr->CreatePacketEncoder());
        if(baseIoHandler->_PacketParserPtr == NULL || baseIoHandler->_PacketEncoderPtr == NULL) {
            MY_LOG(ERROR, "%s", "cerate connection faild, new packet encoder or parser faild");
            delete baseIoHandler;
            continue;
        } 


        baseIoHandler->_EopllEventType = EPOLLIN | EPOLLOUT;
        nextEventLoop->AddIOEvent(baseIoHandler);
    }
    return true;
}



bool AcceptorHandler::DoWrite()
{
    return false;
}

void AcceptorHandler::Destroy() {
    delete this;
}

}
