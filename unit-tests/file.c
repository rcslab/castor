#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#define BUFF_MAX 1024

#define fn ".rr_dummy_test_file.a.b.c.test"

int main()
{
    int result;
    FILE *fp;

    fp = fopen(fn, "w");
    fprintf(fp, "castor");
    fclose(fp);

    result = remove(fn);
    assert(result == 0);

    result = remove(fn);
    assert(result != 0);

    return 0;
}
