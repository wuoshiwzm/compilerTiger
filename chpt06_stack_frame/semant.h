#ifndef _SEMANT_H_
#define _SEMANT_H_

#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "frame.h"
#include "myframe.h"
#include "translate.h"
/**
    语义分析-类型检查
*/
void SEM_transProg(A_exp exp);


/* 将变量 转义成表达式类型 */
static struct expty transVar(Tr_level level, S_table venv, S_table tenv, A_var v);

/* 将表达式 转义成表达式类型 */
static struct expty transExp(Tr_level level, S_table venv, S_table tenv, A_exp e);

static void transDec(Tr_level level, S_table venv, S_table tenv, A_dec d);

static Ty_ty transTy(S_table tb, A_ty ty);

static Ty_tyList makeFormalTyList(S_table t, A_fieldList fl);

static Ty_ty actual_ty(Ty_ty ty);

static bool args_match(Tr_level level, S_table v, S_table t, A_expList el, Ty_tyList fl, A_exp fun);

static bool ty_match(Ty_ty a, Ty_ty b);

static bool efields_match(Tr_level level, S_table v, S_table t, Ty_ty ty, A_exp e);

static Ty_fieldList makeFieldTys(S_table t, A_fieldList fs);

// 获取变量的访问
static Tr_access varTrAccess();

// 新的level 意味着发生 函数嵌套声明，则必然有 formals
// ...Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);
static Tr_level newLevel(Tr_level level, Ty_tyList formalTys);


// 退出函数，则 globalLevel 退回上一级
static void quitFun();

// 进入函数，则 globalLevel 增加一层
static void enterFun(Ty_tyList formalTys);

#endif
