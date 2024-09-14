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
#include "errormsg.h"
#include "codegen.h"
#include "printtree.h"

/*
 * >>>>>>>>>>>>>>>>>>>>>>>>>> init & declare <<<<<<<<<<<<<<<<<<<<<<<<<<
 */

// 汇编指令字符串
char buf[100];
static Temp_label fname = NULL; // initialized in codegen and accessed in munchStm and munchExp
static Temp_temp munchExp(T_exp e);

static void munchStm(T_stm s);

static void emit(AS_instr instr);

// 把所有实在参数传递到正确位置
static Temp_tempList munchArgs(int *i, T_expList args);

// ???
static Temp_tempList callDefs();

// 将 Temp_temp 转为 Temp_tempList
Temp_tempList singleTemp(Temp_temp t);

/**
 * >>>>>>>>>>>>>>>>>>>>>>>>>> 生成全局汇编指令列表 <<<<<<<<<<<<<<<<<<<<<<<<<<
 */
// 汇编指令(instruction)数据结构 struct AS_instrList_ { AS_instr head; AS_instrList tail;};
// 全局汇编指令表 和 最新指令
static AS_instrList iList = NULL, iList_last = NULL;

// 加入汇编列表
static void emit(AS_instr instr) {
  printf("    call emit: %s \n", instr->u.MOVE.assem);
  if (iList == NULL) {
    iList = iList_last = AS_InstrList(instr, NULL);
  } else {
    iList_last = iList_last->tail = AS_InstrList(instr, NULL);
  }
}

Temp_tempList singleTemp(Temp_temp t) {
  return Temp_TempList(t, NULL);
}

/*
  maximal munch 算法，从树的根结点开始，导找适合它的最大瓦片，用这个瓦片覆盖根结点和附近的其他几个结点，
  遗留下的每个子树，再应用重复相同的算法
  树结构： enum {T_SEQ, T_LABEL, T_JUMP, T_CJUMP, T_MOVE, T_EXP} kind;
 */
