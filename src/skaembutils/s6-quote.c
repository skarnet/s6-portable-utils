/* ISC license. */

#include <string.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr.h>
#include <skalibs/allreadwrite.h>
#include <skalibs/stralloc.h>
#include <skalibs/skamisc.h>

#define USAGE "s6-quote [ -n ] [ -u ] [ -d delim ] string"

int main (int argc, char const *const *argv)
{
  stralloc sa = STRALLOC_ZERO ;
  char const *delim = "\"" ;
  size_t delimlen ;
  size_t startquote = 1 ;
  int nl = 1 ;
  PROG = "s6-quote" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "nud:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : nl = 0 ; break ;
        case 'u' : startquote = 0 ; break ;
        case 'd': delim = l.arg ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) strerr_dieusage(100, USAGE) ;
  delimlen = strlen(delim) ;
  if (startquote)
  {
    if (!delimlen) strerr_dief1x(100, "no character to quote with!") ;
    if (!stralloc_catb(&sa, delim, 1))
      strerr_diefu1sys(111, "stralloc_catb") ;
  }
  if (!string_quote_nodelim_mustquote(&sa, *argv, strlen(*argv), delim, delimlen))
    strerr_diefu1sys(111, "quote") ;
  if (startquote)
  {
    if (!stralloc_catb(&sa, delim, 1))
      strerr_diefu1sys(111, "stralloc_catb") ;
  }
  if (nl)
  {
    if (!stralloc_catb(&sa, "\n", 1))
      strerr_diefu1sys(111, "stralloc_catb") ;
  }
  if (allwrite(1, sa.s, sa.len) < sa.len)
    strerr_diefu1sys(111, "write to stdout") ;
  return 0 ;
}
