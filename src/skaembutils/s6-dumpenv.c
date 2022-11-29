/* ISC license. */

#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/uio.h>

#include <skalibs/types.h>
#include <skalibs/bytestr.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr.h>
#include <skalibs/djbunix.h>

#define USAGE "s6-dumpenv [ -N | -n ] [ -m mode ] envdir"
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const *const *argv, char const *const *envp)
{
  unsigned int mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ;
  int chomp = 0 ;
  size_t dirlen ;
  PROG = "s6-dumpenv" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "nNm:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : chomp = 0 ; break ;
        case 'N' : chomp = 1 ; break ;
        case 'm' : if (!uint0_oscan(l.arg, &mode)) dieusage() ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) dieusage() ;

  if (mkdir(argv[0], mode) < 0)
  {
    struct stat st ;
    if (errno != EEXIST) strerr_diefu2sys(111, "mkdir ", argv[0]) ;
    if (stat(argv[0], &st) < 0)
      strerr_diefu2sys(111, "stat ", argv[0]) ;
    if (!S_ISDIR(st.st_mode))
    {
      errno = ENOTDIR ;
      strerr_diefu2sys(111, "mkdir ", argv[0]) ;
    }
  }
  dirlen = strlen(argv[0]) ;
    
  for (; *envp ; envp++)
  {
    size_t varlen = str_chr(*envp, '=') ;
    struct iovec const v[2] =
    {
      { .iov_base = (char *)*envp + varlen + 1, .iov_len = strlen(*envp + varlen + 1) },
      { .iov_base = "\n", .iov_len = 1 }
    } ;
    char fn[dirlen + varlen + 2] ;
    memcpy(fn, argv[0], dirlen) ;
    fn[dirlen] = '/' ;
    memcpy(fn + dirlen + 1, *envp, varlen) ;
    fn[dirlen + 1 + varlen] = 0 ;
    if (!openwritevnclose_suffix(fn, v, 1 + chomp, "=.tmp"))
      strerr_diefu2sys(111, "open ", fn) ;
  }
  return 0 ;
}
