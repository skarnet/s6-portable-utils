/* ISC license. */

#include <errno.h>
#include <regex.h>
#include <string.h>
#include <skalibs/bytestr.h>
#include <skalibs/sgetopt.h>
#include <skalibs/buffer.h>
#include <skalibs/uint.h>
#include <skalibs/strerr2.h>
#include <skalibs/stralloc.h>
#include <skalibs/skamisc.h>

#define USAGE "s6-grep [ -E | -F ] [ -i ] [ -c ] [ -n ] [ -q ] [ -v ] pattern"

typedef struct flags_s flags_t, *flags_t_ref ;
struct flags_s
{
  unsigned int extended : 1 ;
  unsigned int ignorecase: 1 ;
  unsigned int fixed : 1 ;
  unsigned int count : 1 ;
  unsigned int num : 1 ;
  unsigned int quiet : 1 ;
  unsigned int not : 1 ;
} ;
#define FLAGS_ZERO { .extended = 0, .ignorecase = 0, .fixed = 0, .count = 0, .num = 0, .quiet = 0, .not = 0 }

int main (int argc, char const *const *argv)
{
  unsigned int count = 0 ;
  flags_t flags = FLAGS_ZERO ;
  PROG = "s6-grep" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "EFicnqv", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'E': flags.extended = 1 ; break ;
        case 'F': flags.fixed = 1 ; break ;
        case 'i': flags.ignorecase = 1 ; break ;
        case 'c': flags.count = 1 ; break ;
        case 'n': flags.num = 1 ; break ;
        case 'q': flags.quiet = 1 ; break ;
        case 'v': flags.not = 1 ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) strerr_dieusage(100, USAGE) ;
  {
    stralloc line = STRALLOC_ZERO ;
    regex_t re ;
    unsigned int num = 0 ;
    unsigned int arglen = str_len(argv[0]) ;
    if (!flags.fixed)
    {
      register int e = regcomp(&re, argv[0], REG_NOSUB | (flags.extended ? REG_EXTENDED : 0) | (flags.ignorecase ? REG_ICASE : 0)) ;
      if (e)
      {
        char buf[256] ;
        regerror(e, &re, buf, 256) ;
        strerr_diefu2x(111, "compile regular expression: ", buf) ;
      }
    }

    for (;;)
    {
      register int r ;
      line.len = 0 ;
      r = skagetln(buffer_0f1, &line, '\n') ;
      if (!r) break ;
      if (r < 0)
      {
        if ((errno != EPIPE) || !stralloc_catb(&line, "\n", 1))
          strerr_diefu1sys(111, "read from stdin") ;
      }
      num++ ; line.s[line.len-1] = 0 ;
      if (flags.fixed)
      {
        if (flags.ignorecase)
          r = case_str(line.s, argv[0]) >= arglen ;
        else
          r = !strstr(line.s, argv[0]) ;
      }
      else
      {
        r = regexec(&re, line.s, 0, 0, 0) ;
        if (r && r != REG_NOMATCH)
        {
          char buf[256] ;
          regerror(r, &re, buf, 256) ;
          strerr_diefu2x(111, "match regular expression: ", buf) ;
        }
      }
      line.s[line.len-1] = '\n' ;
      if (!r ^ flags.not)
      {
        count++ ;
        if (!flags.quiet && !flags.count)
        {
          if (flags.num)
          {
            char fmt[UINT_FMT] ;
            register unsigned int n = uint_fmt(fmt, num) ;
            fmt[n++] = ':' ;
            if (buffer_put(buffer_1, fmt, n) < (int)n)
              strerr_diefu1sys(111, "write to stdout") ;
          }
          if (buffer_put(buffer_1, line.s, line.len) < (int)line.len)
            strerr_diefu1sys(111, "write to stdout") ;
        }
      }
    }
    if (flags.quiet) return !count ;
    stralloc_free(&line) ;
    if (!flags.fixed) regfree(&re) ;
  }
  if (flags.count)
  {
    char fmt[UINT_FMT] ;
    register unsigned int n = uint_fmt(fmt, count) ;
    fmt[n++] = '\n' ;
    if (buffer_put(buffer_1, fmt, n) < (int)n)
      strerr_diefu1sys(111, "write to stdout") ;
  }
  return !count ;
}
