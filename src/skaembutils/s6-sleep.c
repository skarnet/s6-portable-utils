/* ISC license. */

#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/types.h>
#include <skalibs/tai.h>
#include <skalibs/djbunix.h>
#include <skalibs/iopause.h>

#define USAGE "s6-sleep [ -m ] duration prog..."

int main (int argc, char const *const *argv, char const *const *envp)
{
  unsigned int n ;
  int milli = 0 ;
  PROG = "s6-sleep" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "m", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'm': milli = 1 ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) strerr_dieusage(100, USAGE) ;
  if (!uint0_scan(argv[0], &n)) strerr_dieusage(100, USAGE) ;

  {
    tain_t deadline ;
    if (milli) tain_from_millisecs(&deadline, n) ;
    else tain_uint(&deadline, n) ;
    tain_now_g() ;
    tain_add_g(&deadline, &deadline) ;
    deepsleepuntil_g(&deadline) ;
  }

  pathexec0_run(argv+1, envp) ;
  strerr_dieexec(111, argv[1]) ;
}
