/*
 * 中间代码生成的
 */
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

/************************** 全局语句label栈结构 **************************/
/* Generic Stack Structure START */

/* Push a new node into the stack */
void GS_push(stack_node *plist, void *key) {
  int size = sizeof(struct stack_node_);
  stack_node list = *plist;
  if (list == NULL) {
    list = checked_malloc(size);
    list->key = key;
    list->next = NULL;
  } else {
    stack_node head = checked_malloc(size);
    head->key = key;
    head->next = list;
    list = head;
  }
  *plist = list;
}

/* Pop a node from the stack */
void GS_pop(stack_node *plist) {
  stack_node list = *plist;
  if (list != NULL) {
    stack_node head = list;
    list = head->next;
    checked_free(head);
  }
  *plist = list;
}

void *GS_peek(stack_node *plist) {
  return (*plist)->key;
}

/* Empty the stack */
void GS_empty(stack_node *plist) {
  while (*plist != NULL) {
    GS_pop(plist);
  }
}

/* Check if a node matching the given key is existent.
 * The 3rd arg is a function comparing the key of node and the given one (2nd arg).
 * It should return TRUE if the two are deemed to be equal.
 */
bool GS_check(stack_node list, void *key, bool (*compare)(void *, void *)) {
  stack_node cursor;
  for (cursor = list; cursor != NULL; cursor = cursor->next) {
    if (compare(cursor->key, key)) {
      return TRUE;
    }
  }
  return FALSE;
}

/* Generic Stack Structure END */

static stack_node loop_label_list = NULL;

static void LL_push(Temp_label label) {
  GS_push(&loop_label_list, label);
}

static void LL_pop() {
  GS_pop(&loop_label_list);
}

static Temp_label LL_peek() {
  return GS_peek(&loop_label_list);
}


/*
  Tr_level: 层级数， parent
  Tr_access: F_access + level(F_access，层级信息)
  F_access: 栈帧偏移量
 */
struct Tr_level_ {
  // 层次值
  int depth;
  Tr_level parent;
  F_frame frame;
};

struct Tr_access_ {
  Tr_level level;
  F_access access;
};

/*
 * Level
 */
// 全局只有一个 outermost 生成一个最外层的 level,最外层不包含栈帧和形参数据
static Tr_level outermost = NULL;

Tr_level Tr_outermost() {
  if (outermost == NULL) {
    outermost = (Tr_level) checked_malloc(sizeof(struct Tr_level_));
    outermost->depth = 0;
//        outermost->frame = NULL;
    outermost->parent = NULL;
    outermost->frame = F_newFrame(Temp_namedlabel("main"), NULL);
  }
  return outermost;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals) {
  Tr_printLevel(parent);
  F_frame frame = F_newFrame(name, U_BoolList(TRUE, formals));// 在 formals 基础上添加一个 为 TRUE 的做为 静态态
  Tr_level level = checked_malloc(sizeof(struct Tr_level_));
  level->parent = parent;
  level->frame = frame;
  level->depth = parent->depth + 1;
  return level;
}

Tr_level Tr_getParent(Tr_level level) {
  if (level == outermost) {
    return outermost;
  } else {
    return level->parent;
  }
}


/*
 * Access
 */
// 带 level 的 formals, 获取这一级的 formal 的所有访问 Tr_accessList
Tr_accessList Tr_formals(Tr_level level) {
  F_accessList f_accessList = F_formals(level->frame);
  Tr_accessList tr_list = NULL;
  Tr_accessList tr_list_head = NULL; // 第一个 tr_list


  for (; f_accessList != NULL; f_accessList = f_accessList->tail) {
    if (tr_list_head == NULL) {
      tr_list = (Tr_accessList) checked_malloc(sizeof(struct Tr_accessList_));
      tr_list_head = tr_list;
    } else {
      tr_list->tail = (Tr_accessList) checked_malloc(sizeof(struct Tr_accessList_));
      tr_list = tr_list->tail;
    }

    tr_list->head = (Tr_access) checked_malloc(sizeof(struct Tr_access_));
    // 对应这一级的所有形参， 所以Tr_accessList 所有的 Tr_access 节点的 level 都是一样的
    tr_list->head->level = level;
    tr_list->head->access = f_accessList->head;
  }

  if (tr_list != NULL) {
    tr_list->tail = NULL;
  }
  return tr_list_head;
}

