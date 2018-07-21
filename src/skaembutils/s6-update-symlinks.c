/* ISC license. */

#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <skalibs/direntry.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>
#include <skalibs/random.h>

#define USAGE "s6-update-symlinks /destdir /srcdir [ /srcdir ... ]"

#define MAGICNEW ":s6-update-symlinks-new"
#define MAGICOLD ":s6-update-symlinks-old"

#define CONFLICT -2
#define ERROR -1
#define MODIFIED 0
#define OVERRIDEN 1

static stralloc errdst = STRALLOC_ZERO ;
static stralloc errsrc = STRALLOC_ZERO ;


typedef struct stralloc3 stralloc3, *stralloc3_ref ;
struct stralloc3
{
  stralloc dst ;
  stralloc src ;
  stralloc tmp ;
} ;

#define STRALLOC3_ZERO { STRALLOC_ZERO, STRALLOC_ZERO, STRALLOC_ZERO }


static void cleanup (stralloc *sa, unsigned int pos)
{
  int e = errno ;
  rm_rf_in_tmp(sa, pos) ;
  errno = e ;
}


static int makeuniquename (stralloc *sa, char const *path, char const *magic)
{
  size_t base = sa->len ;
  int wasnull = !sa->s ;
  if (!stralloc_cats(sa, path)) return 0 ;
  if (!stralloc_cats(sa, magic)) goto err ;
  if (!random_sauniquename(sa, 8)) goto err ;
  if (!stralloc_0(sa)) goto err ;
  return 1 ;

err:
  if (wasnull) stralloc_free(sa) ; else sa->len = base ;
  return 0 ;
}


