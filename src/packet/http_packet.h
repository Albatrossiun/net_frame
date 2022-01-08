#ifndef __my_net_src_oacket_http_packet_h__
#define __my_net_src_oacket_http_packet_h__
#include <src/common/common.h>
#include <src/common/log.h>

namespace my_net {

// 封装HTTP包进行封装
// 协议参考https://www.w3.org/Protocols/rfc2616/rfc2616.html

enum HttpDataType {
    HDT_UNKNOW,    // 未知
    HDT_REQUEST,   // 是接受到的请求数据 
    HDT_RESPONSE   // 是准备发回的响应数据
};


// 定义HTTP版本
enum HTTP_VERSION {
    HTTP_1_0, // http 1.0
    HTTP_1_1  // http 1.1 
};

// 定义HTTP方法
// 方法参考 https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods
enum HTTP_METHOD {
    HTTP_METHOD_NULL, // 没有指定方法
    HTTP_METHOD_GET,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_CONNECT, 
    HTTP_METHOD_PATCH, 
    HTTP_METHOD_DELETE,
    HTTP_METHOD_OTHER // 其他的方法
};

class HttpPacket{
    friend class HttpParser;
public:
    HttpPacket();
    virtual ~HttpPacket();

    HTTP_VERSION GetHttpVersion() const { return _Version; }
    void SetHttpVersion(HTTP_VERSION version);

    // 获取Http数据报的head数据
    bool GetHeader(const std::string& key, std::string& value);
    // 添加Http数据报的head
    bool AddHeader(const std::string& key, const std::string& value);
    // 获取所有的head数据
    const std::map<std::string, std::string>& GetHeaders() const { return _HeaderMap; }
    // 批量设置head数据
    bool SetHeaders(const std::map<std::string, std::string>& headers);
    // 清除所有的head数据
    void ClearHeaders() { _HeaderMap.clear(); }
    // 获取Http的body数据
    const std::string& GetBody() const { return _Body; }
    // 追加Body数据
    void AppendBody(const void* data, size_t len);
    // 获取序列号
    uint32_t GetSerialNumber() const { return _SerialNumber; }
    // 设置序列号
    void SetSerialNumber(uint32_t number) { _SerialNumber = number; }
    // 获取是否是长连接
    bool GetIsKeepAlive() const { return _IsKeepAlive; }
    // 设置是否为长连接
    void SetIsKeepAlive(bool isKeepAlive) { _IsKeepAlive = isKeepAlive; } 
protected:
    bool _IsKeepAlive; 
    HTTP_VERSION _Version;
    uint32_t _SerialNumber;
    std::map<std::string, std::string> _HeaderMap;
    std::string _Body;
}; 
typedef std::shared_ptr<HttpPacket> HttpPacketPtr;

// 请求类型的HttpPacket
class HttpRequestPacket : public HttpPacket {
public:
    HttpRequestPacket()
        :_HttpMethod(HTTP_METHOD_NULL)
    {}
    ~HttpRequestPacket()
    {}

    HTTP_METHOD GetMethod() const { return _HttpMethod; }
    void SetMethod(HTTP_METHOD method) { _HttpMethod = method; }
    const std::string& GetUri() const { return _Uri; }
    void SetUri(const std::string& uri) {_Uri = uri; }
    bool GetHost(std::string& host) {
        auto it = _HeaderMap.find("host");
        if(it == _HeaderMap.end()) {
            return false;
        }
        host = it->second;
        return true;
    }
    bool SetHost(const std::string& host) {
        // 如果已经设置过了 不能重复设置
        if(_HeaderMap.find("host") != _HeaderMap.end()) {
            return false;
        }
        _HeaderMap["host"] = host;
        return true;
    }
private:
    // 对于请求类型的Packet 应该有一个uri来表示要访问的资源 or 接口
    std::string _Uri;
    // 对于请求类型的Packet 应该有一个对应的http方法
    HTTP_METHOD _HttpMethod;
};
typedef std::shared_ptr<HttpRequestPacket> HttpRequestPacketPtr;

// 回复类型的HttpPacket
class HttpResponsePacket : public HttpPacket {
public:
    HttpResponsePacket()
        :_StatusCode(-1)
    {}
    ~HttpResponsePacket()
    {}

    int GetStatusCode() const { return _StatusCode; }
    void SetStatusCode(int code);

    const std::string& GetStatus() const { return _Status; }
    void SetStatus(const std::string& status) { _Status = status; }
private:
    // 状态
    std::string _Status;
    // 状态码 400 500 200 之类的数字
    int _StatusCode;
};
typedef std::shared_ptr<HttpResponsePacket> HttpResponsePacketPtr;

}

#endif //__my_net_src_oacket_http_packet_h__
