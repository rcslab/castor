
/*** !!! this code has been autogenerated by utils/gen_sal.py, do not modify it directly. !!! ***/


#ifndef __CASTOR_PRETTY_PRINT__
#define __CASTOR_PRETTY_PRINT__
extern void pretty_print_READ(RRLogEntry entry);
extern void pretty_print_LINK(RRLogEntry entry);
extern void pretty_print_UNLINK(RRLogEntry entry);
extern void pretty_print_CHDIR(RRLogEntry entry);
extern void pretty_print_FCHDIR(RRLogEntry entry);
extern void pretty_print_CHMOD(RRLogEntry entry);
extern void pretty_print_CHOWN(RRLogEntry entry);
extern void pretty_print_GETPID(RRLogEntry entry);
extern void pretty_print_MOUNT(RRLogEntry entry);
extern void pretty_print_UNMOUNT(RRLogEntry entry);
extern void pretty_print_SETUID(RRLogEntry entry);
extern void pretty_print_GETUID(RRLogEntry entry);
extern void pretty_print_GETEUID(RRLogEntry entry);
extern void pretty_print_SENDMSG(RRLogEntry entry);
extern void pretty_print_ACCEPT(RRLogEntry entry);
extern void pretty_print_GETPEERNAME(RRLogEntry entry);
extern void pretty_print_GETSOCKNAME(RRLogEntry entry);
extern void pretty_print_ACCESS(RRLogEntry entry);
extern void pretty_print_CHFLAGS(RRLogEntry entry);
extern void pretty_print_FCHFLAGS(RRLogEntry entry);
extern void pretty_print_GETPPID(RRLogEntry entry);
extern void pretty_print_DUP(RRLogEntry entry);
extern void pretty_print_GETEGID(RRLogEntry entry);
extern void pretty_print_GETGID(RRLogEntry entry);
extern void pretty_print_ACCT(RRLogEntry entry);
extern void pretty_print_REBOOT(RRLogEntry entry);
extern void pretty_print_SYMLINK(RRLogEntry entry);
extern void pretty_print_READLINK(RRLogEntry entry);
extern void pretty_print_UMASK(RRLogEntry entry);
extern void pretty_print_CHROOT(RRLogEntry entry);
extern void pretty_print_GETGROUPS(RRLogEntry entry);
extern void pretty_print_SETGROUPS(RRLogEntry entry);
extern void pretty_print_SETITIMER(RRLogEntry entry);
extern void pretty_print_SWAPON(RRLogEntry entry);
extern void pretty_print_GETITIMER(RRLogEntry entry);
extern void pretty_print_GETDTABLESIZE(RRLogEntry entry);
extern void pretty_print_DUP2(RRLogEntry entry);
extern void pretty_print_SELECT(RRLogEntry entry);
extern void pretty_print_FSYNC(RRLogEntry entry);
extern void pretty_print_SETPRIORITY(RRLogEntry entry);
extern void pretty_print_SOCKET(RRLogEntry entry);
extern void pretty_print_CONNECT(RRLogEntry entry);
extern void pretty_print_GETPRIORITY(RRLogEntry entry);
extern void pretty_print_BIND(RRLogEntry entry);
extern void pretty_print_SETSOCKOPT(RRLogEntry entry);
extern void pretty_print_LISTEN(RRLogEntry entry);
extern void pretty_print_GETRUSAGE(RRLogEntry entry);
extern void pretty_print_GETSOCKOPT(RRLogEntry entry);
extern void pretty_print_SETTIMEOFDAY(RRLogEntry entry);
extern void pretty_print_FCHOWN(RRLogEntry entry);
extern void pretty_print_FCHMOD(RRLogEntry entry);
extern void pretty_print_SETREUID(RRLogEntry entry);
extern void pretty_print_SETREGID(RRLogEntry entry);
extern void pretty_print_RENAME(RRLogEntry entry);
extern void pretty_print_FLOCK(RRLogEntry entry);
extern void pretty_print_MKFIFO(RRLogEntry entry);
extern void pretty_print_SENDTO(RRLogEntry entry);
extern void pretty_print_SHUTDOWN(RRLogEntry entry);
extern void pretty_print_MKDIR(RRLogEntry entry);
extern void pretty_print_RMDIR(RRLogEntry entry);
extern void pretty_print_UTIMES(RRLogEntry entry);
extern void pretty_print_ADJTIME(RRLogEntry entry);
extern void pretty_print_QUOTACTL(RRLogEntry entry);
extern void pretty_print_LGETFH(RRLogEntry entry);
extern void pretty_print_GETFH(RRLogEntry entry);
extern void pretty_print_SETFIB(RRLogEntry entry);
extern void pretty_print_NTP_ADJTIME(RRLogEntry entry);
extern void pretty_print_SETGID(RRLogEntry entry);
extern void pretty_print_SETEGID(RRLogEntry entry);
extern void pretty_print_SETEUID(RRLogEntry entry);
extern void pretty_print_PATHCONF(RRLogEntry entry);
extern void pretty_print_FPATHCONF(RRLogEntry entry);
extern void pretty_print_GETRLIMIT(RRLogEntry entry);
extern void pretty_print_SETRLIMIT(RRLogEntry entry);
extern void pretty_print_UNDELETE(RRLogEntry entry);
extern void pretty_print_FUTIMES(RRLogEntry entry);
extern void pretty_print_POLL(RRLogEntry entry);
extern void pretty_print_CLOCK_SETTIME(RRLogEntry entry);
extern void pretty_print_CLOCK_GETRES(RRLogEntry entry);
extern void pretty_print_FFCLOCK_GETCOUNTER(RRLogEntry entry);
extern void pretty_print_FFCLOCK_SETESTIMATE(RRLogEntry entry);
extern void pretty_print_FFCLOCK_GETESTIMATE(RRLogEntry entry);
extern void pretty_print_NTP_GETTIME(RRLogEntry entry);
extern void pretty_print_ISSETUGID(RRLogEntry entry);
extern void pretty_print_LCHOWN(RRLogEntry entry);
extern void pretty_print_LCHMOD(RRLogEntry entry);
extern void pretty_print_LUTIMES(RRLogEntry entry);
extern void pretty_print_FHOPEN(RRLogEntry entry);
extern void pretty_print_SETRESUID(RRLogEntry entry);
extern void pretty_print_SETRESGID(RRLogEntry entry);
extern void pretty_print_EXTATTRCTL(RRLogEntry entry);
extern void pretty_print_EXTATTR_SET_FILE(RRLogEntry entry);
extern void pretty_print_EXTATTR_GET_FILE(RRLogEntry entry);
extern void pretty_print_EXTATTR_DELETE_FILE(RRLogEntry entry);
extern void pretty_print_GETRESUID(RRLogEntry entry);
extern void pretty_print_GETRESGID(RRLogEntry entry);
extern void pretty_print_KQUEUE(RRLogEntry entry);
extern void pretty_print_EXTATTR_SET_FD(RRLogEntry entry);
extern void pretty_print_EXTATTR_GET_FD(RRLogEntry entry);
extern void pretty_print_EXTATTR_DELETE_FD(RRLogEntry entry);
extern void pretty_print_EACCESS(RRLogEntry entry);
extern void pretty_print_NMOUNT(RRLogEntry entry);
extern void pretty_print_KENV(RRLogEntry entry);
extern void pretty_print_LCHFLAGS(RRLogEntry entry);
extern void pretty_print_UUIDGEN(RRLogEntry entry);
extern void pretty_print_SENDFILE(RRLogEntry entry);
extern void pretty_print_EXTATTR_SET_LINK(RRLogEntry entry);
extern void pretty_print_EXTATTR_GET_LINK(RRLogEntry entry);
extern void pretty_print_EXTATTR_DELETE_LINK(RRLogEntry entry);
extern void pretty_print_SWAPOFF(RRLogEntry entry);
extern void pretty_print_EXTATTR_LIST_FD(RRLogEntry entry);
extern void pretty_print_EXTATTR_LIST_FILE(RRLogEntry entry);
extern void pretty_print_EXTATTR_LIST_LINK(RRLogEntry entry);
extern void pretty_print_AUDIT(RRLogEntry entry);
extern void pretty_print_AUDITON(RRLogEntry entry);
extern void pretty_print_GETAUID(RRLogEntry entry);
extern void pretty_print_SETAUID(RRLogEntry entry);
extern void pretty_print_GETAUDIT(RRLogEntry entry);
extern void pretty_print_SETAUDIT(RRLogEntry entry);
extern void pretty_print_GETAUDIT_ADDR(RRLogEntry entry);
extern void pretty_print_SETAUDIT_ADDR(RRLogEntry entry);
extern void pretty_print_AUDITCTL(RRLogEntry entry);
extern void pretty_print_PREAD(RRLogEntry entry);
extern void pretty_print_LSEEK(RRLogEntry entry);
extern void pretty_print_TRUNCATE(RRLogEntry entry);
extern void pretty_print_FTRUNCATE(RRLogEntry entry);
extern void pretty_print_CPUSET(RRLogEntry entry);
extern void pretty_print_CPUSET_SETID(RRLogEntry entry);
extern void pretty_print_CPUSET_GETID(RRLogEntry entry);
extern void pretty_print_CPUSET_GETAFFINITY(RRLogEntry entry);
extern void pretty_print_CPUSET_SETAFFINITY(RRLogEntry entry);
extern void pretty_print_FACCESSAT(RRLogEntry entry);
extern void pretty_print_FCHMODAT(RRLogEntry entry);
extern void pretty_print_FCHOWNAT(RRLogEntry entry);
extern void pretty_print_LINKAT(RRLogEntry entry);
extern void pretty_print_MKDIRAT(RRLogEntry entry);
extern void pretty_print_MKFIFOAT(RRLogEntry entry);
extern void pretty_print_READLINKAT(RRLogEntry entry);
extern void pretty_print_RENAMEAT(RRLogEntry entry);
extern void pretty_print_SYMLINKAT(RRLogEntry entry);
extern void pretty_print_UNLINKAT(RRLogEntry entry);
extern void pretty_print_LPATHCONF(RRLogEntry entry);
extern void pretty_print_CAP_ENTER(RRLogEntry entry);
extern void pretty_print_CAP_GETMODE(RRLogEntry entry);
extern void pretty_print_GETLOGINCLASS(RRLogEntry entry);
extern void pretty_print_SETLOGINCLASS(RRLogEntry entry);
extern void pretty_print_RCTL_GET_RACCT(RRLogEntry entry);
extern void pretty_print_RCTL_GET_RULES(RRLogEntry entry);
extern void pretty_print_RCTL_GET_LIMITS(RRLogEntry entry);
extern void pretty_print_RCTL_ADD_RULE(RRLogEntry entry);
extern void pretty_print_RCTL_REMOVE_RULE(RRLogEntry entry);
extern void pretty_print_POSIX_FALLOCATE(RRLogEntry entry);
extern void pretty_print_POSIX_FADVISE(RRLogEntry entry);
extern void pretty_print_CAP_RIGHTS_LIMIT(RRLogEntry entry);
extern void pretty_print_CAP_IOCTLS_LIMIT(RRLogEntry entry);
extern void pretty_print_CAP_IOCTLS_GET(RRLogEntry entry);
extern void pretty_print_CAP_FCNTLS_LIMIT(RRLogEntry entry);
extern void pretty_print_CAP_FCNTLS_GET(RRLogEntry entry);
extern void pretty_print_BINDAT(RRLogEntry entry);
extern void pretty_print_CONNECTAT(RRLogEntry entry);
extern void pretty_print_CHFLAGSAT(RRLogEntry entry);
extern void pretty_print_ACCEPT4(RRLogEntry entry);
extern void pretty_print_FDATASYNC(RRLogEntry entry);
extern void pretty_print_FSTAT(RRLogEntry entry);
extern void pretty_print_FSTATAT(RRLogEntry entry);
extern void pretty_print_FHSTAT(RRLogEntry entry);
extern void pretty_print_GETDIRENTRIES(RRLogEntry entry);
extern void pretty_print_STATFS(RRLogEntry entry);
extern void pretty_print_FSTATFS(RRLogEntry entry);
extern void pretty_print_GETFSSTAT(RRLogEntry entry);
extern void pretty_print_FHSTATFS(RRLogEntry entry);
extern void pretty_print_MKNODAT(RRLogEntry entry);
extern void pretty_print_KEVENT(RRLogEntry entry);
extern void pretty_print_NULL_EVENT(RRLogEntry entry);
extern void pretty_print_EXIT(RRLogEntry entry);
extern void pretty_print_DATA(RRLogEntry entry);
extern void pretty_print_LOCKED_EVENT(RRLogEntry entry);
extern void pretty_print_MUTEX_INIT(RRLogEntry entry);
extern void pretty_print_MUTEX_DESTROY(RRLogEntry entry);
extern void pretty_print_MUTEX_LOCK(RRLogEntry entry);
extern void pretty_print_MUTEX_TRYLOCK(RRLogEntry entry);
extern void pretty_print_MUTEX_UNLOCK(RRLogEntry entry);
extern void pretty_print_THREAD_CREATE(RRLogEntry entry);
extern void pretty_print_FORKEND(RRLogEntry entry);
extern void pretty_print_COND_INIT(RRLogEntry entry);
extern void pretty_print_COND_DESTROY(RRLogEntry entry);
extern void pretty_print_COND_WAIT(RRLogEntry entry);
extern void pretty_print_COND_SIGNAL(RRLogEntry entry);
extern void pretty_print_COND_BROADCAST(RRLogEntry entry);
extern void pretty_print_THREAD_ONCE(RRLogEntry entry);
extern void pretty_print_BARRIER_INIT(RRLogEntry entry);
extern void pretty_print_BARRIER_DESTROY(RRLogEntry entry);
extern void pretty_print_BARRIER_WAIT(RRLogEntry entry);
extern void pretty_print_SPIN_INIT(RRLogEntry entry);
extern void pretty_print_SPIN_DESTROY(RRLogEntry entry);
extern void pretty_print_SPIN_TRYLOCK(RRLogEntry entry);
extern void pretty_print_SPIN_LOCK(RRLogEntry entry);
extern void pretty_print_SPIN_UNLOCK(RRLogEntry entry);
extern void pretty_print_RWLOCK_INIT(RRLogEntry entry);
extern void pretty_print_RWLOCK_DESTROY(RRLogEntry entry);
extern void pretty_print_RWLOCK_UNLOCK(RRLogEntry entry);
extern void pretty_print_RWLOCK_RDLOCK(RRLogEntry entry);
extern void pretty_print_RWLOCK_TRYRDLOCK(RRLogEntry entry);
extern void pretty_print_RWLOCK_WRLOCK(RRLogEntry entry);
extern void pretty_print_RWLOCK_TRYWRLOCK(RRLogEntry entry);
extern void pretty_print_RWLOCK_TIMEDRDLOCK(RRLogEntry entry);
extern void pretty_print_RWLOCK_TIMEDWRLOCK(RRLogEntry entry);
extern void pretty_print_MUTEX_TIMEDLOCK(RRLogEntry entry);
extern void pretty_print_THREAD_EXIT(RRLogEntry entry);
extern void pretty_print_THREAD_DETACH(RRLogEntry entry);
extern void pretty_print_THREAD_JOIN(RRLogEntry entry);
extern void pretty_print_THREAD_KILL(RRLogEntry entry);
extern void pretty_print_THREAD_TIMEDJOIN(RRLogEntry entry);
extern void pretty_print_RDTSC(RRLogEntry entry);
extern void pretty_print_ATOMIC_LOAD(RRLogEntry entry);
extern void pretty_print_ATOMIC_STORE(RRLogEntry entry);
extern void pretty_print_ATOMIC_RMW(RRLogEntry entry);
extern void pretty_print_ATOMIC_XCHG(RRLogEntry entry);
extern void pretty_print_INLINE_ASM(RRLogEntry entry);
extern void pretty_print_WAIT(RRLogEntry entry);
extern void pretty_print_SEMCTL(RRLogEntry entry);
extern void pretty_print_SEM_OPEN(RRLogEntry entry);
extern void pretty_print_SEM_CLOSE(RRLogEntry entry);
extern void pretty_print_SEM_UNLINK(RRLogEntry entry);
extern void pretty_print_SEM_POST_START(RRLogEntry entry);
extern void pretty_print_SEM_POST_END(RRLogEntry entry);
extern void pretty_print_SEM_WAIT(RRLogEntry entry);
extern void pretty_print_SEM_TRYWAIT(RRLogEntry entry);
extern void pretty_print_SEM_TIMEDWAIT(RRLogEntry entry);
extern void pretty_print_SEM_GETVALUE(RRLogEntry entry);
extern void pretty_print_SEM_INIT(RRLogEntry entry);
extern void pretty_print_SEM_DESTROY(RRLogEntry entry);
extern void pretty_print_MMAPFD(RRLogEntry entry);
extern void pretty_print_PIPE(RRLogEntry entry);
extern void pretty_print_GETTIME(RRLogEntry entry);
extern void pretty_print_SYSCTL(RRLogEntry entry);
extern void pretty_print_GETCWD(RRLogEntry entry);
extern void pretty_print_SHMGET(RRLogEntry entry);
extern void pretty_print_SEMGET(RRLogEntry entry);
extern void pretty_print_SEMOP(RRLogEntry entry);
extern void pretty_print_SHM_OPEN(RRLogEntry entry);
extern void pretty_print_SHM_UNLINK(RRLogEntry entry);
extern void pretty_print_OPEN(RRLogEntry entry);
extern void pretty_print_OPENAT(RRLogEntry entry);
extern void pretty_print_CLOSE(RRLogEntry entry);
extern void pretty_print_WRITE(RRLogEntry entry);
extern void pretty_print_IOCTL(RRLogEntry entry);
extern void pretty_print_FCNTL(RRLogEntry entry);
extern void pretty_print_PWRITE(RRLogEntry entry);
extern void pretty_print_READV(RRLogEntry entry);
extern void pretty_print_PREADV(RRLogEntry entry);
extern void pretty_print_WRITEV(RRLogEntry entry);
extern void pretty_print_RECVMSG(RRLogEntry entry);
extern void pretty_print_RECVFROM(RRLogEntry entry);
extern void pretty_print_PIPE2(RRLogEntry entry);
extern void pretty_print_GETTIMEOFDAY(RRLogEntry entry);
extern void pretty_print_FORK(RRLogEntry entry);
#endif
