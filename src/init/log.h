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

#ifndef _INIT_LOG_H_
#define _INIT_LOG_H_

#if 0

#include <cutils/klog.h>

#define ERROR(x...)   KLOG_ERROR("init", x)
#define NOTICE(x...)  KLOG_NOTICE("init", x)
#define INFO(x...)    KLOG_INFO("init", x)

#else

#define KLOG_ERROR_LEVEL   3
#define KLOG_WARNING_LEVEL 4
#define KLOG_NOTICE_LEVEL  5
#define KLOG_INFO_LEVEL    6
#define KLOG_DEBUG_LEVEL   7


#define ERROR(fmt,args...)    \
        fprintf(stderr, "%s:%d| ERROR: " fmt, __FILE__, __LINE__, ## args)

#define INFO(fmt,args...)    \
        fprintf(stdout, "%s:%d| INFO: " fmt, __FILE__, __LINE__, ## args)

#define NOTICE(fmt,args...)    \
        fprintf(stdout, "%s:%d| NOTICE: " fmt, __FILE__, __LINE__, ## args)

#endif

#define LOG_UEVENTS        0  /* log uevent messages if 1. verbose */

#endif
