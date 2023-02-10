/* ISC license. */

#include <string.h>
#include <skalibs/allreadwrite.h>
#include <skalibs/types.h>
#include <skalibs/strerr.h>

#define USAGE "s6-expr arithmetic expression"
#define bail() strerr_dief1x(2, "invalid expression")

enum expr_opnum_e
{
  T_DATA,
  T_AND,
  T_OR,
  T_LEFTP,
  T_RIGHTP,
  T_EQUAL,
  T_NEQUAL,
  T_GREATER,
  T_GREATERE,
  T_LESSER,
  T_LESSERE,
  T_PLUS,
  T_MINUS,
  T_TIMES,
  T_DIV,
  T_MOD
} ;

struct expr_token_s
{
  char const *string ;
  enum expr_opnum_e op ;
  unsigned int type ;
} ;

struct expr_node_s
{
  enum expr_opnum_e op ;
  unsigned int type ;
  unsigned int arg1 ;
  unsigned int arg2 ;
  long data ;
} ;

static unsigned int expr_lex (struct expr_node_s *tree, char const *const *argv)
{
  static struct expr_token_s const tokens[16] =
  {
    { "+", T_PLUS, 3 },
    { "-", T_MINUS, 3 },
    { "*", T_TIMES, 2 },
    { "/", T_DIV, 2 },
    { "%", T_MOD, 2 },
    { "(", T_LEFTP, 7 },
    { ")", T_RIGHTP, 8 },
    { "=", T_EQUAL, 4 },
    { "!=", T_NEQUAL, 4 },
    { "<", T_LESSER, 4 },
    { "<=", T_LESSERE, 4 },
    { ">", T_GREATER, 4 },
    { ">=", T_GREATERE, 4 },
    { "|", T_OR, 6 },
    { "&", T_AND, 5 },
    { 0, 0, 0 }
  } ;
  unsigned int pos = 0 ;

  for (; argv[pos] ; pos++)
  {
    unsigned int i = 0 ;
    for (i = 0 ; tokens[i].string ; i++)
      if (!strcmp(argv[pos], tokens[i].string))
      {
        tree[pos].op = tokens[i].op ;
        tree[pos].type = tokens[i].type ;
        break ;
      }
    if (!tokens[i].string)
    {
      tree[pos].op = T_DATA ;
      tree[pos].type = 0 ;
      if (!long_scan(argv[pos], &tree[pos].data)) bail() ;
    }
  }
  return pos ;
}

static void expr_reduce (struct expr_node_s *tree, unsigned int *stack, unsigned int *sp, unsigned int type)
{
  if (tree[stack[*sp-1]].type == type)
  {
    tree[stack[*sp-1]].arg1 = stack[*sp-2] ;
    tree[stack[*sp-1]].arg2 = stack[*sp] ;
    stack[*sp-2] = stack[*sp-1] ;
    *sp -= 2 ;
  }
  tree[stack[*sp]].type = type + 7 ;
}

static unsigned int expr_parse (struct expr_node_s *tree, unsigned int n)
{
  static char const table[9][15] =
  {
    "xsssssssxzzzzzz",
    "xxxxxxxx!zzzzzz",
    "mxxxxxxxMszzzzz",
    "mxxxxxxxMaszzzz",
    "mxxxxxxxMacszzz",
    "mxxxxxxxMacAszz",
    "mxxxxxxxMacAOsz",
    "xsssssssxzzzzzz",
    "mxxxxxxxMacAOEs"
  } ;
  unsigned int stack[n] ;
  unsigned int sp = 0, pos = 0 ;
  char cont = 1 ;
  stack[0] = n + 1 ;
  tree[n].type = 8 ; /* add ) for the final reduce */
  tree[n+1].type = 1 ; /* add EOF */
  while (cont)
  {
    switch (table[tree[pos].type][tree[stack[sp]].type])
    {
      case 'x' :  bail() ;
      case '!' :  cont = 0 ; break ;
      case 's' :  stack[++sp] = pos++ ; break ;
      case 'M' :
        if (tree[stack[sp-2]].type != 7) bail() ;
        stack[sp-2] = stack[sp-1] ;
        sp -= 2 ;
      case 'm' :  expr_reduce(tree, stack, &sp, 2) ; break ;
      case 'a' :  expr_reduce(tree, stack, &sp, 3) ; break ;
      case 'c' :  expr_reduce(tree, stack, &sp, 4) ; break ;
      case 'A' :  expr_reduce(tree, stack, &sp, 5) ; break ;
      case 'O' :  expr_reduce(tree, stack, &sp, 6) ; break ;
      case 'E' :  tree[stack[sp]].type = 14 ; break ;
      case 'z' : 
      default : strerr_dief1x(101, "internal error in parse, please submit a bug-report.") ; /* can't happen */
    }
  }
  if (sp != 2) bail() ;
  return stack[1] ;
}

static long expr_run (struct expr_node_s const *tree, unsigned int root)
{
  switch (tree[root].op)
  {
    case T_DATA :
      return tree[root].data ;
    case T_OR :
    {
      long r = expr_run(tree, tree[root].arg1) ;
      return r ? r : expr_run(tree, tree[root].arg2) ;
    }
    case T_AND :
    {
      long r = expr_run(tree, tree[root].arg1) ;
      return r ? expr_run(tree, tree[root].arg2) ? r : 0 : 0 ;
    }
    case T_EQUAL :
      return expr_run(tree, tree[root].arg1) == expr_run(tree, tree[root].arg2) ;
    case T_NEQUAL :
      return expr_run(tree, tree[root].arg1) != expr_run(tree, tree[root].arg2) ;
    case T_GREATER :
      return expr_run(tree, tree[root].arg1) > expr_run(tree, tree[root].arg2) ;
    case T_GREATERE :
      return expr_run(tree, tree[root].arg1) >= expr_run(tree, tree[root].arg2) ;
    case T_LESSER :
      return expr_run(tree, tree[root].arg1) < expr_run(tree, tree[root].arg2) ;
    case T_LESSERE :
      return expr_run(tree, tree[root].arg1) <= expr_run(tree, tree[root].arg2) ;
    case T_PLUS :
      return expr_run(tree, tree[root].arg1) + expr_run(tree, tree[root].arg2) ;
    case T_MINUS :
      return expr_run(tree, tree[root].arg1) - expr_run(tree, tree[root].arg2) ;
    case T_TIMES :
      return expr_run(tree, tree[root].arg1) * expr_run(tree, tree[root].arg2) ;
    case T_DIV :
      return expr_run(tree, tree[root].arg1) / expr_run(tree, tree[root].arg2) ;
    case T_MOD :
      return expr_run(tree, tree[root].arg1) % expr_run(tree, tree[root].arg2) ;
    default : strerr_dief1x(101, "internal error in expr_run, please submit a bug-report") ;
  }
}

int main (int argc, char const *const *argv)
{
  char fmt[LONG_FMT] ;
  long val ;
  size_t len ;
  PROG = "s6-expr" ;
  if (argc <= 1) return 2 ;
  {
    struct expr_node_s tree[argc + 1] ;
    val = expr_run(tree, expr_parse(tree, expr_lex(tree, argv+1))) ;
  }
  len = long_fmt(fmt, val) ;
  fmt[len++] = '\n' ;
  if (allwrite(1, fmt, len) < len)
    strerr_diefu1sys(111, "write to stdout") ;
  return !val ;
}
