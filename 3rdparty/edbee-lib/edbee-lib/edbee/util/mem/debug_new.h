/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */


#pragma once

#include <stdlib.h>
#include <stdio.h>
//#ifdef HAVE_MALLOC_H
//#include <malloc.h>
//#endif
//#include "memoryleak.h"


void* debug_malloc      (size_t size, const char* file, const int line);
void  debug_free        (void* p,     const char* file, const int line);
void* operator new      (size_t size, const char* file, const int line);
void  operator delete   (void* p,     const char* file, const int line);
void  operator delete   (void* p) throw();
void* operator new[]    (size_t size, const char* file, const int line);
void  operator delete[] (void* p,     const char* file, const int line);
void  operator delete[] (void* p) throw();

namespace edbee {

    void pause_memleak_detection(bool value);

} // edbee

#define debug_new new(__FILE__, __LINE__)
#define new       debug_new
#define malloc(A) debug_malloc((A), __FILE__, __LINE__)
#define free(A)   debug_free((A), __FILE__, __LINE__)

