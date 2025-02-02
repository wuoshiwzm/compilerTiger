#include <stdio.h>
#include "util.h"
#include "myframe.h"
#include "translate.h"
#include "errormsg.h"
#include "env.h"
#include "semant.h"

// 调试
#define debug(...) U_debug ( "[semant] ", __VA_ARGS__ )

// 构造函数
struct expty expTy(Tr_exp e, Ty_ty t) {
  struct expty et;
  et.exp = e;
  et.ty = t;
  return et;
}

// 核心函数，类型检查, 翻译代码，返回 中间代码的 片段列表
F_fragList SEM_transProg(A_exp exp) {
  struct expty et;
  S_table t = E_base_tenv(); /* 类型环境 */
  S_table v = E_base_venv(); /* 值环境 */

  Tr_level mainLevel = Tr_newLevel(Tr_outermost(), Temp_namedlabel("__main"), NULL);

  // 转换结果 { Tr_exp exp; Ty_ty ty; };
  struct expty result = transExp(mainLevel, v, t, exp);
  Tr_printTrExp(result.exp);

  // 添加 main() 到 全局片段表 中 Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals);
  Tr_procEntryExit(mainLevel, result.exp, Tr_formals(mainLevel));

  // 返回 F_fragList, 所有记住的片段都存放在 Translate 内的一个私有片段表中，然后使用 getResult 来获取
  return Tr_getResult();


  // 片段1 栈帧 + 函数体

  // 片段2 字符串文字

  // 打印中间代码 tree

  // 返回中间代码 片段

}

// 获取 ty 的原生类型
static Ty_ty actual_ty(Ty_ty ty) {
  if (!ty) return ty;
  /* ty->kind == Ty_name 说明 ty 的类型名是是重命名后的，如： a:int , b: string  等 */
  if (ty->kind == Ty_name) return actual_ty(ty->u.name.ty);
  else return ty;
}

static bool ty_match(Ty_ty a, Ty_ty b) {
  /* 获取实际类型 */
  Ty_ty t = actual_ty(a);
  Ty_ty e = actual_ty(b);
  int tk = t->kind;
  int ek = e->kind;

  /*
   t 和 e  有一个是nil (void), 不指定类型 则返回 true
   如果是 Ty_record 或 Ty_array，则要完全相同 : t == e
   如果不是 Ty_record 和 Ty_array 则只要 kind　相同: t->kind == e->kind
   */
  return (((tk == Ty_record || tk == Ty_array) && t == e) ||
          (tk == Ty_record && ek == Ty_nil) ||
          (ek == Ty_record && tk == Ty_nil) ||
          (tk != Ty_record && tk != Ty_array && tk == ek));
}

/**
 * @brief 判断实参是否符合 函数 fun 的定义
 *
 * @param v    值符号表
 * @param tt   类型符号表
 * @param ell  实参
 * @param fll  形参，域类型
 * @param fun  函数表达式
 * @return
 */
static bool args_match(Tr_level level, S_table v, S_table tt, A_expList ell, Ty_tyList fll, A_exp fun) {
  /* 表达式类型	*/
  struct expty et;
  A_expList el = ell; /* 传 */
  Ty_tyList fl = fll;

  /* 判断当前的第 i 个实参 与 第 i 个形参 类型是否匹配 */
  while (el && fl) {
    /* 转义函数表达式 第一个 */
    et = transExp(level, v, tt, el->head);

    /* 判断表达式实参 el->head 类型 与 定义的形参 fl->head 的类型是否一样,
     * 即 fun(x,y) 与 fun(int a, int b) 定义时的类型是否匹配 */
    if (!ty_match(et.ty, fl->head)) {
      EM_error(fun->pos, "the parameter type do not match the def.");
      return FALSE;
    }

    /* 继续下一个参数 */
    el = el->tail;
    fl = fl->tail;
  }

  /* 要么多传参数，要么少传参了 */
  if (el || fl) {
    EM_error(fun->pos, "The num of parameter is not the same as the def.");
    return FALSE;
  }

  return TRUE;
}

/**
 * 检查 record 的定义是否匹配
 * @param v     值符号表
 * @param tt    类型符号表
 * @param ty    efield 定义
 * @param e     要比较的表达式
 * @return
 */
static bool efields_match(Tr_level level, S_table v, S_table t, Ty_ty tyy, A_exp e) {
  struct expty et;
  Ty_fieldList ty = tyy->u.record; /* 定义的fields 类型 */
  A_efieldList fl = e->u.record.fields; /* 表达式的 fields*/

  /* 比较 field list 里面的属性的类型是否一致 */
  while (ty && fl) {
    et = transExp(level, v, t, fl->head->exp);
    if ((ty->head->name != fl->head->name) || !ty_match(ty->head->ty, et.ty)) {
      EM_error(e->pos, "unmatched name: type in %s", S_name(e->u.record.typ));
      return FALSE;
    }
    ty = ty->tail;
    fl = fl->tail;
  }

  /* ty 和 fl 有一个不是NULL, 说明个数不匹配 */

  if (ty && !fl) {
    EM_error(e->pos, "less fields in %s", S_name(e->u.record.typ));

  } else if (!ty && fl) {
    EM_error(e->pos, "too many field in %s", S_name(e->u.record.typ));

  }

  if (ty || fl) {
    EM_error(e->pos, "the records define un match, either one of them is none.");
    return FALSE;
  }
  return TRUE;
}

