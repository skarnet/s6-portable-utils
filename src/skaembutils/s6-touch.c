/* ISC license. */

#include <skalibs/posixplz.h>
#include <skalibs/strerr2.h>

#define USAGE "s6-touch file ..."

int main (int argc, char const *const *argv)
{
  char const *const *p = argv + 1 ;
  PROG = "s6-touch" ;
  if (argc < 2) strerr_dieusage(100, USAGE) ;
  for (; *p ; p++) if (!touch(*p)) strerr_diefu2sys(111, "touch ", *p) ;
  return 0 ;
}
