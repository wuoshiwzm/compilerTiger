/**
    munchExp 针对 RISC 指令集
*/

#ifndef _CODEGEN_RISC_H_
#define _CODEGEN_RISC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "myframe.h"
#include "errormsg.h"
#include "codegen.h"

//int main(int argc, string *argv){
//  char buf[100];
//
//  sprintf(buf, "abc test s1  ");
//  printf("%s\n", buf);
//
//  return 1;
//
//}



char buf[100];

// struct Temp_temp_ {int num;};
static Temp_temp munchExp(T_exp e);

static void munchStm(T_stm s);

static void emit(AS_instr instr);

// 把所有实在参数传递到正确位置
static Temp_tempList munchArgs(int i, T_expList args, int *stack_size);

// 将 Temp_temp 转为 Temp_tempList
Temp_tempList singleTemp(Temp_temp t);

// 汇编指令(instruction)数据结构 struct AS_instrList_ { AS_instr head; AS_instrList tail;};
// 全局汇编指令表
static AS_instrList iList = NULL, iList_last = NULL;

static void emit(AS_instr instr) {
  if (iList == NULL) {
    iList_last = iList = AS_InstrList(instr, NULL);
  } else {
    AS_instrList newiList = AS_InstrList(instr, NULL);
    iList_last = iList->tail = newiList;
  }
}

Temp_tempList singleTemp(Temp_temp t) {
  return Temp_TempList(t, NULL);
}