// 转义变量
static struct expty transVar(Tr_level level, S_table venv, S_table tenv, A_var v) {

  /* 声明 环境， 类型变量， 域链表变量 */
  E_enventry x; /* 环境 */
  E_enventry entry;
  struct expty et, et2;
  Ty_fieldList fl;

  // 中间代码 IR tree
  Tr_exp trexp;
  int offset;

  switch (v->kind) {
    // 简单变量
    case A_simpleVar:
      debug("transVar: A_simpleVar = %s", S_name(v->u.simple));
      entry = S_look(venv, v->u.simple);
      if (entry == NULL) printf(" entry is null: %s \n.", S_name(v->u.simple));
      if (entry && entry->kind == E_varEntry) {
        // 创建 Tr_exp
        trexp = Tr_simpleVar(entry->u.var.access, level);
        return expTy(trexp, actual_ty(entry->u.var.ty));
      } else {
        // 值环境中不存在，或者存在 ，但绑定的值不是简单变量，说明一个符号应该是简单变量，但对应的值不是简单变量，报语义错误
        EM_error(v->pos, " undefined var %s", S_name(v->u.simple));
        // 固定返回 int, 让编译器继续往下走。 return expTy(NULL, Ty_Int());
      }

      /*
       * var record (a.b) 域变量
       * 如 type intlist = {hd: int, tl: intlist}  intlist.hd
       * 其中 intlist.hd   intlist 对应 A_var var, hd 对应 S_symbol sym
       * */
    case A_fieldVar:
      debug("transVar: A_fieldVar = %s.%s", S_name(v->u.field.sym), S_name(v->u.field.var->u.simple));
      et = transVar(level, venv, tenv, v->u.field.var);

      // 1. 解析"."前的变量，应该是一个record
      Ty_ty actualTy = actual_ty(et.ty);
      if (actualTy->kind != Ty_record) {
        EM_error(v->pos, "'var.field' var is not a record.");
        // 让编译器继续运行 return expTy(NULL, Ty_Record(NULL));
      }

      // 2. 找到 record 中的对应的field
      offset = 0;
      /*
       * 遍历当前的 Ty_record 环境 的所有 field ,
       * 找到v的符号( hd )对应的那个field: {hd: int}
       * 他的类型就是对应的 intlist.hd 的类型*/
      for (fl = et.ty->u.record; fl; fl = fl->tail) {
        offset++;
        if (fl->head->name == v->u.field.sym) {
          return expTy(Tr_fieldVar(et.exp, offset), actual_ty(fl->head->ty));
        }
      }
      /* 没找到， 说明当前的自定义域中没有 v 对应的 field */
      EM_error(v->pos, "no field %s in record: %s", S_name(v->u.field.var->u.simple), S_name(v->u.field.sym));
      break;
      // 返回空 Ty_Record, 让编译继续类型检查 return expTy(NULL, Ty_Record(NULL));

      /*
       * 数组下标 arr[b]
       * 其中 arr 对应 A_var var,
       * b 对应 A_exp exp, 不一定是个数字 ，也可以是个表达式，如变量名，或计算表达式
       * */
    case A_subscriptVar:
      debug(" translate array var, shu zu xia biao");

      // 1. 转义'['前的变量，应该是一个数组变量
      et = transVar(level, venv, tenv, v->u.subscript.var);
      if (actual_ty(et.ty)->kind != Ty_array) {
        EM_error(v->pos, "not a array");
        break;
      }

      // 2. 转义数组'[]'下标中的部分, 结果应该是数字或值为数字的变量
      struct expty exp_et = transExp(level, venv, tenv, v->u.subscript.exp);
      if (actual_ty(exp_et.ty)->kind != Ty_int) {
        EM_error(v->pos, " the subscript of arr must be int !");
      }
      // 下标是整型，则返回数组的类型，就是 arr[b] 的类型
      return expTy(Tr_arrayVar(et.exp, exp_et.exp), actual_ty(et.ty->u.array));
    default:
      printf("will never goto here.");
  }
}

/**
 * 解析表达式类型
 * @param v 值环境 venv
 * @param t 类型环境 tenv
 * @param e 表达式
 * @return 返回结构体struct expty  {Tr_exp exp; Ty_ty ty;};
 */
static struct expty transExp(Tr_level level, S_table v, S_table t, A_exp e) {
  int i;
  struct expty expty;
  A_exp exp;

  Ty_ty recordty, arrayty;
  A_expList list;
  Tr_exp trexp;

