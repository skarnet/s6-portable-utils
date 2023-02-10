/* ISC license. */

#include <sys/stat.h>

#include <skalibs/types.h>
#include <skalibs/strerr.h>

#define USAGE "s6-chmod mode file"

int main (int argc, char const *const *argv)
{
  mode_t mode = 0 ;
  unsigned int m ;
  PROG = "s6-chmod" ;
  if (argc < 3) strerr_dieusage(100, USAGE) ;
  if (!uint0_oscan(argv[1], &m)) strerr_dieusage(100, USAGE) ;

  if (m & 0001) mode |= S_IXOTH ;
  if (m & 0002) mode |= S_IWOTH ;
  if (m & 0004) mode |= S_IROTH ;
  if (m & 0010) mode |= S_IXGRP ;
  if (m & 0020) mode |= S_IWGRP ;
  if (m & 0040) mode |= S_IRGRP ;
  if (m & 0100) mode |= S_IXUSR ;
  if (m & 0200) mode |= S_IWUSR ;
  if (m & 0400) mode |= S_IRUSR ;
  if (m & 01000) mode |= S_ISVTX ;
  if (m & 02000) mode |= S_ISGID ;
  if (m & 04000) mode |= S_ISUID ;

  if (chmod(argv[2], mode) == -1)
    strerr_diefu2sys(111, "change mode of ", argv[2]) ;
  return 0 ;
}
