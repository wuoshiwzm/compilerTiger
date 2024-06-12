#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "table.h"
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "absyn.h"
#include "translate.h"
#include "myframe.h"
#include "printtree.h"


#define debug(...) U_debug ( "[translate] ", __VA_ARGS__ )

struct Tr_level_{
    // 层次值
    int depth;
    Tr_level parent;
    F_frame frame;
};

struct Tr_access_{
    Tr_level level;
    F_access access;
};

struct Cx {patchList trues; patchList falses; T_stm stm};

/*
 * Level
 */
// 全局只有一个 outermost 生成一个最外层的 level,最外层不包含栈帧和形参数据
static Tr_level outermost = NULL;
Tr_level Tr_outermost(){
    if(outermost == NULL){
        outermost = (Tr_level)checked_malloc(sizeof (struct Tr_level_));
        outermost->depth = 0;
//        outermost->frame = NULL;
        outermost->parent = NULL;
        outermost->frame = F_newFrame(Temp_namedlabel("main"), NULL);
    }
    return outermost;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals){
    Tr_printLevel(parent);
    F_frame frame = F_newFrame(name,  U_BoolList(TRUE, formals));// 在 formals 基础上添加一个 为 TRUE 的做为 静态态
    Tr_level level = checked_malloc(sizeof (struct Tr_level_));
    level->parent=parent;
    level->frame=frame;
    level->depth = parent->depth+1;
    return level;
}

Tr_level Tr_getParent(Tr_level level){
    if(level == outermost){
        return outermost;
    }else{
        return level->parent;
    }
}


/*
 * Access
 */
// 带 level 的 formals, 获取这一级的 formal 的所有访问 Tr_accessList
Tr_accessList Tr_formals(Tr_level level){
    F_accessList f_accessList = F_formals(level->frame);
    Tr_accessList tr_list = NULL;
    Tr_accessList tr_list_head = NULL; // 第一个 tr_list


    for (; f_accessList != NULL; f_accessList = f_accessList->tail) {
        if (tr_list_head == NULL){
            tr_list = (Tr_accessList) checked_malloc(sizeof (struct Tr_accessList_));
            tr_list_head = tr_list;
        }else{
             tr_list->tail = (Tr_accessList) checked_malloc(sizeof (struct Tr_accessList_));
             tr_list = tr_list->tail;
        }

        tr_list->head = (Tr_access) checked_malloc(sizeof (struct Tr_access_));
        // 对应这一级的所有形参， 所以Tr_accessList 所有的 Tr_access 节点的 level 都是一样的
        tr_list->head->level = level;
        tr_list->head->access = f_accessList->head;
    }

    if (tr_list != NULL){
        tr_list->tail = NULL;
    }
    return tr_list_head;
}

// escape 默认为 TRUE, 进栈帧
Tr_access Tr_allocLocal(Tr_level level){
    Tr_access tr_acc = checked_malloc(sizeof (struct Tr_access_));
    tr_acc->level = level;
    // 获取 level 的 frame 的 allocLocal， F_access
    tr_acc->access = F_allocLocal(level->frame);
    return tr_acc;
}


/*
 * 中间代码 IR tree
 */
static Tr_exp Tr_Ex(T_exp exp){
  Tr_exp e = (Tr_exp)checked_malloc(sizeof (struct Tr_exp));
  e->kind = Tr_ex;
  e->u.ex = exp;
  return e;
}

static Tr_exp Tr_Nx(T_stm nx){
  Tr_exp e = (Tr_exp)checked_malloc(sizeof (struct Tr_exp));
  e->kind = Tr_nx;
  e->u.nx = nx;
  return e;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm){
  Tr_exp e = (Tr_exp)checked_malloc(sizeof (struct Tr_exp));
  trNx->kind = Tr_cx;

  e->u.cx.trues = trues;
  e->u.cx.falses = falses;
  e->u.cx.stm = stm;

  return e;
}

// T_Eseq(T_stm stm, T_exp exp) 执行 stm, 返回 exp
// T_exp T_Temp(Temp_temp temp) 生成一个临时变量
static T_exp unEx(Tr_exp e) {
  switch (e->kind) {
    case Tr_ex: // 有返回值表达式
      return e->u.ex;

    case Tr_nx:
      // 1. 先计算无返回值表达式 e->u.nx
      // 2. 返回值固定设为 T_Const(0))
      return T_Eseq(e->u.nx, T_Const(0));

    case Tr_cx:
      Temp_temp r = Temp_newtemp();
      T_exp er = T_Temp(r) // 临时变量，用来存返回值

      // t:真值回填标号 f:假值回填标号
      Temp_label t = Temp_newlabel(), f = Temp_newlabel();
      // 将真/假值回填表中的标号全部换成 t, f
      doPatch(e->u.cx.trues, t);
      doPatch(e->u.cx.falses, f);

      return T_Eseq(T_Move(er, T_Const(1)), // er 先默认赋值 1
                    T_Eseq(e->u.cx.stm, // 执行条件判断语句，如果为true 就会跳转到 t, 否则跳转 f
                           T_Eseq(T_Label(f), // 跳转到 f
                                  T_Eseq(T_Move(er, T_Const(0)), // er 赋值为 0
                                         T_Eseq(T_Label(t),  // 跳转到 t
                                                er // 最后返回 er (为1或0)
                                         )))));
  }
  assert(0);
}

