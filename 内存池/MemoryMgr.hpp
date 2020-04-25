#ifndef __MEMORYMGR_HPP__
#define __MEMORYMGR_HPP__
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <mutex>       // 互斥锁和自解锁
#include <assert.h>   //断言, 调试使用的
#include "CELLTimestamp.hpp"
// 内存池管理内存块 和 拥有 很多内存块, 内存管理工具 管理内存池

//#define _DEBUG 1     /* 调试信息输出开关 */

static std::mutex  _memoryMutex;      //申请内存锁


//调试输出的代码,
#ifdef _DEBUG
    #define xPrintf(...)   printf(__VA_ARGS__)
#else
    #define xPrintf(...)
#endif /* _DEBUG */


#define  MAX_MEMORY_SIZE 1024   // 最大内存单元的大小

class MemoryAlloc;  // 内存池
class MemoryMgr;    // 内存管理工具
class MemoryBlock;  // 内存块 最小单元


// 内存块头 最小单元
class MemoryBlock{
public:
    //下面都是每个内存块的所属信息
    int nID;  // 内存块编号,如果每个内存块占用4kb, 则可以记录 8TB 的内存空间大小
    int nRef; // 引用次数
    MemoryAlloc* pAlloc;  // 所属的内存池
    MemoryBlock* pNext;   // 下一个内存块的位置
    
    bool bPool;   // 是否在内存池中.(需要的空间大于单个内存块容量 或 内存池已满 向操作系统申请内存)
    // 64位系统为16或8字节内存对齐, 32位则为4字节内存对齐
    //由于16字节内存对齐的关系,系统会将该结构补充到32字节, 会有15字节为无用填充位, 将其作为预留保存下来.
    
    MemoryBlock(){}
    ~MemoryBlock(){}

private:
    char cNULL[ sizeof(char*) -1];  //预留的无用位置,目的为占用内存对齐后多余的部分内存

};



//内存池
class MemoryAlloc{
public:
    MemoryAlloc():_pBuf(nullptr),_pHeader(nullptr),_nSize(0),_nBlockSize(0) {
        xPrintf("MemoryAlloc\n");
    }
    
    ~MemoryAlloc(){
        if ( nullptr != _pBuf )
            free(_pBuf);
    }
    
