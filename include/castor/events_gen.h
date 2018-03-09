
/*** !!! this code has been autogenerated by utils/gen_sal.py, do not modify it directly. !!! ***/


#ifndef __EVENTS_GEN_H
#define __EVENTS_GEN_H


#define RREVENT_TABLE_GEN\
	RREVENT(READ,	0xA000)\
	RREVENT(LINK,	0xA001)\
	RREVENT(UNLINK,	0xA002)\
	RREVENT(CHDIR,	0xA003)\
	RREVENT(FCHDIR,	0xA004)\
	RREVENT(MKNOD,	0xA005)\
	RREVENT(CHMOD,	0xA006)\
	RREVENT(CHOWN,	0xA007)\
	RREVENT(GETPID,	0xA008)\
	RREVENT(MOUNT,	0xA009)\
	RREVENT(UNMOUNT,	0xA00A)\
	RREVENT(SETUID,	0xA00B)\
	RREVENT(GETUID,	0xA00C)\
	RREVENT(GETEUID,	0xA00D)\
	RREVENT(ACCEPT,	0xA00E)\
	RREVENT(GETSOCKNAME,	0xA00F)\
	RREVENT(ACCESS,	0xA010)\
	RREVENT(CHFLAGS,	0xA011)\
	RREVENT(FCHFLAGS,	0xA012)\
	RREVENT(GETPPID,	0xA013)\
	RREVENT(GETEGID,	0xA014)\
	RREVENT(GETGID,	0xA015)\
	RREVENT(ACCT,	0xA016)\
	RREVENT(REBOOT,	0xA017)\
	RREVENT(SYMLINK,	0xA018)\
	RREVENT(READLINK,	0xA019)\
	RREVENT(UMASK,	0xA01A)\
	RREVENT(CHROOT,	0xA01B)\
	RREVENT(GETGROUPS,	0xA01C)\
	RREVENT(SETGROUPS,	0xA01D)\
	RREVENT(SETITIMER,	0xA01E)\
	RREVENT(SWAPON,	0xA01F)\
	RREVENT(GETITIMER,	0xA020)\
	RREVENT(GETDTABLESIZE,	0xA021)\
	RREVENT(FSYNC,	0xA022)\
	RREVENT(SETPRIORITY,	0xA023)\
	RREVENT(SOCKET,	0xA024)\
	RREVENT(CONNECT,	0xA025)\
	RREVENT(GETPRIORITY,	0xA026)\
	RREVENT(BIND,	0xA027)\
	RREVENT(SETSOCKOPT,	0xA028)\
	RREVENT(LISTEN,	0xA029)\
	RREVENT(GETRUSAGE,	0xA02A)\
	RREVENT(SETTIMEOFDAY,	0xA02B)\
	RREVENT(FCHOWN,	0xA02C)\
	RREVENT(FCHMOD,	0xA02D)\
	RREVENT(SETREUID,	0xA02E)\
	RREVENT(SETREGID,	0xA02F)\
	RREVENT(RENAME,	0xA030)\
	RREVENT(FLOCK,	0xA031)\
	RREVENT(MKFIFO,	0xA032)\
	RREVENT(SENDTO,	0xA033)\
	RREVENT(SHUTDOWN,	0xA034)\
	RREVENT(MKDIR,	0xA035)\
	RREVENT(RMDIR,	0xA036)\
	RREVENT(UTIMES,	0xA037)\
	RREVENT(ADJTIME,	0xA038)\
	RREVENT(QUOTACTL,	0xA039)\
	RREVENT(LGETFH,	0xA03A)\
	RREVENT(GETFH,	0xA03B)\
	RREVENT(SETFIB,	0xA03C)\
	RREVENT(NTP_ADJTIME,	0xA03D)\
	RREVENT(SETGID,	0xA03E)\
	RREVENT(SETEGID,	0xA03F)\
	RREVENT(SETEUID,	0xA040)\
	RREVENT(PATHCONF,	0xA041)\
	RREVENT(FPATHCONF,	0xA042)\
	RREVENT(UNDELETE,	0xA043)\
	RREVENT(FUTIMES,	0xA044)\
	RREVENT(CLOCK_SETTIME,	0xA045)\
	RREVENT(CLOCK_GETRES,	0xA046)\
	RREVENT(FFCLOCK_GETCOUNTER,	0xA047)\
	RREVENT(FFCLOCK_SETESTIMATE,	0xA048)\
	RREVENT(FFCLOCK_GETESTIMATE,	0xA049)\
	RREVENT(NTP_GETTIME,	0xA04A)\
	RREVENT(ISSETUGID,	0xA04B)\
	RREVENT(LCHOWN,	0xA04C)\
	RREVENT(GETDENTS,	0xA04D)\
	RREVENT(LCHMOD,	0xA04E)\
	RREVENT(LUTIMES,	0xA04F)\
	RREVENT(FHOPEN,	0xA050)\
	RREVENT(SETRESUID,	0xA051)\
	RREVENT(SETRESGID,	0xA052)\
	RREVENT(EXTATTRCTL,	0xA053)\
	RREVENT(EXTATTR_SET_FILE,	0xA054)\
	RREVENT(EXTATTR_GET_FILE,	0xA055)\
	RREVENT(EXTATTR_DELETE_FILE,	0xA056)\
	RREVENT(GETRESUID,	0xA057)\
	RREVENT(GETRESGID,	0xA058)\
	RREVENT(KQUEUE,	0xA059)\
	RREVENT(EXTATTR_SET_FD,	0xA05A)\
	RREVENT(EXTATTR_GET_FD,	0xA05B)\
	RREVENT(EXTATTR_DELETE_FD,	0xA05C)\
	RREVENT(EACCESS,	0xA05D)\
	RREVENT(NMOUNT,	0xA05E)\
	RREVENT(KENV,	0xA05F)\
	RREVENT(LCHFLAGS,	0xA060)\
	RREVENT(UUIDGEN,	0xA061)\
	RREVENT(EXTATTR_SET_LINK,	0xA062)\
	RREVENT(EXTATTR_GET_LINK,	0xA063)\
	RREVENT(EXTATTR_DELETE_LINK,	0xA064)\
	RREVENT(SWAPOFF,	0xA065)\
	RREVENT(EXTATTR_LIST_FD,	0xA066)\
	RREVENT(EXTATTR_LIST_FILE,	0xA067)\
	RREVENT(EXTATTR_LIST_LINK,	0xA068)\
	RREVENT(AUDIT,	0xA069)\
	RREVENT(AUDITON,	0xA06A)\
	RREVENT(GETAUID,	0xA06B)\
	RREVENT(SETAUID,	0xA06C)\
	RREVENT(GETAUDIT,	0xA06D)\
	RREVENT(SETAUDIT,	0xA06E)\
	RREVENT(GETAUDIT_ADDR,	0xA06F)\
	RREVENT(SETAUDIT_ADDR,	0xA070)\
	RREVENT(AUDITCTL,	0xA071)\
	RREVENT(PREAD,	0xA072)\
	RREVENT(CPUSET,	0xA073)\
	RREVENT(CPUSET_SETID,	0xA074)\
	RREVENT(CPUSET_GETID,	0xA075)\
	RREVENT(CPUSET_GETAFFINITY,	0xA076)\
	RREVENT(CPUSET_SETAFFINITY,	0xA077)\
	RREVENT(FACCESSAT,	0xA078)\
	RREVENT(FCHMODAT,	0xA079)\
	RREVENT(FCHOWNAT,	0xA07A)\
	RREVENT(LINKAT,	0xA07B)\
	RREVENT(MKDIRAT,	0xA07C)\
	RREVENT(MKFIFOAT,	0xA07D)\
	RREVENT(READLINKAT,	0xA07E)\
	RREVENT(RENAMEAT,	0xA07F)\
	RREVENT(SYMLINKAT,	0xA080)\
	RREVENT(UNLINKAT,	0xA081)\
	RREVENT(LPATHCONF,	0xA082)\
	RREVENT(CAP_ENTER,	0xA083)\
	RREVENT(CAP_GETMODE,	0xA084)\
	RREVENT(GETLOGINCLASS,	0xA085)\
	RREVENT(SETLOGINCLASS,	0xA086)\
	RREVENT(RCTL_GET_RACCT,	0xA087)\
	RREVENT(RCTL_GET_RULES,	0xA088)\
	RREVENT(RCTL_GET_LIMITS,	0xA089)\
	RREVENT(RCTL_ADD_RULE,	0xA08A)\
	RREVENT(RCTL_REMOVE_RULE,	0xA08B)\
	RREVENT(POSIX_FALLOCATE,	0xA08C)\
	RREVENT(POSIX_FADVISE,	0xA08D)\
	RREVENT(CAP_RIGHTS_LIMIT,	0xA08E)\
	RREVENT(CAP_IOCTLS_LIMIT,	0xA08F)\
	RREVENT(CAP_IOCTLS_GET,	0xA090)\
	RREVENT(CAP_FCNTLS_LIMIT,	0xA091)\
	RREVENT(CAP_FCNTLS_GET,	0xA092)\
	RREVENT(BINDAT,	0xA093)\
	RREVENT(CONNECTAT,	0xA094)\
	RREVENT(CHFLAGSAT,	0xA095)\
	RREVENT(ACCEPT4,	0xA096)\
	RREVENT(FDATASYNC,	0xA097)\
	RREVENT(LSEEK,	0xA098)\
	RREVENT(STAT,	0xA099)\
	RREVENT(GETDIRENTRIES,	0xA09A)\
	RREVENT(TRUNCATE,	0xA09B)\
	RREVENT(LSTAT,	0xA09C)\
	RREVENT(FHSTAT,	0xA09D)\
	RREVENT(GETFSSTAT,	0xA09E)\
	RREVENT(GETPEERNAME,	0xA09F)\
	RREVENT(FSTATFS,	0xA0A0)\
	RREVENT(MKNODAT,	0xA0A1)\
	RREVENT(FSTATAT,	0xA0A2)\
	RREVENT(FHSTATFS,	0xA0A3)\
	RREVENT(SENDMSG,	0xA0A4)\
	RREVENT(STATFS,	0xA0A5)\
	RREVENT(FTRUNCATE,	0xA0A6)\
	RREVENT(GETRLIMIT,	0xA0A7)\
	RREVENT(FSTAT,	0xA0A8)\
	RREVENT(SETRLIMIT,	0xA0A9)\
	RREVENT(KEVENT,	0xA0AA)
#endif
