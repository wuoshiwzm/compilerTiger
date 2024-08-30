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


// 汇编字符串
char buf[100];

static Temp_label fname = NULL; // initialized in codegen and accessed in munchStm and munchExp

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
      // 汇编标注， typedef S_symbol Temp_label;
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

      //  目标地址为 TEMP （寄存器）
      if (dst->kind == T_TEMP) {
        //　向 TEMP MOVE 数据
        // TEMP_dst <- CONST, 将 CONST 加在 0 (F_ZERO) 上
        if (src->kind == T_CONST) {
          sprintf(buf, "(MIPS)ADD `d0, `s0, %d", src->u.CONST);
          AS_instr instr = AS_Oper(String(buf),
                                   Temp_TempList(dst->u.TEMP, NULL),
                                   Temp_TempList(F_ZERO(), NULL),
                                   NULL);
          emit(instr);
        }
        // TEMP_dst <- CONST + TEMP(right)
        else if (src->kind == T_BINOP && src->u.BINOP.op == T_plus && src->u.BINOP.left->kind == T_CONST) {
          T_exp left_exp = src->u.BINOP.left;
          T_exp right_exp = src->u.BINOP.right;
          sprintf(buf, "(MIPS)ADD `d0, `s0, %d", left_exp->u.CONST);
          AS_instr instr = AS_Oper(String(buf),
                                   Temp_TempList(dst->u.TEMP, NULL),
                                   Temp_TempList(munchExp(right_exp), NULL),
                                   NULL);
        }
        // TEMP_dst <- TEMP(left) + CONST
        else if (src->kind == T_BINOP && src->u.BINOP.op == T_plus && src->u.BINOP.right->kind == T_CONST) {
          T_exp left_exp = src->u.BINOP.left;
          T_exp right_exp = src->u.BINOP.right;
          sprintf(buf, "(MIPS)ADD `d0, `s0, %d", right_exp->u.CONST);
          AS_instr instr = AS_Oper(String(buf),
                                   Temp_TempList(dst->u.TEMP, NULL),
                                   Temp_TempList(munchExp(left_exp), NULL),
                                   NULL);
          emit(instr);
        }
        // TEMP_dst <- TEMP(left) - CONST
        else if(src->kind == T_BINOP && src->u.BINOP.op == T_minus && src->u.BINOP.right->kind == T_CONST){
          T_exp left_exp = src->u.BINOP.left;
          T_exp right_exp = src->u.BINOP.right;
          sprintf(buf, "(MIPS)ADD `d0, `s0, -%d", right_exp->u.CONST);
          AS_instr instr = AS_Oper(String(buf),
                                   Temp_TempList(dst->u.TEMP, NULL),
                                   Temp_TempList(munchExp(left_exp), NULL),
                                   NULL);
        }

        // lw $dst, offset($src) 源地址为 MEM 内存  MIPS 指令
        else if (src->kind == T_MEM) {
          T_exp src_addr = src->u.MEM;
          if (src->kind == T_BINOP && src->u.BINOP.op == T_plus && src->u.BINOP.right->kind == T_CONST) {
            T_exp left_exp = src->u.BINOP.left;
            T_exp right_exp = src->u.BINOP.right;
            // TEMP_dst <- MEM[SP + FRAME_SIZE + CONST]  栈帧偏移, Temp_labelstring(fname)
            if (left_exp->kind == T_TEMP && left_exp->u.TEMP = F_FP()){
              sprintf(buf, "(MIPS)lw `d0, %d+%s_framesize(`s0)", right_exp->u.CONST, Temp_labelstring(fname));
              AS_instr instr = AS_Oper(String(buf), Temp_TempList(dst->u.TEMP, NULL), Temp_TempList(F_SP(), NULL), NULL);
              emit(instr);
            }
            // TEMP_dst <- MEM[TEMP + CONST]
            else{
              sprintf(buf, "(MIPS)lw `d0, %d(`s0)", right_exp->u.CONST);
              AS_instr instr = AS_Oper(String(buf), Temp_TempList(dst->u.TEMP, NULL), Temp_TempList(munchExp(left), NULL), NULL);
              emit(instr);
            }
          }
          // TEMP_dst <- MEM[TEMP]
          else {
            sprintf(buf, "(MIPS)lw `d0, 0(`s0)", right_exp->u.CONST);
            AS_instr instr = AS_Oper(String(buf), Temp_TempList(dst->u.TEMP, NULL), Temp_TempList(munchExp(src_addr), NULL), NULL);
            emit(instr);
          }
        }
        // TEMP <- TEMP
        else {
          sprintf(buf, "(MIPS)add `d0, `s0, `s1");
          AS_instr instr = AS_Oper(String(buf),Temp_TempList(dst->u.TEMP, NULL),
                                   Temp_TempList(F_ZERO(), Temp_TempList(munchExp(src), NULL),
                                   NULL );
        }
      }
      // 目标地址为 MEM 内存, 只修改 MEM值， 不返回
      else if(dst->kind == T_MEM){
        // 目标内存地址
        T_exp addr = dst->u.MEM;

        if (dst_addr->kind == T_BINOP && addr->u.BINOP.op == T_plus && addr->u.BINOP.right->kind == T_CONST) {
          T_exp left = addr->u.BINOP.left;
          T_exp right = addr->u.BINOP.right;

          // MEM[SP + FRAME_SIZE + CONST] <- TEMP
          if (left->kind ==T_TEMP && left->u.TEMP == F_FP()){
            sprintf(buf, "(MIPS)sw `s0, `%d+%s_framesize(`d0) or s1???", right_exp->u.CONST, Temp_labelstring(fname));
            AS_instr instr = AS_Oper(String(buf),
                                     NULL,
                                     Temp_TempList(munchExp(src), Temp_TempList(F_SP(), NULL)),
                                     NULL);
            emit(instr);
          }
          // MEM[TEMP + CONST] <- TEMP
          else {
            sprintf(buf, "(MIPS)sw `s0, `%d(`d0)", right_exp->u.CONST);
            AS_instr instr = AS_Oper(String(buf),
                                     NULL,
                                     Temp_TempList(munchExp(src), Temp_TempList(munchExp(), NULL)),
                                     NULL);
            emit(instr);
          }
        }
        // MEM[TEMP] <- TEMP
        else{
            sprintf(buf, "sw `s0, 0(`d0)");
            AS_instr instr = AS_Oper(String(buf),
                                     NULL,
                                     Temp_TempList(munchExp(src), Temp_TempList(munchExp(addr), NULL)),
                                     NULL );

        }
      } else {
        // 不会到这里 MOVE 的 dst 只可能是 MEM 或 TEMP
        assert(0);
      }

    case T_JUMP:
      T_exp dst = s->u.JUMP.exp;

      // dst: Temp_label
      // j 命令用于无条件跳转到指定地址
      if (dst->kind == T_NAME) {
        Temp_label lab = dst->u.NAME;
        // 跳转语句的副作用
        Temp_temp t = munchExp(dst);
        sprintf(buf, "j `j0");
        AS_instr instr = AS_Oper(String(buf),
                                 NULL,
                                 NULL,
                                 AS_Targets(Temp_LabelList(lab, NULL)));
        emit(instr);
      }
      // jr 命令用于无条件跳转到寄存器中存储的地址
      else {
        sprintf(buf, "jr `j0");
        AS_instr instr = AS_Oper(String(buf),
                                 NULL,
                                 Temp_TempList(munchExp(dst), NULL),
                                 NULL);
        emit(instr);
      }
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
      break;
    default:
      assert(0)

  }
}


