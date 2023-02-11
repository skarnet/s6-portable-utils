/* ISC license. */

#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <skalibs/uint32.h>
#include <skalibs/uint64.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr.h>
#include <skalibs/tai.h>
#include <skalibs/stralloc.h>
#include <skalibs/skamisc.h>

int main (int argc, char const *const *argv)
{
  stralloc sa = STRALLOC_ZERO ;
  tain prev ;
  int defined = 0 ;
  PROG = "s6-tai64ndiff" ;

  for (;;)
  {
    unsigned int p = 0 ;
    char prefix[23] = "[          .          ]" ;
    int r = skagetln(buffer_0f1, &sa, '\n') ;
    if (r == -1)
      if (errno != EPIPE)
        strerr_diefu1sys(111, "read from stdin") ;
      else r = 1 ;
    else if (!r) break ;
    if (sa.len > TIMESTAMP)
    {
      tain cur ;
      p = timestamp_scan(sa.s, &cur) ;
      if (p)
      {
        if (defined)
        {
          tain diff ;
          int64_t secs ;
          size_t len ;
          tain_sub(&diff, &cur, &prev) ;
          secs = tai_sec(tain_secp(&diff)) ;
          len = int64_fmt(0, secs) ;
          if (len > 10)
          {
            char fmtn[9] ;
            size_t m = 1 + (len < 20) ;
            m += int64_fmt(prefix + m, secs) ;
            prefix[m++] = '.' ;
            uint320_fmt(fmtn, tain_nano(&diff), 9) ;
            memcpy(prefix + m, fmtn, 22 - m) ;
          }
          else
          {
            int64_fmt(prefix + 11 - len, secs) ;
            uint320_fmt(prefix + 12, tain_nano(&diff), 9) ;
          }
        }
        prev = cur ;
        defined = 1 ;
        if (buffer_put(buffer_1, prefix, 23) < 23)
          strerr_diefu1sys(111, "write to stdout") ;
      }
      else defined = 0 ;
    }
    else defined = 0 ;
    if (buffer_put(buffer_1, sa.s + p, sa.len - p) < (ssize_t)(sa.len - p))
      strerr_diefu1sys(111, "write to stdout") ;
    sa.len = 0 ;
  }
  return 0 ;
}
