/* ISC license. */

#include <skalibs/sgetopt.h>
#include <skalibs/bytestr.h>
#include <skalibs/direntry.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr2.h>

#define USAGE "s6-ls [ -0 ] [ -a | -A ] [ -x exclude ] dir"

int main (int argc, char const *const *argv)
{
  unsigned int all = 0 ;
  char const *exclude = 0 ;
  char delim = '\n' ;
  PROG = "s6-ls" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "0aAx:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case '0': delim = '\0' ; break ;
        case 'a': all = 1 ; break ;
        case 'A': all = 2 ; break ;
        case 'x': exclude = l.arg ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) strerr_dieusage(100, USAGE) ;
  {
    direntry *d ;
    DIR *dir = opendir(*argv) ;
    if (!dir)
      strerr_diefu2sys(111, "open directory ", *argv) ;
    while ((d = readdir(dir)))
    {
      if ((d->d_name[0] == '.') && (all < 2))
      {
        if (!all || !d->d_name[1] || ((d->d_name[1] == '.') && !d->d_name[2])) continue ;
      }
      if (exclude && !str_diff(exclude, d->d_name)) continue ;
      if ((buffer_puts(buffer_1, d->d_name) < 0)
       || (buffer_put(buffer_1, &delim, 1) < 0))
        strerr_diefu1sys(111, "write to stdout") ;
    }
    dir_close(dir) ;
  }
  if (!buffer_flush(buffer_1))
    strerr_diefu1sys(111, "write to stdout") ;
  return 0 ;
}
