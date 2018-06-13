#include <unistd.h>
#include <errno.h>

struct xlat {
	int val;
	const char *str;
};

#define	X(a)	{ a, #a },
#define	XEND	{ 0, NULL }

struct xlat errno_xlat_table[] = {
X(EPERM)
X(ENOENT)
X(ESRCH)
X(EINTR)
X(EIO)
X(ENXIO)
X(E2BIG)
X(ENOEXEC)
X(EBADF)
X(ECHILD)
X(EDEADLK)
X(ENOMEM)
X(EACCES)
X(EFAULT)
X(ENOTBLK)
X(EBUSY)
X(EEXIST)
X(EXDEV)
X(ENODEV)
X(ENOTDIR)
X(EISDIR)
X(EINVAL)
X(ENFILE)
X(EMFILE)
X(ENOTTY)
X(ETXTBSY)
X(EFBIG)
X(ENOSPC)
X(ESPIPE)
X(EROFS)
X(EMLINK)
X(EPIPE)
X(EDOM)
X(ERANGE)
X(EAGAIN)
X(EINPROGRESS)
X(EALREADY)
X(ENOTSOCK)
X(EDESTADDRREQ)
X(EMSGSIZE)
X(EPROTOTYPE)
X(ENOPROTOOPT)
X(EPROTONOSUPPORT)
X(ESOCKTNOSUPPORT)
X(EOPNOTSUPP)
X(EPFNOSUPPORT)
X(EAFNOSUPPORT)
X(EADDRINUSE)
X(EADDRNOTAVAIL)
X(ENETDOWN)
X(ENETUNREACH)
X(ENETRESET)
X(ECONNABORTED)
X(ECONNRESET)
X(ENOBUFS)
X(EISCONN)
X(ENOTCONN)
X(ESHUTDOWN)
X(ETOOMANYREFS)
X(ETIMEDOUT)
X(ECONNREFUSED)
X(ELOOP)
X(ENAMETOOLONG)
X(EHOSTDOWN)
X(EHOSTUNREACH)
X(ENOTEMPTY)
X(EPROCLIM)
X(EUSERS)
X(EDQUOT)
X(ESTALE)
X(EREMOTE)
X(EBADRPC)
X(ERPCMISMATCH)
X(EPROGUNAVAIL)
X(EPROGMISMATCH)
X(EPROCUNAVAIL)
X(ENOLCK)
X(ENOSYS)
X(EFTYPE)
X(EAUTH)
X(ENEEDAUTH)
X(EIDRM)
X(ENOMSG)
X(EOVERFLOW)
X(ECANCELED)
X(EILSEQ)
X(ENOATTR)
X(EDOOFUS)
X(EBADMSG)
X(EMULTIHOP)
X(ENOLINK)
X(EPROTO)
X(ENOTCAPABLE)
X(ECAPMODE)
X(ENOTRECOVERABLE)
X(EOWNERDEAD)
X(ELAST)
XEND
};

const char * xlat_errno(int errnum) {
    for (int i = 0; errno_xlat_table[i].str != 0; i++) {
	if (errno_xlat_table[i].val == errnum) {
	    return errno_xlat_table[i].str;
	}
    }
    return NULL;
}