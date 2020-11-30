/* ISC license. */

#include <unistd.h>
#include <errno.h>

#include <skalibs/sgetopt.h>
#include <skalibs/types.h>
#include <skalibs/strerr2.h>
#include <skalibs/exec.h>

#define USAGE "s6-nice [ -I | -i ] [ -n value ] prog..."

int main (int argc, char const *const *argv)
{
  int incr = 10 ;
  int strict = 0 ;
  PROG = "s6-nice" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "Iin:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'I' : strict = 0 ; break ;
        case 'i' : strict = 1 ; break ;
        case 'n': if (!int_scan(l.arg, &incr)) strerr_dieusage(100, USAGE) ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) strerr_dieusage(100, USAGE) ;

  errno = 0 ;
  if ((nice(incr) < 0) && errno)
  {
    char fmt[1+UINT_FMT] ;
    fmt[int_fmt(fmt, incr)] = 0 ;
    if (strict) strerr_diefu2sys(111, "nice to ", fmt) ;
    else strerr_warnwu2sys("nice to ", fmt) ;
  }
  xexec(argv) ;
}
