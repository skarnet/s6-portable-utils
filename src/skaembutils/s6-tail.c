/* ISC license. */

#include <sys/types.h>
#include <errno.h>
#include <skalibs/sgetopt.h>
#include <skalibs/allreadwrite.h>
#include <skalibs/bytestr.h>
#include <skalibs/uint.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>
#include <skalibs/skamisc.h>
#include <skalibs/siovec.h>

#define USAGE "s6-tail [ -c chars | -n lines | -1..9 ] [ file ]"

typedef int tailfunc_t (int, size_t) ;
typedef tailfunc_t *tailfunc_t_ref ;

static int pluslines (int fd, size_t n)
{
  if (n) n-- ;
  {
    char buf[BUFFER_INSIZE] ;
    buffer b = BUFFER_INIT(&buffer_read, fd, buf, BUFFER_INSIZE) ;
    unsigned int count = 0 ;
    while (count < n)
    {
      register ssize_t r = buffer_fill(&b) ;
      if (r <= 0) return !r ;
      while (!buffer_isempty(&b) && (count < n))
      {
        siovec_t v[2] ;
        size_t i ;
        buffer_rpeek(&b, v) ;
        i = siovec_bytechr(v, 2, '\n') ;
        if (i < buffer_len(&b))
        {
          count++ ; i++ ;
        }
        buffer_rseek(&b, i) ;
      }
    }
    b.op = &buffer_write ;
    b.fd = 1 ;
    if (!buffer_flush(&b)) return 0 ;
  }
  return (fd_cat(fd, 1) >= 0) ;
}

static int pluschars (int fd, size_t n)
{
  if (n-- > 1)
  {
    int nil = open_write("/dev/null") ;
    if (nil < 0) return 0 ;
    if (!fd_catn(fd, nil, n))
    {
      register int e = errno ;
      fd_close(nil) ;
      errno = e ;
      return 0 ;
    }
    fd_close(nil) ;
  }
  return (fd_cat(fd, 1) >= 0) ;
}

static int minuslines (int fd, size_t n)
{
  char buf[BUFFER_INSIZE] ;
  buffer b = BUFFER_INIT(&buffer_read, fd, buf, BUFFER_INSIZE) ;
  size_t head = 0, tail = 0 ;
  stralloc tab[n+1] ;
  for (; head <= n ; head++) tab[head] = stralloc_zero ;
  head = 0 ;
  for (;;)
  {
    register int r ;
    r = skagetln(&b, tab + tail, '\n') ;
    if (!r) break ;
    if (r < 0)
    {
      if (errno == EPIPE) break ;
      else goto err ;
    }
    tail = (tail + 1) % (n+1) ;
    if (tail == head)
    {
      tab[head].len = 0 ;
      head = (head + 1) % (n+1) ;
    }
  }
  buffer_init(&b, &buffer_write, 1, buf, BUFFER_INSIZE) ;
  for (; head != tail ; head = (head + 1) % (n+1))
  {
    if (buffer_put(&b, tab[head].s, tab[head].len) < tab[head].len)
      goto err ;
  }
  for (head = 0 ; head <= n ; head++) stralloc_free(tab + head) ;
  return buffer_flush(&b) ;
 err:
  for (head = 0 ; head <= n ; head++) stralloc_free(tab + head) ;
  return 0 ;
}

static int minuschars (int fd, size_t n)
{
  char buf[BUFFER_INSIZE + n] ;
  buffer b = BUFFER_INIT(&buffer_read, fd, buf, BUFFER_INSIZE + n) ;
  for (;;)
  {
    register ssize_t r = buffer_fill(&b) ;
    if (!r) break ;
    if (r < 0) return 0 ;
    buffer_rseek(&b, buffer_len(&b)) ;
    buffer_unget(&b, n) ;
  }
  b.op = &buffer_write ;
  b.fd = 1 ;
  return buffer_flush(&b) ;
}

int main (int argc, char const *const *argv)
{
  tailfunc_t_ref f = &minuslines ;
  unsigned int n = 10 ;
  int gotit = 0 ;
  PROG = "s6-tail" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "123456789n:c:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case '1' :
        case '2' :
        case '3' :
        case '4' :
        case '5' :
        case '6' :
        case '7' :
        case '8' :
        case '9' :
        {
          if (gotit) strerr_dieusage(100, USAGE) ;
          gotit = 1 ;
          f = &minuslines ;
          n = opt - '0' ;
          break ;
        }
        case 'n':
        {
          if (gotit) strerr_dieusage(100, USAGE) ;
          gotit = 1 ;
          f = &minuslines ;
          if (*l.arg == '-') l.arg++ ;
          else if (*l.arg == '+')
          {
            f = &pluslines ;
            l.arg++ ;
          }
          if (!uint0_scan(l.arg, &n)) strerr_dieusage(100, USAGE) ;
          break ;
        }
        case 'c':
        {
          if (gotit) strerr_dieusage(100, USAGE) ;
          gotit = 1 ;
          f = &minuschars ;
          if (*l.arg == '-') l.arg++ ;
          else if (*l.arg == '+')
          {
            f = &pluschars ;
            l.arg++ ;
          }
          if (!uint0_scan(l.arg, &n)) strerr_dieusage(100, USAGE) ;
          break ;
        }
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc)
  {
    if (!(*f)(0, n))
      strerr_diefu1sys(111, "tail stdin") ;
  }
  else
  {
    int fd = open_readb(argv[0]) ;
    if (fd == -1) strerr_diefu3sys(111, "open ", argv[0], " for reading") ;
    if (!(*f)(fd, n))
      strerr_diefu2sys(111, "tail ", argv[0]) ;
    fd_close(fd) ;
  }
  return 0 ;
}
