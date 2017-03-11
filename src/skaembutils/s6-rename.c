/* ISC license. */

#include <stdio.h>
#include <skalibs/strerr2.h>

#define USAGE "s6-rename old new"

int main (int argc, char const *const *argv)
{
  PROG = "s6-rename" ;
  if (argc < 3) strerr_dieusage(100, USAGE) ;
  if (rename(argv[1], argv[2]) == -1)
    strerr_diefu4sys(111, "rename ", argv[1], " to ", argv[2]) ;
  return 0 ;
}
