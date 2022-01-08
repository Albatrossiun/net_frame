#ifndef __my_net_src_packet_http_parser_h__
#define __my_net_src_packet_http_parser_h__
#include <src/common/common.h>
#include <src/common/log.h>
#include <src/packet/http_packet.h>
#include <src/third/http_parser/http_parser.h>

namespace my_net{

// http包的解析器
// 真正进行解析的是开源的代码 在src/net/http_parser目录下
// 此处的HttpParser类仅仅是用来取出开源解析器的解析结果

// 只实现对http的支持 不支持https


// 使用开源库解析Url的函数

// 举例:  http://baidu.com/test/a=b&c=d
// GetHost将得到 baidu.com
// GetUri将i到 /test/a=b&c=d
bool GetHost(const std::string &url, std::string& host);
bool GetUri(const std::string &url, std::string& uri);
// HttpParser继承PacketParser 
// 用来解析http数据
class HttpParser{
public:
    HttpParser();
    ~HttpParser();

    void SetType(HttpDataType type) { _Type = type; }
    HttpDataType GetType() const { return _Type; }

    void SetMaxLenLimit(size_t urlMaxLen, size_t headMaxLen, size_t bodyMaxLen)
    { _UrlMaxLen = urlMaxLen; _HeadMaxLen = headMaxLen; _BodyMaxLen = bodyMaxLen; }

    // 初始化
    bool Init();

    // 解析数据
    // 传入两个参数 
    // buf是数组 是从socket fd中recv到的数据
    // len是buf中数据的长度
    bool Decode(const char* buf, size_t len);
    
    // 收到的数据解析后 会new一个packet用来存储解析后的http数据
    // 调用这个函数获取HttpParser的解析结果
    HttpPacketPtr GetPacket();

    // 下边的函数是为了使用开源的Http解析器而定义的
    // 是一种固定的写法 不用太纠结这些函数
    int OnMessageBegin();
    int OnMessageComplete();
    int OnHeadersComplete();
    int OnUrl(const char* data, size_t len);
    int OnStatus(const char* data, size_t len);
    int OnHeaderField(const char* data, size_t len);
    int OnHeaderValue(const char* data, size_t len);
    int OnBody(const char* data, size_t len);

private:
    // 解析URL的函数 
    // URL示例 http://movie.douban.com/subject/26968242/
    //      http:// 协议
    //      movie.douban.com 豆瓣的网址 通过DNS后被解析成ip:port
    //      subject/26968242/ 豆瓣网上某个帖子的资源路径
    bool ParseUrl(HttpRequestPacketPtr httpRequestPacketPtr);
    // 解析HTTP使用了什么方法
    bool ParseMethod(HttpRequestPacketPtr httpRequestPacketPtr);
    // 解析HTTP用了什么版本
    bool ParseVersion(HttpPacketPtr httpPacketPtr);


    // 类内部使用的httpHead信息 几个常用的数据被单独存储
    struct HeadInfo{
        std::string _Url;    // 如果是request类型的http报文 会携带url
        std::string _Status; // http状态信息
        std::string _Field; // http报文的头部字段名称
        std::string _Value; // http报文的头部字段值
        // 每次解析出上边的一对_Field和_Value会被存到下边这个map里
        std::map<std::string, std::string> _HeaderMap; 

        // 清空上边的所有数据
        void Clear(); 
    };
private:
    // 这两个结构体 来自开源的http解析器
    http_parser _HttpParser;
    http_parser_settings _HttpParserSetting;

    // 对http报文携带数据的最大长度限制
    // 如果为0表示不限制最大长度
    size_t _UrlMaxLen;
    size_t _HeadMaxLen;
    size_t _BodyMaxLen; 

    // 因为http是建立在tcp连接上的
    // tcp是流式的数据
    // 那么如果使用http1.1 同一个连接上 可能会传输多个http报文过来
    // 每解析出一个完整的报文 就要创建一个HttpPacket 顺序记录在list里
    std::list<HttpPacketPtr> _PacketList;
    HttpPacketPtr _CurrentPacket; // 当前正在解析的一个http报文 解析完以后放到上边的list里存起来
    HttpDataType _Type; // 当前正在解析的这个报文的类型 http有两种报文 表示请求的报文和表示应答的报文
    HeadInfo _HeadInfo; // 当前正在解析的这个报文的head部分的数据
    
    bool _Inited; // 表示当前这个解析器是否已经初始化好了
};
typedef std::shared_ptr<HttpParser> HttpParserPtr;

}
#endif // __my_net_src_packet_http_parser_h__
