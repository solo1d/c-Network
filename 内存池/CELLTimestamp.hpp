
#ifndef __CELLTIMESTAMP_HPP__
#define __CELLTIMESTAMP_HPP__

#include <chrono>
#include <iostream>
#include <string>
#include <cstring>
using namespace std::chrono;

class  CELLTimestamp{
private:

    
public:
    CELLTimestamp(){
        update();
    }
    ~CELLTimestamp(){}
    
    
    // 初始化时间点.
    void update(void){
        _begin = high_resolution_clock::now(); // 得到当前时间,相当于初始化
    }
    
    //获得当前时间差的 秒
    double getElapsedSecond(void){
        // 将返回的微秒  扩展到 秒
        return  getElapsedTimeInMicroSec() * 0.000001;
    }
    
    //获得当前时间差的 毫秒
    double getElapsedTimeInMilliSec(void){
        // 将返回的微秒  扩展到 毫秒
        return this->getElapsedTimeInMicroSec() * 0.001;
    }
    
    //得到微秒级别的 当前时间差, 微秒
    long long  getElapsedTimeInMicroSec(){
        // 当前时间 减去曾经的一个时间点. 得到经过的时间,  单位是微秒
        return  duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
    }
    
protected:
    
    // 一个时间点 <高分辨率时钟>, 分辨到微秒或纳秒级别
    time_point<high_resolution_clock>  _begin;
};
#endif /* __CELLTIMESTAMP_HPP__ */