  struct expty left, right, final, final2, final3, final4, final5, lo, hi;
  struct expty assVar, assExp;

  struct expty let_body;
  struct expty if_if, if_then, if_else;
  A_oper op;

  // record
  Ty_fieldList tyfList;
  Ty_field tyField;
  A_efieldList eFields;
  A_efield a_efield;

  // array
  struct expty arrsize;
  struct expty arrinit;

  // while
  struct expty whilecond;
  struct expty whilebody;

  // for
  struct expty for_from, for_to, for_body, for_var;

  // let
  A_decList dlist;

  // A_seqExp
  A_expList expList;
  Tr_exp *trexp_arr;

  // A_callExp
  Ty_tyList formals;
  A_expList args;
  Ty_ty ty;
  E_enventry callinfo; /* 函数调用: funx(a,b) */
  int arg_number;
  Tr_exp *arg_exps;

  switch (e->kind) {
    // 变量表达式
    case A_varExp:
      return transVar(level, v, t, e->u.var);

      // NIL 表达式
    case A_nilExp:
      return expTy(Tr_nilExp(), Ty_Nil());

      // 字符串
    case A_stringExp:
      trexp = Tr_stringExp(e->u.stringg);
      return expTy(trexp, Ty_String());

    case A_intExp:
      trexp = Tr_intExp(e->u.intt);
      return expTy(trexp, Ty_Int());

      /* 双目运算
       typedef enum {A_plusOp, A_minusOp, A_timesOp, A_divideOp,
       A_eqOp, A_neqOp, A_ltOp, A_leOp, A_gtOp, A_geOp} A_oper; */
    case A_opExp:
      op = e->u.op.oper;  // 符号
      left = transExp(level, v, t, e->u.op.left);
      right = transExp(level, v, t, e->u.op.right);
      A_exp rr = e->u.op.right;
      // 1. 加减乘除 +,-,*,/
      if (0 <= op && op <= 3) {
        debug("trans +-*/.");
        if ((left.ty->kind != Ty_int) || (right.ty->kind != Ty_int)) {
          EM_error(e->pos, "left & right num need to be int for oper: + - * / \n");
        }
        // 除法除0时报错
        if (op == 3 && rr->u.intt == 0) {
          EM_error(e->pos, "devide by zero.\n");
          break;
        }
        trexp = Tr_arithExp(op, left.exp, right.exp);
        return expTy(trexp, Ty_Int());
      } else if (3 < op && op < 10) {
        // 2. 比较等式 A_eqOp, A_neqOp, A_ltOp, A_leOp, A_gtOp, A_geOp
        // 2.1 比较 record类型 和 nil */
        if (op == 4 || op == 5) {
          if ((left.ty->kind == Ty_record || left.ty->kind == Ty_nil) &&
              (right.ty->kind == Ty_record || right.ty->kind == Ty_nil)) {
            trexp = Tr_logicExp(op, left.exp, right.exp, FALSE);
            return expTy(trexp, Ty_Int());
          }
        }
        // 2.2 比较其他类型
        if (left.ty->kind != Ty_int && left.ty->kind != Ty_string) {
          EM_error(e->u.op.left->pos, " left type should be int,or record-nil ! \n");
        }
        if (right.ty->kind != Ty_int && right.ty->kind != Ty_string) {
          EM_error(e->u.op.right->pos, " right type should be int,or record-nil ! \n");
        }
        // 判断是否比较字符串
        if (left.ty->kind == Ty_string) {
          trexp = Tr_logicExp(op, left.exp, right.exp, TRUE);
        } else {
          trexp = Tr_logicExp(op, left.exp, right.exp, FALSE);
        }
        return expTy(trexp, Ty_Int());
      } else {
        /* 未定义的运算符 */
        EM_pfun(e->pos, "  undefined oper: %d \n", op);
        assert(0);
      }

      // record 表达式 struct {S_symbol typ; A_efieldList fields;} record;
    case A_recordExp:
      debug("translate A_recordExp");
      // 在类型环境 tenv 中查找符号 e->u.record.typ 绑定的类型
      recordty = actual_ty(S_look(t, e->u.record.typ));

      // 找不到 record 绑定的类型
      if (!recordty) {
        EM_error(e->pos, "record : %s not defined\n", S_name(e->u.record.typ));
        break;
      }
      if (recordty->kind != Ty_record) {
        EM_error(e->pos, " %s type not a record. \n", S_name(e->u.record.typ));
      }
      // record 属性个数
      int cnt = 0;
      // 判断 record 中的 fields 的定义是否匹配
      if (efields_match(level, v, t, recordty, e)) {
        // 计算 field 的个数,生成中间代码
        for (tyfList = recordty->u.record; tyfList != NULL; tyfList = tyfList->tail) {
          if (tyfList->head != NULL) cnt++;
        }
        trexp = Tr_recordExp_new(cnt);
        return expTy(trexp, recordty);
      }
      // 失败后继续编译 return expTy(NULL, Ty_Record(NULL));

      /* 数组表达式 */
    case A_arrayExp:
      debug("A_arrayExp");
      arrayty = actual_ty(S_look(t, e->u.array.typ));
      if ((!arrayty) || (arrayty->kind != Ty_array)) {
        EM_error(e->pos, "the exp should be array but it is not or undefined : %s \n", S_name(e->u.array.typ));
      }

      // 判断数组大小，默认值类型 是否与数组定义一致
      arrsize = transExp(level, v, t, e->u.array.size);
      arrinit = transExp(level, v, t, e->u.array.init); /* 数组的默认值 如 int a[2]='test' */
      if (arrsize.ty->kind != Ty_int) {
        EM_error(e->pos, "array size should be int \n");
        break;
      }
      if (!ty_match(arrinit.ty, arrayty->u.array)) {
        EM_error(e->pos, "type is not array \n");
        break;
      }
      trexp = Tr_arrayExp(arrsize.exp, arrinit.exp);
      return expTy(trexp, arrayty);

      /* 赋值表达式  var := exp */
    case A_assignExp:
      assVar = transVar(level, v, t, e->u.assign.var);
      assExp = transExp(level, v, t, e->u.assign.exp);

      /* 判断赋值类型与定义类型是否一致 */
      if (!ty_match(assVar.ty, assExp.ty)) {
        EM_error(e->pos, "assign type is not same as defined. \n");
      }
      /* 赋值表达式没有返回值 */
      trexp = Tr_assignExp(assVar.exp, assExp.exp);
      return expTy(trexp, Ty_Void());

      /* while 表达式 */
    case A_whileExp:
      // 1. 检验与转义条件部分 while() 中止条件判断，应该是 int (0, 1)
      whilecond = transExp(level, v, t, e->u.whilee.test);
      if (whilecond.ty->kind != Ty_int) {
        EM_error(e->pos, "while() condition should be int 1/0 \n");
      }
      // 2. 转义循环体
      whilebody = transExp(level, v, t, e->u.whilee.body);
      trexp = Tr_whileExp(whilecond.exp, whilebody.exp);
      return expTy(trexp, Ty_Void());

      /* for表达式 */
    case A_forExp:
      /* struct {S_symbol var; A_exp lo,hi,body; bool escape;} forr; */
      for_from = transExp(level, v, t, e->u.forr.lo);
      for_to = transExp(level, v, t, e->u.forr.hi);
      if ((for_from.ty->kind != Ty_int) || (for_to.ty->kind != Ty_int)) {
        EM_error(e->pos, "the from , to is not number. \n");
      }

      /* 进入值环境 v */
      S_beginScope(v);
      /* 声明 int i 的初始化 */
      transDec(level, v, t, A_VarDec(e->pos, e->u.forr.var, S_Symbol("int"), e->u.forr.lo));
      /* 函数体 */
      for_body = transExp(level, v, t, e->u.forr.body);
      S_endScope(v);

      for_var = transVar(level, v, t, A_SimpleVar(e->pos, e->u.forr.var));
      trexp = Tr_forExp(for_var.exp, for_from.exp, for_to.exp, for_body.exp);
      return expTy(trexp, Ty_Void());

      /* let 声明表达式
       * let ...decs... in ...exp_body... end */
    case A_letExp:
      debug("A_letExp");

      // 1. scope 开始
      /* 声明只在域scope内有效 */
      S_beginScope(v);
      S_beginScope(t);

      // 2. 生成中间代码准备
      // 2.1 计算let in 之前的声明个数
      i = 0;
      for (dlist = e->u.let.decs; dlist; dlist = dlist->tail) {
        if (dlist->head != NULL) i++;
      }

      // 2.2 初始化 Tr_exp 数组，保存表达式
      //     包括：每个变量的声明 , in 和 end 之间的 seqExp ，一共 i+1 个
      trexp_arr = (Tr_exp *) checked_malloc((i + 1) * sizeof(Tr_exp));
      i = 0;

      // 3. 按顺序转义声明
      for (dlist = e->u.let.decs; dlist; dlist = dlist->tail) {
        if (dlist->head != NULL) {
          trexp_arr[i++] = transDec(level, v, t, dlist->head);
        }
      }

      // 4. 转义 let 的函数体 in 和 end 之前的部分
      let_body = transExp(level, v, t, e->u.let.body); // 返回 struct expty 格式

      // 5. 生成中间代码，转义 in ... end 中的表达式列表 seqExp
      trexp_arr[i] = let_body.exp;
      let_body.exp = Tr_seqExp(trexp_arr, i + 1);

      S_endScope(t);
      S_endScope(v);
      return let_body;


      /* 表达式列表：按每一个表达式转义 */
    case A_seqExp:
      debug("A_seqExp");
      // 1. 计算总的 exp 的个数
      i = 0;
      for (list = e->u.seq; list != NULL; list = list->tail) {
        if (list->head != NULL) i++;
      }

      // 2. 如果数组中没有表达式，则返回 void
      if (i == 0) {
        return expTy(Tr_voidExp(), Ty_Void());
      }

      // 3. 创建 Tr_exp 数组保存中间代码
      trexp_arr = (Tr_exp *) checked_malloc((i + 1) * sizeof(Tr_exp));

      // 4. 类型检查数组中的表达式
      for (expList = e->u.seq, i = 0; expList != NULL; expList = expList->tail) {
        exp = expList->head;
        if (exp != NULL) {
          // 4.1 转义表达式
          expty = transExp(level, v, t, exp);

          // 4.2 将中间代码 Tr_exp 写入数组
          trexp_arr[i++] = expty.exp;
        }
      }

      // 5. 将生成的表达式数组 使用 Tr_seqExp 转义,
      //    将最后一个表达式的类型 expty.ty 作为整个 seqExp 的返回值
      expty = expTy(Tr_seqExp(trexp_arr, i), expty.ty);
      return expty;

      /* if表达式 */
    case A_ifExp:
      // 1. 判断条件
      if_if = transExp(level, v, t, e->u.iff.test);
      if_then = transExp(level, v, t, e->u.iff.then);
      if (if_if.ty->kind != Ty_int) {
        EM_error(e->pos, "the exp in if() is not 1/0\n");
      }

      // 2. 判断函数体
      //    包含 else 部分，返回类型要和 then 部分一致
      if (e->u.iff.elsee) {
        if_else = transExp(level, v, t, e->u.iff.elsee);
        if (!ty_match(if_then.ty, if_else.ty)) {
          EM_error(e->pos, "return type of then and else should be the same.\n");
        }
      }
      trexp = Tr_ifExp(if_if.exp, if_then.exp, if_else.exp);
      return expTy(trexp, if_then.ty);

      /* 调用表达式 */
    case A_callExp:
      debug("A_callExp func: %s", S_name(e->u.call.func));

      // 1. 获取调用信息 在值环境 v 中查找函数 func 的参数，返回值
      callinfo = S_look(v, e->u.call.func);

      // 2. 表达式形式为 函数名funx(参数params)的形式，否则表示函数 funx 未定义
      if (callinfo && callinfo->kind == E_funEntry) {

        // 3. 参数数组(Tr_exp *)
        // 3.1 数组size
        i = 0;
        for (formals = callinfo->u.fun.formals; formals != NULL; formals = formals->tail) {
          if (formals->head != NULL) {
            i++;
          }
        }
        arg_exps = (i > 0) ? (Tr_exp *) checked_malloc(i * sizeof(Tr_exp)) : NULL;

        // 4. 检查参数 判断实参(e->u.call.args) 和 形参(callinfo->u.fun.formals)类型是否一致
        if (args_match(level, v, t, e->u.call.args, callinfo->u.fun.formals, e)) {

          // 5. 生成 IR 中间代码 | 参数类型正确，返回值类型为 定义的或 void, callinfo->u.fun.label: 函数名
          trexp = Tr_callExp(level, Tr_getParent(callinfo->u.fun.level), callinfo->u.fun.label, arg_exps, i);
          if (callinfo->u.fun.result) return expTy(trexp, actual_ty(callinfo->u.fun.result));
          else return expTy(trexp, Ty_Void());
        } else {
          // 6. 参数检查失败
          EM_error(e->pos, "args match failed.\n");
          // return expTy(NULL, Ty_Void());
        }
      } else {
        EM_error(e->pos, "func: %s undefined.\n", S_name(e->u.call.func));
      }
      break; // never get here.

      /* break表达式 */
    case A_breakExp:
      trexp = Tr_breakExp();
      return expTy(trexp, Ty_Void());

    default:
      /* 应该走不到这里，否则就是漏掉的类型 */
      assert(0);
  }
}

