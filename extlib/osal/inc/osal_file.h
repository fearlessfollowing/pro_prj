#ifndef _OSAL_FILE_H_
#define _OSAL_FILE_H_

#include <osal.h>

#if defined(__cplusplus)
extern "C" {
#endif

int OSAL_fileReadFile(char *fileName, Uint8 *addr, Uint32 readSize, Uint32 *actualReadSize);
int OSAL_fileWriteFile(char *fileName, Uint8 *addr, Uint32 size);
int OSAL_fileWriteVideoData(char *fileName, Uint8 *Yaddr, Uint8 *UVaddr, Uint32 width, Uint32 height, Uint32 pitch);

#if defined(__cplusplus)
}
#endif

#endif /* _OSAL_FILE_H_ */
