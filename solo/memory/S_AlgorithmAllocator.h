#pragma once
#include <stdlib.h>
#include "S_Allocator.h"

namespace solo
{

template <typename T>
class S_AlgorithmAllocator : public std::allocator<T>
{
//public:
//    typedef size_t size_type;
//    typedef ptrdiff_t difference_type;
//    typedef T* pointer;
//    typedef const T* const_pointer;
//    typedef T& reference;
//    typedef const T& const_reference;
//    typedef T value_type;

//    S_AlgorithmAllocator(){}
//    ~S_AlgorithmAllocator(){}

//    template <class U> struct rebind { typedef S_AlgorithmAllocator<U> other; };
//    template <class U> S_AlgorithmAllocator(const S_AlgorithmAllocator<U>&){}

//    pointer address(reference x) const {return &x;}
//    const_pointer address(const_reference x) const {return &x;}
//    size_type max_size() const throw() {return size_t(-1) / sizeof(value_type);}

//    pointer allocate(size_type n, void *hint = 0)
//    {
//        return static_cast<pointer>( S_Allocator::singleton()->allocate( n * sizeof(T) ) );
//    }

//    void deallocate(pointer p, size_type n)
//    {
//        S_Allocator::singleton()->deallocate(p);
//    }

//    void construct(pointer p, const T& val)
//    {
//        new(static_cast<void*>(p)) T(val);
//    }

//    void construct(pointer p)
//    {
//        new(static_cast<void*>(p)) T();
//    }

//    void destroy(pointer p)
//    {
//        p->~T();
//    }
};

}
