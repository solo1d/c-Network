//
//  Alloc.h
//  test
//
//  Created by ns on 2020/4/5.
//  Copyright © 2020 ns. All rights reserved.
//

#ifndef __ALLOCTOR_H__
#define __ALLOCTOR_H__

#include <iostream>
#include "MemoryMgr.hpp"
void* operator new(size_t size);
void* operator new[](size_t size);
void  operator delete(void*  p);// throw();
void  operator delete[](void*  p);// throw();;

void* mem_alloc(size_t  size);
void  mem_free(void* p);


#endif /* __ALLOCTOR_H__ */
