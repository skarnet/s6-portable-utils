/* ISC license. */

#include <errno.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/buffer.h>
#include <skalibs/stralloc.h>
#include <skalibs/skamisc.h>

#define USAGE "s6-quote-filter [ -u ] [ -d delim ]"

int main (int argc, char const *const *argv)
{
  stralloc src = STRALLOC_ZERO ;
  stralloc dst = STRALLOC_ZERO ;
  char const *delim = "\"" ;
  unsigned int delimlen ;
  unsigned int startquote = 1 ;
  PROG = "s6-quote-filter" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "ud:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'u' : startquote = 0 ; break ;
        case 'd': delim = l.arg ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  delimlen = str_len(delim) ;
  if (startquote)
  {
    if(!delimlen) strerr_dief1x(100, "no character to quote with!") ;
    if (!stralloc_catb(&dst, delim, 1))
      strerr_diefu1sys(111, "stralloc_catb") ;
  }
  for (;;)
  {
    int r ;
    src.len = 0 ;
    r = skagetln(buffer_0f1, &src, '\n') ;
    if (!r) break ;
    if ((r < 0) && (errno != EPIPE))
      strerr_diefu1sys(111, "read from stdin") ;
    dst.len = startquote ;
    if (!string_quote_nodelim_mustquote(&dst, src.s, src.len - (r > 0), delim, delimlen))
    {
      int e = errno ;
      buffer_flush(buffer_1) ;
      errno = e ;
      strerr_diefu1sys(111, "quote") ;
    }
    if (startquote)
    {
      if (!stralloc_catb(&dst, delim, 1))
        strerr_diefu1sys(111, "stralloc_catb") ;
    }
    if (r > 0)
    {
      if (!stralloc_catb(&dst, "\n", 1))
        strerr_diefu1sys(111, "stralloc_catb") ;
    }
    if (buffer_put(buffer_1, dst.s, dst.len) < 0)
      strerr_diefu1sys(111, "write to stdout") ;
  }
  return 0 ;
}
