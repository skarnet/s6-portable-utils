/* ISC license. */

#include <unistd.h>
#include <signal.h>
#include <skalibs/types.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr.h>
#include <skalibs/sig.h>

#define USAGE "s6-pause [ -t ] [ -h ] [ -a ] [ -q ] [ -b ] [ -i ] [ -p signal,signal... ]"
#define dieusage() strerr_dieusage(100, USAGE)

#define PAUSE_MAX 64

int main (int argc, char const *const *argv)
{
  PROG = "s6-pause" ;
  unsigned int sigs[PAUSE_MAX] ;
  size_t nsig = 0 ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "thaqbip:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 't' : if (nsig >= PAUSE_MAX) dieusage() ; sigs[nsig++] = SIGTERM ; break ;
        case 'h' : if (nsig >= PAUSE_MAX) dieusage() ; sigs[nsig++] = SIGHUP ; break ;
        case 'a' : if (nsig >= PAUSE_MAX) dieusage() ; sigs[nsig++] = SIGALRM ; break ;
        case 'q' : if (nsig >= PAUSE_MAX) dieusage() ; sigs[nsig++] = SIGQUIT ; break ;
        case 'b' : if (nsig >= PAUSE_MAX) dieusage() ; sigs[nsig++] = SIGABRT ; break ;
        case 'i' : if (nsig >= PAUSE_MAX) dieusage() ; sigs[nsig++] = SIGINT ; break ;
        case 'p' :
        {
          size_t n ;
          if (!uint_scanlist(sigs + nsig, PAUSE_MAX - nsig, l.arg, &n)) dieusage() ;
          nsig += n ;
          break ;
        }
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }

  while (nsig--) sig_ignore(sigs[nsig]) ;
  pause() ;
  return 0 ;
}
