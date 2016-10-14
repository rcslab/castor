#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
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

#define DIR_A "./test_A"
#define DIR_B "./test_B"

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
    result = mkdir(DIR_A, S_IRWXU);
    assert(result == 0);
    result = chdir(DIR_A);
    assert(result == 0);
    result = chdir("..");
    assert(result == 0);
    result = rmdir(DIR_A);
    assert(result == 0);
    result = mkdir(DIR_A, S_IRWXU);
    assert(result == 0);
    result = rename(DIR_A, DIR_B);
    assert(result == 0);
    result = access(DIR_B, R_OK);
    assert(result == 0);
    struct stat sb;
    result = lstat(DIR_B, &sb);
    assert(result == 0);
    unlink(DIR_A);
    unlink(DIR_B);
    return 0;
}
