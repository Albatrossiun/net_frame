#ifndef __my_net_src_packet_http_encoder_factory_h__
#define __my_net_src_packet_http_encoder_factory_h__

#include <src/common/common.h>
#include <src/packet/http_encoder.h>


namespace my_net {

class HttpEncoderFactory{
public:
    HttpEncoderFactory(bool isResponse);
    virtual ~HttpEncoderFactory();
    HttpEncoderPtr CreatePacketEncoder();
private:
    bool _IsResponse;
}; 
typedef std::shared_ptr<HttpEncoderFactory> HttpEncoderFactoryPtr;

}

#endif // __my_net_src_packet_http_encoder_factory_h__