// escape 默认为 TRUE, 进栈帧
Tr_access Tr_allocLocal(Tr_level level) {
  Tr_access tr_acc = checked_malloc(sizeof(struct Tr_access_));
  tr_acc->level = level;
  // 获取 level 的 frame 的 allocLocal， F_access
  tr_acc->access = F_allocLocal(level->frame);
  return tr_acc;
}


/*
 * 中间代码 IR tree
 */

static Tr_exp Tr_Ex(T_exp exp) {
  Tr_exp e = checked_malloc(sizeof(struct Tr_exp_));
  e->kind = Tr_ex;
  e->u.ex = exp;
  return e;
}

static Tr_exp Tr_Nx(T_stm nx) {
  Tr_exp e = checked_malloc(sizeof(struct Tr_exp_));
  e->kind = Tr_nx;
  e->u.nx = nx;
  return e;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
  Tr_exp e = (Tr_exp) checked_malloc(sizeof(struct Tr_exp_));
  e->kind = Tr_cx;
  e->u.cx.trues = trues;
  e->u.cx.falses = falses;
  e->u.cx.stm = stm;
  return e;
}

// T_Eseq(T_stm stm, T_exp exp) 执行 stm, 返回 exp
// T_exp T_Temp(Temp_temp temp) 生成一个临时变量
static T_exp unEx(Tr_exp e) {

  T_exp er;

  switch (e->kind) {
    case Tr_ex: // 有返回值表达式
      return e->u.ex;

    case Tr_nx:
      // 1. 先计算无返回值表达式 e->u.nx
      // 2. 返回值固定设为 T_Const(0))
      return T_Eseq(e->u.nx, T_Const(0));

    case Tr_cx:
      er = T_Temp(Temp_newtemp()); // 临时变量，用来存返回值  T_exp T_Temp(Temp_temp temp) Temp_temp Temp_newtemp(void) {
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
  Temp_label t, f;
  T_stm stm;

  switch (exp->kind) {
    case Tr_ex:
      stm = T_Exp(exp->u.ex);

    case Tr_nx:
      stm = exp->u.nx;

    case Tr_cx:
      // t:真值回填标号 f:假值回填标号
      t = Temp_newlabel();
      f = Temp_newlabel();
      // 将真/假值回填表中的标号全部换成 t, f
      doPatch(exp->u.cx.trues, t);
      doPatch(exp->u.cx.falses, f);
      stm = T_Seq(exp->u.cx.stm, T_Seq(T_Label(t), T_Label(f)));
  }

  return stm;
}

// 条件结构体 struct Cx {patchList trues; patchList falses; T_stm stm}
// T_stm T_Cjump(T_relOp op, T_exp left, T_exp right, Temp_label true, Temp_label false) 为真时跳转 true，否则 false
static struct Cx unCx(Tr_exp exp) {
//  struct Cx cx = checked_malloc(sizeof(struct Cx));
//  Temp_label t = Temp_newlabel(), f = Temp_newlabel();

  struct Cx cx;
  switch (exp->kind) {
    case Tr_ex:
      // cx.stm = T_Exp(exp.u.ex);
      cx.stm = T_Cjump(T_ne, exp->u.ex, T_Const(0), NULL, NULL);
      cx.trues = PatchList(&cx.stm->u.CJUMP.true, NULL); // 要跳转到的 true 分支
      cx.falses = PatchList(&cx.stm->u.CJUMP.false, NULL); // 要跳转到的 false 分支
      return cx;

    case Tr_cx:
      return exp->u.cx;

    case Tr_nx: // 不会走到这一步 ？
      cx.stm = exp->u.nx;
      cx.trues = NULL;
      cx.falses = NULL;
      return cx;
  }
}


/************************** 中间代码转换 **************************/

/************* 1. 值的 IR 生成 *************/

// NULL 对应 0 地址
static Tr_exp Tr_NIL = NULL;

Tr_exp Tr_nilExp() {
  if (Tr_NIL == NULL) {
    Tr_NIL = Tr_Ex(T_Mem(T_Const(0)));
  }
  return Tr_NIL;
}

// void 对应 0
static Tr_exp Tr_NOOP = NULL;

Tr_exp Tr_voidExp() {
  if (Tr_NOOP == NULL) {
    Tr_NOOP = Tr_Ex(T_Const(0));
  }
  return Tr_NOOP;
}

Tr_exp Tr_stringExp(string str) {
  // 生成符号
  Temp_label tl = Temp_newlabel();
  // 更新当前片段为 这个字符串
  F_String(tl, str);
  return Tr_Ex(T_Name(tl));
}

Tr_exp Tr_intExp(int val) {
  T_exp te = T_Const(val);
  Tr_exp tre = Tr_Ex(te);
  return tre;
}

// 数组 type-id[n] of v => n: 元素个数, v: 初始值, type-id: 数组类型, 索引为 0 ~ n-1
// struct {S_symbol typ; A_exp size, init;} array;
Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init) {
  // 使用外部函数 initArray, 返回存储地址
  T_exp ex = F_externalCall("initArray", T_ExpList(unEx(size), T_ExpList(unEx(init), NULL)));
  return Tr_Ex(ex);
}

