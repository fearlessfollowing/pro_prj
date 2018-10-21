/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
* Android编译屏障和内存屏障:
* 现代 CPU中指令的执行次序并不一定严格按顺序执行的，没有相关性的指令可以打乱次序执行，
* 以充分利用CPU的指令流水线，提高执行速度。另外，编译器也会对指令进行优化，
* 例如：调整指令顺序来利用CPU的指令流水线。
* 这些优化方式，绝大部分情况工作的很好，但是在一些比较复杂的情况可能出问题，
* 例如执行同步代码时就有可能因为这种优化导致同步原语之后的指令在同步原语前执行。
* 内存屏障和编译屏障就是用来告诉CPU和编译器停止优化的方法。
*
* 编译屏障是使用伪指令“memory”告诉编译器不能把“memory”前后的代码混淆在一起，
* 这时“memory”也起到一种优化屏障的作用。内存屏障是在代码中插入特殊指令，
* 如arm中的dmb，dsb和isb指令，x86中的sfence，lfence和mfence指令。
* CPU遇到这些特殊指令，要等待前面指令执行完成了，才执行后面的指令。
* 这些指令的作用好像是一道屏障把前后指令隔开，这样就防止了CPU把前后两段指令颠倒执行。
*
*
* 1.  arm平台的内存屏障指令
*  	dsb：数据同步屏障指令。它的作用是等待所有之前的指令完成后再执行后面的指令。
*	dmb：数据内存屏障指令。它的作用是等待前面访问内存的指令完成后再执行后面访问内存的指令。
*	isb：指令同步屏障。它的作用是等待流水线中所有指令执行完成后再执行后面的指令。
*
* 2.  x86平台上的内存屏障指令
*	sfence：存储屏障指令。它的作用是等待前面写内存的指令完成后再执行后面写内存的指令。
*	lfence：读取屏障指令。它的作用是等待前面读取内存的指令完成后再执行后面读取内存的指令。
*	mfence：混合屏障指令。它的作用是等待前面读写内存的指令完成后再执行后面读写内存的指令
*/
#ifndef ANDROID_CUTILS_ATOMIC_ARM_H
#define ANDROID_CUTILS_ATOMIC_ARM_H

#include <stdint.h>

#ifndef ANDROID_ATOMIC_INLINE
#define ANDROID_ATOMIC_INLINE inline __attribute__((always_inline))
#endif


/* 编译屏障 */
extern ANDROID_ATOMIC_INLINE void android_compiler_barrier()
{
    __asm__ __volatile__ ("" : : : "memory");
}


/* 内存屏障 */
extern ANDROID_ATOMIC_INLINE void android_memory_barrier()
{
#if ANDROID_SMP == 0
    android_compiler_barrier();
#else
    __asm__ __volatile__ ("dmb" : : : "memory");
#endif

}

extern ANDROID_ATOMIC_INLINE
int32_t android_atomic_acquire_load(volatile const int32_t *ptr)
{
    int32_t value = *ptr;
    android_memory_barrier();
    return value;
}

extern ANDROID_ATOMIC_INLINE
int32_t android_atomic_release_load(volatile const int32_t *ptr)
{
    android_memory_barrier();
    return *ptr;
}

extern ANDROID_ATOMIC_INLINE
void android_atomic_acquire_store(int32_t value, volatile int32_t *ptr)
{
    *ptr = value;
    android_memory_barrier();
}

extern ANDROID_ATOMIC_INLINE
void android_atomic_release_store(int32_t value, volatile int32_t *ptr)
{
    android_memory_barrier();
    *ptr = value;
}

extern ANDROID_ATOMIC_INLINE
int android_atomic_cas(int32_t old_value, int32_t new_value,
                       volatile int32_t *ptr)
{
    int32_t prev, status;
    do {
        __asm__ __volatile__ ("ldrex %0, [%3]\n"
                              "mov %1, #0\n"
                              "teq %0, %4\n"
#ifdef __thumb2__
                              "it eq\n"
#endif
                              "strexeq %1, %5, [%3]"
                              : "=&r" (prev), "=&r" (status), "+m"(*ptr)
                              : "r" (ptr), "Ir" (old_value), "r" (new_value)
                              : "cc");
    } while (__builtin_expect(status != 0, 0));
    return prev != old_value;
}

