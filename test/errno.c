#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/capsicum.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <sys/param.h>
#include <sys/mount.h>

#define BAD_FD 5555
#define BAD_PATH "/$#@%!!"
#define BAD_ID 666

int main()
{
    int64_t result = sendto(BAD_FD, 0, 0, 0, 0, 0);
    assert((result == -1) && (errno == EBADF));
    result = sendmsg(BAD_FD, NULL, 0);
    assert((result == -1) && (errno == EFAULT));
    result = pread(BAD_FD, NULL, 0, 0);
    assert((result == -1) && (errno == EBADF));
    result = recvmsg(BAD_FD, NULL, 0);
    assert((result == -1) && (errno == EFAULT));
    result = recvfrom(BAD_FD, NULL, 0, 0, 0, 0);
    assert((result == -1) && (errno == EBADF));
    result = lseek(BAD_FD, 0, 0);
    assert((result == -1) && (errno == EBADF));
    result = flock(BAD_FD, 0);
    assert((result == -1) && (errno == EBADF));
    result = ftruncate(BAD_FD, 0);
    assert((result == -1) && (errno == EBADF));
    result = truncate(BAD_PATH, 0);
    assert((result == -1) && (errno == ENOENT));
    result = fsync(BAD_FD);
    assert((result == -1) && (errno == EBADF));
    result = fchdir(BAD_FD);
    assert((result == -1) && (errno == EBADF));
    result = fstat(BAD_FD, NULL);
    assert((result == -1) && (errno == EBADF));
    result = fstatat(BAD_FD, NULL, NULL, 0);
    assert((result == -1) && (errno == EFAULT));
    result = stat(BAD_PATH, NULL);
    assert((result == -1) && (errno == ENOENT));
    result = lstat(BAD_PATH, NULL);
    assert((result == -1) && (errno == ENOENT));
    result = getpeername(BAD_FD, NULL, NULL);
    assert((result == -1) && (errno == EFAULT));
    result = getsockname(BAD_FD, NULL, NULL);
    assert((result == -1) && (errno == EFAULT));
    cap_rights_limit(BAD_FD, NULL);
    assert((result == -1) && (errno == EFAULT));
    result = link(BAD_PATH, BAD_PATH);
    assert((result == -1) && (errno == ENOENT));
    result = symlink(BAD_PATH, BAD_PATH);
    assert((result == -1) && (errno == EACCES));
    result = unlink(BAD_PATH);
    assert((result == -1) && (errno == ENOENT));
    result = rename(BAD_PATH, BAD_PATH);
    assert((result == -1) && (errno == ENOENT));
    result = mkdir(BAD_PATH, 0);
    assert((result == -1) && (errno == EACCES));
    result = rmdir(BAD_PATH);
    assert((result == -1) && (errno == ENOENT));
    result = chmod(BAD_PATH, 0);
    assert((result == -1) && (errno == ENOENT));
    result = access(BAD_PATH, 0);
    assert((result == -1) && (errno == ENOENT));
    result = chdir(BAD_PATH);
    assert((result == -1) && (errno == ENOENT));
    result = getrlimit(0, NULL);
    assert((result == -1) && (errno == EFAULT));
    result = setrlimit(0, NULL);
    assert((result == -1) && (errno == EFAULT));
    result = getrusage(0, NULL);
    assert((result == -1) && (errno == EFAULT));
    result = select(-1, 0, 0, 0, 0);
    assert((result == -1) && (errno == EINVAL));
    /* open(BAD_PATH, 0, 0); */
    /* assert((result == -1) && (errno == ENOENT)); */
    fstatfs(BAD_FD, NULL);
    assert((result == -1) && (errno == EBADF));
    statfs(BAD_PATH, NULL);
    assert((result == -1) && (errno == ENOENT));
    result = setuid(BAD_ID);
    assert((result == -1) && (errno == EPERM));
    result = seteuid(BAD_ID);
    assert((result == -1) && (errno == EPERM));
    result = setgid(BAD_ID);
    assert((result == -1) && (errno == EPERM));
    result = setegid(BAD_ID);
    assert((result == -1) && (errno == EPERM));

    return 0;
}

