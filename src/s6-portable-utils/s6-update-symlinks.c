/* ISC license. */

#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include <skalibs/direntry.h>
#include <skalibs/strerr.h>
#include <skalibs/stralloc.h>
#include <skalibs/djbunix.h>
#include <skalibs/random.h>

#define USAGE "s6-update-symlinks /destdir /srcdir [ /srcdir ... ]"

#define UPDATESYMLINKS_MAGICNEW ":s6-update-symlinks-new"
#define UPDATESYMLINKS_MAGICOLD ":s6-update-symlinks-old"

#define CONFLICT -2
#define ERROR -1
#define MODIFIED 0
#define OVERRIDEN 1

typedef struct stralloc5 stralloc5, *stralloc5_ref ;
struct stralloc5
{
  stralloc dst ;
  stralloc src ;
  stralloc tmp ;
  stralloc errdst ;
  stralloc errsrc ;
} ;

#define STRALLOC5_ZERO { STRALLOC_ZERO, STRALLOC_ZERO, STRALLOC_ZERO, STRALLOC_ZERO, STRALLOC_ZERO }

static void updatesymlinks_cleanup (stralloc *sa, unsigned int pos)
{
  int e = errno ;
  rm_rf_in_tmp(sa, pos) ;
  errno = e ;
}

static int updatesymlinks_makeuniquename (stralloc *sa, char const *path, char const *magic)
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

static int updatesymlinks_addlink (stralloc5 *blah, unsigned int dstpos, unsigned int srcpos)
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
        blah->errdst.len = blah->errsrc.len = 0 ;
        if (!stralloc_cats(&blah->errdst, blah->dst.s + dstpos) || !stralloc_0(&blah->errdst)
         || !stralloc_cats(&blah->errsrc, blah->src.s + srcpos) || !stralloc_0(&blah->errsrc))
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
        switch (updatesymlinks_addlink(blah, dstbase, srcbase))
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

int main (int argc, char const *const *argv)
{
  PROG = "s6-update-symlinks" ;
  if (argc < 3) strerr_dieusage(100, USAGE) ;
  {
    stralloc5 blah = STRALLOC5_ZERO ;
    char const *const *p = argv + 1 ;
    size_t i = strlen(argv[1]) ;
    char target[i+1] ;
    for (; *p ; p++) if (**p != '/') strerr_dieusage(100, USAGE) ;
    p = argv + 2 ;
    memcpy(target, argv[1], i+1) ;
    while (i && (target[i-1] == '/')) target[--i] = 0 ;
    if (!i) strerr_diefu1x(100, "replace root directory") ;
    if (!updatesymlinks_makeuniquename(&blah.dst, target, UPDATESYMLINKS_MAGICNEW))
      strerr_diefu2sys(111, "make random unique name based on ", target) ;
    if ((unlink(blah.dst.s) == -1) && (errno != ENOENT))
      strerr_diefu2sys(111, "unlink ", blah.dst.s) ;

    for (; *p ; p++)
    {
      int r ;
      blah.src.len = 0 ;
      if (!stralloc_cats(&blah.src, *p) || !stralloc_0(&blah.src))
        strerr_diefu1sys(111, "make stralloc") ;
      r = updatesymlinks_addlink(&blah, 0, 0) ;
      if (r < 0)
      {
        stralloc_free(&blah.tmp) ;
        stralloc_free(&blah.src) ;
        updatesymlinks_cleanup(&blah.dst, 0) ;
        stralloc_free(&blah.dst) ;
        if (r == CONFLICT)
          strerr_dief4x(100, "destination ", blah.errdst.s, " conflicts with source ", blah.errsrc.s) ;
        else
          strerr_dief2sys(111, "error processing ", *p) ;
      }
    }
    stralloc_free(&blah.tmp) ;

    if (rename(blah.dst.s, target) == -1) /* be atomic if possible */
    {
      blah.src.len = 0 ;
      if (!updatesymlinks_makeuniquename(&blah.src, target, UPDATESYMLINKS_MAGICOLD))
      {
        updatesymlinks_cleanup(&blah.dst, 0) ;
        strerr_diefu2sys(111, "make random unique name based on ", target) ;
      }

      if (rename(target, blah.src.s) == -1)
      {
        updatesymlinks_cleanup(&blah.dst, 0) ;
        strerr_diefu4sys(111, "rename ", target, " to ", blah.src.s) ;
      }
     /* XXX: unavoidable race condition here: target does not exist */
      if (rename(blah.dst.s, target) == -1)
      {
        rename(blah.src.s, target) ;
        updatesymlinks_cleanup(&blah.dst, 0) ;
        strerr_diefu4sys(111, "rename ", blah.dst.s, " to ", target) ;
      }
      stralloc_free(&blah.dst) ;
      if (rm_rf_in_tmp(&blah.src, 0) == -1)
        strerr_warnwu2sys("remove old directory ", blah.src.s) ;
      stralloc_free(&blah.src) ;
    }
  }
  return 0 ;
}
