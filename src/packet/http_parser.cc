#include <src/packet/http_parser.h>
namespace my_net{

// 为了使用开源的http解析器 我们需要封装c函数
// 因为开源的http解析器只提供c语言接口
// 所谓的c函数 也就是不能是类或结构体的成员函数 不能使用引用传递参数 不能使用模板
bool GetHost(const std::string &url, std::string& host) {
    host.clear();
    http_parser_url u;
    if (http_parser_parse_url(url.c_str(), url.length(), 0, &u) != 0) {
        return false;
    }
    if ((u.field_set & (1 << UF_HOST)) != 0) {
        host.append(url.c_str() + u.field_data[UF_HOST].off, 
                       u.field_data[UF_HOST].len);
    }
    if ((u.field_set & (1 << UF_PORT)) != 0) {
        host.append(":");
        host.append(url.c_str() + u.field_data[UF_PORT].off, 
                        u.field_data[UF_PORT].len);
    }
    return true;
}

bool GetUri(const std::string &url, std::string& uri) {
    uri.clear();
    http_parser_url u;
    if (http_parser_parse_url(url.c_str(), url.length(), 0, &u) != 0) {
        return false;
    }
    if ((u.field_set & (1 << UF_PATH)) != 0) {
        uri.append(url.c_str() + u.field_data[UF_PATH].off);
    } else {
        uri.append("/");
    }
    return true;
}

int OnMessageBegin(http_parser* parser)
{
    HttpParser* decoder = 
        reinterpret_cast<HttpParser*>(parser->data);
    return decoder->OnMessageBegin();
}

int OnMessageComplete(http_parser* parser)
{
    HttpParser* decoder = 
        reinterpret_cast<HttpParser*>(parser->data);
    return decoder->OnMessageComplete();
}

int OnHeadersComplete(http_parser* parser)
{
    HttpParser* decoder = 
        reinterpret_cast<HttpParser*>(parser->data);
    return decoder->OnHeadersComplete();
}

int OnUrl(http_parser* parser, const char* data, size_t len)
{
    HttpParser* decoder = 
        reinterpret_cast<HttpParser*>(parser->data);
    return decoder->OnUrl(data, len);
}

int OnStatus(http_parser* parser, const char* data, size_t len)
{
    HttpParser* decoder = 
        reinterpret_cast<HttpParser*>(parser->data);
    return decoder->OnStatus(data, len);
}

int OnHeaderField(http_parser* parser, const char* data, size_t len)
{
    HttpParser* decoder = 
        reinterpret_cast<HttpParser*>(parser->data);
    return decoder->OnHeaderField(data, len);
}

int OnHeaderValue(http_parser* parser, const char* data, size_t len)
{
    HttpParser* decoder = 
        reinterpret_cast<HttpParser*>(parser->data);
    return decoder->OnHeaderValue(data, len);
}

int OnBody(http_parser* parser, const char* data, size_t len)
{
    HttpParser* decoder = 
        reinterpret_cast<HttpParser*>(parser->data);
    return decoder->OnBody(data, len);
}


HttpParser::HttpParser()
    :_UrlMaxLen(1024 * 4),  // 4kb
     _HeadMaxLen(1024 * 8), // 8kb
     _BodyMaxLen(1024 * 1024), // 1mb
     _Type(HttpDataType::HDT_UNKNOW),
     _Inited(false)
{
    // 初始化开源的http解析器的设置
    http_parser_settings_init(&_HttpParserSetting);
    _HttpParserSetting.on_message_begin = my_net::OnMessageBegin;
    _HttpParserSetting.on_url = my_net::OnUrl;
    _HttpParserSetting.on_status = my_net::OnStatus;
    _HttpParserSetting.on_header_field = my_net::OnHeaderField;
    _HttpParserSetting.on_header_value = my_net::OnHeaderValue;
    _HttpParserSetting.on_body = my_net::OnBody;
    _HttpParserSetting.on_headers_complete = my_net::OnHeadersComplete;
    _HttpParserSetting.on_message_complete = my_net::OnMessageComplete;
}

HttpParser::~HttpParser()
{}

// 初始化
bool HttpParser::Init() {
    if(_Type == HttpDataType::HDT_UNKNOW) {
        MY_LOG(ERROR, "%s", "init failed, http data type not set");
        return false;
    }
    // 初始化开源解析器
    http_parser_init(&_HttpParser, 
                     _Type==HttpDataType::HDT_REQUEST ? HTTP_REQUEST : HTTP_RESPONSE);
    _HttpParser.data = this;
    _Inited = true;
    return true;
}

bool HttpParser::Decode(const char* buf, size_t len) {
    if(!_Inited) {
        MY_LOG(ERROR, "%s", "decode failed, parser not inited");
        return false;
    }
    size_t parsed = http_parser_execute(&_HttpParser, 
                                        &_HttpParserSetting,
                                        buf,
                                        len);
    // 解析失败 
    if(parsed != len ||
       _HttpParser.http_errno != HPE_OK) {
        MY_LOG(ERROR, "parse http failed, [%s], [%s], [%d]",
                     http_errno_description(HTTP_PARSER_ERRNO(&_HttpParser)),
                     http_errno_name(HTTP_PARSER_ERRNO(&_HttpParser)),
                     _Type);
        return false;
    }
    return true;
}

HttpPacketPtr HttpParser::GetPacket() {
    // 取出最早解析出的包返回
    if(_PacketList.empty()) {
        return NULL;
    }
    HttpPacketPtr packet = _PacketList.front();
    _PacketList.pop_front();
    return packet;
}


int HttpParser::OnMessageBegin()
{
    assert(_Type == HttpDataType::HDT_REQUEST || _Type == HttpDataType::HDT_RESPONSE);
    _HeadInfo.Clear(); 
    return 0;
}

int HttpParser::OnMessageComplete()
{
    assert(_CurrentPacket != NULL);
    _CurrentPacket->SetIsKeepAlive(http_should_keep_alive(&_HttpParser) != 0);
    _PacketList.push_back(_CurrentPacket);
    _CurrentPacket.reset();
    return 0;
}

int HttpParser::OnHeadersComplete()
{
    std::string& field = _HeadInfo._Field;
    const std::string& value = _HeadInfo._Value;
    if (!field.empty()) {
        std::transform(field.begin(), field.end(), field.begin(), tolower);
        _HeadInfo._HeaderMap[field] = value;
    }

    if (_Type == HttpDataType::HDT_REQUEST) {
        if (_HeadInfo._Url.empty()) {
            MY_LOG(ERROR, "%s", "packet do not have url");
            return -1;
        }

        HttpRequestPacketPtr packet(new HttpRequestPacket);
        if (packet == NULL) {
            MY_LOG(ERROR, "%s", "create http Request packet failed");
            return -1;
        }

        if (!ParseUrl(packet)) {
            MY_LOG(ERROR, "parse url [%s] failed", _HeadInfo._Url.c_str());
            return -1;
        }

        if (!ParseMethod(packet)) {
            MY_LOG(ERROR, "parse method [%d] failed", _HttpParser.method);
            return -1;
        }

        _CurrentPacket = packet;
    } else if (_Type == HttpDataType::HDT_RESPONSE) {
        HttpResponsePacketPtr packet(new HttpResponsePacket);
        if (packet == NULL) {
            MY_LOG(ERROR, "%s", "create http response packet failed");
            return -1;
        }
        packet->SetStatus(_HeadInfo._Status);
        packet->SetStatusCode(_HttpParser.status_code);
        _CurrentPacket = packet;
    }

    assert(_CurrentPacket != NULL);
    if (!ParseVersion(_CurrentPacket)) {
        MY_LOG(ERROR, "parse version faied, [%d][%d]", _HttpParser.http_major, _HttpParser.http_minor);
        _CurrentPacket.reset();
        return -1;
    }
    _CurrentPacket->_HeaderMap.swap(_HeadInfo._HeaderMap);
    _HeadInfo.Clear();
    return 0;
}

int HttpParser::OnUrl(const char* data, size_t len)
{
    assert(_Type == HttpDataType::HDT_REQUEST);
    std::string& url = _HeadInfo._Url;
    if (_UrlMaxLen != 0 
        && _UrlMaxLen < len + url.size()) {
        MY_LOG(ERROR, "url longer than max len [%d][%d]",
                        _UrlMaxLen, len + url.size());
        return -1;
    }
    url.append(data, len);
    return 0;
}

int HttpParser::OnStatus(const char* data, size_t len)
{
    assert(_Type == HttpDataType::HDT_RESPONSE);
    _HeadInfo._Status.append(data, len);
    return 0;
}

int HttpParser::OnHeaderField(const char* data, size_t len)
{
    std::string& field = _HeadInfo._Field;
    std::string& value = _HeadInfo._Value;
    
    if (!value.empty()) {
        std::transform(field.begin(), field.end(), field.begin(), tolower);
        _HeadInfo._HeaderMap[field] = value;
        field.clear();
        value.clear();
    }

    field.append(data, len);
    return 0;
}

int HttpParser::OnHeaderValue(const char* data, size_t len)
{
    _HeadInfo._Value.append(data, len);
    return 0;
}

int HttpParser::OnBody(const char* data, size_t len)
{
    assert(_CurrentPacket != NULL);
    size_t bodySize = _CurrentPacket->GetBody().size();
    if (_BodyMaxLen != 0 
        && _BodyMaxLen < len + bodySize) {
        MY_LOG(ERROR, "body longer than max len [%d][%d]",
                    _BodyMaxLen, len + bodySize);
        return -1;
    }
    _CurrentPacket->AppendBody(data, len);
    return 0;
}

bool HttpParser::ParseUrl(HttpRequestPacketPtr packet) {
    std::string uri;
    // 对url进行解析 得到uri部分 保存到packet里
    if(GetUri(_HeadInfo._Url, uri)) {
        packet->SetUri(uri);
    } else {
        // 如果解析失败了 那这个url可能不是标准类型的url  直接原样保存到uri里
        packet->SetUri(_HeadInfo._Url);
    }
    return true;
}

// 将开源解析器定义的HTTP方法对应到我们定义的方法上
bool HttpParser::ParseMethod(HttpRequestPacketPtr packet) {
    HTTP_METHOD method;
    switch (_HttpParser.method) {
    case HTTP_GET:
        method = HTTP_METHOD::HTTP_METHOD_GET;
        break;
    case HTTP_HEAD:
        method = HTTP_METHOD::HTTP_METHOD_HEAD;
        break;
    case HTTP_POST:
        method = HTTP_METHOD::HTTP_METHOD_POST;
        break;
    case HTTP_PUT:
        method = HTTP_METHOD::HTTP_METHOD_PUT;
        break;
    case HTTP_CONNECT:
        method = HTTP_METHOD::HTTP_METHOD_CONNECT;
        break;
    case HTTP_PATCH:
        method = HTTP_METHOD::HTTP_METHOD_PATCH;
        break;
    case HTTP_DELETE:
        method = HTTP_METHOD::HTTP_METHOD_DELETE;
        break;
    default:
        method = HTTP_METHOD::HTTP_METHOD_OTHER;
        break;
    }
    packet->SetMethod(method);
    return true;
}

bool HttpParser::ParseVersion(HttpPacketPtr packet) {
    if(_HttpParser.http_major == 1) {
        if(_HttpParser.http_minor == 0) {
            _CurrentPacket->SetHttpVersion(HTTP_VERSION::HTTP_1_0);
            return true;
        } else if(_HttpParser.http_minor == 1) {
            _CurrentPacket->SetHttpVersion(HTTP_VERSION::HTTP_1_1);
            return true;
        } 
    }
    MY_LOG(ERROR, "parse http version failed, get [%d.%d]",
                    _HttpParser.http_major,
                    _HttpParser.http_minor);
    return false;
}

void HttpParser::HeadInfo::Clear() {
    _Url.clear();
    _Status.clear();
    _Field.clear();
    _Value.clear();
    _HeaderMap.clear();
}

}