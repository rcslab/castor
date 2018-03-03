#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

static inline void hex_dump(char * cp, int size)
{
    int i;
    for (i  = 0; i < size; i++) {
	printf("0x%02x", cp[i]);
	if (i%16==0)
	    printf("\n");
    }
    printf("\n");
}

#endif