/************* 2. record 类型   cnt: record 中属性的个数 *************/
// 初始化 record, 相当于  malloc(cnt*F_wordSize)
Tr_exp Tr_recordExp_new(int cnt) {
  debug("record fields=%d", cnt);
  T_exp call = F_externalCall("malloc", T_ExpList(T_Const(cnt * F_wordSize), NULL));
  Temp_temp t = Temp_newtemp();
  // MOVE 语句
  T_stm mov = T_Seq(T_Move(T_Temp(t), call), NULL);
  T_exp ex = T_Eseq(mov, T_Temp(t));
  return Tr_Ex(ex);
}



/************* 3. 运算的 IR 生成 *************/
// 二元算术运算  +-*/
Tr_exp Tr_arithExp(A_oper oper, Tr_exp left, Tr_exp right) {
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
Tr_exp Tr_logicExp(A_oper oper, Tr_exp left, Tr_exp right, bool isStrCompare) {
  T_stm stm;
  // 真/假值回填表
  patchList tlist;
  patchList flist;

  // 只做逻辑运算， 不跳转执行 Temp_label true, Temp_label false
  // T_stm T_Cjump(T_relOp op, T_exp left, T_exp right, Temp_label true, Temp_label false );

  // 字符串只有 =, != 两种判断
  if (isStrCompare) {
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
  } else {
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
  return Tr_Cx(tlist, flist, stm);
}



/************* 4. 变量的 IR 生成  semant.c :: transVar *************/

// 单变量
Tr_exp Tr_simpleVar(Tr_access acc, Tr_level level) {
  debug("Tr_simpleVar");

  T_exp texp;
  F_access access = acc->access;
  // 判断当前 level 是否和 变量访问的level (acc->level) 一致，一致就直接用， 不一致就沿着***静态连*** 找
  if (level != acc->level) {
    // 沿着静态链往上找对应的 level
    texp = F_Exp(F_staticLink(), T_Temp(F_FP()));
    level = level->parent;

    // 静态链是一个指向其**外部函数**栈帧的指针链 静态链就是栈帧的链
    // F_staticLink(): 栈帧的基地址， texp: 栈帧的漂移地址
    while (level != acc->level) {
      texp = F_Exp(F_staticLink(), texp);
      // level 没有找到，再往上找父level
      level = level->parent;
    }

    texp = F_Exp(acc->access, texp);
  } else {
    texp = F_Exp(acc->access, T_Temp(F_FP()));
  }
  return Tr_Ex(texp);
}


// 域变量: a.b, 通过 base 地址计算域变量的地址
Tr_exp Tr_fieldVar(Tr_exp base, int field_offset) {
  return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(base), T_Const(field_offset * F_wordSize))));
}