static int addlink (stralloc3 *blah, unsigned int dstpos, unsigned int srcpos)
{
  if (symlink(blah->src.s + srcpos, blah->dst.s + dstpos) >= 0) return MODIFIED ;
  if (errno != EEXIST) return ERROR ;

  {
    size_t dstbase = blah->dst.len ;
    size_t srcbase = blah->src.len ;
    size_t tmpbase = blah->tmp.len ;
    size_t dststop ;
    size_t srcstop ;
    signed int diffsize = 0 ;
    int collect = 1 ;

    {
      size_t n = strlen(blah->dst.s + dstpos) ;
      if (!stralloc_readyplus(&blah->dst, n+1)) return ERROR ;
      stralloc_catb(&blah->dst, blah->dst.s + dstpos, n) ;
    }
    stralloc_catb(&blah->dst, "/", 1) ;
    dststop = blah->dst.len ;

    {
      int r ;
      DIR *dir = opendir(blah->dst.s + dstpos) ;
      if (!dir)
      {
        blah->dst.len = dstbase ;
        if (errno != ENOTDIR) return ERROR ;
        if ((unlink(blah->dst.s + dstpos) == -1)
         || (symlink(blah->src.s + srcpos, blah->dst.s + dstpos) == -1))
          return ERROR ;
        return OVERRIDEN ; /* replaced a link to a normal file */
      }
      r = sareadlink(&blah->src, blah->dst.s + dstpos) ;
      if ((r == -1) && (errno != EINVAL))
      {
        blah->dst.len = dstbase ;
        dir_close(dir) ;
        return ERROR ;
      }
      if (r < 0)
      {
        for (;;)
        {
          direntry *d ;
          errno = 0 ;
          d = readdir(dir) ;
          if (!d) break ;
          if ((d->d_name[0] == '.') && (!d->d_name[1] || ((d->d_name[1] == '.') && !d->d_name[2])))
            continue ;
          diffsize-- ;  /* need to know the size for collect */
        }
        if (errno)
        {
          blah->src.len = srcbase ;
          blah->dst.len = dstbase ;
          dir_close(dir) ;
          return ERROR ;
        }
      }
      else if ((unlink(blah->dst.s + dstpos) == -1)
            || (mkdir(blah->dst.s + dstpos, 0777) == -1)
            || !stralloc_catb(&blah->src, "/", 1))
      {
        blah->src.len = srcbase ;
        blah->dst.len = dstbase ;
        dir_close(dir) ;
        return ERROR ;
      }
      else         /* expand */
      {
        srcstop = blah->src.len ;
        for (;;)
        {
          direntry *d ;
          errno = 0 ;
          d = readdir(dir) ;
          if (!d) break ;
          if ((d->d_name[0] == '.') && (!d->d_name[1] || ((d->d_name[1] == '.') && !d->d_name[2])))
            continue ;
          diffsize-- ;
          blah->dst.len = dststop ;
          blah->src.len = srcstop ;
          if (!stralloc_cats(&blah->dst, d->d_name) || !stralloc_0(&blah->dst)
           || !stralloc_cats(&blah->src, d->d_name) || !stralloc_0(&blah->src)
           || (symlink(blah->src.s + srcbase, blah->dst.s + dstbase) == -1))
          {
            blah->src.len = srcbase ;
            blah->dst.len = dstbase ;
            dir_close(dir) ;
            return ERROR ;
          }
        }
        if (errno)
        {
          blah->src.len = srcbase ;
          blah->dst.len = dstbase ;
          dir_close(dir) ;
          return ERROR ;
        }
      }
      dir_close(dir) ;
    }

    blah->src.len = srcbase ;
    {
      size_t n = strlen(blah->src.s + srcpos) ;
      if (!stralloc_readyplus(&blah->src, n+1))
      {
        blah->dst.len = dstbase ;
        return ERROR ;
      }
      stralloc_catb(&blah->src, blah->src.s + srcpos, n) ;
    }
    stralloc_catb(&blah->src, "/", 1) ;
    srcstop = blah->src.len ;


   /* prepare tmp for recursion */

    {
      DIR *dir = opendir(blah->src.s + srcpos) ;
      if (!dir)
      {
        blah->src.len = srcbase ;
        blah->dst.len = dstbase ;
        if (errno != ENOTDIR) return ERROR ;
        errdst.len = errsrc.len = 0 ;
        if (!stralloc_cats(&errdst, blah->dst.s + dstpos) || !stralloc_0(&errdst)
         || !stralloc_cats(&errsrc, blah->src.s + srcpos) || !stralloc_0(&errsrc))
          return ERROR ;
        return CONFLICT ; /* dst is a dir but src is not */
      }
      for (;;)
      {
        direntry *d ;
        errno = 0 ;
        d = readdir(dir) ;
        if (!d) break ;
        if ((d->d_name[0] == '.') && (!d->d_name[1] || ((d->d_name[1] == '.') && !d->d_name[2])))
          continue ;
        if (!stralloc_cats(&blah->tmp, d->d_name) || !stralloc_0(&blah->tmp))
        {
          blah->tmp.len = tmpbase ;
          blah->src.len = srcbase ;
          blah->dst.len = dstbase ;
          dir_close(dir) ;
          return ERROR ;
        }
      }
      if (errno)
      {
        blah->tmp.len = tmpbase ;
        blah->src.len = srcbase ;
        blah->dst.len = dstbase ;
        dir_close(dir) ;
        return ERROR ;
      }
      dir_close(dir) ;
    }


   /* recurse */

    {
      size_t i = tmpbase ;
      while (i < blah->tmp.len)
      {
        diffsize++ ;
        blah->dst.len = dststop ;
        blah->src.len = srcstop ;
        {
          size_t n = strlen(blah->tmp.s + i) + 1 ;
          if (!stralloc_catb(&blah->dst, blah->tmp.s + i, n)
           || !stralloc_catb(&blah->src, blah->tmp.s + i, n))
          {
            blah->tmp.len = tmpbase ;
            blah->src.len = srcbase ;
            blah->dst.len = dstbase ;
            return ERROR ;
          }
          i += n ;
        }
        switch (addlink(blah, dstbase, srcbase))
        {
          case ERROR :
            blah->tmp.len = tmpbase ;
            blah->src.len = srcbase ;
            blah->dst.len = dstbase ;
            return ERROR ;
          case CONFLICT :
            blah->tmp.len = tmpbase ;
            blah->src.len = srcbase ;
            blah->dst.len = dstbase ;
            return CONFLICT ;
          case MODIFIED :
            collect = 0 ;
        }
      }
    }
    blah->tmp.len = tmpbase ;
    blah->src.len = srcbase ;
    blah->dst.len = dstbase ;


   /* collect */

    if (collect && !diffsize)
    {
      if (rm_rf_in_tmp(&blah->dst, dstpos) == -1) return ERROR ;
      if (symlink(blah->src.s + srcpos, blah->dst.s + dstpos) == -1) return ERROR ;
      return OVERRIDEN ;
    }
  }
  return MODIFIED ;
}

