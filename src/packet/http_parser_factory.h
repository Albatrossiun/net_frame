#ifndef __my_net_src_packet_http_parser_factor_h__
#define __my_net_src_packet_http_parser_factor_h__

#include <src/common/common.h>
#include <src/packet/http_parser.h>

namespace my_net {

class HttpParserFactory{
public:
    HttpParserFactory(bool isResponse,
                      size_t urlMaxLen = 0,
                      size_t headMaxLen = 0,
                      size_t bodyMaxLen = 0);
    virtual ~HttpParserFactory();

    HttpParserPtr CreatePacketParser();

private:
    bool _IsResponse;
    size_t _UrlMaxLen;
    size_t _HeadMaxLen;
    size_t _BodyMaxLen; 
};
typedef std::shared_ptr<HttpParserFactory> HttpParserFactoryPtr;

}

#endif // __my_net_src_packet_http_parser_factor_h__
