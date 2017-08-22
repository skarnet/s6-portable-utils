/* ISC license. */

#include <unistd.h>
#include <skalibs/strerr2.h>
#include <skalibs/iobuffer.h>
#include <skalibs/djbunix.h>

#define USAGE "seekablepipe tempfile prog..."

#define N 8192

int main (int argc, char const *const *argv, char const *const *envp)
{
  iobuffer b ;
  int fdr, fdw ;
  int r ;
  PROG = "seekablepipe" ;
  if (argc < 3) strerr_dieusage(100, USAGE) ;
  fdw = open_trunc(argv[1]) ;
  if (fdw < 0)
    strerr_diefu2sys(111, "create temporary ", argv[1]) ;
  fdr = open_readb(argv[1]) ;
  if (fdr < 0)
    strerr_diefu3sys(111, "open ", argv[1], " for reading") ;
  if (unlink(argv[1]) < 0)
    strerr_diefu2sys(111, "unlink ", argv[1]) ;
  if (ndelay_off(fdw) < 0)
    strerr_diefu1sys(111, "set fdw blocking") ;
  if (!iobuffer_init(&b, 0, fdw))
    strerr_diefu1sys(111, "iobuffer_init") ;
  while ((r = iobuffer_fill(&b)) > 0)
    if (!iobuffer_flush(&b))
      strerr_diefu2sys(111, "write to ", argv[1]) ;
  if (r < 0) strerr_diefu1sys(111, "read from stdin") ;
  iobuffer_finish(&b) ;
  fd_close(fdw) ;
  if (fd_move(0, fdr) < 0)
    strerr_diefu1sys(111, "move fdr to stdin") ;
  xpathexec_run(argv[2], argv+2, envp) ;
}
