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
* Android�������Ϻ��ڴ�����:
* �ִ� CPU��ָ���ִ�д��򲢲�һ���ϸ�˳��ִ�еģ�û������Ե�ָ����Դ��Ҵ���ִ�У�
* �Գ������CPU��ָ����ˮ�ߣ����ִ���ٶȡ����⣬������Ҳ���ָ������Ż���
* ���磺����ָ��˳��������CPU��ָ����ˮ�ߡ�
* ��Щ�Ż���ʽ�����󲿷���������ĺܺã�������һЩ�Ƚϸ��ӵ�������ܳ����⣬
* ����ִ��ͬ������ʱ���п�����Ϊ�����Ż�����ͬ��ԭ��֮���ָ����ͬ��ԭ��ǰִ�С�
* �ڴ����Ϻͱ������Ͼ�����������CPU�ͱ�����ֹͣ�Ż��ķ�����
*
* ����������ʹ��αָ�memory�����߱��������ܰѡ�memory��ǰ��Ĵ��������һ��
* ��ʱ��memory��Ҳ��һ���Ż����ϵ����á��ڴ��������ڴ����в�������ָ�
* ��arm�е�dmb��dsb��isbָ�x86�е�sfence��lfence��mfenceָ�
* CPU������Щ����ָ�Ҫ�ȴ�ǰ��ָ��ִ������ˣ���ִ�к����ָ�
* ��Щָ������ú�����һ�����ϰ�ǰ��ָ������������ͷ�ֹ��CPU��ǰ������ָ��ߵ�ִ�С�
*
*
* 1.  armƽ̨���ڴ�����ָ��
*  	dsb������ͬ������ָ����������ǵȴ�����֮ǰ��ָ����ɺ���ִ�к����ָ�
*	dmb�������ڴ�����ָ����������ǵȴ�ǰ������ڴ��ָ����ɺ���ִ�к�������ڴ��ָ�
*	isb��ָ��ͬ�����ϡ����������ǵȴ���ˮ��������ָ��ִ����ɺ���ִ�к����ָ�
*
* 2.  x86ƽ̨�ϵ��ڴ�����ָ��
*	sfence���洢����ָ����������ǵȴ�ǰ��д�ڴ��ָ����ɺ���ִ�к���д�ڴ��ָ�
*	lfence����ȡ����ָ����������ǵȴ�ǰ���ȡ�ڴ��ָ����ɺ���ִ�к����ȡ�ڴ��ָ�
*	mfence���������ָ����������ǵȴ�ǰ���д�ڴ��ָ����ɺ���ִ�к����д�ڴ��ָ��
*/
#ifndef ANDROID_CUTILS_ATOMIC_ARM_H
#define ANDROID_CUTILS_ATOMIC_ARM_H

#include <stdint.h>

#ifndef ANDROID_ATOMIC_INLINE
#define ANDROID_ATOMIC_INLINE inline __attribute__((always_inline))
#endif


/* �������� */
extern ANDROID_ATOMIC_INLINE void android_compiler_barrier()
{
    __asm__ __volatile__ ("" : : : "memory");
}


/* �ڴ����� */
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
	ldrex  prev��[*ptr]
	add  tmp,  prev,  increment
	strex  status,  tmp, [*ptr]
} whiile(status != 0)

��addָ���ǰ������������ȥ�Ƚ�İ����ָ�ldrex��strex����������AMRV6�������ͬ��ָ�
ldrexָ��������ǰ�ָ��ptrָ�������(*ptr)�ŵ�prev�����У�ͬʱ��ִ�д�������һ�����(tag)�������ָ��ptr�ĵ�ַ��
��ʾ����ڴ��ַ�Ѿ���һ��CPU���ڷ��ʡ���ִ�е�strexָ��ʱ���������Ƿ����ptr�ĵ�ַ��ǣ������Ǵ��ڣ�
strexָ����addָ��ִ�еĵĽ��д��ָ��ptrָ��ĵ�ַ�����ҷ���0��Ȼ������ñ�ǡ����صĽ��0�����status�����У�����ѭ����������
�����strexָ��ִ��ǰ�������̵߳��������л������л�������ldrxָ�����õı�־���ᱻ�������ʱ��ִ��strexָ��ʱ������û���������־��
strexָ�������ɶ�ptrָ��Ĵ洢����������status�����еķ��ؽ����1������ѭ�����ܽ��������¿�ʼִ�У�ֱ���ɹ�Ϊֹ��
__builtin_expect��gcc���ڽ���������������������һ��������һ�����ʽ���ڶ���������һ��ֵ�����ʽ�ļ�����Ҳ�Ǻ����Ľ����
__builtin_expect����������gccԤ����ʽ�����ܵ�ֵ��ʲô������gcc�����Ԥ��ֵ���Ż����롣�����б��ĺ�����Ԥ�⡰status!=0��
������ʽ��ֵΪ��0����Ҳ����Ԥ��whileѭ����������
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
