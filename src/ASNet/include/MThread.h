#ifndef __MTHREAD_H__
#define __MTHREAD_H__

#include <list>
#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>

typedef std::function<void()> HandlerFunc;

class MThread 
{
public:
    MThread();
    ~MThread();

    int Start(int threadNum = 1);
    int Stop();
    int Post(HandlerFunc handler);
    int Dispatch(HandlerFunc handler);

private:
    void Worker();
    
private:
    int m_nThreadNum; 
    
    std::atomic<bool> m_bStart;
    std::list<HandlerFunc> m_listHandlerFunc;
    std::condition_variable m_cvHandlerFunc;
    std::mutex m_mutexHandlerFunc;
    std::list<std::thread*> m_listThread;
} ;

#endif