/* 转义声明表达式 */
Tr_exp transDec(Tr_level level, S_table v, S_table t, A_dec d) {

  Tr_exp trexp;

  struct expty e;
  Ty_ty dec_ty;
  Ty_ty tt;

  // 函数声明
  A_fundec fdec;
  A_fundecList fdecl;
  Ty_tyList dec_fun_params_ty;
  Ty_ty fun_ty;
  Ty_tyList formalTys, ts; /* 形参 */

  A_fieldList fl;
  struct expty funbody;
  E_enventry fentry; /* 函数入口 */

  // 类型声明
  A_nametyList nl;
  Ty_ty resTy, namety, recordTy;

  // IR
  Tr_exp lval;
  Tr_exp rval;

  switch (d->kind) {

    /* 声明变量 */
    case A_varDec:
      debug(" A_varDec ");

      // 获取要初始化的值类型 (有可能是 int, string, record 等 ... )
      e = transExp(level, v, t, d->u.var.init); /* 初始化 */

      // 1. 声明 var id : type-id := exp 指明变量类型
      if (d->u.var.typ != NULL) {
        printf("var dec with named type. \n");
        // 查找类型对应的 符号
        tt = S_look(t, d->u.var.typ);
        if (tt == NULL) {
          // 定义的类型 d->u.var.typ 找不到
          EM_error(d->pos, " you name a var with type, but the type do not existed. ");
          S_enter(v, d->u.var.var, E_VarEntry(varTrAccess(level), Ty_Int()));
        }
        // 生成变量的  Tr_access
        Tr_access newAccess = varTrAccess(level);
        S_enter(v, d->u.var.var, E_VarEntry(newAccess, tt));
        // Tr_printAccess(newAccess);
        // EM_pfun(d->pos, " ==========> after var dec,  name: %s ,  type: %s \n", S_name(d->u.var.var), S_name(d->u.var.typ));
      }

      // 2. 声明 var id := exp, 未指明变量类型
      printf("var dec without named type. \n");
      S_enter(v, d->u.var.var, E_VarEntry(varTrAccess(level), e.ty));

      // 3. 生成赋值的 IR 代码
      // struct expty transVar(Tr_level level, S_table venv, S_table tenv, A_var v)
      // Tr_exp Tr_assignExp(Tr_exp lval, Tr_exp rval)
      lval = transVar(level, v, t, A_SimpleVar(d->pos, d->u.var.var)).exp;
      rval = e.exp;
      trexp = Tr_assignExp(lval, rval);
      break;

      /* 声明函数*/
    case A_functionDec:
      // 1.函数添加进全局环境
      for (fdecl = d->u.function; fdecl; fdecl = fdecl->tail) {
        // 1.1 解析函数
        // EM_pfun(fdecl->head->pos, "func name: %s \n", S_name(fdecl->head->name));

        // 1.2 判断返回值
        if (fdecl->head->result) {
          fun_ty = S_look(t, fdecl->head->result);
          /* 找不到 fun_ty，说明返回值类型未定义 */
          if (!fun_ty) {
            EM_error(fdecl->head->pos, "undefined type for return type\n");
            fun_ty = Ty_Void();
          }
        } else {
          fun_ty = Ty_Void();
        }

        // 1.3 获取形参类型
        formalTys = makeFormalTyList(t, fdecl->head->params);

        // 1.4 函数声明，添加栈帧
        Tr_level nlevel = newLevel(level, formalTys, fdecl->head->name);
        Temp_label lable = fdecl->head->name;

        // 1.5 将函数 添加进全局类型环境
        S_enter(v, fdecl->head->name, E_FunEntry(nlevel, lable, formalTys, fun_ty));
      }

      // 2.形参类型检查
      for (fdecl = d->u.function; fdecl; fdecl = fdecl->tail) {

        // 2.1 获取当前要检查的函数
        fdec = fdecl->head;
        // EM_pfun(fdec->pos, " start check the formals of function for func: %s\n", S_name(fdec->name));
        S_beginScope(v);

        // 2.2 获取形参
        formalTys = makeFormalTyList(t, fdec->params);

        // 2.3 遍历形参,将形参添加进全局环境中
        for (fl = fdec->params, ts = formalTys; fl && ts; fl = fl->tail, ts = ts->tail) {
          /* S_enter(S_table: 值环境,
           * S_symbol sym : fieldlist 中 field 的 name 符号,
           * void *value : 形参); */
          S_enter(v, fl->head->name, E_VarEntry(varTrAccess(level), ts->head));
        }

        // 2.4 解析函数体,判断返回值类型与实际体类型是否一致
        funbody = transExp(level, v, t, fdec->body);  // 函数体
        fentry = S_look(v, fdec->name); /* 函数入口 */

        if (!ty_match(fentry->u.fun.result, funbody.ty)) {
          EM_error(d->pos, "result type is not the same as the func body return type. \n");
        }
        printf("end  function .>>>>\n");
        S_endScope(v);
      }

      // 3. IR 返回 void
      trexp = Tr_voidExp();
      break;

      /* 声明自定义类型 */
    case A_typeDec:
      // 1. 判断不同类型是否同名

      // 1. 将新建的类型名 mytype arrtype 添加进 全局类型环境
      /* d->u.type 为多个 namety A_nametyList， 每一个元素为一个 A_namety 将多个类型声明的每一段 (nl->head) 添加进全局的 类型环境 t , */
      for (nl = d->u.type; nl; nl = nl->tail) {
        /* 将符号 name 添加进 类型环境 t 中，添加绑定: sym->value
         void S_enter(S_table t, S_symbol sym, void *value);
         这里 value 给个 NULL */
        // EM_pfun(d->pos, "new type name:  %s \n", S_name(nl->head->name));
        S_enter(t, nl->head->name, Ty_Name(nl->head->name, NULL));
      }

      // 2. 将类型环境中的 type 与 抽象语法 Ty_ty->u.name.ty 关联
      for (nl = d->u.type; nl; nl = nl->tail) {
        Ty_ty tt = S_look(t, nl->head->name);
        // printf("check kind: %d,  name: %s \n", tt->kind, S_name(tt->u.name.sym));
        assert(tt->kind == Ty_name);
        tt->u.name.ty = transTy(t, nl->head->ty);
      }

      for (nl = d->u.type; nl; nl = nl->tail) {
        /* resTy 为 record 类型 （即 field list）， {key: int, children: int}*/
        recordTy = transTy(t, nl->head->ty);

        /* 如果某一条声明语句对应的类型名字不存在，说明这种类型还没有声明，不能用它来声明新的类型 */
        if (recordTy->kind != Ty_name) {
          EM_error(d->pos, " use undefined type: %s", S_name(nl->head->name));
          break;
        }

        /* 类型环境中查找 声明名字 a (符号) 对应的类型 */
        namety = S_look(t, nl->head->name);

        /* 将新建的类型名 a 的类型设置为对应的类型 intt */
        namety->u.name.ty = recordTy;
      }

      // 3. IR 返回 void
      trexp = Tr_voidExp();
      break;

    default:
      printf("default of transDec");
      assert(0);
  }
  return trexp;
}

