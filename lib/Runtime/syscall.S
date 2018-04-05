
#include <SYS.h>

.text;
.p2align 4,0x90;
.globl __rr_syscall_long;
.type __rr_syscall_long,@function;
__rr_syscall_long:
LSYSCALL(syscall)

	.section .note.GNU-stack,"",%progbits

