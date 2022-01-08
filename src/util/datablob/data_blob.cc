#include <src/util/datablob/data_blob.h>

namespace my_net{

void DataBlob::Append(const void* data, size_t len) {
    if(len == 0) {
        return;
    }
    size_t n = _Data.size() + len;
    _Data.reserve(n);
    for(size_t i = 0; i < len; i++) {
        _Data.push_back(((char*)data)[i]);
    }
}
}

