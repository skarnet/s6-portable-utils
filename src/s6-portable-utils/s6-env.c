/* ISC license. */

#include <string.h>
#include <errno.h>

#include <skalibs/sgetopt.h>
#include <skalibs/strerr.h>
#include <skalibs/env.h>
#include <skalibs/exec.h>

#include <s6-portable-utils/config.h>

#define USAGE "s6-env [ -i ] [ name=value... ] prog..."

int main (int argc, char const *const *argv, char const *const *envp)
{
  stralloc modifs = STRALLOC_ZERO ;
  char const *arg_zero[2] = { S6_PORTABLE_UTILS_BINPREFIX "s6-printenv", 0 } ;
  char const *env_zero[1] = { 0 } ;
  PROG = "s6-env" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "i", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'i': envp = env_zero ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  for (; argc ; argc--, argv++)
  {
    if (!strchr(*argv, '=')) break ;
    if (!stralloc_cats(&modifs, *argv) || !stralloc_0(&modifs))
      strerr_diefu1sys(111, "stralloc_cats") ;
  }
  if (!argc) argv = arg_zero ;
  xmexec_em(argv, envp, modifs.s, modifs.len) ;
}
