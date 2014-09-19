/* ISC license. */

#include <skalibs/allreadwrite.h>
#include <skalibs/strerr2.h>
#include <skalibs/tai.h>

#define USAGE "s6-clock [ tai64nlabel ]"

static int getit (void)
{
  char fmt[TIMESTAMP+1] ;
  timestamp(fmt) ;
  fmt[TIMESTAMP] = '\n' ;
  if (allwrite(1, fmt, TIMESTAMP+1) < TIMESTAMP+1)
    strerr_diefu1sys(111, "write to stdout") ;
  return 0 ;
}

static int setit (char const *h)
{
  tain_t a ;
  if (!timestamp_scan(h, &a)) strerr_dieusage(100, USAGE) ;
  if (!tain_setnow(&a)) strerr_diefu1sys(111, "taia_setnow") ;
  return 0 ;
}

int main (int argc, char const *const *argv)
{
  PROG = "s6-clock" ;
  return (argc < 2) ? getit() : setit(argv[1]) ;
}
