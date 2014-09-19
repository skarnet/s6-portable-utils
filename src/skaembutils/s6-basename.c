/* ISC license. */

#include <skalibs/sgetopt.h>
#include <skalibs/allreadwrite.h>
#include <skalibs/bytestr.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>

#define USAGE "s6-basename [ -n ] file [ suffix ]"

int main (int argc, char const *const *argv)
{
  stralloc sa = STRALLOC_ZERO ;
  int nl = 1 ;
  PROG = "s6-basename" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "n", &l) ;
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
  if (!sabasename(&sa, argv[0], str_len(argv[0])))
    strerr_diefu2sys(111, "get basename of ", argv[0]) ;
  if (argc >= 2)
  {
    unsigned int n = str_len(argv[1]) ;
    if ((n < sa.len) && !byte_diff(argv[1], n, sa.s + sa.len - n))
      sa.len -= n ;
  }
  if (nl && !stralloc_catb(&sa, "\n", 1))
    strerr_diefu2sys(111, "get basename of ", argv[0]) ;
  if (allwrite(1, sa.s, sa.len) < sa.len)
    strerr_diefu1sys(111, "write to stdout") ;
  return 0 ;
}
