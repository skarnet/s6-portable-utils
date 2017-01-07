/* ISC license. */

#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <skalibs/sgetopt.h>
#include <skalibs/bytestr.h>
#include <skalibs/uint.h>
#include <skalibs/diuint.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <skalibs/djbunix.h>
#include <skalibs/skamisc.h>

#define USAGE "s6-cut [ -b list | -c list | -f list ] [ -d delim ] [ -n ] [ -s ] [ file... ]"

static int diuint_cmpleft (void const *a, void const *b)
{
  return ((diuint const *)a)->left - ((diuint const *)b)->left ;
}

static void diuintalloc_normalize (genalloc *list)
{
  size_t i = 1, cur = 0 ;
  size_t len = genalloc_len(diuint, list) ;
  register diuint *const s = genalloc_s(diuint, list) ;
  qsort(s, len, sizeof(diuint), &diuint_cmpleft) ;
  for (; i < len ; i++)
    if (!s[cur].right) break ;
    else if (s[i].left > s[cur].right) s[++cur] = s[i] ;
    else if (s[cur].right < s[i].right)
      s[cur].right = s[i].right ;
  genalloc_setlen(diuint, list, cur+1) ;
}

static void scanlist (genalloc *list, char const *s)
{
  register size_t i = 0 ;
  genalloc_setlen(diuint, list, 0) ;
  while (s[i])
  {
    char const sep[4] = ", \t" ;
    diuint iv ;
    if (s[i] == '-') iv.left = 1 ;
    else
    {
      size_t j = uint_scan(s+i, &iv.left) ;
      if (!j || !iv.left) strerr_dief2x(100, "invalid list argument: ", s) ;
      i += j ;
    }
    if (s[i] != '-') iv.right = iv.left ;
    else
    {
      size_t j = uint_scan(s + ++i, &iv.right) ;
      if (!j) iv.right = 0 ;
      else if (iv.right < iv.left)
        strerr_dief2x(100, "invalid list argument: ", s) ;
      else i += j ;
    }
    switch (byte_chr(sep, 4, s[i]))
    {
      case 0 :
      case 1 :
      case 2 : i++ ;
      case 3 : break ;
      case 4 :
        strerr_dief2x(100, "invalid list argument: ", s) ;
    }
    if (!genalloc_append(diuint, list, &iv))
      strerr_diefu1sys(111, "build interval list") ;
  }
}

static int doit (int fd, diuint const *s, size_t len, unsigned int flags, char delim)
{
  char buf[BUFFER_INSIZE] ;
  buffer b = BUFFER_INIT(&buffer_flush1read, fd, buf, BUFFER_INSIZE) ;
  for (;;)
  {
    int r ;
    satmp.len = 0 ;
    r = skagetln(&b, &satmp, '\n') ;
    if ((r == -1) && (errno != EPIPE)) return 0 ;
    if (!r) break ;
    if (flags & 2)
    {
      register size_t i = 0 ;
      for (; i < len ; i++)
      {
        register size_t j = s[i].right ;
        if (s[i].left >= satmp.len) break ;
        if (!j || (j > satmp.len))
        {
          j = satmp.len ;
          r = 0 ;
        }
        if (buffer_put(buffer_1, satmp.s + s[i].left - 1, j + 1 - s[i].left) == -1)
          return 0 ;
      }
    }
    else
    {
      register size_t i = 0, j = 0, count = 1 ;
      for (; i < len ; i++)
      {
        for (; count < s[i].left ; count++)
        {
          j += byte_chr(satmp.s + j, satmp.len - j, delim) ;
          if (j == satmp.len) break ;
          j++ ;
        }
        if (j == satmp.len)
        {
          if (count == 1)
          {
            if ((flags & 1) && (buffer_put(buffer_1, satmp.s, satmp.len) < 0))
              return 0 ;
            r = 0 ;
          }
          break ;
        }
        for (; !s[i].right || (count <= s[i].right) ; count++)
        {
          register size_t k = byte_chr(satmp.s + j, satmp.len - j, delim) ;
          if ((count > s[0].left) && (buffer_put(buffer_1, &delim, 1) < 0)) return 0 ;
          if (buffer_put(buffer_1, satmp.s + j, k) < 0) return 0 ;
          j += k ;
          if (j == satmp.len)
          {
            r = 0 ;
            break ;
          }
          j++ ;
        }
        if (j == satmp.len) break ;
      }
    }
    if ((r > 0) && (buffer_put(buffer_1, "\n", 1) < 0)) return 0 ;
  }
  return 1 ;
}

int main (int argc, char const *const *argv)
{
  genalloc list = GENALLOC_ZERO ; /* array of diuint */
  char delim = '\t' ;
  unsigned int what = 0 ;
  PROG = "s6-cut" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    int flagnodel = 1 ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "nsb:c:f:d:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n': break ;  /* ignored */
        case 's': flagnodel = 0 ; break ;
        case 'd': delim = *l.arg ; break ;
        case 'b':
        case 'c':
        {
          if (what) strerr_dieusage(100, USAGE) ;
          what = 2 ;
          scanlist(&list, l.arg) ;
          break ;
        }
        case 'f':
        {
          if (what) strerr_dieusage(100, USAGE) ;
          what = 4 ;
          scanlist(&list, l.arg) ;
          break ;
        }
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    what += flagnodel ;
    argc -= l.ind ; argv += l.ind ;
  }
  if (!genalloc_len(diuint, &list)) strerr_dieusage(100, USAGE) ;
  diuintalloc_normalize(&list) ;

  if (!argc)
  {
    if (!doit(0, genalloc_s(diuint, &list), genalloc_len(diuint, &list), what, delim))
      strerr_diefu1sys(111, "cut stdin") ;
  }
  else
  {
    for (; *argv ; argv++)
    {
      if ((argv[0][0] == '-') && !argv[0][1])
      {
        if (!doit(0, genalloc_s(diuint, &list), genalloc_len(diuint, &list), what, delim))
          strerr_diefu1sys(111, "process stdin") ;
      }
      else
      {
        int fd = open_readb(*argv) ;
        if (fd == -1)
          strerr_diefu3sys(111, "open ", *argv, " for reading") ;
        if (!doit(fd, genalloc_s(diuint, &list), genalloc_len(diuint, &list), what, delim))
          strerr_diefu2sys(111, "cut ", *argv) ;
        fd_close(fd) ;
      }
    }
  }
  return 0 ;
}
