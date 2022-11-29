/* ISC license. */

#include <sys/stat.h>
#include <skalibs/strerr.h>
#include <skalibs/djbunix.h>

#define USAGE "s6-hiercopy src dst"

int main (int argc, char const *const *argv)
{
  PROG = "s6-hiercopy" ;
  if (argc < 3) strerr_dieusage(100, USAGE) ;
  umask(0) ;
  if (!hiercopy(argv[1], argv[2]))
    strerr_diefu4sys(111, "copy hierarchy from ", argv[1], " to ", argv[2]) ;
  return 0 ;
}
