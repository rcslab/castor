
#ifndef __FDINFO_H__
#define __FDINFO_H__

#include <limits.h>

typedef enum FDType {
    FDTYPE_INVALID,
    FDTYPE_FILE,
    FDTYPE_FIFO,
    FDTYPE_PIPE,
    FDTYPE_SOCKETPAIR,
    FDTYPE_SOCKET,
    FDTYPE_SOCKET_UNIX,
    FDTYPE_SHM,
    FDTYPE_KQUEUE,
} FDType;

typedef struct FDInfo {
    int realFd;
    FDType type;
    // File Info
    uint64_t offset;
    int flags;
    int mode;
    char path[PATH_MAX];
} FDInfo;

#define FDTABLE_SIZE 4096
extern FDInfo fdTable[FDTABLE_SIZE];

void FDInfo_OpenFile(int fd, const char *path, int flags, int mode);
void FDInfo_OpenMmapLazy(int fd);
void FDInfo_OpenShm(int fd, int realFd);
FDType FDInfo_GetType(int fd);
int FDInfo_GetFD(int fd);
void FDInfo_Close(int fd);
void FDInfo_FTruncate(int fd, off_t length);

#endif /* __FDINFO_H__ */

