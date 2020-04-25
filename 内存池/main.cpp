/*
 * 内存池测试
 * 2020.4.4 21:47
 */


#include "Alloctor.h"
#include <iostream>
#include <thread>
#include <mutex>   //互斥锁
#include "CELLTimestamp.hpp"




/*
 * 线程部分
 */
std::mutex m;
const int tCount = 8;
const int mCount = 100000;    //所有线程申请内存次数的最大值
const int nCount = mCount/tCount;    //每个线程申请的次数

void workFun(int index){
    
    char* data[nCount];
 
    for (size_t i =0; i < nCount; i++){
        data[i] = new char[ (unsigned)(rand()) % 1024 +1 ];
    }
    
    for (size_t i =0; i < nCount; i++){
        delete[] data[i];
    }
     
    /*
    for ( int n = 0; n < nCount; n++){
    }
    */
}


int main(void){
    std::thread  t[tCount];
    for ( int n=0; n < tCount; n++)
        t[n] = std::thread(workFun, n);
    
    CELLTimestamp tTime;
    for ( int n=0; n < tCount; n ++)
        t[n].join();
    
    std::cout << tTime.getElapsedTimeInMilliSec() <<   ", Hello ,main  thread"  << std::endl;
    return 0;
}
