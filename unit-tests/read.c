
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>

#include <castor/rrshared.h>
#include <castor/rr_debug.h>

void dump_statb(struct stat sb)
{
    printf("stat.st_dev     = 0x%08lx\n", sb.st_dev);
    printf("stat.st_ino     = 0x%08lx\n", sb.st_ino);
    printf("stat.st_mode    = 0x%08x\n", sb.st_mode);
    printf("stat.st_nlink   = 0x%08lx\n", sb.st_nlink);
    printf("stat.st_uid     = 0x%08x\n", sb.st_uid);
    printf("stat.st_gid     = 0x%08x\n", sb.st_gid);
    printf("stat.st_size    = 0x%016lx\n", sb.st_size);
    printf("stat.st_atim    = %ld.%.9ld\n", sb.st_atim.tv_sec, sb.st_atim.tv_nsec);
    printf("stat.st_mtim    = %ld.%.9ld\n", sb.st_mtim.tv_sec, sb.st_atim.tv_nsec);
    printf("stat.st_ctim    = %ld.%.9ld\n", sb.st_ctim.tv_sec, sb.st_ctim.tv_nsec);
    printf("stat.st_blocks  = 0x%016lx\n", sb.st_blocks);
    printf("stat.st_blksize = 0x%08x\n", sb.st_blksize);
    printf("stat.st_flags   = 0x%08x\n", sb.st_flags);
}

void check(int fd, char * rr, const struct stat * sb) 
{
//    int len;
//    char norm[2048];
    lseek(fd, 0, SEEK_SET);
    /* XXX: Not easy to read the rrMode global anymore */
    /*if (rrMode != RRMODE_REPLAY) {
	len = syscall(SYS_read, fd, (void *)&norm, sb->st_size);
	assert(len == sb->st_size);

	for (int i = 0; i < sb->st_size; i++) {
	    if (rr[i] != norm[i]) {
		printf("Mismatch at %d\n", i);
	    }
	}
    }*/
}

int main(int argc, const char *argv[])
{
    struct stat sb;
    int fd;
    ssize_t len;
    char rr[2048];
    int result;

    fd = open("read.c", O_RDONLY);
    fstat(fd, &sb);
    dump_statb(sb);
    stat("read.c", &sb);
    dump_statb(sb);
    len = read(fd, (void *)&rr, (size_t)sb.st_size);
    assert(len == sb.st_size);
    check(fd, rr, &sb);
    bzero(rr, (size_t)len);
    len = pread(fd, (void *)&rr, (size_t)sb.st_size, 0);
    assert(len == sb.st_size);
    check(fd, rr, &sb);
    result = close(fd);
    assert(result == 0);
    return 0;
}