int main (int argc, char *const *argv)
{
  stralloc3 blah = STRALLOC3_ZERO ;
  PROG = "s6-update-symlinks" ;
  if (argc < 3) strerr_dieusage(100, USAGE) ;
  {
    char *const *p = argv + 1 ;
    for (; *p ; p++) if (**p != '/') strerr_dieusage(100, USAGE) ;
  }
  {
    size_t i = strlen(argv[1]) ;
    while (i && (argv[1][i-1] == '/')) argv[1][--i] = 0 ;
    if (!i) strerr_diefu1x(100, "replace root directory") ;
  }
  if (!random_init())
    strerr_diefu1sys(111, "init random generator") ;
  if (!makeuniquename(&blah.dst, argv[1], MAGICNEW))
    strerr_diefu2sys(111, "make random unique name based on ", argv[1]) ;
  if ((unlink(blah.dst.s) == -1) && (errno != ENOENT))
    strerr_diefu2sys(111, "unlink ", blah.dst.s) ;

  {
    char *const *p = argv + 2 ;
    for (; *p ; p++)
    {
      int r ;
      blah.src.len = 0 ;
      if (!stralloc_cats(&blah.src, *p) || !stralloc_0(&blah.src))
        strerr_diefu1sys(111, "make stralloc") ;
      r = addlink(&blah, 0, 0) ;
      if (r < 0)
      {
        stralloc_free(&blah.tmp) ;
        stralloc_free(&blah.src) ;
        cleanup(&blah.dst, 0) ;
        stralloc_free(&blah.dst) ;
        if (r == CONFLICT)
          strerr_dief4x(100, "destination ", errdst.s, " conflicts with source ", errsrc.s) ;
        else
          strerr_dief2sys(111, "error processing ", *p) ;
      }
    }
  }
  stralloc_free(&blah.tmp) ;

  if (rename(blah.dst.s, argv[1]) == -1) /* be atomic if possible */
  {
    blah.src.len = 0 ;
    if (!makeuniquename(&blah.src, argv[1], MAGICOLD))
    {
      cleanup(&blah.dst, 0) ;
      strerr_diefu2sys(111, "make random unique name based on ", argv[1]) ;
    }

    if (rename(argv[1], blah.src.s) == -1)
    {
      cleanup(&blah.dst, 0) ;
      strerr_diefu4sys(111, "rename ", argv[1], " to ", blah.src.s) ;
    }
   /* XXX: unavoidable race condition here: argv[1] does not exist */
    if (rename(blah.dst.s, argv[1]) == -1)
    {
      rename(blah.src.s, argv[1]) ;
      cleanup(&blah.dst, 0) ;
      strerr_diefu4sys(111, "rename ", blah.dst.s, " to ", argv[1]) ;
    }
    stralloc_free(&blah.dst) ;
    if (rm_rf_in_tmp(&blah.src, 0) == -1)
      strerr_warnwu2sys("remove old directory ", blah.src.s) ;
    stralloc_free(&blah.src) ;
  }

  return 0 ;
}
