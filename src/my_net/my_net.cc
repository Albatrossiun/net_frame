#include <src/my_net/my_net.h>
#include <src/util/url_tools/addr_parser.h>
namespace my_net{

namespace {
TcpClientSocketPtr CreateTcpClientSocket(const std::string& remoteSpec, 
                                         const std::string& localSpec) {
    in_addr_t remoteAddr = 0;
    uint16_t remotePort = 0;
    in_addr_t localAddr = 0;
    uint16_t localPort = 0;
    int proto = -1;
    if(!AddrParser::ParseUrl(remoteSpec, proto, remoteAddr, remotePort)) {
        MY_LOG(ERROR, "create tcp client socket faild, parser remote addr [%s] failed",
            remoteSpec.c_str());
        return NULL;
    }
    
    if(proto != AddrParser::PROTO::HTTP) {
        MY_LOG(ERROR, "%s", "onle support http protoco.");
        return NULL;
    }

    if(!localSpec.empty() && !AddrParser::ParseUrl(localSpec, proto, localAddr, localPort)) {
        MY_LOG(ERROR, "create tcp client socket fialde, parser local addr [%s] failed",
            localSpec.c_str());
        return NULL;
    }

    if(proto != AddrParser::PROTO::HTTP) {
        MY_LOG(ERROR, "%s", "onle support http protoco.");
        return NULL;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        char BUFF[1024] = {0};
        strerror_r(errno, BUFF, 1024);
        MY_LOG(ERROR, "create tcp socket fd failed, error : [%s]", BUFF);
        return NULL;
    }

    TcpClientSocketPtr tcpClientSocket(new TcpClientSocket(localAddr, localPort,
                                                           remoteAddr, remotePort));
    tcpClientSocket->SetFd(fd);
    return tcpClientSocket;
}
};

MyNet::MyNet()
    :_ThreadPool("transportTP", 0),
     _Running(false)
{}

MyNet::~MyNet()
{
    Stop();
}

bool MyNet::Init(const MyNetConfig& config) {
    if(config._ThreadNum == 0) {
        MY_LOG(ERROR, "thread num can't be [%d]", config._ThreadNum);
        return false;
    }
    _ThreadNum = config._ThreadNum;
    _Config = config;
    if(!_ThreadPool.SetThreadNum(_ThreadNum)) {
        MY_LOG(ERROR, "%s", "set thread num failed");
        return false;
    }
    // 初始化所有的EventLoop
    for(size_t i = 0; i < _ThreadNum; i++) {
        EventLoopPtr ptr(new EventLoop());
        if(!ptr->Init()) {
            MY_LOG(ERROR, "init transport failed, init No.%u EventLoop failed", i);
            return false;
        }
        _EventLoopPtrVec.push_back(ptr);
    }

    _PacketEncoderFactoryPtr.reset(new HttpEncoderFactory(true));
    _PacketParserFactoryPtr.reset(new HttpParserFactory(false));
    return true;
}

bool MyNet::Start() {
    if(_Running) {
        MY_LOG(ERROR, "%s", "transport is running, can not start again.");
        return false;
    }

    _Running = true;
    // 启动线程池 每个线程执行一个EventLoop
    // MY_LOG(DEBUG, "%s", "transport thread pool starting...");
    _ThreadPool.Start();
    for(size_t i = 0; i < _ThreadNum; i++) {
        // 因为每个EventLoop都是Runnable的子类
        // 所以可以直接把EventLoop的指针交给线程池 
        // 线程池会执行EventLoop的Run函数
        //  MY_LOG(DEBUG, "%s", "add One Eventloop");
        _ThreadPool.AddTask(RunnablePtr(_EventLoopPtrVec[i]));
    }
    // MY_LOG(DEBUG, "%s", "start all event loop success");
    return true;
}

bool MyNet::Stop() {
    if(!_Running) {
        return true;
    }

    // 依次让每个EventLoop停下
    for(auto ptr : _EventLoopPtrVec) {
        ptr->Stop();
    }
    // 每个EventLoop停下后 停下线程池
    _ThreadPool.Stop();

    _Running = false;
    return true;
}

EventLoopPtr MyNet::GetOneEventLoop() {
    size_t index = _EventLoopIndex % _ThreadNum;
    _EventLoopIndex += 1;
    return _EventLoopPtrVec[index];
}

HttpClientHandlerPtr MyNet::CreateConnection(const std::string& remoteSpec,
                             const std::string& localSpec,
                             size_t timeout) {
    // 创建一个socket
    TcpClientSocketPtr socket = CreateTcpClientSocket(remoteSpec, localSpec);
    if(socket == NULL) {
        MY_LOG(ERROR, "%s", "create connection failed");
        return NULL;
    }
    if(!socket->SetNonBlocking(true)) {
        MY_LOG(ERROR, "%s", "set no block failed");
        return NULL;
    }
    if(_Config._NoDelay) {
        socket->SetTcpNoDelay(true);
    }
    if(_Config._SoLinger) {
        socket->SetSoLinger(true, 1);
    }
    if(_Config._CloseOnExec) {
        socket->SetCloseExec(true);
    }
    if(_Config._RecvBuffSize != 0) {
        socket->SetSoRcvBuf(_Config._RecvBuffSize);
    }
    if(_Config._SendBuffSize != 0) {
        socket->SetSoSndBuf(_Config._SendBuffSize);
    }

    HttpClientHandlerPtr httpClientHandlerPtr(new HttpClientHandler);

    // 创建连接
    BaseIoHandler* baseIoHandler = new BaseIoHandler(GetOneEventLoop());
    baseIoHandler->SetSocket(socket);
    baseIoHandler->SetPacketHandler(httpClientHandlerPtr);
    httpClientHandlerPtr->SetBaseIOHandler(baseIoHandler);

    HttpParserPtr parser = _PacketParserFactoryPtr->CreatePacketParser();
    if(parser == NULL) {
        MY_LOG(ERROR, "%s", "create packet parser failed");
        delete baseIoHandler;
        return NULL;
    }
    baseIoHandler->SetPacketParser(parser);
    HttpEncoderPtr encoder = _PacketEncoderFactoryPtr->CreatePacketEncoder();
    if(encoder == NULL) {
        MY_LOG(ERROR, "%s", " create pakcet encoder failed");
        delete baseIoHandler;
        return NULL;
    }
    baseIoHandler->SetPacketEncoder(encoder);
    
    if(!baseIoHandler->Connect(timeout)) {
        MY_LOG(ERROR, "connect to [%s] failed", remoteSpec.c_str());
        delete baseIoHandler;
        return NULL;
    }
    return httpClientHandlerPtr;
}

AcceptorHandler* MyNet::CreateAcceptor() {
    AcceptorHandler* acceptor =  new AcceptorHandler(
        GetOneEventLoop(),
        GetEventLoopPtrVec());
    return acceptor;
}

bool MyNet::StartServer(std::string ipAndPort,
        MyNetUserHandlerPtr handler) {
        // 创建ListenSocket
    in_addr_t localAddr = 0;
    uint16_t localPort = 0;
    int proto = -1;
    AddrParser::ParseUrl(ipAndPort, proto, localAddr, localPort);
    MY_LOG(DEBUG, "local port is [%u]", localPort);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    TcpListenSocketPtr ls(new TcpListenSocket(localAddr, localPort));
    ls->SetFd(fd);    
    int enable = 1;
    ls->SetOption(SO_REUSEADDR, &enable, sizeof(enable));
    ls->Bind();
    ls->SetNonBlocking(true);
    ls->SetCloseExec(true);
    ls->Listen();

    auto acceptor = new AcceptorHandler(
        GetOneEventLoop(),
        GetEventLoopPtrVec());
    acceptor->SetListenSocket(ls);
    AcceptorHandler::AcceptorConfig acceptorConfig; 
    acceptorConfig._PacketHandler = handler;
    MY_LOG(DEBUG, "[%x]", acceptorConfig._PacketHandler.get());
    acceptorConfig._KeepAliveTimeout = 0;
    acceptorConfig._PacketEncoderFactoryPtr = _PacketEncoderFactoryPtr;
    acceptorConfig._PacketParserFactoryPtr = _PacketParserFactoryPtr;
    acceptor->Start(acceptorConfig);
    Start();
    return true;
}
}
