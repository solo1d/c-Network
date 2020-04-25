//
//  Alloctor.cpp
//  test
//
//  Created by ns on 2020/4/5.
//  Copyright © 2020 ns. All rights reserved.
//

#include  "Alloctor.h"
 

//调用内存池的, 由内存池管理
void* operator new(size_t size){
    return MemoryMgr::Instace().allocMem(size);
}

void* operator new[](size_t size){
    return MemoryMgr::Instace().allocMem(size);
}

void operator delete(void* p){ // throw() {
    MemoryMgr::Instace().freeMem(p);
}

void operator delete[](void* p ){ // throw() {
    MemoryMgr::Instace().freeMem(p);
}



//调用系统的, 内存池不管理的.
void* mem_alloc(size_t  size){
    return malloc(size);
}

void  mem_free(void* p){
    free(p);
}
