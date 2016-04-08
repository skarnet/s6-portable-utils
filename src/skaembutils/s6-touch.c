/* ISC license. */

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>

#define USAGE "s6-touch file ..."

int main (int argc, char const *const *argv)
{
  static struct timespec const now2[2] = { { .tv_sec = 0, .tv_nsec = UTIME_NOW }, { .tv_sec = 0, .tv_nsec = UTIME_NOW } } ;
  char const *const *p = argv + 1 ;
  PROG = "s6-touch" ;
  if (argc < 2) strerr_dieusage(100, USAGE) ;
  for (; *p ; p++)
  {
    register int fd = open_append(*p) ;
    if (fd < 0) strerr_diefu2sys(111, "open_append ", *p) ;
    if (futimens(fd, now2) < 0) strerr_diefu2sys(111, "futimens ", *p) ; 
    fd_close(fd) ;
  }
  return 0 ;
}