static void munchStm(T_stm s) {
  printf(">>> Start munchStm stm, kind: %d\n", s->kind);

  // 打印 stm 解析
  FILE *fp = fopen("stm.tree", "w");
  printStmList(fp, T_StmList(s, NULL));
  fprintf(fp, "<<<\n");

  T_exp src;
  T_exp dst;
  AS_instr cjump;
  Temp_temp ltem;
  Temp_temp rtem;

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
      src = s->u.MOVE.src;
      dst = s->u.MOVE.dst;
      printf("move dst:  %d\n", dst->kind);

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
        else if (src->kind == T_BINOP && src->u.BINOP.op == T_minus && src->u.BINOP.right->kind == T_CONST) {
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
          T_exp left_exp = src->u.BINOP.left;
          T_exp right_exp = src->u.BINOP.right;
          if (src->kind == T_BINOP && src->u.BINOP.op == T_plus && src->u.BINOP.right->kind == T_CONST) {
            // TEMP_dst <- MEM[SP + FRAME_SIZE + CONST]  栈帧偏移, Temp_labelstring(fname)
            if (left_exp->kind == T_TEMP && left_exp->u.TEMP == F_FP()) {
              sprintf(buf, "(MIPS)lw `d0, %d+%s_framesize(`s0)", right_exp->u.CONST, Temp_labelstring(fname));
              AS_instr instr = AS_Oper(String(buf), Temp_TempList(dst->u.TEMP, NULL), Temp_TempList(F_SP(), NULL),
                                       NULL);
              emit(instr);
            }
              // TEMP_dst <- MEM[TEMP + CONST]
            else {
              sprintf(buf, "(MIPS)lw `d0, %d(`s0)", right_exp->u.CONST);
              AS_instr instr = AS_Oper(String(buf),
                                       Temp_TempList(dst->u.TEMP, NULL),
                                       Temp_TempList(munchExp(left_exp), NULL),
                                       NULL);
              emit(instr);
            }
          }
            // TEMP_dst <- MEM[TEMP]
          else {
            sprintf(buf, "(MIPS)lw `d0, 0(`s0)", right_exp->u.CONST);
            AS_instr instr = AS_Oper(String(buf),
                                     Temp_TempList(dst->u.TEMP, NULL),
                                     Temp_TempList(munchExp(src_addr), NULL),
                                     NULL);
            emit(instr);
          }
        }
          // TEMP <- TEMP
        else {
          printf("MOVE TEMP <- TEMP \n");
          sprintf(buf, "(MIPS)add `d0, `s0, `s1");
          AS_instr instr = AS_Oper(String(buf),
                                   Temp_TempList(dst->u.TEMP, NULL),
                                   Temp_TempList(F_ZERO(), Temp_TempList(munchExp(src), NULL)),
                                   NULL);
          emit(instr);
          printf("done...  \n");
        }
      }
        // 目标地址为 MEM 内存, 只修改 MEM值， 不返回
      else if (dst->kind == T_MEM) {
        // 目标内存地址
        T_exp addr = dst->u.MEM;
        if (addr->kind == T_BINOP && addr->u.BINOP.op == T_plus && addr->u.BINOP.right->kind == T_CONST) {
          T_exp left = addr->u.BINOP.left;
          T_exp right = addr->u.BINOP.right;

          // MEM[SP + FRAME_SIZE + CONST] <- TEMP
          if (left->kind == T_TEMP && left->u.TEMP == F_FP()) {
            sprintf(buf, "(MIPS)sw `s0, `%d+%s_framesize(`d0) or s1???", right->u.CONST, Temp_labelstring(fname));
            AS_instr instr = AS_Oper(String(buf),
                                     NULL,
                                     Temp_TempList(munchExp(src), Temp_TempList(F_SP(), NULL)),
                                     NULL);
            emit(instr);
          }
            // MEM[TEMP + CONST] <- TEMP
          else {
            sprintf(buf, "(MIPS)sw `s0, `%d(`d0)", right->u.CONST);
            AS_instr instr = AS_Oper(String(buf),
                                     NULL,
                                     Temp_TempList(munchExp(src), Temp_TempList(munchExp(dst), NULL)),
                                     NULL);
            emit(instr);
          }
        }
          // MEM[TEMP] <- TEMP
        else {
          sprintf(buf, "sw `s0, 0(`d0)");
          AS_instr instr = AS_Oper(String(buf),
                                   NULL,
                                   Temp_TempList(munchExp(src), Temp_TempList(munchExp(addr), NULL)),
                                   NULL);
          emit(instr);
        }
      }
        // 不会到这里 MOVE 的 dst 只可能是 MEM 或 TEMP
      else {
        assert(0);
      }
      break;

    case T_JUMP:
      dst = s->u.JUMP.exp;
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
      ltem = munchExp(s->u.CJUMP.left);
      rtem = munchExp(s->u.CJUMP.right);

      T_exp left = s->u.CJUMP.left;
      T_exp right = s->u.CJUMP.right;
      T_relOp oper = s->u.CJUMP.op;
      Temp_label true_label = s->u.CJUMP.true;
      Temp_label false_label = s->u.CJUMP.false;

      AS_instr true_cjump;
      AS_instr false_cjump;

      // 分支跳转，如果为真跳转到 true 分支， 否则跳转到 false  分支
      AS_targets true_targets = AS_Targets(Temp_LabelList(true_label, NULL));
      AS_targets false_targets = AS_Targets(Temp_LabelList(false_label, NULL));

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
          AS_instr true_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), true_targets);
          emit(true_cjump);

          sprintf(buf, "BGE `d0 `d1 `falsebr");
          AS_instr false_cjump = AS_Oper(String(buf), singleTemp(ltem), singleTemp(rtem), false_targets);
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
      assert(0);
  }
}


