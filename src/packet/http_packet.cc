#include <src/packet/http_packet.h>

namespace my_net {

// 匿名名字空间
// 这里边定义的变量不会和其他文件中定义的变量冲突
// 因为这里边定义的变量的作用域只有这个文件内
namespace{
    std::map<int, std::string> CODE_TO_STRING = {
        {200, "OK"},
        {201, "Created"},
        {202, "Accepted"},
        {204, "No Content"},
        {206, "Partial Content"},
        {301, "Moved Permanently"},
        {302, "Moved Temporarily"},
        {303, "See Other"},
        {304, "Not Modified"},
        {307, "Temporary Redirect"},
        {307, "Temporary Redirect"},
        {400, "Bad Request"},
        {401, "Temporary Redirect"},
        {402, "Payment Required"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Not Allowed"},
        {406, "Not Acceptable"},
        {408, "Request Time-out"},
        {409, "Conflict"},
        {410, "Gone"},
        {411, "Length Required"},
        {412, "Precondition Failed"},
        {413, "Request Entity Too Large"},
        {414, "Request-URI Too Large"},
        {415, "Unsupported Media Type"},
        {416, "Requested Range Not Satisfiable"},
        {500, "Internal Server Error"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {504, "Gateway Timeout"},
    };
};

HttpPacket::HttpPacket()
    :_IsKeepAlive(false),
     _Version(HTTP_VERSION::HTTP_1_0),
     _SerialNumber(0)
{}

HttpPacket::~HttpPacket()
{}


void HttpPacket::SetHttpVersion(HTTP_VERSION version) {
    _Version = version;
    // http 1.1版本支持长连接
    // 默认是长连接
    if(version == HTTP_VERSION::HTTP_1_1) {
        _IsKeepAlive = true;
    }
}

bool HttpPacket::GetHeader(const std::string& key, std::string& value) {
    // 先统一转换成小写string
    std::string lKey(key);
    std::transform(lKey.begin(), lKey.end(), lKey.begin(), tolower);
    auto it = _HeaderMap.find(lKey);
    if(it == _HeaderMap.end()) {
        return false;
    }
    value = it->second;
    return true;
}

bool HttpPacket::AddHeader(const std::string& key, const std::string& value) {
    if(key.empty()) {
        MY_LOG(ERROR, "%s", "key is empty");
        return false;
    }
    std::string lKey(key);
    std::transform(lKey.begin(), lKey.end(), lKey.begin(), tolower);
    _HeaderMap[lKey] = value; 
    return true;
}

void HttpPacket::AppendBody(const void* data, size_t len) {
    if(len == 0 || data == NULL) {
        return;
    }
    _Body += std::string((const char*)data, len);
}


void HttpResponsePacket::SetStatusCode(int code) {
    _StatusCode = code;    
    auto it = CODE_TO_STRING.find(code);
    if(it != CODE_TO_STRING.end()) {
        _Status = it->second;
    } else {
        _Status = "UNKNOW";
    }
}   
}
