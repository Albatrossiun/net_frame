#include <src/packet/http_encoder_factory.h>

namespace my_net{

HttpEncoderFactory::HttpEncoderFactory(bool isResponse)
    :_IsResponse(isResponse)
{}

HttpEncoderFactory::~HttpEncoderFactory()
{}

HttpEncoderPtr HttpEncoderFactory::CreatePacketEncoder() {
    HttpEncoder* encoder = new HttpEncoder;
    if(_IsResponse) {
        encoder->SetHttpPacketType(HttpDataType::HDT_RESPONSE);
    } else {
        encoder->SetHttpPacketType(HttpDataType::HDT_REQUEST);
    }
    return HttpEncoderPtr(encoder);
}

}