// 表达式转换
static Temp_temp munchExp(T_exp exp) {

  Temp_temp new_temp;
  Temp_temp ret_temp = Temp_newtemp();
  AS_instr instr;

  printf(">>> munchExp kind: %d \n", exp->kind);

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

      AS_instr binop_instr;

      switch (exp->u.BINOP.op) {
        case T_plus:
          // right + left (CONST) => ret_temp
          if (lexp->kind == T_CONST) {
            sprintf(buf, "addi `d0, `s0, %d\n", lexp->u.CONST);
            instr = AS_Oper(String(buf),
                            Temp_TempList(ret_temp, NULL),
                            Temp_TempList(munchExp(rexp), NULL),
                            NULL);
          }
            // right (CONST) + left => ret_temp
          else if (rexp->kind == T_CONST) {
            sprintf(buf, "addi `d0, `s0, %d\n", rexp->u.CONST);
            instr = AS_Oper(String(buf),
                            Temp_TempList(ret_temp, NULL),
                            Temp_TempList(munchExp(lexp), NULL),
                            NULL);
          }
            // left + right => ret_temp
          else {
            sprintf(buf, "addi `d0, `s0, s1\n");
            instr = AS_Oper(String(buf),
                            Temp_TempList(ret_temp, NULL),
                            Temp_TempList(munchExp(rexp), Temp_TempList(munchExp(lexp), NULL)),
                            NULL);
          }
          emit(instr);
          break;

        case T_minus:
          // left - right (CONST) => ret_temp
          if (rexp->kind == T_CONST) {
            sprintf(buf, "addi `d0, `s0, %d\n", -rexp->u.CONST);
            instr = AS_Oper(String(buf),
                            Temp_TempList(ret_temp, NULL),
                            Temp_TempList(munchExp(lexp), NULL),
                            NULL);
            emit(instr);
          }
            // left - right => ret_temp
          else {
            sprintf(buf, "addi `d0, `s0, s1\n");
            instr = AS_Oper(String(buf),
                            Temp_TempList(ret_temp, NULL),
                            Temp_TempList(munchExp(lexp), Temp_TempList(munchExp(rexp), NULL)),
                            NULL);
            emit(instr);
          }
          // s0 * s1 => 特殊寄存器 (汇编指令自己完成)
          // 特殊寄存器 => ret_temp
        case T_mul:
          sprintf(buf, "mult --> `s0, `s1\n");
          instr = AS_Oper(String(buf),
                          NULL,
                          Temp_TempList(munchExp(lexp), Temp_TempList(munchExp(rexp), NULL)),
                          NULL);
          emit(instr);
          /*
           * mflo 指令用于将乘法操作的结果的低32位（即存储在 LO 寄存器中的值）移动到指定的通用寄存器中。
           * 当你执行了一个乘法操作（如 mult 或 multu）后，乘法结果的高32位被存储在 HI 寄存器中，而低
           * 32位被存储在 LO 寄存器中。mflo 指令就是用来访问这个低32位结果的。
           */
          sprintf(buf, "mflo `d0");
          instr = AS_Oper(String(buf),
                          Temp_TempList(ret_temp, NULL),
                          NULL,
                          NULL);
          emit(instr);
          break;
        case T_div:
          // div left, right
          sprintf(buf, "div `s0, `s1");
          instr = AS_Oper(String(buf),
                          NULL,
                          Temp_TempList(munchExp(lexp), Temp_TempList(munchExp(rexp), NULL)),
                          NULL);
          emit(instr);
          // mflo r
          sprintf(buf, "mflo `d0");
          instr = AS_Oper(String(buf),
                          Temp_TempList(ret_temp, NULL),
                          NULL,
                          NULL);
          emit(instr);
          break;

        case T_and:
          sprintf(buf, "AND `d0, `s0, `s1\n");
          instr = AS_Oper(String(buf),
                          Temp_TempList(ret_temp, NULL),
                          Temp_TempList(munchExp(lexp), Temp_TempList(munchExp(rexp), NULL)),
                          NULL);
          emit(instr);
          break;

        case T_or:
          sprintf(buf, "OR `d0, `s0,`s1\n");
          instr = AS_Oper(String(buf),
                          Temp_TempList(ret_temp, NULL),
                          Temp_TempList(munchExp(lexp), Temp_TempList(munchExp(rexp), NULL)),
                          NULL);
          emit(instr);
          break;

          // 逻辑左移常数位(右侧常数给出)
        case T_lshift:
          sprintf(buf, "SLL `d0, `s0, %d\n", rexp->u.CONST);
          instr = AS_Oper(String(buf),
                          Temp_TempList(ret_temp, NULL),
                          Temp_TempList(munchExp(lexp), NULL),
                          NULL);
          emit(instr);
          break;

          // 逻辑右移指定位数
        case T_rshift:
          sprintf(buf, "SRL `d0, `s0, %d\n", rexp->u.CONST);
          instr = AS_Oper(String(buf),
                          Temp_TempList(ret_temp, NULL),
                          Temp_TempList(munchExp(lexp), NULL),
                          NULL);
          emit(instr);
          break;

          // 算术右移
        case T_arshift:
          sprintf(buf, "sra `d0, `s0, %d\n", rexp->u.CONST);
          instr = AS_Oper(String(buf),
                          Temp_TempList(ret_temp, NULL),
                          Temp_TempList(munchExp(lexp), NULL),
                          NULL);
          emit(instr);
          break;

        case T_xor:
          sprintf(buf, "XOR `d0, `s0, `s1`\n");
          instr = AS_Oper(String(buf),
                          Temp_TempList(ret_temp, NULL),
                          Temp_TempList(munchExp(lexp), Temp_TempList(munchExp(rexp), NULL)),
                          NULL);
          emit(instr);
          break;
      }
      return ret_temp;

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

      // 常量 直接把值传给 exp 给地址赋值为常量， 并返回这个地址
    case T_CONST:
      new_temp = Temp_newtemp();
      sprintf(buf, "li `d0, %d \n", exp->u.CONST);
      AS_instr const_instr = AS_Oper(String(buf),
                                     Temp_TempList(new_temp, NULL),
                                     NULL,
                                     NULL);
      emit(const_instr);
      return new_temp;

      // addi r, 0, label
    case T_NAME:
      new_temp = Temp_newtemp();
      Temp_label lab = exp->u.NAME;
      sprintf(buf, "addi `d0, `s0, %s", Temp_labelstring(lab));
      AS_instr instr = AS_Oper(String(buf),
                               Temp_TempList(ret_temp, NULL),
                               Temp_TempList(F_ZERO(), NULL),
                               NULL);
      emit(instr);
      return new_temp;
      break;

      // ***函数调用
    case T_CALL: {
      int total = 0; // 参数个数
      int i;
      AS_instr instr;
      T_exp funexp = exp->u.CALL.fun;
      Temp_tempList arg = munchArgs(&total, exp->u.CALL.args); // loop variable
      Temp_tempList args = NULL; // 寄存器参数

      // 1. 栈指针下移4个单位，这4个位置空出来，对应存在寄存器中的前4个参数，
      //    其他的进栈帧  F_AN(0) F_AN(1) F_AN(2) F_AN(3)
      // addi sp, sp - n*4, 栈地址 - 4* F_wordsize
      sprintf(buf, "addi `d0, `s0, %d", -(total < 4 ? 4 : total) * F_wordSize);
      instr = AS_Oper(String(buf),
                      Temp_TempList(F_SP(), NULL),
                      Temp_TempList(F_SP(), NULL),
                      NULL);
      emit(instr);

      // 2. 参数存入寄存器或栈帧
      i = 0;
      for (Temp_tempList param = F_getRegList(argregs); arg; arg = arg->tail, ++i) {

        // 前4个参数存入寄存器 F_AN(0) F_AN(1) F_AN(2) F_AN(3):
        // 0 地址F_ZERO + 当前参数地址 arg->head -> 寄存器地址 param->head
        if (param) {
          sprintf(buf, "add `d0, `s0, `s1");
          instr = AS_Move(String(buf),
                          Temp_TempList(param->head, NULL),
                          Temp_TempList(arg->head, Temp_TempList(F_ZERO(), NULL)));
          emit(instr);
          // 更新寄存器中参数
          args = Temp_TempList(param->head, args);
          param = param->tail;
        }
          // 其他参数进入栈帧, 将 s0 寄存器中的数据(参数arg)存储到 s1 地址(栈指针 F_SP )加 %d (i * F_wordSize) 的位置。
        else {
          sprintf(buf, "sw `s0, %d(`s1)", i * F_wordSize);
          instr = AS_Oper(String(buf),
                          NULL,
                          Temp_TempList(arg->head, Temp_TempList(F_SP(), NULL)),
                          NULL);
          emit(instr);
        }
      }

      // 3. 执行调用（将函数地址传递给对应的执行寄存器）
      // 3.1 jal j0: 将当前地址存入寄存器（ra），再跳转到目标地址（即原流程的地址继续往下执行 j0）
      if (funexp->kind == T_NAME) {
        // 原表达式地址，执行完函数调用后要返回的
        Temp_label label = funexp->u.NAME;
        sprintf(buf, "jal `r0");
        instr = AS_Oper(String(buf),
                        callDefs(), // 处理返回值：执行函数的返回值：F_RV()：返回值寄存器：F_RA()：返回地址寄存器，F_TEMPS()：临时变量寄存器
                        args,     // 处理参数：寄存器中参数
                        AS_Targets(Temp_LabelList(label, NULL)));// 处理调用：做完操作后跳到函数的 label 处调用这个函数
        emit(instr);
      }
        // 3.2 如果 funexp->kind 不是T_NAME 则无法跳转到这个函数处进行执行
      else {
        assert(0);
        printf("\nOooopps, try to call a function with no addr!\n");
      }

      // 4. 执行完毕，栈指针归位 SP + 4 * F_wordSize
      sprintf(buf, "addi `d0, `s0, %d", (total < 4 ? 4 : total) * F_wordSize);
      instr = AS_Oper(String(buf),
                      Temp_TempList(F_SP(), NULL),
                      Temp_TempList(F_SP(), NULL),
                      NULL);
      emit(instr);

      // 5. 返回函数的返回值（RV 寄存器中）
      return F_RV();
      break;
    }

      // 基本块中 去掉了 ESEQ, 所以没有 case T_ESEQ
    default:
      assert(0); // will never get to here.
  }
}

// 函数参数转换
static Temp_tempList munchArgs(int *n, T_expList args) {
  if (!args) {
    return NULL;
  }
  ++*n;
  return Temp_TempList(munchExp(args->head), munchArgs(n, args->tail));
}

// 全局调用到的函数返回值 RV，返回地址 RA，临时变量 TEMPS
static Temp_tempList callDefs() {
  static Temp_tempList defs = NULL;
  if (!defs)
    defs = Temp_TempList(F_RV(), Temp_TempList(F_RA(), F_TEMP()));
  return defs;
}

// 汇编指令生成
AS_instrList F_codegen(F_frame frame, T_stmList stmlist) {
  printf(">>> Start code gen ...\n");
  AS_instrList il;
  T_stmList sl;
  // 全局栈帧
  fname = F_name(frame);
  for (sl = stmlist; sl; sl = sl->tail)
    munchStm(sl->head);
  il = iList; // 全局变量 static AS_instrList iList
  iList = iList_last = NULL; // 全局汇编指令表 和 最新指令清空
  return F_procEntryExit2(il);
}

#endif
