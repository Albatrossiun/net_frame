#ifndef __my_net_src_packet_http_encoder_h__
#define __my_net_src_packet_http_encoder_h__

#include <src/common/common.h>
#include <src/common/log.h>
#include <src/packet/http_packet.h>
#include <src/util/datablob/data_blob.h>

namespace my_net {
class HttpEncoder{
public:
    HttpEncoder();
    virtual ~HttpEncoder();
    virtual bool Encode(HttpPacketPtr packetPtr, DataBlobPtr dataBlod);
    void SetHttpPacketType(HttpDataType type) {_Type = type;}

    void SetChunkedThreshold(size_t threshold) { _ChunkedThreshold = threshold; }
    size_t GetChunkedThreshold() { return _ChunkedThreshold; }

    void SetChunkedSize(size_t size) { _ChunkedSize = size; }
    size_t GetChunkedSize() { return _ChunkedSize; }

private:
    bool EncodeRequest(HttpPacketPtr packetPtr, DataBlobPtr dataBlod);
    bool EncodeResponse(HttpPacketPtr packetPtr, DataBlobPtr dataBlod);
private:
    HttpDataType _Type; 
    size_t _ChunkedThreshold;
    size_t _ChunkedSize;
};
typedef std::shared_ptr<HttpEncoder> HttpEncoderPtr;

}

#endif // __my_net_src_packet_http_encoder_h__
