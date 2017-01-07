/* ISC license. */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <skalibs/sgetopt.h>
#include <skalibs/bytestr.h>
#include <skalibs/uint.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr2.h>

#define USAGE "s6-mkdir [ -p ] [ -v ] [ -m mode ] dir"

static int doit (char const *s, unsigned int mode, int verbose, int ee)
{
  if (mkdir(s, mode) == -1)
  {
    if (ee || (errno != EEXIST))
    {
      strerr_warnwu2sys("mkdir ", s) ;
      return 111 ;
    }
  }
  else if (verbose)
  {
    buffer_puts(buffer_2, PROG) ;
    buffer_puts(buffer_2, ": created directory ") ;
    buffer_puts(buffer_2, s) ;
    buffer_putflush(buffer_2, "\n", 1) ;
  }
  return 0 ;
}

static int doparents (char const *s, unsigned int mode, int verbose)
{
  size_t n = str_len(s), i = 0 ;
  char tmp[n+1] ;
  for (; i < n ; i++)
  {
    if ((s[i] == '/') && i)
    {
      register int e ;
      tmp[i] = 0 ;
      e = doit(tmp, mode, verbose, 0) ;
      if (e) return e ;
    }
    tmp[i] = s[i] ;
  }
  return doit(s, mode, verbose, 0) ;
}

int main (int argc, char const *const *argv)
{
  int parents = 0, verbose = 0 ;
  unsigned int mode = 0777 ;
  int e = 0 ;
  PROG = "s6-mkdir" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "pvm:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'p': parents = 1 ; break ;
        case 'v': verbose = 1 ; break ;
        case 'm': if (uint_oscan(l.arg, &mode)) break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  for ( ; *argv ; argv++)
    e |= parents ? doparents(*argv, mode, verbose) :
                   doit(*argv, mode, verbose, 1) ;
  return e ;
}
