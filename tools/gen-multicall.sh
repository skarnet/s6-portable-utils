#!/bin/sh -e

LC_ALL=C ; export LC_ALL

P="$1"
p=`echo $P | tr - _`

echo '/* ISC license. */'
echo
echo '#include <skalibs/nonposix.h>'
echo
{ echo '#include <string.h>' ; echo '#include <stdlib.h>' ; cat src/$P/*.c | grep '^#include <' | grep -vF '<skalibs/' | grep -vF "<$P/" ; } | sort -u

cat <<EOF

#include <skalibs/skalibs.h>

#include <$P/config.h>

typedef int emain_func (int, char const *const *, char const *const *) ;
typedef emain_func *emain_func_ref ;

typedef struct multicall_app_s multicall_app, *multicall_app_ref ;
struct multicall_app_s
{
  char const *name ;
  emain_func_ref mainf ;
} ;

static int multicall_app_cmp (void const *a, void const *b)
{
  char const *name = a ;
  multicall_app const *p = b ;
  return strcmp(name, p->name) ;
}
EOF

for i in `ls -1 src/$P/deps-exe` ; do
  j=`echo $i | tr - _`
  echo
  grep -v '^#include ' < src/$P/${i}.c | grep -vF '/* ISC license. */' | sed -e "s/int main (.*)$/int ${j}_main (int argc, char const *const *argv, char const *const *envp)/"
  echo
  echo '#undef USAGE'
  echo '#undef dieusage'
  echo '#undef dienomem'
  echo '#undef bail'
done

cat <<EOF

static int ${p}_main (int, char const *const *, char const *const *) ;

static multicall_app const multicall_apps[] =
{
EOF

for i in `{ echo $P ; ls -1 src/$P/deps-exe ; } | sort` ; do
  j=`echo $i | tr - _`
  echo "  { .name = \"${i}\", .mainf = &${j}_main },"
done

cat <<EOF
} ;

#define USAGE "$P subcommand [ arguments... ]"
#define dieusage() strerr_dieusage(100, USAGE)

static int ${p}_main (int argc, char const *const *argv, char const *const *envp)
{
  multicall_app const *p ;
  PROG = "$P" ;
  if (argc < 2) dieusage() ;
  p = bsearch(argv[1], multicall_apps, sizeof(multicall_apps) / sizeof(multicall_app), sizeof(multicall_app), &multicall_app_cmp) ;
  if (!p) strerr_dief2x(100, "unknown subcommand: ", argv[1]) ;
  return (*(p->mainf))(argc-1, argv+1, envp) ;
}

int main (int argc, char const *const *argv, char const *const *envp)
{
  multicall_app const *p ;
  char const *name = strrchr(argv[0], '/') ;
  if (name) name++ ; else name = argv[0] ;
  p = bsearch(name, multicall_apps, sizeof(multicall_apps) / sizeof(multicall_app), sizeof(multicall_app), &multicall_app_cmp) ;
  return p ? (*(p->mainf))(argc, argv, envp) : ${p}_main(argc, argv, envp) ;
}
EOF
