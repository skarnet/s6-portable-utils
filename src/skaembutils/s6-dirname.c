/* ISC license. */

#include <string.h>

#include <skalibs/sgetopt.h>
#include <skalibs/allreadwrite.h>
#include <skalibs/strerr.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>

#define USAGE "s6-dirname [ -n ] file"

int main (int argc, char const *const *argv)
{
  stralloc sa = STRALLOC_ZERO ;
  int nl = 1 ;
  PROG = "s6-dirname" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "n", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : nl = 0 ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }

  if (!argc) strerr_dieusage(100, USAGE) ;
  if (!sadirname(&sa, argv[0], strlen(argv[0])))
    strerr_diefu2sys(111, "get dirname of ", argv[0]) ;
  if (nl && !stralloc_catb(&sa, "\n", 1))
    strerr_diefu2sys(111, "get dirname of ", argv[0]) ;
  if (allwrite(1, sa.s, sa.len) < sa.len)
    strerr_diefu1sys(111, "write to stdout") ;
  return 0 ;
}