    /*
     *  申请内存
     */
    void* allocMemory(size_t nSize){
        std::lock_guard<std::mutex> lg(_mutex);
        // 如果内存池没有初始化, 那么就进行初始化,然后再分配内存
        if ( ! _pBuf ){
            initMemory();
        }
        MemoryBlock* pReturn = nullptr;  // 这个是要返回的值
        if (nullptr ==  _pHeader){  // 判断有没有块可以用
            // 进入到这里,就代表 内存池已经用完了,使用的是从操作系统申请的内存
            pReturn = reinterpret_cast<MemoryBlock*>(malloc(nSize + sizeof(MemoryBlock))); //数据+头部
            pReturn->bPool = false;  //表示这个内存单元在 内存池中.
            pReturn->nID   = -1;     //内存块编号只给-1 . 因为不在内存池中
            pReturn->nRef  = 1;     //引用次数 初始化为0, 还没有被引用
            pReturn->pAlloc = nullptr;  // 所属于当前的这个内存池
            pReturn->pNext  = nullptr;   //下一块内存单元的位置, 目前未知
        }
        else{
            pReturn = _pHeader;
            _pHeader = _pHeader->pNext;
            assert(0 == pReturn->nRef);   //断言. 引用次数必须为0, 代表这个内存块没有被占用和使用过
            pReturn->nRef++;         // 增加引用次数
        }
        xPrintf("MemoryAlloc::allocMem() : 申请内存. 地址= %p, ID= %d ,size= %lu \n", pReturn, pReturn->nID, nSize);
        return reinterpret_cast<char*>(pReturn) + sizeof(MemoryBlock);  // 将头部偏移掉
    }
    
    
    /*
     *  释放内存
     */
    void freeMemory(void* pMem){
        char* pDat = static_cast<char*>(pMem);
        MemoryBlock* pBlock =  reinterpret_cast<MemoryBlock*>(pDat - sizeof(MemoryBlock));
        assert( 1 == pBlock->nRef );
        // 引用次数为0 了.
        if (pBlock->bPool){  //是否在内存池中, 还是在操作系统中, true在内存池, false 在系统
            std::lock_guard<std::mutex> lg(_mutex);
            pBlock->nRef--;  //引用次数 -1
            if ( pBlock->nRef != 0)
                return ;   // 该内存单元被多次引用了.
            // 在 内存池中的操作
            pBlock->pNext = _pHeader;
            _pHeader = pBlock;
        }
        else{  // 操作系统中的操作
            if (--pBlock->nRef != 0)
            {
                return;
            }
            free(pBlock);
        }
    }
    
    
    /*
     *  初始化内存池, 向操作系统申请内存, 并初始化, 将其作为内存池
     */
    void  initMemory(void){
        xPrintf("initMemory:_nSzie=%d,_nBlockSzie=%d\n", _nSzie, _nBlockSzie);
        
        assert( nullptr == _pBuf); //断言, _pBuf必须以等于nullptr, 否则就报错.
        if ( _pBuf )  // _pBuf 必须等于nullptr, 否则就返回,代表内存池已经初始化过了
            return ;
        //申请内存
        size_t sizeMemoryBlock = sizeof(MemoryBlock) + _nSize;
        size_t bufSize = sizeMemoryBlock * _nBlockSize;   // (数据+头) * 内存单元数量
        _pBuf = static_cast<char*>(malloc(bufSize));  // 向操作系统申请 堆内存
        if (_pBuf == nullptr){
            std::cerr << "内存申请失败,系统内存容量不足" << __LINE__  << std::endl;
            return;
        }
        
        //初始化内存池
        _pHeader = reinterpret_cast<MemoryBlock*>(_pBuf);   //内存池的开始位置 就是第一个内存单元的位置
        _pHeader->bPool = true;  //表示这个内存单元在 内存池中.
        _pHeader->nID   = 0;     //内存块编号从0开始
        _pHeader->nRef  = 0;     //引用次数 初始化为0, 还没有被引用
        _pHeader->pAlloc = this;  // 所属于当前的这个内存池
        _pHeader->pNext  = nullptr;
        
        MemoryBlock* pTemp = nullptr;
        MemoryBlock* pPrevious = _pHeader;
        
        //遍历内存池, 将内存块初始化
        for(size_t n = 1; n < _nBlockSize ; n++){  //n从1开始, 跳过_pHeader指向的头指针
            pTemp =  reinterpret_cast<MemoryBlock*>(_pBuf + ( n * sizeMemoryBlock));
            //得到下个内存单元的首地址
            pTemp->bPool = true;  //表示这个内存单元在 内存池中.
            pTemp->nID   = static_cast<int>(n);     //内存块编号从0开始
            pTemp->nRef  = 0;     //引用次数 初始化为0, 还没有被引用
            pTemp->pAlloc = this;  // 所属于当前的这个内存池
            pPrevious->pNext = pTemp;   //下一个内存单元的位置.
            pPrevious = pTemp;
        }
        pTemp->pNext = nullptr;
    }

protected:
    char*    _pBuf;    // 内存池地址
    MemoryBlock*  _pHeader;   // 第一个 内存池中内存单元的指针,也就是头指针
    size_t   _nSize;   // unsigned long ,内存池中 单个内存块也就是内存单元占据内存的 大小
    size_t   _nBlockSize;   // 内存池中 可以容纳内存块(内存单元) 的数量.
    std::mutex _mutex;     // 申请 内存和释放 内存池中元素时用到的锁,  系统申请内存和释放并不使用
};






// C++11之前非常有用, 声明这个对象的时候,就相当于初始化了.  C++11之后就用处不大了.
template <size_t nSize, size_t nBlickSize>   //使用确定类型的模版
class MemoryAlloctor: public MemoryAlloc{
public:
    MemoryAlloctor()/* :MemoryAlloc((nSize %  sizeof(char*) == 0 ) ? nSize : (nSize + sizeof(char*)  - nSize %   sizeof(char*)) ,nBlickSize)*/
    {
        //内存对齐 补齐算法.避免申请的内存出现不对齐问题
        size_t sizeof_char =sizeof(void*);
        size_t nSize_num =  nSize %  sizeof_char;
        _nSize = ( nSize_num == 0 ) ? nSize : (nSize + sizeof_char - nSize_num ) ;
        _nBlockSize = nBlickSize;
    }
    ~MemoryAlloctor(){}
    
};