/* 转义类型声明 a:intt ->  transTy(S_table t, A_ty int)
 * 返回类型声明部分 具体的类型 Ty_ty*/
static Ty_ty transTy(S_table tb, A_ty ty) {
  Ty_ty tt = NULL;
  Ty_fieldList fieldTys;
  switch (ty->kind) {
    case A_nameTy:  /* type mytype = intt 中的 intt, 判断是否是已经定义过的 类型*/
      tt = S_look(tb, ty->u.name);
      if (!tt) EM_error(ty->pos, "undefined type : %s .\n", S_name(ty->u.name));
      return tt;

    case A_recordTy:  /* type mytype = {a:intt, b:str} 中的 {a:intt, b:str}*/
      fieldTys = makeFieldTys(tb, ty->u.record);
      return Ty_Record(fieldTys);

    case A_arrayTy:   /* type mytype = array of intt 中的 array of intt*/
      /* 查找符号 S_symbol array 对应的类型，就是要返回的类型 */
      tt = S_look(tb, ty->u.array);

      /* 如果 array of intt 中的 intt 没有定义，则报错 */
      if (!tt) {
        EM_error(ty->pos, "undefined array type: %s.\n", S_name(ty->u.array));
      }
      return Ty_Array(tt);
    default:
      printf("transTy failed.\n");
      assert(0);
  }
}

