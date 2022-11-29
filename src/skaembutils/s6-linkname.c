/* ISC license. */

#include <skalibs/buffer.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>

#define USAGE "s6-linkname [ -n ] [ -f ] link"
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const *const *argv)
{
  stralloc sa = STRALLOC_ZERO ;
  int path = 0, nl = 1 ;
  PROG = "s6-linkname" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "nf", &l) ;
      if (opt == -1) break ;
      switch(opt)
      {
        case 'n' : nl = 0 ; break ;
        case 'f' : path = 1 ; break ;
        default :  dieusage() ;
      }
    }
    argv += l.ind ; argc -= l.ind ;
  }
  if (!argc) dieusage() ;

  if ((path ? sarealpath(&sa, *argv) : sareadlink(&sa, *argv)) < 0)
    strerr_diefu2sys(111, "resolve ", *argv) ;

  if ((buffer_put(buffer_1small, sa.s, sa.len) < 0)
   || (nl && (buffer_put(buffer_1small, "\n", 1)) < 0)
   || (!buffer_flush(buffer_1small)))
    strerr_diefu1sys(111, "write to stdout") ;

 /* stralloc_free(&sa) ; */
  return 0 ;
}
