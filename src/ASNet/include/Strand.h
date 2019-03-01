#ifndef __STRAND_H__
#define __STRAND_H__

#include "MThread.h"

class Strand 
{
public:
    Strand(MThread& workers);
    ~Strand();

    int Post(HandlerFunc handler);
    int Dispatch(HandlerFunc handler);

private:
    void Warp(HandlerFunc handler);

private:
    MThread& m_tWorkers;
    std::mutex m_mutexHandlerFunc;
} ;

#endif