/* 构造域列表 fieldList 的类型  type mytype = {a:intt, b:str} 中的 {a:intt, b:str} */
static Ty_fieldList makeFieldTys(S_table t, A_fieldList fs) {

  A_fieldList fl;
  Ty_fieldList tfl = NULL, head;
  Ty_ty ty;
  Ty_field tmp;

  for (fl = fs; fl; fl = fl->tail) {
    /* 解析 head type 的名字
     * 符号表 t 中查找符号 sym  void *S_look(S_table t, S_symbol sym);
     * struct A_field_ {S_symbol name, typ; A_pos pos; bool escape;};
     * */
    ty = S_look(t, fl->head->typ);
    if (!ty) {
      EM_error(fl->head->pos, "undefined type: %s.\n", S_name(fl->head->typ));
    } else {
      /* 生成 head 对应的一个域 (a:intt)*/
      tmp = Ty_Field(fl->head->name, ty);

      /* 生成并更新 Ty_fieldList tfl 的链表 */
      if (tfl) {
        /* 更新链表 Ty_fieldList tfl */
        tfl->tail = Ty_FieldList(tmp, NULL);
        tfl = tfl->tail;
      } else {
        /* 初始化链表 Ty_fieldList tfl， tail 为 NULL, Head 为第一个 */
        /* Ty_fieldList Ty_FieldList(Ty_field head, Ty_fieldList tail); */
        tfl = Ty_FieldList(tmp, NULL);
        head = tfl;
      }
    }
  }
  return head;
}

