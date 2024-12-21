/*
 * main.c
 */
#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "errormsg.h"
#include "temp.h" /* needed by translate.h */
#include "tree.h" /* needed by frame.h */
#include "assem.h"
#include "frame.h" /* needed by translate.h and printfrags prototype */
#include "translate.h"
#include "types.h"
#include "env.h"
#include "semant.h" /* function prototype for transProg */
#include "canon.h"
#include "prabsyn.h"
#include "printtree.h"
#include "parse.h"
#include "codegen.h"
#include "graph.h"
#include "flowgraph.h"


extern bool anyErrors;

/* print the assembly language instructions to filename.s */
static void doProc(FILE *out, F_frame frame, T_stm body) {
  AS_proc proc;


  printf(">>> Start doProc ...\n");
  T_stmList stmList;
  AS_instrList iList;

  // 1. 先不构造基本块
  stmList = C_linearize(body);
  stmList = C_traceSchedule(C_basicBlocks(stmList));
  printStmList(stdout, stmList);
  // 生成的语句表 最后都有一个    MOVE( TEMP t101, CONST 0)

  iList = F_codegen(frame, stmList); /* 9 */
  fprintf(out, "BEGIN %s\n", Temp_labelstring(F_name(frame)));

  Temp_map map = Temp_layerMap(F_tempMap(), Temp_name());

  // 2. 初始化全局寄存器 F_tempMap()
  AS_printInstrList(out, iList, Temp_layerMap(F_tempMap(), Temp_name()));
  fprintf(out, "END %s\n\n", Temp_labelstring(F_name(frame)));

  // 3. 控制流图 本章的代码生成只处理函数体，函数的进入 和 退出业务由此函数处理
  proc = F_procEntryExit3(frame, iList);
  G_graph graph = FG_AssemFlowGraph(proc->body);
  fprintf(out, "\n>>>>>>> Flow Graph <<<<<<<<\n");
  G_show(out, G_nodes(graph), map, (void (*)(FILE *out, void *, Temp_map)) FG_Showinfo);//(void (*)(void *))FG_Showinfo

}

int main(int argc, string *argv) {
  A_exp absyn_root; // 语法树根
  S_table base_env, base_tenv;
  F_fragList frags;
  char outfile[100];
  FILE *out = stdout;

  if (argc == 2) {
    absyn_root = parse(argv[1]);
    if (!absyn_root)
      return 1;

#if 0
    pr_exp(out, absyn_root, 0); /* print absyn data structure */
    fprintf(out, "\n");
#endif

//   Esc_findEscape(absyn_root); /* set varDec's escape field */
    printf(">>>>>> Start SEM_transProg. \n");
    frags = SEM_transProg(absyn_root);
    if (anyErrors) return 1; /* don't continue */
    printf(">>>>>> End SEM_transProg. \n");

    /* 指令选择，生成编译文件 xxx.s */
    printf(">>>>>> 开始指令选择. \n");
    sprintf(outfile, "%s.s", argv[1]);
    out = fopen(outfile, "w");
    printf(">>>>>> 解析片段  frags... \n");

    /* Chapter 8, 9, 10, 11 & 12 */
    for (; frags; frags = frags->tail) {
//      F_printFrag(frags->head);

      if (frags->head->kind == F_procFrag) {
        doProc(out, frags->head->u.proc.frame, frags->head->u.proc.body);
      } else if (frags->head->kind == F_stringFrag) {
        fprintf(out, "String frag: %s\n", frags->head->u.stringg.str);
      }
    }

    printf(">>> >>> close file \n");
    fclose(out);
    return 0;
  }
  EM_error(0, "usage: tiger file.tig");
  return 1;
}
