/* ISC license. */

#include <sys/stat.h>

#include <skalibs/sgetopt.h>
#include <skalibs/types.h>
#include <skalibs/strerr.h>
#include <skalibs/djbunix.h>

#define USAGE "s6-mkfifo [ -m mode ] fifo..."

int main (int argc, char const *const *argv)
{
  unsigned int mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH ;
  PROG = "s6-mkfifo" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "m:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'm': if (uint0_oscan(l.arg, &mode)) break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) strerr_dieusage(100, USAGE) ;
  umask(S_IXUSR|S_IXGRP|S_IXOTH) ;
  for (; *argv ; argv++)
    if (mkfifo(*argv, mode) < 0)
      strerr_diefu2sys(111, "mkfifo ", *argv) ;
  return 0 ;
}
