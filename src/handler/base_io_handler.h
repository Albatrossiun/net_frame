#ifndef __my_net_src_handler_base_io_handler_h__
#define __my_net_src_handler_base_io_handler_h__
#include <src/io_loop/io_handler.h>
#include <src/io_loop/timer.h>
#include <src/io_loop/epoll.h>
#include <src/io_loop/event_loop.h>
#include <src/socket/tcp_socket.h>

namespace my_net{

class BaseIoHandler : public IOHandler {
public:
    class ConnectionTimer : public TimeItem {
    public:
        ConnectionTimer(Connection* conn)
            :_Conn(conn)
        {}
        void Process() {
            _Conn->TimeOutProcess();
        }
    private:
        Connection* _Conn;    
    };

    friend class ConnectionTimer;

    enum CONNECTION_STATE {
        CS_NONE,
        CS_CONNECTING, // 正在建立连接
        CS_CONNECTED,  // 成功建立的连接
        CS_CLOSING,    // 正在关闭
        CS_CLOSED      // 已经关闭
    };

    // 处理连接过程中各种可能的错误类型
    enum PROCESS_ERROR {
        PE_NONE, // 没有错误
        PE_CONNECT_FAILED, // 连接失败
        PE_CONNECT_CLOSE, // 连接被关闭
        PE_READ_FAILED, // 读取错误
        PE_WRITE_FAILED, // 写入失败
        PE_DECODE_FAILED, // 解析数据包失败
        PE_CONNECT_TIMEOUT, // 超时
        PE_PACKET_TIMEOUT, // 等待数据包超时
        PE_HTTP_KEEPALIVE_TIMEOUT, // Http长连接超时
        PE_HTTP_IDLE_TIMEOUT, // Http空闲超时
    };
protected:
    BaseIoHandler(EventLoopPtr eventLoopPtr);
    virtual ~BaseIoHandler();

    EventLoopPtr GetEventLoop() const { return _EventLoopPtr; }

    SocketPtr GetSocket() const { return _SocketPtr; }
    void SetSocket(SocketPtr socketPtr) { _SocketPtr = socketPtr; }

    UserConnection* GetUserConnection() const { return _UserConnectionP; } 
    void SetUserConnection(UserConnection* userConnectionP);
    void ClearUserConnection() { _UserConnectionP = NULL; }

    void SetPacketParser(PacketParserPtr packetParserPtr) { _PacketParserPtr = packetParserPtr; }
    PacketParserPtr GetPacketParser() const { return _PacketParserPtr; }

    void SetPacketEncoder(PacketEncoderPtr packetEncoderPtr) { _PacketEncoderPtr = packetEncoderPtr; }
    PacketEncoderPtr GetPacketEncoder() const { return _PacketEncoderPtr; }

    std::mutex& GetMutex() { return _Mutex; }

    // 设置处理packet的句柄函数
    void SetPacketHandler(PacketHandler& packetHandler) { _PakcetHandler = packetHandler; }

    // 发送数据的函数
    virtual bool SendPacket(PacketPtr& packet);
    virtual bool SendPacketWithoutLock(PacketPtr packet);

    // 建立连接
    virtual bool Connect(size_t timeout);
    // 关闭连接
    virtual void Close();
    virtual void CloseWithoutLock();

    // 获取当前是否处于连接状态
    virtual bool IsConnected();    
    virtual bool IsClosed();

    // 实现父类的虚函数OnRead
    virtual bool OnRead();
    // 实现父类的虚函数OnWrite
    virtual void OnWrite();

    // 向_EventLoop中加入超时事件
    bool AddTimeItem(TimeItemPtr timeItem, int timeout);
    // 从_EventLoop中删除超时事件
    bool DeleteTimeItem(TimeItemPtr timeItem);

    // 处理超时事件
    void TimeOutProcess();
  
    // 检查正在连接的socket 是否完成了连接
    bool CheckConnectingState();

    // 是否在所有的包发送完成后 关闭连接
    void SetCloseAfterAllPacketSent(bool close) { _CloseAfterSendFinish = close; }
    
    void ReportErrorAndClose(PROCESS_ERROR pe);

    // 增加'可写'事件处理句柄函数
    void AddWriteableHandler(WriteableHandler& handler);
    void AddWriteableHandlerWithOutLock(WriteableHandler&);

    void NotifyWriteable(bool inLock);

    void Destroy();
    bool ConnectWithoutLock(size_t timeout);
protected:
    volatile CONNECTION_STATE _State;
    EventLoopPtr _EventLoopPtr;
    SocketPtr _SocketPtr;
    UserConnection* _UserConnectionP;
    PacketParserPtr _PacketParserPtr;
    PacketEncoderPtr _PacketEncoderPtr;
    std::shared_ptr<ConnectionTimer> _ConnectionTimerPtr;
    // 原子变量 int 不需要加锁就可以多线程操作的变量
    std::atomic<std::uint64_t> _BytesRecv;
    std::atomic<std::uint64_t> _BytesSend;

    std::list<DataBlobPtr> _DataBlobPtrList;
    std::list<WriteableHandler> _WriteableHandlerList;

    // 用户注册的 用来处理packet的句柄函数
    PacketHandler _PakcetHandler; 

    std::mutex _Mutex;
    bool _CloseAfterSendFinish;
};


}
#endif