/* ISC license. */

#include <skalibs/allreadwrite.h>
#include <skalibs/sgetopt.h>
#include <skalibs/types.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/skamisc.h>
#include <skalibs/random.h>

#define USAGE "s6-uniquename [ -n randomlen ] prefix"
#define usage() strerr_dieusage(100, USAGE)

int main (int argc, char const *const *argv)
{
  stralloc sa = STRALLOC_ZERO ;
  unsigned int n = 8 ;
  PROG = "s6-uniquename" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "n:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : if (!uint0_scan(l.arg, &n)) usage() ; break ;
        default : usage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (argc < 1) usage() ;
  if (!random_init())
    strerr_diefu1sys(111, "init random generator") ;
  if (!stralloc_cats(&sa, argv[0])) strerr_diefu1sys(111, "stralloc_cats") ;
  if (!(n ? random_sauniquename(&sa, n) : sauniquename(&sa)))
    strerr_diefu1sys(111, "make unique name") ;
  if (!stralloc_catb(&sa, "\n", 1)) strerr_diefu1sys(111, "stralloc_cats") ;
  if (allwrite(1, sa.s, sa.len) < sa.len) strerr_diefu1sys(111, "write to stdout") ;
  return 0 ;
}
