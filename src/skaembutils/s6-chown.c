/* ISC license. */

#include <sys/types.h>
#include <unistd.h>
#include <skalibs/sgetopt.h>
#include <skalibs/uint.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>

#define USAGE "s6-chown [ -U ] [ -u uid ] [ -g gid ] file"

int main (int argc, char const *const *argv, char const *const *envp)
{
  int uid = -1, gid = -1 ;
  PROG = "s6-chown" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "Uu:g:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'u':
        {
          unsigned int u ;
          if (!uint0_scan(l.arg, &u)) strerr_dieusage(100, USAGE) ;
          uid = u ;
          break ;
        }
        case 'g':
        {
          unsigned int g ;
          if (!uint0_scan(l.arg, &g)) strerr_dieusage(100, USAGE) ;
          gid = g ;
          break ;
        }
        case 'U':
        {
          unsigned int x ;
          char const *s = env_get2(envp, "UID") ;
          if (!s) strerr_dienotset(100, "UID") ;
          if (!uint0_scan(s, &x)) strerr_dieinvalid(100, "UID") ;
          uid = x ;
          s = env_get2(envp, "GID") ;
          if (!s) strerr_dienotset(100, "GID") ;
          if (!uint0_scan(s, &x)) strerr_dieinvalid(100, "GID") ;
          gid = x ;
          break ;
        }
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) strerr_dieusage(100, USAGE) ;
  if (chown(*argv, uid, gid) == -1)
    strerr_diefu2sys(111, "chown ", argv[0]) ;
  return 0 ;
}