/* 构造形参列表 tyList 的类型  function treeLeaves(t : tree, f: from, t: to) : int = ...*/
static Ty_tyList makeFormalTyList(S_table t, A_fieldList fl) {
  Ty_tyList listtail = NULL;
  Ty_tyList listhead = NULL;

  A_fieldList flist = fl;
  Ty_ty head;

  for (; flist; flist = flist->tail) {
    /* 遍历每一个形参定义: "t: tree" */
    head = S_look(t, flist->head->typ);
    if (!head) {
      EM_error(flist->head->pos, "undefined type: %s.\n", S_name(flist->head->typ));
    }
    /* 生成链表 */
    if (!listhead) {
      listtail = Ty_TyList(head, NULL); /* 生成 head 类型对应的 tylist */
      listhead = listtail;
    } else {
      listtail->tail = Ty_TyList(head, NULL);
      listtail = listtail->tail;
    }
  }
  return listhead;
}

/* -----------------------------------------------------  栈帧  ----------------------------------------------------- */

static U_boolList boolListFormalTys(Ty_tyList formalTys) {
  printf("call boolListFormalTys\n");
  Ty_tyList tlist = formalTys;
  U_boolList boolList_head = NULL;
  U_boolList boolList = NULL;
  for (; tlist != NULL; tlist = tlist->tail) {
    // 更新 boolList_head 和 boolList
    if (boolList_head == NULL) {
      boolList = U_BoolList(TRUE, NULL);
      boolList_head = boolList;
    } else {
      boolList->tail = U_BoolList(TRUE, NULL);
      boolList = boolList->tail;
    }
  }
  return boolList_head;
}