// maximal munch 算法，从树的根结点开始，导找适合它的最大瓦片，用这个瓦片覆盖根结点和附近的其他几个结点，
// 遗留下的每个子树，再应用重复相同的算法
// 树结构： enum {T_SEQ, T_LABEL, T_JUMP, T_CJUMP, T_MOVE, T_EXP} kind;
static void munchStm(T_stm s) {

  switch (s->kind) {

    case T_SEQ:
      munchStm(s->u.SEQ.left);
      munchStm(s->u.SEQ.right);
      break;

    case T_LABEL:
      // typedef S_symbol Temp_label;
      sprintf(buf, "%s:\n", S_name(s->u.LABEL));
      AS_instr label_instr = AS_Label(String(buf), s->u.LABEL);
      emit(label_instr);
      break;

    case T_EXP:
      munchExp(s->u.EXP);
      break;

    case T_MOVE:
      T_exp src = s->u.MOVE.src;
      T_exp dst = s->u.MOVE.dst;
      Temp_temp tem_src = munchExp(src);
      Temp_temp tem_dst = munchExp(dst);
      // RISC 需要用到的 临时变量
      Temp_temp tem_new = Temp_newtemp();
      sprintf(buf, "add(move) `s0 `d0 #0\n");
      // AS_instr AS_Move(string a, Temp_tempList d, Temp_tempList s);
      AS_instr move_instr = AS_Move(String(buf), singleTemp(tem_dst), singleTemp(tem_src));
      emit(move_instr);
      break;
      /*
      if (dst->kind == T_MEM) {

        if (
            dst->u.MEM->kind == T_BINOP &&
            dst->u.MEM->u.BINOP.op == T_plus &&
            dst->u.MEM->u.BINOP.right->kind == T_CONST) {
          munchExp(dst->u.MEM->u.BINOP.left);
          munchExp(src);
          emit("STORE")
        } else if (
            dst->u.MEM->kind == T_BINOP &&
            dst->u.MEM->u.BINOP.op == T_plus &&
            dst->u.MEM->u.BINOP.left->kind == T_CONST) {
          munchExp(dst->u.MEM->u.BINOP.right);
          munchExp(src);
          emit("STORE")
        } else if (src->kind == T_MEM) {
          munchExp(dst->u.MEM);
          munchExp(src->u.MEM);
          emit("MOVEM");
        } else {
          munchExp(src);
          munchExp(dst->u.MEM);
          emit("STORE");
        }
        munchExp(src);
      } else if (dst->kind == T_TEMP) {
        // 某些情况下，赋值操作可以被视为一个加法操作。具体来说，如果目标是一个临时变量，那么赋值操作可以被视为一个加法操作，
        // 因为在这种情况下，源的值可以被加到目标的地址上，这样就可以将源的值复制到目标的位置。
        // MOVE(TEMP^i, src)
        munchExp(src);
        emit("ADD");
      } else {
        // 不会到这里 MOVE 的 dst 只可能是 MEM 或 TEMP
        assert(0);
      }*/

    case T_JUMP:
      // 跳转语句的副作用
      Temp_temp t = munchExp(s->u.JUMP.exp);
      sprintf(buf, "JMP `d0\n");

      // 生成跳转目标 AS_targets
      AS_targets targets = AS_Targets(s->u.JUMP.jumps);
      AS_instr jump_instr = AS_Oper(String(buf), singleTemp(t), NULL, targets);
      emit(jump_instr);
      break;

    case T_CJUMP:
      // struct {T_relOp op; T_exp left, right; Temp_label true, false;} CJUMP;
      AS_instr cjump;
      T_relOp oper = s->u.CJUMP.op;
      Temp_temp ltem = munchExp(s->u.CJUMP.left);
      Temp_temp rtem = munchExp(s->u.CJUMP.right);
      Temp_label true = s->u.CJUMP.true;
      Temp_label false = s->u.CJUMP.false;

      // 分支跳转，如果为真跳转到 true 分支， 否则跳转到 false  分支
      AS_targets true_targets = AS_Targets(Temp_labelList(s->u.CJUMP.true, NULL));
      AS_targets false_targets = AS_Targets(Temp_labelList(s->u.CJUMP.false, NNLL));

      // RISC 的条件跳转 只有单一分支，相当于只有 if(){}  没有 else 分支， else  相当于一个新的分支， 突出一个 "精简"
      switch (oper) {
        case T_eq:
          // 处理等于的情况
          sprintf(buf, "BEQ `d0 `d1 `truebr");
          true_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), true_targets);
          emit(true_cjump);

          sprintf(buf, "BNE `d0 `d1 `falsebr");
          false_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), false_targets);
          emit(false_cjump);
          break;

        case T_ne:
          // 处理不等于的情况
          sprintf(buf, "BNE `d0 `d1 `truebr");
          true_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), true_targets);
          emit(true_cjump);

          sprintf(buf, "BEQ `d0 `d1 `falsebr");
          false_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), false_targets);
          emit(false_cjump);
          break;

        case T_lt:
          // 处理小于的情况
          sprintf(buf, "BLT `d0 `d1 `truebr");
          true_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), true_targets);
          emit(true_cjump);

          sprintf(buf, "BGE `d0 `d1 `falsebr");
          false_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), false_targets);
          emit(false_cjump);
          break;

        case T_gt:
          // 处理大于的情况
          sprintf(buf, "BGT `d0 `d1 `truebr");
          true_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), true_targets);
          emit(true_cjump);

          sprintf(buf, "BLE `d0 `d1 `falsebr");
          false_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), false_targets);
          emit(false_cjump);
          break;

        case T_le:
          // 处理小于等于的情况
          sprintf(buf, "BLE `d0 `d1 `truebr");
          true_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), true_targets);
          emit(true_cjump);

          sprintf(buf, "BGT `d0 `d1 `falsebr");
          false_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), false_targets);
          emit(false_cjump);
          break;

        case T_ge:
          // 处理大于等于的情况
          sprintf(buf, "BGE `d0 `d1 `truebr");
          true_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), true_targets);
          emit(true_cjump);

          sprintf(buf, "BLT `d0 `d1 `falsebr");
          false_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), false_targets);
          emit(false_cjump);
          break;

        case T_ult:
          // 处理无符号小于的情况
          sprintf(buf, "BLTU `d0 `d1 `truebr");
          true_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), true_targets);
          emit(true_cjump);

          sprintf(buf, "BGEU `d0 `d1 `falsebr");
          false_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), false_targets);
          emit(false_cjump);
          break;

        case T_ule:
          // 处理无符号小于等于的情况
          sprintf(buf, "BLEU `d0 `d1 `truebr");
          true_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), true_targets);
          emit(true_cjump);

          sprintf(buf, "BGTU `d0 `d1 `falsebr");
          false_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), false_targets);
          emit(false_cjump);
          break;

        case T_ugt:
          // 处理无符号大于的情况
          sprintf(buf, "BGTU `d0 `d1 `truebr");
          true_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), true_targets);
          emit(true_cjump);

          sprintf(buf, "BLEU `d0 `d1 `falsebr");
          false_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), false_targets);
          emit(false_cjump);
          break;

        case T_uge:
          // 处理无符号大于等于的情况
          sprintf(buf, "BGEU `d0 `d1 `truebr");
          true_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), true_targets);
          emit(true_cjump);

          sprintf(buf, "BLTU `d0 `d1 `falsebr");
          false_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), false_targets);
          emit(false_cjump);
          break;


        default:
          assert(0); // impossible to get to here.
      }
  }

}

static Temp_temp munchExp(T_exp exp) {

  switch (exp->kind) {
    case T_BINOP:
      break;
    case T_MEM:
      break;
    case T_TEMP:
      break;
    case T_ESEQ:
      break;
    case T_NAME:
      break;
    case T_CONST:
      break;
    case T_CONST:
      break;
    default:
      assert(0); // will not get to here.
  }



}


#endif
