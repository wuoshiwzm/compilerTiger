/*
 * parse.c - Parse source file.
 */
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "types.h"
#include "temp.h"
#include "translate.h"
#include "myframe.h"
#include "env.h"
#include "absyn.h"
#include "errormsg.h"
#include "parse.h"
#include "prabsyn.h"
#include "semant.h"

extern int yyparse(void);

extern A_exp absyn_root;

/*
 *
 * 解析为抽象语法树
 * parse source file fname;
   return abstract syntax data structure */
A_exp parse(string fname) {
  EM_reset(fname);
  if (yyparse() == 0) /* parsing worked */
    return absyn_root;
  else
    printf("oooops! not pass syntax! no need to do type check.  \n");
  return NULL;
}

int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "usage: a.out filename\n");
    exit(1);
  }

  printf("Start parsing %s ...\n", argv[1]);

  // 翻译成AST
  A_exp ast = parse(argv[1]);

  printf("End paring ... \n");
  pr_exp(stdout, ast, 4);


  /* 类型检查 生成片段 */
  // if (temp) {
  //   F_fragList flist = SEM_transProg(temp);
  // }
  return 0;
}
