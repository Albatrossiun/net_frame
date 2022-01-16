#ifndef __my_net_src_my_net_my_net_user_handler_h__
#define __my_net_src_my_net_my_net_user_handler_h__

#include <src/common/common.h>
namespace my_net {

class HttpPacket;
class HttpHandler;

class MyNetUserHandler {
public:
    MyNetUserHandler();
    virtual ~MyNetUserHandler();

    // do your code in this function
    virtual bool Process(HttpHandler& handler, std::shared_ptr<HttpPacket>& packetPtr, int error) = 0;
};
typedef std::shared_ptr<MyNetUserHandler> MyNetUserHandlerPtr;
}
#endif // __my_net_src_my_net_my_net_user_handler_h__