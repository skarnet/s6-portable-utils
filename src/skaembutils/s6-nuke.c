/* ISC license. */

#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/sig.h>

#define USAGE "s6-nuke [ -h | -t | -k ]"

int main (int argc, char const *const *argv)
{
  int doterm = 0, dohangup = 0, dokill = 0 ;
  PROG = "s6-nuke" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "htk", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'h': dohangup = 1 ; break ;
        case 't': doterm = 1 ; break ;
        case 'k': dokill = 1 ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }

  if (dohangup)
  {
    sig_ignore(SIGHUP) ;
    kill(-1, SIGHUP) ;
  }

  if (doterm)
  {
    sig_ignore(SIGTERM) ;
    kill(-1, SIGTERM) ;
    kill(-1, SIGCONT) ;
  }

  if (dokill) kill(-1, SIGKILL) ;

  if (errno) strerr_diefu1sys(111, "kill") ;
  return 0 ;
}
