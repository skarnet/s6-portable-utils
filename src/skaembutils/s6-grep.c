/* ISC license. */

#include <string.h>
#include <errno.h>
#include <regex.h>
#include <string.h>

#include <skalibs/posixplz.h>
#include <skalibs/bytestr.h>
#include <skalibs/sgetopt.h>
#include <skalibs/buffer.h>
#include <skalibs/types.h>
#include <skalibs/strerr.h>
#include <skalibs/stralloc.h>
#include <skalibs/skamisc.h>

#define USAGE "s6-grep [ -E | -F ] [ -i ] [ -c ] [ -n ] [ -q ] [ -v ] pattern"
#define dieusage() strerr_dieusage(100, USAGE)

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

static void xout (char const *s, size_t len)
{
  if (buffer_put(buffer_1, s, len) < 0)
    strerr_diefu1sys(111, "write to stdout") ;
}

int main (int argc, char const *const *argv)
{
  unsigned int count = 0 ;
  flags_t flags = FLAGS_ZERO ;
  PROG = "s6-grep" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "EFicnqv", &l) ;
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
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) dieusage() ;
  {
    stralloc line = STRALLOC_ZERO ;
    regex_t re ;
    unsigned int num = 0 ;
    size_t arglen = strlen(argv[0]) ;
    if (!flags.fixed)
    {
      int e = skalibs_regcomp(&re, argv[0], REG_NOSUB | (flags.extended ? REG_EXTENDED : 0) | (flags.ignorecase ? REG_ICASE : 0)) ;
      if (e)
      {
        char buf[256] ;
        regerror(e, &re, buf, 256) ;
        strerr_diefu2x(111, "compile regular expression: ", buf) ;
      }
    }

    for (;;)
    {
      int r ;
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
            size_t n = uint_fmt(fmt, num) ;
            fmt[n++] = ':' ;
            xout(fmt, n) ;
          }
          xout(line.s, line.len) ;
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
    size_t n = uint_fmt(fmt, count) ;
    fmt[n++] = '\n' ;
    xout(fmt, n) ;
  }
  return !count ;
}
