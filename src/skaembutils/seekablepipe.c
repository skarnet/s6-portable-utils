/* ISC license. */

#include <unistd.h>

#include <skalibs/posixplz.h>
#include <skalibs/strerr.h>
#include <skalibs/djbunix.h>
#include <skalibs/exec.h>

#define USAGE "seekablepipe tempfile prog..."

#define N 8192

int main (int argc, char const *const *argv)
{
  int fdr, fdw ;
  PROG = "seekablepipe" ;
  if (argc < 3) strerr_dieusage(100, USAGE) ;
  fdw = open_trunc(argv[1]) ;
  if (fdw < 0)
    strerr_diefu2sys(111, "create temporary ", argv[1]) ;
  fdr = open_readb(argv[1]) ;
  if (fdr < 0)
    strerr_diefu3sys(111, "open ", argv[1], " for reading") ;
  unlink_void(argv[1]) ;
  if (ndelay_off(fdw) < 0)
    strerr_diefu1sys(111, "set fdw blocking") ;
  if (fd_cat(0, fdw) < 0)
    strerr_diefu1sys(111, "read from stdin") ;
  close(fdw) ;
  if (fd_move(0, fdr) < 0)
    strerr_diefu1sys(111, "move temporary file descriptor") ;
  xexec(argv+2) ;
}
