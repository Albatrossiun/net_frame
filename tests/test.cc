#include <src/common/common.h>
#include <src/common/log.h>
#include <iostream>

#include <tests/test_thread_pool.h>

using namespace std;

typedef bool(*TestFuncP)();

bool DoTest(TestFuncP func, string name) {
    MY_LOG(INFO, "---TestName [%s]---", name.c_str());    
    bool ok = (*func)();
    MY_LOG(INFO, "---End [%s]---\n\n", name.c_str());
    return ok;
}

int main() {
    map<string, TestFuncP> TestList;
    TestList["Test ThreadPool"] = &TestThreadPool;
    for(auto it = TestList.begin(); it != TestList.end(); it++) {
        if(!DoTest(it->second, it->first)) {
            return -1;
        }    
    }
    return 0;
}
