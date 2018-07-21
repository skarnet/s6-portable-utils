/* ISC license. */

#include <skalibs/sysdeps.h>

#ifdef SKALIBS_HASLINKAT
#include <skalibs/nonposix.h>
#endif

#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <skalibs/posixplz.h>
#include <skalibs/sgetopt.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>
#include <skalibs/random.h>
#include <skalibs/skamisc.h>

#define USAGE "s6-ln [ -n ] [ -s ] [ -f ] [ -L ] [ -P ] src... dest"

typedef int linkfunc_t (char const *, char const *) ;
typedef linkfunc_t *linkfunc_t_ref ;

typedef void ln_t (char const *, char const *, linkfunc_t_ref) ;
typedef ln_t *ln_t_ref ;

#ifdef SKALIBS_HASLINKAT

static int linknoderef (char const *old, char const *new)
{
  return linkat(AT_FDCWD, old, AT_FDCWD, new, 0) ;
}

static int linkderef (char const *old, char const *new)
{
  return linkat(AT_FDCWD, old, AT_FDCWD, new, AT_SYMLINK_FOLLOW) ;
}

#else /* can't implement SUSv4, default to link */

# define linknoderef link
# define linkderef link

#endif

static void force (char const *old, char const *new, linkfunc_t_ref doit)
{
  if ((*doit)(old, new) == -1)
  {
    size_t base = satmp.len ;
    if (errno != EEXIST)
      strerr_diefu5sys(111, "make a link", " from ", new, " to ", old) ;
    if (!stralloc_cats(&satmp, new)
     || !random_sauniquename(&satmp, 8)
     || !stralloc_0(&satmp))
      strerr_diefu2sys(111, "make a unique name for ", old) ;
    if ((*doit)(old, satmp.s + base) == -1)
      strerr_diefu3sys(111, "make a link", " to ", old) ;
    if (rename(satmp.s + base, new) == -1)
    {
      unlink_void(satmp.s + base) ;
      strerr_diefu2sys(111, "atomically replace ", new) ;
    }
    satmp.len = base ;
  }
}

static void noforce (char const *old, char const *new, linkfunc_t_ref doit)
{
  if ((*doit)(old, new) == -1)
    strerr_diefu5sys(111, "make a link", " from ", new, " to ", old) ;
}

int main (int argc, char const *const *argv)
{
  linkfunc_t_ref mylink = &link ; /* default to system behaviour */
  ln_t_ref f = &noforce ;
  int nodir = 0 ;
  PROG = "s6-ln" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "nsfLP", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : nodir = 1 ; break ;
        case 's': mylink = &symlink ; break ;
        case 'f': f = &force ; break ;
        case 'L': if (mylink != &symlink) mylink = &linkderef ; break ;
        case 'P': if (mylink != &symlink) mylink = &linknoderef ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (argc < 2) strerr_dieusage(100, USAGE) ;
  if (!random_init())
    strerr_diefu1sys(111, "init random generator") ;
  if (argc > 2)
  {
    stralloc sa = STRALLOC_ZERO ;
    unsigned int i = 0 ;
    size_t base ;
    if (!stralloc_cats(&sa, argv[argc-1]) || !stralloc_catb(&sa, "/", 1))
      strerr_diefu1sys(111, "stralloc_cats") ;
    base = sa.len ;
    for (; i < (unsigned int)(argc-1) ; i++)
    {
      sa.len = base ;
      if (!sabasename(&sa, argv[i], strlen(argv[i])))
        strerr_diefu1sys(111, "sabasename") ;
      if (!stralloc_0(&sa)) strerr_diefu1sys(111, "stralloc_0") ;
      (*f)(argv[i], sa.s, mylink) ;
    }
    return 0 ;
  }

  {
    struct stat st ;
    if (nodir ? lstat(argv[1], &st) : stat(argv[1], &st) < 0)
    {
      if (errno != ENOENT) strerr_diefu2sys(111, "stat ", argv[1]) ;
      (*f)(argv[0], argv[1], mylink) ;
      return 0 ;
    }
    if (!S_ISDIR(st.st_mode))
    {
      (*f)(argv[0], argv[1], mylink) ;
      return 0 ;
    }
  }

  {
    stralloc sa = STRALLOC_ZERO ;
    if (!stralloc_cats(&sa, argv[1])
      || !stralloc_catb(&sa, "/", 1)
      || !sabasename(&sa, argv[0], strlen(argv[0]))
      || !stralloc_0(&sa))
        strerr_diefu1sys(111, "stralloc_catb") ;
      (*f)(argv[0], sa.s, mylink) ;
  }
  return 0 ;
}
