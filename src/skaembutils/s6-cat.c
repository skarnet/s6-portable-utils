/* ISC license. */

#include <skalibs/strerr2.h>
#include <skalibs/djbunix.h>

int main (void)
{
  PROG = "s6-cat" ;
  if (fd_cat(0, 1) < 0) strerr_diefu1sys(111, "fd_cat") ;
  return 0 ;
}