// 本地变量
static Tr_access varTrAccess(Tr_level level) {
  return Tr_allocLocal(level);
}

static Tr_level newLevel(Tr_level level, Ty_tyList formalTys, S_symbol funname) {
  return Tr_newLevel(level, Temp_namedlabel(S_name(funname)), boolListFormalTys(formalTys));
}

/* 初始化类型环境 Ty_ ty environment 的符号表 */
S_table E_base_tenv(void) {
  /* 初始化一个空符号表 */
  S_table init_t = S_empty();
  S_enter(init_t, S_Symbol("int"), Ty_Int());
  S_enter(init_t, S_Symbol("string"), Ty_String());
  return init_t;
}

/*
 * 初始化全局值环境 val environment 的符号表
 * 初始化时全局只有 tiger 的标准函数 在值环境中
 */
S_table E_base_venv(void) {
  S_table t = S_empty();
  Tr_level level = NULL;

  /*
  function print(s : string)
    Print s on standard output.
  */
  level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("print"), U_BoolList(TRUE, NULL));
  S_enter(t, S_Symbol("print"), E_FunEntry(level, Temp_newlabel(), Ty_TyList(Ty_String(), NULL), Ty_Void()));
  /*
  function flush()
    Flush the standard output buffer.
  */
  level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("flush"), NULL);
  S_enter(t, S_Symbol("flush"), E_FunEntry(level, Temp_newlabel(), NULL, Ty_Void()));
  /*
  function getchar() : string
    Read a character from standard input; return empty string on end of file.
  */
  level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("getchar"), NULL);
  S_enter(t, S_Symbol("getchar"), E_FunEntry(level, Temp_newlabel(), NULL, Ty_String()));
  /*
  function ord(s: string) : int
    Give ASCII value of first character of s; yields -1 if s is empty string. ord 函数， 获取字符（不是字符串）对应的 ASC 码
  */
  level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("ord"), U_BoolList(TRUE, NULL));
  S_enter(t, S_Symbol("ord"), E_FunEntry(level, Temp_newlabel(), Ty_TyList(Ty_String(), NULL), Ty_Int()));
  /*
  function chr(i: int) : string
    Single-character string from ASCII value i; halt program if i out of range.
  */
  level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("chr"), U_BoolList(TRUE, NULL));
  S_enter(t, S_Symbol("chr"), E_FunEntry(level, Temp_newlabel(), Ty_TyList(Ty_Int(), NULL), Ty_String()));
  /*
  function size(s: string) : int
    Number of characters in s.
  */
  level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("size"), U_BoolList(TRUE, NULL));
  S_enter(t, S_Symbol("size"), E_FunEntry(level, Temp_newlabel(), Ty_TyList(Ty_String(), NULL), Ty_Int()));
  /*
  function substring(s:string, first:int, n:int) : string
    Substring of string s, starting with character first, n characters long. Char-acters are numbered starting at 0.
  */
  level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("substring"),
                      U_BoolList(TRUE, U_BoolList(TRUE, U_BoolList(TRUE, NULL))));
  S_enter(t, S_Symbol("substring"), E_FunEntry(level, Temp_newlabel(),
                                               Ty_TyList(Ty_String(), Ty_TyList(Ty_Int(), Ty_TyList(Ty_Int(), NULL))),
                                               Ty_String()));
  /*
  function concat (s1: string, s2: string) : string
    Concatenation of s1 and s2.
  */
  level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("concat"), U_BoolList(TRUE, U_BoolList(TRUE, NULL)));
  S_enter(t, S_Symbol("concat"),
          E_FunEntry(level, Temp_newlabel(), Ty_TyList(Ty_String(), Ty_TyList(Ty_String(), NULL)), Ty_String())
  );
  /*
  function not(i : integer) : integer
    Return (i=0).
  */
  level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("not"), U_BoolList(TRUE, NULL));
  S_enter(t, S_Symbol("not"), E_FunEntry(level, Temp_newlabel(), Ty_TyList(Ty_Int(), NULL), Ty_Int()));
  /*
  function exit(i: int)
    Terminate execution with code i.
  */
  level = Tr_newLevel(Tr_outermost(), Temp_namedlabel("exit"), U_BoolList(TRUE, NULL));
  S_enter(t, S_Symbol("exit"), E_FunEntry(level, Temp_newlabel(), Ty_TyList(Ty_Int(), NULL), Ty_Void()));
  return t;
}