// T_stm T_Seq(T_stm left, T_stm right);
static T_stm unNx(Tr_exp exp) {
  switch (exp->kind) {
    case Tr_ex:
      return T_Exp(exp->u.ex);

    case Tr_nx:
      return exp->u.nx;

    case Tr_cx:
      Temp_temp r = Temp_newtemp();
      T_exp er = T_Temp(r) // 返回值

      // t:真值回填标号 f:假值回填标号
      Temp_label t = Temp_newlabel(), f = Temp_newlabel();
      // 将真/假值回填表中的标号全部换成 t, f
      doPatch(e->u.cx.trues, t);
      doPatch(e->u.cx.falses, f);

      return T_Seq(exp->u.cx.stm, T_Seq(T_Label(t), T_Label(f)));
  }
}

// 条件结构体 struct Cx {patchList trues; patchList falses; T_stm stm}
// T_stm T_Cjump(T_relOp op, T_exp left, T_exp right, Temp_label true, Temp_label false) 为真时跳转 true，否则 false
static struct Cx unCx(Tr_exp exp) {
  struct Cx cx = (Cx) checked_malloc(sizeof(struct Cx));
  Temp_label t = Temp_newlabel(), f = Temp_newlabel();

  struct Cx cx;
  switch (exp->kind) {
    case Tr_ex:
      // cx.stm = T_Exp(exp.u.ex);
      cx.stm = T_Cjump(T_ne, exp->u.ex, T_Const(0), NULL, NULL);
      cx.trues = PatchList(&cx.stm->u.CJUMP.true, NULL); // 要跳转到的 true 分支
      cx.falses = PatchList(cx.stm->u.CJUMP.false, NULL); // 要跳转到的 false 分支
      return cx;

    case Tr_nx: // 不会走到这一步 ？
      cx.stm = exp->u.nx;
      cx.trues = NULL;
      cx.falses = NULL;
      return cx;

    case Tr_cx:
      return exp->u.cx;
  }
}


// ************* exp（AST） 的中间代码转换 *************

// ************* 1. 值的 IR 生成 *************

// NULL 对应 0 地址
static Tr_exp Tr_NIL = NULL;

Tr_exp Tr_nilExp(){
  if (Tr_NIL == NULL){
    Tr_NIL = Tr_Ex(T_Mem(T_Const(0)));
  }
  return Tr_NIL;
}

// void 对应 0
static Tr_exp Tr_NOOP = NULL;

// void
Tr_exp Tr_voidExp(void){
  if (Tr_NOOP == NULL){
    Tr_NOOP = Tr_Ex(T_Const(0));
  }
  return Tr_NOOP;
}

// 字符串
Tr_exp Tr_stringExp(string str){
  // 生成符号
  Temp_label  tl = Temp_newlabel();
  // 更新当前片段为 这个字符串
  F_String(tl, str);
  return Tr_Ex(T_Name(tl));
}

// int值
Tr_exp Tr_intExp(int val){
  T_exp te = T_Const(val);
  Tr_exp tre = Tr_Ex(te);
  return tre;
}

// 数组
Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init){
  /* 数组创建的汇编代码
      T_Move   var <- 0
      T_Label  start
      T_CJump	 var < size, t, f
      T_Label(t)
      T_Move   offset <- base + var * wordSize
      T_Move   Mem(offset) <- init
      T_Move   var <- var + 1
      T_Jump	 start
      T_Label(f)
  */

  // todo...
  debug("Tr_arrayExp");
}



// ************* 2. record 类型   cnt: record 中属性的个数 *************

Tr_exp Tr_recordExp_new(int cnt){
  debug("record fields=%d", cnt);
  // 初始化 record, 相当于  malloc(cnt*F_wordSize)
  T_exp call = F_externalCall("malloc", T_ExpList(T_Const(cnt*F_wordSize), NULL));
  Temp_temp t = Temp_newtemp();
  // MOVE 语句
  T_stm mov = T_Seq(T_Move(T_Temp(t), call), NULL);
  T_exp ex = T_Eseq(mov, T_Temp(t));
  return Tr_Ex(ex);
}



// ************* 3. 运算的 IR 生成 *************

// 二元算术运算  +-*/
Tr_exp Tr_arithExp(A_oper oper, Tr_exp left, Tr_exp right){
  T_exp exp;
  switch (oper) {
    case A_plusOp:
      exp = T_Binop(T_plus, unEx(left), unEx(right));
      break;
    case A_minusOp:
      exp = T_Binop(T_minus, unEx(left), unEx(right));
      break;
    case A_timesOp:
      exp = T_Binop(T_mul, unEx(left), unEx(right));
      break;
    case A_divideOp:
      exp = T_Binop(T_div, unEx(left), unEx(right));
      break;
    default:
      // 不会走到这里
      printf("Illegal arithmetic operator");
      exit(2);
  }
  return Tr_Ex(exp);
}

