#ifndef __my_net_src_util_datablob_h__
#define __my_net_src_util_datablob_h__

#include <src/common/common.h>

namespace my_net {

class DataBlob {
public:
    DataBlob()
        :_Offset(0)
    {}
    ~DataBlob()
    {}
    bool Empty() const { return _Data.empty(); }
    const std::vector<char>& GetData() const {
        return _Data;
    }
    const void* GetRestData() const {
        if(_Data.empty()) {
            return NULL;
        }
        return &_Data[_Offset];
    }
    size_t Size() {
        return _Data.size();
    }
    size_t RestSize() {
        return _Data.size() - _Offset;
    }
    size_t AddOffset(size_t n) {
        _Offset += n;
        return _Offset;
    }
    void Append(const void* data, size_t len);

private:
    size_t _Offset;
    std::vector<char> _Data;
};
typedef std::shared_ptr<DataBlob> DataBlobPtr;

}

#endif // __my_net_src_util_datablob_h__
