/* ISC license. */

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>

#include <skalibs/allreadwrite.h>
#include <skalibs/sgetopt.h>
#include <skalibs/buffer.h>
#include <skalibs/strerr.h>
#include <skalibs/stralloc.h>
#include <skalibs/genalloc.h>
#include <skalibs/djbunix.h>
#include <skalibs/skamisc.h>

#define USAGE "s6-sort [ -bcfru0 ]"

typedef int strncmp_func (char const *, char const *, size_t) ;
typedef strncmp_func *strncmp_func_ref ;
typedef int qsortcmp_func (void const *, void const *) ;
typedef qsortcmp_func *qsortcmp_func_ref ;

typedef struct sort_global_s sort_global, *sort_global_ref ;
struct sort_global_s
{
  strncmp_func_ref comp ;
  unsigned char flagnoblanks : 1 ;
  unsigned char flagreverse : 1 ;
  unsigned char flaguniq : 1 ;
} ;
#define SORT_GLOBAL_ZERO { .flagnoblanks = 0, .flagreverse = 0, .flaguniq = 0, .comp = &strncmp }

static sort_global_ref sort_G ;

static int compit (char const *s1, size_t n1, char const *s2, size_t n2)
{
  int r ;
  if (sort_G->flagnoblanks)
  {
    while ((*s1 == ' ') || (*s1 == '\t')) (s1++, n1--) ;
    while ((*s2 == ' ') || (*s2 == '\t')) (s2++, n2--) ;
  }
  r = (*sort_G->comp)(s1, s2, n1 < n2 ? n1 : n2) ;
  if (!r) r = n1 - n2 ;
  return sort_G->flagreverse ? -r : r ;
}

static int sacmp (stralloc const *a, stralloc const *b)
{
  return compit(a->s, a->len - 1, b->s, b->len - 1) ;
}

static ssize_t sort_slurplines (genalloc *lines, char sep)
{
  ssize_t i = 0 ;
  for (;; i++)
  {
    stralloc sa = STRALLOC_ZERO ;
    int r = skagetln(buffer_0, &sa, sep) ;
    if (!r) break ;
    if ((r < 0) && ((errno != EPIPE) || !stralloc_catb(&sa, &sep, 1)))
      return -1 ;
    stralloc_shrink(&sa) ;
    if (!genalloc_append(stralloc, lines, &sa)) return -1 ;
  }
  return i ;
}

static void sort_uniq (genalloc *lines)
{
  size_t len = genalloc_len(stralloc, lines) ;
  stralloc *s = genalloc_s(stralloc, lines) ;
  size_t i = 1 ;
  for (; i < len ; i++)
    if (!sacmp(s+i-1, s+i)) stralloc_free(s+i-1) ;
}

static ssize_t sort_outputlines (stralloc const *s, size_t len)
{
  size_t i = 0 ;
  for (; i < len ; i++)
    if (buffer_put(buffer_1, s[i].s, s[i].len) < 0) return 0 ;
  return buffer_flush(buffer_1) ;
}

static int sort_check (stralloc const *s, size_t len)
{
  size_t i = 1 ;
  for (; i < len ; i++)
    if (sacmp(s+i-1, s+i) >= !sort_G->flaguniq) return 0 ;
  return 1 ;
}

int main (int argc, char const *const *argv)
{
  genalloc lines = GENALLOC_ZERO ; /* array of stralloc */
  int flagcheck = 0 ;
  char sep = '\n' ;
  sort_global globals = SORT_GLOBAL_ZERO ;
  sort_G = &globals ;
  PROG = "s6-sort" ;
  {
    subgetopt l = SUBGETOPT_ZERO ;
    for (;;)
    {
      int opt = subgetopt_r(argc, argv, "bcfru0", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'b' : sort_G->flagnoblanks = 1 ; break ;
        case 'c' : flagcheck = 1 ; break ;
        case 'f' : sort_G->comp = &strncasecmp ; break ;
        case 'r' : sort_G->flagreverse = 1 ; break ;
        case 'u' : sort_G->flaguniq = 1 ; break ;
        case '0' : sep = '\0' ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }

  if (sort_slurplines(&lines, sep) < 0) strerr_diefu1sys(111, "read from stdin") ;
  if (flagcheck) return !sort_check(genalloc_s(stralloc, &lines), genalloc_len(stralloc, &lines)) ;
  qsort(genalloc_s(stralloc, &lines), genalloc_len(stralloc, &lines), sizeof(stralloc), (qsortcmp_func_ref)&sacmp) ;
  if (sort_G->flaguniq) sort_uniq(&lines) ;
  if (!sort_outputlines(genalloc_s(stralloc, &lines), genalloc_len(stralloc, &lines)))
    strerr_diefu1sys(111, "write to stdout") ;
  return 0 ;
}
