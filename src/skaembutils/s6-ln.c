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
#include <skalibs/strerr.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>
#include <skalibs/skamisc.h>

#define USAGE "s6-ln [ -n ] [ -s ] [ -f ] [ -L ] [ -P ] src... dest"
#define SUFFIX ":s6-ln:XXXXXX"

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

static int doit (char const *old, char const *new, link_func_ref mylink, int force)
{
  if ((*mylink)(old, new) == -1)
  {
    if (!force || errno != EEXIST)
    {
      strerr_warnwu5sys("make a link", " from ", new, " to ", old) ;
      return 1 ;
    }
    {
      size_t newlen = strlen(new) ;
      char fn[newlen + sizeof(SUFFIX)] ;
      memcpy(fn, new, newlen) ;
      memcpy(fn + newlen, SUFFIX, sizeof(SUFFIX)) ;
      if (mklinktemp(old, fn, mylink) == -1)
      {
        strerr_warnwu3sys("make a link", " to ", old) ;
        return 1 ;
      }
      if (rename(fn, new) == -1)
      {
        unlink_void(fn) ;
        strerr_warnwu2sys("atomically replace ", new) ;
        return 1 ;
      }
     /* if old == new, rename() didn't remove fn */
      unlink_void(fn) ;
    }
  }
  return 0 ;
}

int main (int argc, char const *const *argv)
{
  link_func_ref mylink = &link ; /* default to system behaviour */
  int force = 0 ;
  int nodir = 0 ;
  PROG = "s6-ln" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "nsfLP", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : nodir = 1 ; break ;
        case 's': mylink = &symlink ; break ;
        case 'f': force = 1 ; break ;
        case 'L': if (mylink != &symlink) mylink = &linkderef ; break ;
        case 'P': if (mylink != &symlink) mylink = &linknoderef ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (argc < 2) strerr_dieusage(100, USAGE) ;
  if (argc > 2)
  {
    stralloc sa = STRALLOC_ZERO ;
    unsigned int i = 0 ;
    int e = 0 ;
    size_t base ;
    if (!stralloc_cats(&sa, argv[argc-1]) || !stralloc_catb(&sa, "/", 1))
      strerr_diefu1sys(111, "stralloc_cats") ;
    base = sa.len ;
    for (; i < (unsigned int)(argc-1) ; i++)
    {
      sa.len = base ;
      if (!sabasename(&sa, argv[i], strlen(argv[i])))
      {
        strerr_warnwu1sys("sabasename") ;
        e++ ;
        continue ;
      }
      if (!stralloc_0(&sa))
      {
        strerr_warnwu1sys("stralloc_0") ;
        e++ ;
        continue ;
      }
      e += doit(argv[i], sa.s, mylink, force) ;
    }
    return e ;
  }

  {
    struct stat st ;
    if (nodir ? lstat(argv[1], &st) : stat(argv[1], &st) < 0)
    {
      if (errno != ENOENT) strerr_diefu2sys(111, "stat ", argv[1]) ;
      return doit(argv[0], argv[1], mylink, force) ;
    }
    if (!S_ISDIR(st.st_mode))
      return doit(argv[0], argv[1], mylink, force) ;
  }

  {
    stralloc sa = STRALLOC_ZERO ;
    if (!stralloc_cats(&sa, argv[1])
      || !stralloc_catb(&sa, "/", 1)
      || !sabasename(&sa, argv[0], strlen(argv[0]))
      || !stralloc_0(&sa))
        strerr_diefu1sys(111, "stralloc_catb") ;
    return doit(argv[0], sa.s, mylink, force) ;
  }
}