extern ANDROID_ATOMIC_INLINE
int android_atomic_acquire_cas(int32_t old_value, int32_t new_value,
                               volatile int32_t *ptr)
{
    int status = android_atomic_cas(old_value, new_value, ptr);
    android_memory_barrier();
    return status;
}

extern ANDROID_ATOMIC_INLINE
int android_atomic_release_cas(int32_t old_value, int32_t new_value,
                               volatile int32_t *ptr)
{
    android_memory_barrier();
    return android_atomic_cas(old_value, new_value, ptr);
}

extern ANDROID_ATOMIC_INLINE

/*
do {
	ldrex  prev，[*ptr]
	add  tmp,  prev,  increment
	strex  status,  tmp, [*ptr]
} whiile(status != 0)

在add指令的前后有两条看上去比较陌生的指令：ldrex和strex。这两条是AMRV6新引入的同步指令。
ldrex指令的作用是把指针ptr指向的内容(*ptr)放到prev变量中，同时给执行处理器做一个标记(tag)，标记上指针ptr的地址，
表示这个内存地址已经有一个CPU正在访问。当执行到strex指令时，它会检查是否存在ptr的地址标记，如果标记存在，
strex指令会把add指令执行的的结果写入指针ptr指向的地址，并且返回0，然后清除该标记。返回的结果0会放在status变量中，这样循环将结束。
如果在strex指令执行前发生了线程的上下文切换，在切换回来后，ldrx指令设置的标志将会被清除。这时再执行strex指令时，由于没有了这个标志，
strex指令将不会完成对ptr指针的存储操作，而且status变量中的返回结果是1。这样循环不能结束，重新开始执行，直到成功为止。
__builtin_expect是gcc的内建函数，有两个参数，第一个参数是一个表达式，第二个参数是一个值。表达式的计算结果也是函数的结果。
__builtin_expect是用来告诉gcc预测表达式更可能的值是什么，这样gcc会根据预测值来优化代码。代码中表达的含义是预测“status!=0”
这个表达式的值为“0”，也就是预测while循环将结束。
*/
int32_t android_atomic_add(int32_t increment, volatile int32_t *ptr)
{
    int32_t prev, tmp, status;
    android_memory_barrier();
    do {
		
		/* ldrex prev, [ptr]
		 * add tmp, prev, #increment
		 * strex status, tmp, [ptr]
		 */
        __asm__ __volatile__ ("ldrex %0, [%4]\n"
                              "add %1, %0, %5\n"
                              "strex %2, %1, [%4]"
                              : "=&r" (prev), "=&r" (tmp),
                                "=&r" (status), "+m" (*ptr)
                              : "r" (ptr), "Ir" (increment)
                              : "cc");
    } while (__builtin_expect(status != 0, 0));
	
    return prev;
}

extern ANDROID_ATOMIC_INLINE int32_t android_atomic_inc(volatile int32_t *addr)
{
    return android_atomic_add(1, addr);
}

extern ANDROID_ATOMIC_INLINE int32_t android_atomic_dec(volatile int32_t *addr)
{
    return android_atomic_add(-1, addr);
}

extern ANDROID_ATOMIC_INLINE
int32_t android_atomic_and(int32_t value, volatile int32_t *ptr)
{
    int32_t prev, tmp, status;
    android_memory_barrier();
    do {
        __asm__ __volatile__ ("ldrex %0, [%4]\n"
                              "and %1, %0, %5\n"
                              "strex %2, %1, [%4]"
                              : "=&r" (prev), "=&r" (tmp),
                                "=&r" (status), "+m" (*ptr)
                              : "r" (ptr), "Ir" (value)
                              : "cc");
    } while (__builtin_expect(status != 0, 0));
    return prev;
}

extern ANDROID_ATOMIC_INLINE
int32_t android_atomic_or(int32_t value, volatile int32_t *ptr)
{
    int32_t prev, tmp, status;
    android_memory_barrier();
    do {
        __asm__ __volatile__ ("ldrex %0, [%4]\n"
                              "orr %1, %0, %5\n"
                              "strex %2, %1, [%4]"
                              : "=&r" (prev), "=&r" (tmp),
                                "=&r" (status), "+m" (*ptr)
                              : "r" (ptr), "Ir" (value)
                              : "cc");
    } while (__builtin_expect(status != 0, 0));
    return prev;
}

#endif /* ANDROID_CUTILS_ATOMIC_ARM_H */
