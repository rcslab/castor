#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

#define BUFF_MAX 1024

void dump_entries(char * buf, int size)
{
    struct dirent * dent = (struct dirent *)buf;
    int i;

    for (i = 0; i < size; i += dent->d_reclen) {
	dent = (struct dirent *)&buf[i];
	printf("fileno: %d, name: %s\n", dent->d_fileno, dent->d_name);
    }
}

void list_cwd()
{
    char buf[BUFF_MAX];
    long basep;
    int fd;
    int result;

    fd = open(".", O_RDONLY);
    result = getdirentries(fd, buf, BUFF_MAX, &basep);
    assert(result > 0);
    dump_entries(buf, result);
}

int main()
{
    int cwd;
    int result;

    list_cwd();
    cwd = open(".", O_RDONLY);
    result = chdir(".."); //test chdir
    assert(result == 0);
    list_cwd();
    result = fchdir(cwd); //also test fchdir
    assert(result == 0);
    list_cwd();
    return 0;
}