#include <src/packet/http_encoder.h>

namespace my_net {

HttpEncoder::HttpEncoder()
    :_Type(HttpDataType::HDT_UNKNOW),
     _ChunkedThreshold(1024 * 1024), // 大于1MB的response包将被chunked编码
     _ChunkedSize(1024 * 64) // 每个chunked块的大小128k
{}

HttpEncoder::~HttpEncoder()
{}

bool HttpEncoder::Encode(HttpPacketPtr packetPtr, DataBlobPtr dataBlod) {
    if(packetPtr == NULL || dataBlod == NULL) {
        MY_LOG(ERROR, "%s", "Encode http packet failed, NULL ptr");
        return false;
    }
    if(_Type == HttpDataType::HDT_UNKNOW) {
        MY_LOG(ERROR, "%s", "Encode http packet failed, type unknow");
        return false;
    }
    if(_Type == HttpDataType::HDT_REQUEST) {
        return EncodeRequest(packetPtr, dataBlod);
    } else {
        return EncodeResponse(packetPtr, dataBlod);
    }

    MY_LOG(ERROR, "Encode http packet failed, type [%u]", _Type);
    return false;
}

// http 请求报文例子
/*
POST /cgi-bin/process.cgi HTTP/1.1
User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)
Host: www.tutorialspoint.com
Content-Type: application/x-www-form-urlencoded
Content-Length: length
Accept-Language: en-us
Accept-Encoding: gzip, deflate
Connection: Keep-Alive

licenseID=string&content=string&/paramsXML=string
*/

bool HttpEncoder::EncodeRequest(HttpPacketPtr packetPtr, DataBlobPtr dataBlod) {
    // 类型转换
    const HttpRequestPacket* packet =
        dynamic_cast<const HttpRequestPacket*>(packetPtr.get());

    // 创建字符流
    std::stringstream ss;
    
    // 开始创建出http报文
    HTTP_METHOD method = packet->GetMethod();
    switch(method) {
    case HTTP_METHOD_GET:
        ss << "GET ";
        break;
    case HTTP_METHOD_HEAD:
        ss << "HEAD ";
        break;
    case HTTP_METHOD_POST:
        ss << "POST ";
        break;
    case HTTP_METHOD_PUT:
        ss << "PUT ";
        break;
    case HTTP_METHOD_PATCH:
        ss << "PATCH ";
        break;
    case HTTP_METHOD_DELETE:
        ss << "DELETE ";
        break;
    default:
        MY_LOG(ERROR, "encode http request packet failed, unspport method [%u]",
            method);
        return false;
    };

    const std::string& uri = packet->GetUri();
    if(uri.empty()) {
        MY_LOG(ERROR, "%s", "encode http request packet failed, uri is empty");
        return false;
    }
    ss << uri;

    HTTP_VERSION version = packet->GetHttpVersion();
    switch(version) {
    case HTTP_VERSION::HTTP_1_0:
        ss << " HTTP/1.0\r\n";
        break;
    case HTTP_VERSION::HTTP_1_1:
        ss << " HTTP/1.1\r\n";
        break;
    default:
        MY_LOG(ERROR, "encode http request packet failed, unknow version [%u]",
            version);
        return false;
    }; // 第一行填写完成

    // 开始写head信息
    const std::map<std::string, std::string>& headerMap = packet->GetHeaders(); 
    if(version == HTTP_VERSION::HTTP_1_0) {
    // 如果是http 1.0 当连接是长连接时 需要设置keep alive 的head
    // 因为对方看到你是http 1.0  默认你的keep alive是关闭的 此时应该显示的告诉
    // 接收方 我们需要使用长连接
        if(packet->GetIsKeepAlive()) {
            if(headerMap.find("connection") == headerMap.end()) {
                ss << "Connection: Keep-Alive\r\n";            
            }
        }
    } else if(version == HTTP_VERSION::HTTP_1_1) {
    // 如果是 http 1.1 对方默认我们需要长连接
    // 但是如果此时的连接是短连接 需要明确的告诉对方 关闭长连接
        if(!packet->GetIsKeepAlive()) {
            if(headerMap.find("connection") == headerMap.end()) {
                ss << "Connection: Close\r\n";       
            }
        }
    }
    for(auto it = headerMap.begin(); it != headerMap.end(); it++) {
        ss << it->first << ": " << it->second << "\r\n";
    } 
    // 如果有body数据 需要在head里填写body长度
    const std::string body = packet->GetBody();
    if(!body.empty()) {
        if(headerMap.find("content-length") == headerMap.end()) {
            ss << "content-length: " << body.size() << "\r\n";
        }
    } // head 信息填写完成

    // 填写body前的分隔行
    ss << "\r\n";
    dataBlod->Append(ss.str().c_str(), ss.str().size());
    
    // 填写body数据
    if(!body.empty()) {
        dataBlod->Append(body.c_str(), body.size());
    }
    return true;
}


// http response报文例子
/*
HTTP/1.1 200 OK
Date: Mon, 27 Jul 2009 12:28:53 GMT
Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT
Content-Length: 88
Content-Type: text/html
Connection: Closed
 
<html><p>The requested URL /t.html was not found on this server.</p></html>
 */
bool HttpEncoder::EncodeResponse(HttpPacketPtr packetPtr, DataBlobPtr dataBlod) {
    const HttpResponsePacket* packet =
        dynamic_cast<HttpResponsePacket*>(packetPtr.get());
    // 创建字符流
    std::stringstream ss;

    HTTP_VERSION version = packet->GetHttpVersion();
    switch(version) {
    case HTTP_VERSION::HTTP_1_0:
        ss << "HTTP/1.0 ";
        break;
    case HTTP_VERSION::HTTP_1_1:
        ss << "HTTP/1.1 ";
        break;
    default:
        MY_LOG(ERROR, "encode http response packet failed, unknow version [%u]",
            version);
        return false;
    }; 

    int statusCode = packet->GetStatusCode();
    if(statusCode < 0) {
        MY_LOG(ERROR, "encode http response packet failed, unknow status code [%d]",
            statusCode);
        return false;
    }
    const std::string& statusMsg = packet->GetStatus();
    if(statusMsg.empty()) {
        MY_LOG(ERROR, "%s",
            "encode http response packet failed, status str is empty");
        return false;
    }
    ss << statusCode << " " << statusMsg << "\r\n";
    // 第一行填写完毕
    
    // 开始写head信息
    const std::map<std::string, std::string>& headerMap = packet->GetHeaders(); 
    if(version == HTTP_VERSION::HTTP_1_0) {
    // 如果是http 1.0 当连接是长连接时 需要设置keep alive 的head
    // 因为对方看到你是http 1.0  默认你的keep alive是关闭的 此时应该显示的告诉
    // 接收方 我们需要使用长连接
        if(packet->GetIsKeepAlive()) {
            if(headerMap.find("connection") == headerMap.end()) {
                ss << "Connection: Keep-Alive\r\n";            
            }
        }
    } else if(version == HTTP_VERSION::HTTP_1_1) {
    // 如果是 http 1.1 对方默认我们需要长连接
    // 但是如果此时的连接是短连接 需要明确的告诉对方 关闭长连接
        if(!packet->GetIsKeepAlive()) {
            if(headerMap.find("connection") == headerMap.end()) {
                ss << "Connection: Close\r\n";       
            }
        }
    }
    for(auto it = headerMap.begin(); it != headerMap.end(); it++) {
        ss << it->first << ": " << it->second << "\r\n";
    } 
    // response 无论是否有body信息 都需要明确的标注出body长度 
    bool useChunk = false;
    const std::string body = packet->GetBody();
    if(headerMap.find("content-length") == headerMap.end()) {
        // 用户没有填写body的长度 此时 需要判断是否启动chunked
        // 只有在http1.1的时候 并且是长连接 的时候
        // 用户设置的大于0的_ChunkedSize阈值时
        // 并且要发送的body长度大于阈值的时候
        // 才会启动_Chunked
        if(_ChunkedSize > 0 &&
            body.size() > _ChunkedThreshold && 
            packet->GetHttpVersion() == HTTP_VERSION::HTTP_1_1 &&
            packet->GetIsKeepAlive()) {
            useChunk = true;
            ss << "transfer-encoding: chunked\r\n";
        } else {
            ss << "content-length: " << body.size() << "\r\n";
        }
    }
    // head 信息填写完成
    ss << "\r\n";
    dataBlod->Append(ss.str().c_str(), ss.str().size());
    
    // 填写body数据
    if(!body.empty()) {
        if(!useChunk) {
            dataBlod->Append(body.c_str(), body.size());
        } else {
            assert(body.size() > 0);
            size_t index = 0;
            char cc[5] = {'0','\r', '\n', '\r', '\n'};
            while(index < body.size()) {
                int end = index + _ChunkedSize;
                if((size_t)end > body.size()) {
                    end = body.size();
                }
                int payload = end - index;
                // 每个chunked 要以 size\r\n开始
                std::stringstream sHex;
                sHex << std::hex << payload << "\r\n";
                std::string chunkedPayload(sHex.str());
                dataBlod->Append(chunkedPayload.c_str(), chunkedPayload.size()); 
                dataBlod->Append(body.c_str() + index, payload);
                dataBlod->Append(&(cc[1]), 2);
                index = end;            
            }
            dataBlod->Append(cc, 5);
        }
    }
    return true;
}

}
