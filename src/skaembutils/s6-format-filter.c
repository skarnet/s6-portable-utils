/* ISC license. */

#include <errno.h>
#include <skalibs/strerr2.h>
#include <skalibs/buffer.h>
#include <skalibs/stralloc.h>
#include <skalibs/skamisc.h>

#define USAGE "s6-format-filter format [ args... ]"

int main (int argc, char const *const *argv)
{
  stralloc src = STRALLOC_ZERO ;
  stralloc dst = STRALLOC_ZERO ;
  char vars[12] = "s0123456789" ;
  char const *args[12] = { "" } ;
  char const *format ;
  PROG = "s6-format-filter" ;
  argc-- ; args[1] = *argv++ ;
  if (!argc--) strerr_dieusage(100, USAGE) ;
  format = *argv++ ;
  if (argc > 9) argc = 9 ;
  vars[argc+2] = 0 ;
  {
    register unsigned int i = 0 ;
    for (; i < (unsigned int)argc ; i++) args[2+i] = argv[i] ;
  }
  if (!string_format(&dst, vars, format, args))
    strerr_diefu1sys(111, "compile format") ;

  for (;;)
  {
    register int r ;
    src.len = 0 ;
    dst.len = 0 ;
    r = skagetln(buffer_0f1, &src, '\n') ;
    if (!r) break ;
    else if (r < 0)
    {
      if ((errno != EPIPE) || !stralloc_0(&src))
        strerr_diefu1sys(111, "read from stdin") ;
    }
    else src.s[src.len-1] = 0 ;
    args[0] = src.s ;    
    if (!string_format(&dst, vars, format, args))
    {
      int e = errno ;
      buffer_flush(buffer_1) ;
      errno = e ;
      strerr_diefu1sys(111, "format") ;
    }
    if (r > 0)
    {
      if (!stralloc_catb(&dst, "\n", 1))
        strerr_diefu1sys(111, "format") ;
    }
    if (buffer_put(buffer_1, dst.s, dst.len) < 0)
      strerr_diefu1sys(111, "write to stdout") ;
  }
  return 0 ;
}
