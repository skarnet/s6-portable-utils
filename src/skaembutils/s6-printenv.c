/* ISC license. */

#include <string.h>
#include <skalibs/sgetopt.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr.h>
#include <skalibs/netstring.h>

#define USAGE "s6-printenv [ -n ] [ -0 | -d delimchar ]"

int main (int argc, char const *const *argv, char const *const *envp)
{
  char delim = '\n' ;
  int zero = 0, nl = 1 ;
  PROG = "s6-printenv" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "nd:0", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : nl = 0 ; break ;
        case 'd' : delim = *l.arg ; break ;
        case '0' : zero = 1 ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (zero) delim = 0 ;
  for (; *envp ; envp++)
  {
    if (delim || zero)
    {
      if ((buffer_puts(buffer_1, *envp) < 0)
       || ((nl || envp[1]) && (buffer_put(buffer_1, &delim, 1) < 0)))
        strerr_diefu1sys(111, "write to stdout") ;
    }
    else
    {
      size_t written = 0 ;
      if (!netstring_put(buffer_1, *envp, strlen(*envp), &written))
        strerr_diefu1sys(111, "write a netstring to stdout") ;
    }
  }
  if (!buffer_flush(buffer_1))
    strerr_diefu1sys(111, "write to stdout") ;
  return 0 ;
}
