/* ISC license. */

#include <unistd.h>
#include <skalibs/sgetopt.h>
#include <skalibs/types.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>

#define USAGE "s6-chown [ -U ] [ -u uid ] [ -g gid ] file"

int main (int argc, char const *const *argv, char const *const *envp)
{
  uid_t uid = -1 ;
  gid_t gid = -1 ;
  PROG = "s6-chown" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "Uu:g:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'u':
        {
          if (!uid0_scan(l.arg, &uid)) strerr_dieusage(100, USAGE) ;
          break ;
        }
        case 'g':
        {
          if (!gid0_scan(l.arg, &gid)) strerr_dieusage(100, USAGE) ;
          break ;
        }
        case 'U':
        {
          char const *s = env_get2(envp, "UID") ;
          if (!s) strerr_dienotset(100, "UID") ;
          if (!uid0_scan(s, &uid)) strerr_dieinvalid(100, "UID") ;
          s = env_get2(envp, "GID") ;
          if (!s) strerr_dienotset(100, "GID") ;
          if (!gid0_scan(s, &gid)) strerr_dieinvalid(100, "GID") ;
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