/*
 *  内存管理工具,单例模式,需要申请和释放内存的外部可用的基本方法
 */
class MemoryMgr{
private:
    MemoryMgr(){
        init_saAlloc(0, 64, &_mem64);
        init_saAlloc(65, 128, &_mem128);
        init_saAlloc(129, 256, &_mem256);
        init_saAlloc(257, 512, &_mem512);
        init_saAlloc(513, 1024, &_mem1024);
        xPrintf("MemoryMgr\n");
    }
    ~MemoryMgr(){}
public:
    /*
     *  单例模式, 返回内存管理工具的唯一对象, 静态
     */
      static MemoryMgr& Instace(){
        static MemoryMgr mgr;
        return mgr;
    }
    
    
    /*
     *  申请内存
     */
    void* allocMem(size_t nSize){
        if (nSize <= MAX_MEMORY_SIZE ){  //申请的内存长度, 是否小于内存池内存单元的长度.
            // 小于 则分配到内存池
            return _saAlloc[nSize]->allocMemory(nSize);
        }
        else{
            // 大于 则由操作系统分配
            MemoryBlock* pReturn = reinterpret_cast<MemoryBlock*>(malloc(nSize + sizeof(MemoryBlock))); //数据+头部
            pReturn->bPool = false;  //表示这个内存单元在 内存池中.
            pReturn->nID   = -1;     //内存块编号只给-1 . 因为不在内存池中
            pReturn->nRef  = 1;     //引用次数 初始化为0, 还没有被引用
            pReturn->pAlloc = nullptr;  // 所属于当前的这个内存池
            pReturn->pNext  = nullptr;   //下一块内存单元的位置, 目前未知
            
            xPrintf("MemoryMgr::allocMem() : 申请内存. 地址= %p, ID= %d ,size= %lu \n", pReturn, pReturn->nID, nSize);
                  
            return reinterpret_cast<char*>(pReturn) + sizeof(MemoryBlock);  // 将头部偏移掉
        }
    }

    /*
     *  释放内存
     */
    void freeMem(void* pMem){
        char* pDat = static_cast<char*>(pMem);
        MemoryBlock* pBlock =  reinterpret_cast<MemoryBlock*>(pDat - sizeof(MemoryBlock));
        xPrintf("MemoryMgr::freeMem() : 释放内存. 地址= %p, ID= %d \n", pBlock, pBlock->nID);
        if (pBlock->bPool){  // 是否在内存池中
            // 这块内存是在内存池中的
            pBlock->pAlloc->freeMemory(pMem);
        }
        else{
            // 这块内存是在操作系统中申请的
            pBlock->nRef--;
            if (  pBlock->nRef == 0) // 引用次数为0时 释放
                free(pBlock);   // 注意偏移
        }
    }
    
    
    /*
     *  增加内存块的引用计数  ,当想要共享同一块数据的时候. 可以增加这个数据的引用计数, 避免误删除.
     */
    void addRef(void* pMem){
        char* pDat = static_cast<char*>(pMem);
        MemoryBlock* pBlock =  reinterpret_cast<MemoryBlock*>(pDat - sizeof(MemoryBlock));
        pBlock->nRef++;
    }
    
    
private:
    /*
     *  初始化 内存池映射数组
     */
    void init_saAlloc(int nBegin, int nEnd, MemoryAlloc* pMemA){
        for (int n = nBegin; n <= nEnd; n++)
            _saAlloc[n] = pMemA;
    }
    
private:
    //内存池创建的时候,已经添自动添加了头部位置的占用空间了. 真正占用的空间是  64 + 头部长度32 = 96
    MemoryAlloctor<64,100000> _mem64;  //创建一个内存池,该内存池中能容纳 10个内存单元,每个内存单元为64字节(头部32字节+数据64字节)
    MemoryAlloctor<128,1000000> _mem128;
    MemoryAlloctor<256,1000000> _mem256;
    MemoryAlloctor<512,1000000> _mem512;
    MemoryAlloctor<1024,1000000> _mem1024;
    MemoryAlloc* _saAlloc[MAX_MEMORY_SIZE+1];   //MAX_MEMORY_SIZE=1024 , 指向内存池的指针数组65个.
    
};


#endif /* __MEMORYMGR_HPP__ */
