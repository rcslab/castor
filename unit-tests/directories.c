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
    union {
	struct dirent * dent;
	char * cp;
    } dp;
    int i;

    for (i = 0; i < size; i += dp.dent->d_reclen) {
	dp.cp = buf + i;
	printf("fileno: %lu, name: %s\n", dp.dent->d_fileno, dp.dent->d_name);
    }
}

void list_cwd()
{
    char buf[BUFF_MAX]; // we use int's so things are aligned, *sigh*
    long basep;
    int fd;
    int result;

    fd = open(".", O_RDONLY);
    result = getdirentries(fd, buf, BUFF_MAX, &basep);
    assert(result > 0);
    dump_entries(buf, result);
    close(fd);

    fd = open(".", O_RDONLY);
    result = getdents(fd, buf, sizeof(buf));
    assert(result > 0);
    dump_entries(buf, result);
    close(fd);

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
    result = eaccess(DIR_B, R_OK);
    assert(result == 0);

    struct stat sb;
    result = lstat(DIR_B, &sb);
    assert(result == 0);
    rmdir(DIR_B);

    return 0;
}
