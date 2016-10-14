#include <assert.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>

#define PATH "./temp.txt"
int main()
{
    char * str = "I think that I shall never see a thing as lovely as a tree";
    int fd = open(PATH, O_CREAT | O_RDWR | O_TRUNC);
    struct stat sb;

    flock(fd, LOCK_EX | LOCK_NB);
    flock(fd, LOCK_UN);

    write(fd, str, strlen(str));
    ftruncate(fd, 1);
    fsync(fd);
    fstat(fd, &sb);
    printf("%lu\n",sb.st_size);
    assert(sb.st_size == 1);

    write(fd, str, strlen(str));
    close(fd);
    chmod(PATH, S_IRUSR | S_IWUSR);
    stat(PATH, &sb);
    printf("%lu\n",sb.st_size);
    if (truncate(PATH, 2) < 0) {
	perror("truncate");
    }
    stat(PATH, &sb);
    printf("%lu\n",sb.st_size);
    assert(sb.st_size == 2);
    unlink(PATH);
    return 0;
}