// 数组下标: a[10],a[ind]
Tr_exp Tr_arrayVar(Tr_exp base, Tr_exp offset_exp) {
  return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(base), T_Binop(T_mul, unEx(offset_exp), T_Const(F_wordSize)))));
}



/************** 5. 语句 与 表达式 *************/

// 赋值
Tr_exp Tr_assignExp(Tr_exp lval, Tr_exp rval) {
  debug("Tr_assignExp");
  if (rval != NULL) {
    T_stm stm = T_Move(unEx(lval), unEx(rval));
    return Tr_Nx(stm);
  } else {
    //  没有右值则直接返回左值
    return lval;
  }
}

// while
Tr_exp Tr_whileExp(Tr_exp cond, Tr_exp body) {
  debug("Tr_whileExp");

  struct Cx cx_cond = unCx(cond); // cx_cond.stm 为一个 T_Cjump
  Temp_label start = Temp_newlabel();
  Temp_label t = Temp_newlabel(); //　继续循环
  Temp_label done = LL_peek();//Get done label from the list, which should have been prepared before this function is called.

  // 将 true, false的标记 label  写入到 patchList 中
  doPatch(cx_cond.trues, t);
  doPatch(cx_cond.falses, done);

  T_stm whilestm = T_Seq(
      // 条件判断 循环体/结束
      T_Seq(
          T_Label(start),
          cx_cond.stm // 这里的 stm 就是一个 T_Cjump
      ),
      // 循环体部分
      T_Seq(
          T_Label(t),
          T_Seq(
              unNx(body),
              T_Seq(
                  T_Jump(T_Name(start), Temp_LabelList(start, NULL)),
                  T_Label(done)
              )
          )
      )
  );

  //Pop the label as it's no longer used
  LL_pop();

  //在 While 循环结束时，将这个标号 Pop 出来。
  return Tr_Nx(whilestm);
}

// for
Tr_exp Tr_forExp(Tr_exp var, Tr_exp low, Tr_exp high, Tr_exp body) {
  debug("Tr_forExp");

  T_exp val = unEx(var);
  T_exp lval = unEx(low);
  T_exp hval = unEx(high);

  Temp_label start = Temp_newlabel();
  Temp_label t = Temp_newlabel(); //　继续循环
  Temp_label done = LL_peek();//Get done label from the list, which should have been prepared before this function is called.
  T_stm cond = T_Cjump(T_le, val, hval, t, done);

  T_stm forstm = T_Seq(T_Move(val, lval), // val 初始化
                       T_Seq(T_Label(start),
                             T_Seq(cond,
                                   T_Seq(T_Label(t),
                                         T_Seq(unNx(body),
                                               T_Seq(T_Move(val, T_Binop(T_plus, val, T_Const(1))), // val++
                                                     T_Seq(T_Jump(T_Name(start), Temp_LabelList(start, NULL)),
                                                           T_Label(done))))))));
  // 弹出已经不再作用的label: done
  LL_pop();
  return Tr_Nx(forstm);
}

