#include <stdio.h>
#include <stdlib.h> /* for atoi */
#include <string.h> /* for strcpy */
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "errormsg.h"
#include "assem.h"
#include "graph.h"
#include "temp.h"
#include "flowgraph.h"

// 全局控制流图
// 将指令列表  Assem 转换为流图 flow graph


// 一个指令节点的所有定值
Temp_tempList FG_def(G_node n) {
  //　获取 node 的指令
  AS_instr instruction = (AS_instr) G_nodeInfo(n);
  switch (instruction->kind) {
    case I_OPER:
      return instruction->u.OPER.dst; // 二元运算的结果地址
    case I_LABEL:
      return NULL;
    case I_MOVE:
      return instruction->u.MOVE.dst; // 返回赋值的对向
    default:
      assert(0);
  }
}

// 节点 n 的使用
Temp_tempList FG_use(G_node n) {
  AS_instr instruction = (AS_instr) G_nodeInfo(n);
  switch (instruction->kind) {
    case I_OPER:
      return instruction->u.OPER.src; // 二元运算的 src 地址
    case I_LABEL:
      return NULL;
    case I_MOVE:
      return instruction->u.MOVE.src; // 返回赋值的对向
    default:
      assert(0);
  }
}

// 节点 n 的后继 G_nodeList G_succ(G_node n);
G_nodeList FG_succ(G_node n) {
  return G_succ(n);
}


// 节点是否为 MOVE
bool FG_isMove(G_node n) {
  AS_instr instruction = (AS_instr) G_nodeInfo(n);
  return instruction->kind == I_MOVE;
}

// 指令转化为流图
G_graph FG_AssemFlowGraph(AS_instrList il) {

  G_graph fg = G_Graph();
  G_node root = G_Node(fg, NULL);
  G_node prev = root;

  // 当前指令涉及的 label 节点表
  TAB_table label_node_table = TAB_empty();

  for (; il; il = il->tail) {
    AS_instr inst = il->head;
    G_node node = G_Node(fg, (void *) (inst)); // 当前指令对应的节点

    if (inst->kind == I_LABEL) { // 如果是 label 指令, 添加进 label 表
      // TAB_enter(TAB_table t, void *key, void *value)
      TAB_enter(label_node_table, (void *) (inst->u.LABEL.label), (void *) (node));
    } else if (inst->kind == I_OPER && inst->u.OPER.jumps) { // 如果是跳转指令
      // 将跳转的目标 label 添加进图 (struct AS_targets_{ Temp_labelList labels; })
      for (Temp_labelList l = inst->u.OPER.jumps->labels; l != NULL; l = l->tail) {
        // 查找目标 label 节点
        G_node label_node = TAB_look(label_node_table, (void *) (l->head));
        // 添加 node->每一个 label 的边, 表示每个node 可能跳转到的 label
        G_addEdge(node, label_node);
      }
    }

    // 沿指令树添加边
    G_addEdge(prev, node);
    prev = node;
  }

  return fg;
}

void FG_Showinfo(FILE *out, AS_instr instr, Temp_map map){

  // ???
  char* code_str;  // 汇编语句
  char* code_p;  //
  char r[200];  // 打印字符串

  switch (instr->kind) {
    case I_OPER:
      code_str = instr->u.OPER.assem;
      code_p = strrchr(code_str, '\n');  // 将代码中的 换行 \n 换成 \0
      if(code_p != NULL){
        *code_p = '\0';
      }
      // 格式化汇编语句并打印
      AS_format(r, code_str, instr->u.OPER.dst, instr->u.OPER.src, instr->u.OPER.jumps, map);
      break;

    case I_LABEL:
      code_str = instr->u.LABEL.assem;
      code_p = strrchr(code_str, '\n');
      if(code_p != NULL){
        *code_p = '\0';
      }
      AS_format(r, code_str, NULL, NULL, NULL, map);
      break;

    case I_MOVE:
      code_str = instr->u.MOVE.assem;
      code_p = strrchr(code_str, '\n');
      if(code_p != NULL){
        *code_p = '\0';
      }
      AS_format(r, code_str, instr->u.MOVE.dst, instr->u.MOVE.src, NULL, map);
      break;

  }
  fprintf(out,  "[%20s] ", r);
}