// 逻辑算术运算  =, !=, >=, <=, ...
Tr_exp Tr_logicExp(A_oper oper, Tr_exp left, Tr_exp right, bool isStrCompare){
  T_stm stm;
  // 真/假值回填表
  patchList tlist;
  patchList rList;

  // 只做逻辑运算， 不跳转执行 Temp_label true, Temp_label false
  // T_stm T_Cjump(T_relOp op, T_exp left, T_exp right, Temp_label true, Temp_label false );

  // 字符串只有 =, != 两种判断
  if (isStrCompare){
    // 调用外部函数比较字符串
    T_exp callEq = F_externalCall("stringEqual", T_ExpList(unEx(left), T_ExpList(unEx(right), NULL)));
    switch (oper) {
      case A_eqOp:
        // 判断 callEq 结果是否为 1 (true)
        stm = T_Cjump(T_eq, callEq, T_Const(1), NULL, NULL);
        break;
      case A_neqOp:
        stm = T_Cjump(T_eq, callEq, T_Const(0), NULL, NULL);
        break;
      default:
        // 不会走到这里
        printf("Illegal logic operator");
        exit(2);
    }
  }else{
    switch (oper) {
      case A_eqOp:
        stm = T_Cjump(T_eq, unEx(left), unEx(right), NULL, NULL);
        break;
      case A_neqOp:
        stm = T_Cjump(T_ne, unEx(left), unEx(right), NULL, NULL);
        break;
      case A_ltOp:
        stm = T_Cjump(T_lt, unEx(left), unEx(right), NULL, NULL);
        break;
      case A_leOp:
        stm = T_Cjump(T_le, unEx(left), unEx(right), NULL, NULL);
        break;
      case A_gtOp:
        stm = T_Cjump(T_gt, unEx(left), unEx(right), NULL, NULL);
        break;
      case A_geOp:
        stm = T_Cjump(T_ge, unEx(left), unEx(right), NULL, NULL);
        break;
      default:
        // 不会走到这里
        printf("Illegal logic operator");
        exit(2);
    }
  }

  tlist = PatchList(&stm->u.CJUMP.true, NULL);
  flist = PatchList(&stm->u.CJUMP.false, NULL);

  // Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm)
  return Tr_Cx(tlist, flist, stm)
}

// ************* 4. 变量的 IR 生成  semant.c :: transVar *************

// 单变量
Tr_exp Tr_simpleVar(Tr_access acc, Tr_level level){
  debug("Tr_simpleVar");

  T_exp texp;
  F_access access = acc->access;
  // 判断当前 level 是否和 变量访问的level 一致，一致就直接用， 不一致就沿着***静态连*** 找
  if (level != acc->level){
    // 沿着静态链往上找
    int cnt = 0;
    texp = F_Exp(F_staticLink(), T_Temp(F_FP()));
    level = level->parent;

    while (level != acc->level){
      // level 没有找到，再往上找
      texp = F_Exp(F_staticLink(),texp);
      level = level->parent;
    }

    texp = F_Exp(acc->access, &(level->frame));
  }else{
    texp = F_Exp(acc->access, T_Temp(F_FP()));
  }

  return Tr_Ex(texp);
}

// A_fieldVar...

// A_subscriptVar...


// ************* 5. 语句 *************

// 赋值
Tr_exp Tr_assignExp(Tr_exp lval, Tr_exp rval){
  debug("Tr_assignExp");
  if (rval != NULL ){
    T_stm stm =  T_Move(unEx(lval), unEx(rval));
    return Tr_Nx(stm);
  }else{
    //  没有右值则直接返回左值
    return lval;
  }
}

// while
// break
// for




// oper 运算。。。

// if







// ************* 6. 声明 *************

// let

// 函数 function

// 变量 var

// 类型 type















// 片段...
F_fragList Tr_getResult(){
  // return F_getFra;
}


// 调试
void Tr_printLevel(Tr_level level){
  printf(" --- Level: depth = %d; name = %s\n", level->depth, S_name(F_name(level->frame)));
}
void Tr_printAccess(Tr_access access){
//    printf( "--- Access:" );
  Tr_printLevel(access->level);
//    printf( "--- Access offset: %d \n", access->access->u.offset);
}
void printLevel(Tr_level level){
  printf("Level: depth = %d; name = %s \\n", level->depth, S_name(F_name(level->frame)));
}
void Tr_printTree(Tr_exp exp){
  // unNx(exp) 将 exp 转为没有返回值的语句 stm
  printStmList(stdout, T_StmList(unNx(exp), NULL));
}
void Tr_printTrExp(Tr_exp exp){}

//
void Tr_printResult(){
  F_fragList  frags = Tr_getResult();
}