// if
Tr_exp Tr_ifExp(Tr_exp cond, Tr_exp thenexp, Tr_exp elseexp) {

  Temp_label thenlabel = Temp_newlabel(); //　继续循环
  Temp_label elselabel = Temp_newlabel();
  Temp_label done = Temp_newlabel();
  T_stm stm; // 无返回值的 if
  T_exp exp; // 有反回值的 if

  // cond 已经包含 T_Cjump 信息，不需要再构造 T_Cjump
  struct Cx cx_cond = unCx(cond);
  // 配置 trues, false 对应label， 这样cx_cond.stm 就会跳转到对应的 label
  doPatch(cx_cond.trues, thenlabel);
  doPatch(cx_cond.falses, elselabel);

  // eseq ESEQ(s, e) The statement s is evaluated for side effects, then e is evaluated for a result.
  if (elseexp == NULL) { /* if ... then ... */
    stm = T_Seq(cx_cond.stm,
                T_Seq(T_Label(thenlabel),
                      T_Seq(unNx(thenexp),
                            T_Label(done))));
    return Tr_Nx(stm);
  } else if (thenexp->kind == Tr_nx && elseexp->kind == Tr_nx) {  /* if ... then ... else ... 且两个分支都是 stm , 没有返回值 */
    stm = T_Seq(cx_cond.stm,
                T_Seq(T_Label(thenlabel),
                      T_Seq(unNx(thenexp), // then 执行完 跳转到结束
                            T_Seq(T_Jump(T_Name(done), Temp_LabelList(done, NULL)),
                                  T_Seq(T_Label(elselabel),
                                        T_Seq(unNx(elseexp),
                                              T_Label(done)
                                        )
                                  )
                            )
                      )
                )
    );
    return Tr_Nx(stm);
  } else { /* 有返回值,类似于三元运算符: v= if... then val_a else val_b  */
    Temp_temp r = Temp_newtemp();
    exp = T_Eseq(cx_cond.stm,
                 T_Eseq(T_Label(thenlabel),
                        T_Eseq(T_Move(T_Temp(r), unEx(thenexp)), // 返回 then语句的值

                               T_Eseq(T_Jump(T_Name(done), Temp_LabelList(done, NULL)),
                                      T_Eseq(T_Label(elselabel),
                                             T_Eseq(T_Move(T_Temp(r), unEx(elseexp)),
                                                    T_Eseq(T_Label(done), T_Temp(r))// 返回最终的 r
                                             )
                                      )
                               )
                        )
                 )
    );
    return Tr_Ex(exp);
  }
}

// 表达式列表 A_seqExp
Tr_exp Tr_seqExp(Tr_exp* exparr, int size){
  debug("Tr_seqExp");
  T_exp _texp = (T_exp) checked_malloc(sizeof (struct T_exp_));
  T_exp *p = &_texp, head;

  int i = 0;
  int last = size - 1;
  while (i<size){
    Tr_printTrExp(exparr[i]);
    if (i != last){
      // 执行数组前面的所有 exparr[i]
      *p = T_Eseq(unNx(exparr[i]), NULL);
    }else{
      // 返回数组最后面的 Tr_exp
      *p = unEx(exparr[i]);
    }

    if (i == 0) head = *p;

    // 这里利用了 ESEQ 的结构{stm, exp} 组成一个链表来存储所有的 exparr 中的exp， 并返回最一个表达式 T_exp 的值
    // 上面将 (*p)->u.ESEQ.stm 设为unNx(exparr[i])， 这里将p 更新为下 T_Eseq 的下一个 T_exp
    if (i != last) p = &((*p)->u.ESEQ.exp);
    i++;
  }

  return Tr_Ex(head);
}



// oper 运算。。。



// break  跳到 done lable 处
Tr_exp Tr_breakExp() {
  Temp_label l = LL_peek();
  // 跳转到栈顶的 label 处
  T_stm stm = T_Jump(
      T_Name(l),
      Temp_LabelList(l, NULL)
      );
  return Tr_Nx(stm);
}





/************* 6. 声明  semant.c::transDec *************/
// transDec(Tr_level level, S_table v, S_table t, A_dec d)

// let

// 函数 function

// 变量 var

// 类型 type







// 片段...
F_fragList Tr_getResult() {
//  return F_getFra;
}


// 调试
void Tr_printLevel(Tr_level level) {
  printf(" --- Level: depth = %d; name = %s\n", level->depth, S_name(F_name(level->frame)));
}

void Tr_printAccess(Tr_access access) {
//    printf( "--- Access:" );
  Tr_printLevel(access->level);
//    printf( "--- Access offset: %d \n", access->access->u.offset);
}

void printLevel(Tr_level level) {
  printf("Level: depth = %d; name = %s \\n", level->depth, S_name(F_name(level->frame)));
}

void Tr_printTree(Tr_exp exp) {
  // unNx(exp) 将 exp 转为没有返回值的语句 stm
  printStmList(stdout, T_StmList(unNx(exp), NULL));
}

void Tr_printTrExp(Tr_exp exp) {}

//
void Tr_printResult() {
  F_fragList frags = Tr_getResult();
}

