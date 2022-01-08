#include <src/packet/http_parser_factory.h>
namespace my_net {

HttpParserFactory::HttpParserFactory(bool isResponse,
                      size_t urlMaxLen,
                      size_t headMaxLen,
                      size_t bodyMaxLen)
    :_IsResponse(isResponse),
     _UrlMaxLen(urlMaxLen),
     _HeadMaxLen(headMaxLen),
     _BodyMaxLen(bodyMaxLen)
{}

HttpParserFactory::~HttpParserFactory()
{}

HttpParserPtr HttpParserFactory::CreatePacketParser() {
    HttpParser* parser = new HttpParser;
    parser->SetMaxLenLimit(_UrlMaxLen,
                           _HeadMaxLen,
                           _BodyMaxLen);
    if(_IsResponse) {
        parser->SetType(HttpDataType::HDT_RESPONSE); 
    } else {
        parser->SetType(HttpDataType::HDT_REQUEST);
    }
    if(!parser->Init()) {
        delete parser;
        return NULL;
    }
    return HttpParserPtr(parser);
}

}