// 表达式转换
// 返回表达式的值，存在 Temp_temp
static Temp_temp munchExp(T_exp exp) {

  switch (exp->kind) {

    case T_BINOP: {
      T_exp lexp = exp->u.BINOP.left;
      T_exp rexp = exp->u.BINOP.right;
      T_binOp oper = exp->u.BINOP.op;

      Temp_temp ltem = munchExp(lexp);
      Temp_temp rtem = munchExp(rexp);

      Temp_temp t = Temp_newtemp();
      Temp_temp r1 = Temp_newtemp();
      Temp_temp r2 = Temp_newtemp();
      Temp_temp r3 = Temp_newtemp();

      bool with_const = exp->u.BINOP.left->kind == T_CONST || exp->u.BINOP.right->kind == T_CONST;

      if (with_const) {
        // 左右操作数都是常量
        if (lexp->kind == T_CONST && lexp->kind == T_CONST) {
          printf("the left and right exp is all CONST !!!\n")
        } else {
          // 左右操作数只有 1 个是常量779123
          T_exp const_exp = lexp->u.BINOP.left->kind == T_CONST ? lexp : rexp;
          T_exp exp_exp = lexp->u.BINOP.left->kind == T_CONST ? rexp : lexp;
          switch (exp->u.BINOP.op) {
            AS_targets targets = AS_Targets(singleTemp(t));
            case T_plus:
              // s0 + s1 => d0
              sprintf(buf, "ADD `d0, `s0, %d\n", const_exp->u.CONST);
              AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(exp_exp)), NULL, targets));
              emit(binop);

            case T_minus:
              // s0 - s1 => d0
              sprintf(buf, "SUB `d0, `s0, %d\n", const_exp->u.CONST)
              AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(exp_exp)), NULL, targets));
              break;

            case T_mul:
              sprintf(buf, "MUL `d0, `s0, %d\n", const_exp->u.CONST)
              AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(exp_exp)), NULL, targets));
              break;

            case T_div:
              sprintf(buf, "DIV  `d0, `s0, %d\n", const_exp->u.CONST)
              AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(exp_exp)), NULL, targets));
              break;

            case T_and:
              sprintf(buf, "AND `d0, `s0, %d\n", const_exp->u.CONST)
              AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(exp_exp)), NULL, targets));
              break;

            case T_or:
              sprintf(buf, "OR `d0, `s0, %d\n", const_exp->u.CONST)
              AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(exp_exp)), NULL, targets));
              break;

              // 逻辑左移
            case T_shift:
              sprintf(buf, "SLL `d0, `s0, %d\n", const_exp->u.CONST)
              AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(exp_exp)), NULL, targets));
              break;

              // 逻辑右移
            case T_rshift:
              sprintf(buf, "SRL `d0, `s0, %d\n", const_exp->u.CONST)
              AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(exp_exp)), NULL, targets));
              break;

              // 算术右移
            case T_arshift:
              sprintf(buf, "SRA `d0, `s0, %d\n", const_exp->u.CONST)
              AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(exp_exp)), NULL, targets));
              break;

            case T_xor:
              sprintf(buf, "XOR `d0, `s0, %d\n", const_exp->u.CONST)
              AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(exp_exp)), NULL, targets));
              break;
          }
        }
      } else {
        switch (exp->u.BINOP.op) {
          AS_targets targets = AS_Targets(singleTemp(t));
          case T_plus:
            // s0 + s1 => d0
            sprintf(buf, "ADD `d0, `s0, `s1\n");
            AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(lexp)), singleTemp(munchExp(rexp)),
            targets));
            emit(binop);

          case T_minus:
            // s0 - s1 => d0
            sprintf(buf, "SUB `d0, `s0, `s1\n");
            AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(lexp)), singleTemp(munchExp(rexp)),
            targets));
            break;

          case T_mul:
            sprintf(buf, "MUL `d0, `s0, `s1\n");
            AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(lexp)), singleTemp(munchExp(rexp)),
            targets));
            break;

          case T_div:
            sprintf(buf, "DIV  `d0, `s0, `s1\n");
            AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(lexp)), singleTemp(munchExp(rexp)),
            targets));
            break;

          case T_and:
            sprintf(buf, "AND `d0, `s0, `s1\n");
            AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(lexp)), singleTemp(munchExp(rexp)),
            targets));
            break;

          case T_or:
            sprintf(buf, "OR `d0, `s0, `s1\n");
            AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(lexp)), singleTemp(munchExp(rexp)),
            targets));
            break;

            // 逻辑左移
          case T_shift:
            sprintf(buf, "SLL `d0, `s0, `s1\n");
            AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(lexp)), singleTemp(munchExp(rexp)),
            targets));
            break;

            // 逻辑右移
          case T_rshift:
            sprintf(buf, "SRL `d0, `s0, `s1\n");
            AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(lexp)), singleTemp(munchExp(rexp)),
            targets));
            break;

            // 算术右移
          case T_arshift:
            sprintf(buf, "SRA `d0, `s0, `s1\n");
            AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(lexp)), singleTemp(munchExp(rexp)),
            targets));
            break;

          case T_xor:
            sprintf(buf, "XOR `d0, `s0, `s1\n");
            AS_instr binop = AS_Oper(String(buf), singleTemp(munchExp(lexp)), singleTemp(munchExp(rexp)),
            targets));
            break;
        }
      }

      return t;
    }

    // exp 为临时变量，寄存器，直接返回
    case T_TEMP:
      return exp->u.TEMP;

    // 内存加载
    case T_MEM: {
      T_exp mem_exp = exp->u.MEM;

      // 计算获取地址： MEM(exp1 + CONST(i)),  MEM(CONST(i) + exp1)
      if (mem_exp->kind == T_BINOP) {
        T_exp lexp = mem_exp->u.BINOP.left;
        T_exp rexp = mem_exp->u.BINOP.right;
        if (mem_exp->u.BINOP.op == T_plus) {
          // MEM(CONST(i) + exp1)
          if (lexp->kind == T_CONST) {
            Temp_temp new_temp = Temp_newtemp();
            sprintf(buf, "lw `d0, %d(`s0)", lexp->u.CONST);
            AS_instr instr = AS_Oper(String(buf),
                                     Temp_TempList(new_temp, NULL),
                                     Temp_TempList(munchExp(rexp), NULL),
                                     NULL
            );
            emit(instr);
            return new_temp;
          }
            // MEM(exp1 + CONST(i))
          else if (rexp->kind == T_CONST) {
            Temp_temp new_temp = Temp_newtemp();
            sprintf(buf, "lw `d0, %d(`s0)", rexp->u.CONST);
            AS_instr instr = AS_Oper(String(buf),
                                     Temp_TempList(new_temp, NULL),
                                     Temp_TempList(munchExp(lexp), NULL),
                                     NULL
            );
            emit(instr);
            return new_temp;
          }
        }
      }

        // 常量获取地址： MEM[CONST i] ld $t,C($zero)
      else if (mem_exp->kind == T_CONST) {
        sprintf(buf, "lw `d0, %d(`s0)", mem_exp->u.CONST);
        Temp_temp new_temp = Temp_newtemp();
        AS_instr instr = AS_Oper(String(buf),
                                 Temp_TempList(new_temp, NULL), // d0
                                 Temp_TempList(F_ZERO(), NULL), // s0
                                 NULL
        );
        return new_temp;
      }

        // 计算地址 s0 并将它存入寄存器 MEM(T_Exp)	 ld $t,0($s)
      else {
        Temp_temp new_temp = Temp_newtemp();
        sprintf(buf, "ld `d0, 0(`s0)");
        AS_instr instr = AS_Oper(String(buf),
                                 Temp_TempList(new_temp, NULL), // d0
                                 Temp_TempList(munchExp(exp), NULL), // s0
                                 NULL);
        emit(instr);
        return new_temp;
      }
    }

    // 常量 直接把值传给 exp
    case T_CONST: {
      Temp_temp new_temp = Temp_newtemp();
      sprintf(buf, "li `d0, %d \n", exp->u.CONST);
      AS_instr const_instr = AS_Oper(String(buf),
                                     Temp_TempList(new_temp, NULL),
                                     NULL,
                                     NULL);
      emit(const_instr);
      return new_temp;
    }


    case T_NAME: {
      Temp_temp new_temp = Temp_newtemp();
      AS_instr instr = AS_Label(buf, Temp_LabelList(exp->u.NAME, NULL));
      emit(instr);
      return new_temp;
      break;
    }

    // 给地址赋值为常量， 并返回这个地址
    case T_CONST: {
      Temp_temp new_temp = Temp_newtemp();
      sprintf(buf, "li d0`, %d", exp->u.CONST);
      AS_instr instr = AS_Oper(String(buf),
                               Temp_TempList(new_temp, NULL), // d0
                               NULL,
                               NULL );
      emit(instr);
      return new_temp;
    }

    // 一个调用
    case T_CALL: {
      int total = 0;
      // struct {T_exp fun; T_expList args;} CALL;
      Temp_tempList arglist = munchArgs(exp->u.CALL.args);
      T_exp expfun = exp->u.CALL.fun;

      // 要调用的函数名 是一个 LABEL
      if (expfun->kind == T_NAME){

        // 1. 保存寄存器

        // 2. 传递参数

        // 2.1 前4个进寄存器，后面的进内存 栈帧，这里暂时先全部进栈帧


        // 3. 调用函数



      }




      break;
    }

    // 基本块中 去掉了 ESEQ


    default:
      assert(0); // will never get to here.
  }


}


#endif
