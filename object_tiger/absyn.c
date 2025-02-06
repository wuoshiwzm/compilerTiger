#ifndef _ABSYN_C_
#define _ABSYN_C_

/*
 * 抽象语法
 * absyn.c - Abstract Syntax Functions. Most functions create an instance of an
 *           abstract syntax rule.
 *
    A 开头是对应的是原程序中的"一个"实体
    Ty _ 开头则是类型检查中的一个"抽象类" 代表了"一类"实体

    如：
    A_recordTy 是原程序中的 一个type 如：{name:string , age:int}
    Ty_record 则是一种类型的抽象 是 Ty_ty { TyfieldList } tyfield ->{name : ty_string, age: ty_int }
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h" /* symbol table data structures */
#include "absyn.h"

/*变量
 S_symbol => a pointer to struct {string name; S_symbol}; IN A WORD one node of a list
 */
A_var A_SimpleVar(A_pos pos, S_symbol sym)
{
    A_var p = checked_malloc(sizeof(*p));
    p->kind = A_simpleVar;
    p->pos = pos;
    p->u.simple = sym;
    return p;
}

/* 域变量 a.b */
A_var A_FieldVar(A_pos pos, A_var var, S_symbol sym)
{
    A_var p = checked_malloc(sizeof(*p));
    p->kind = A_fieldVar;
    p->pos = pos;
    p->u.field.var = var;
    p->u.field.sym = sym;
    return p;
}

/* 数组下标 */
A_var A_SubscriptVar(A_pos pos, A_var var, A_exp exp)
{
    A_var p = checked_malloc(sizeof(*p));
    p->kind = A_subscriptVar;
    p->pos = pos;
    p->u.subscript.var = var;
    p->u.subscript.exp = exp;
    return p;
}

/* 表达式 */
A_exp A_VarExp(A_pos pos, A_var var)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_varExp;
    p->pos = pos;
    p->u.var = var;
    return p;
}

/* null 表达式 */
A_exp A_NilExp(A_pos pos)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_nilExp;
    p->pos = pos;
    return p;
}

/* int 变量 */
A_exp A_IntExp(A_pos pos, int i)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_intExp;
    p->pos = pos;
    p->u.intt = i;
    return p;
}

A_exp A_DoubleExp(A_pos pos, double d)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_doubleExp;
    p->pos = pos;
    p->u.doublee = d;
    return p;
}

A_exp A_StringExp(A_pos pos, string s)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_stringExp;
    p->pos = pos;
    p->u.stringg = s;
    return p;
}

/* 可调用的表达式 */
A_exp A_CallExp(A_pos pos, S_symbol func, A_expList args)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_callExp;
    p->pos = pos;
    p->u.call.func = func;
    p->u.call.args = args;
    return p;
}

/* 双目运算符 */
A_exp A_OpExp(A_pos pos, A_oper oper, A_exp left, A_exp right)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_opExp;
    p->pos = pos;
    p->u.op.oper = oper;
    p->u.op.left = left;
    p->u.op.right = right;
    return p;
}

/*  */
A_exp A_RecordExp(A_pos pos, S_symbol typ, A_efieldList fields)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_recordExp;
    p->pos = pos;
    p->u.record.typ = typ;
    p->u.record.fields = fields;
    return p;
}

/* exp 序列 */
A_exp A_SeqExp(A_pos pos, A_expList seq)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_seqExp;
    p->pos = pos;
    p->u.seq = seq;
    return p;
}

/* 赋值表达式 */
A_exp A_AssignExp(A_pos pos, A_var var, A_exp exp)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_assignExp;
    p->pos = pos;
    p->u.assign.var = var;
    p->u.assign.exp = exp;
    return p;
}

/* if 表达式 */
A_exp A_IfExp(A_pos pos, A_exp test, A_exp then, A_exp elsee)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_ifExp;
    p->pos = pos;
    p->u.iff.test = test;
    p->u.iff.then = then;
    p->u.iff.elsee = elsee;
    return p;
}

/* while表达式 */
A_exp A_WhileExp(A_pos pos, A_exp test, A_exp body)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_whileExp;
    p->pos = pos;
    p->u.whilee.test = test;
    p->u.whilee.body = body;
    return p;
}

/* for表达式 */
A_exp A_ForExp(A_pos pos, S_symbol var, A_exp lo, A_exp hi, A_exp body)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_forExp;
    p->pos = pos;
    p->u.forr.var = var;
    p->u.forr.lo = lo;
    p->u.forr.hi = hi;
    p->u.forr.body = body;
    p->u.forr.escape = TRUE;
    return p;
}

/* break表达式 */
A_exp A_BreakExp(A_pos pos)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_breakExp;
    p->pos = pos;
    return p;
}

/* let 语句 */
A_exp A_LetExp(A_pos pos, A_decList decs, A_exp body)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_letExp;
    p->pos = pos;
    p->u.let.decs = decs;
    p->u.let.body = body;
    return p;
}

/* array 声明 */
A_exp A_ArrayExp(A_pos pos, S_symbol typ, A_exp size, A_exp init)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind = A_arrayExp;
    p->pos = pos;
    p->u.array.typ = typ;
    p->u.array.size = size;
    p->u.array.init = init;
    return p;
}

/* function 声明 */
A_dec A_FunctionDec(A_pos pos, A_fundecList function)
{
    A_dec p = checked_malloc(sizeof(*p));
    p->kind = A_functionDec;
    p->pos = pos;
    p->u.function = function;
    return p;
}

/* 变量 声明 */
A_dec A_VarDec(A_pos pos, S_symbol var, S_symbol typ, A_exp init)
{

    A_dec p = checked_malloc(sizeof(*p));
    p->kind = A_varDec;
    p->pos = pos;
    p->u.var.var = var;
    p->u.var.typ = typ;
    p->u.var.init = init;
    p->u.var.escape = TRUE;
    return p;
}

/* type 声明 */
/* Ty_record 则是一种类型的抽象 是 Ty_ty { TyfieldList } tyfield ->{name : ty_string, age: ty_int } */
A_dec A_TypeDec(A_pos pos, A_nametyList type)
{
    A_dec p = checked_malloc(sizeof(*p));
    p->kind = A_typeDec;
    p->pos = pos;
    p->u.type = type;
    return p;
}

/* name type */
A_ty A_NameTy(A_pos pos, S_symbol name)
{
    A_ty p = checked_malloc(sizeof(*p));
    p->kind = A_nameTy;
    p->pos = pos;
    p->u.name = name;
    return p;
}

/* record type */
/* A_recordTy 是原程序中的 一个type 如：{name:string , age:int} */
A_ty A_RecordTy(A_pos pos, A_fieldList record)
{
    A_ty p = checked_malloc(sizeof(*p));
    p->kind = A_recordTy;
    p->pos = pos;
    p->u.record = record;
    return p;
}

/* 一个 array 类型的 type */
A_ty A_ArrayTy(A_pos pos, S_symbol array)
{
    A_ty p = checked_malloc(sizeof(*p));
    p->kind = A_arrayTy;
    p->pos = pos;
    p->u.array = array;
    return p;
}

/* 一个域，escape=TRUE表示编译时候要解析 */
A_field A_Field(A_pos pos, S_symbol name, S_symbol typ)
{
    A_field p = checked_malloc(sizeof(*p));
    p->pos = pos;
    p->name = name;
    p->typ = typ;
    p->escape = TRUE;
    return p;
}

/* 域的列表（链表） */
A_fieldList A_FieldList(A_field head, A_fieldList tail)
{
    A_fieldList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

/* 表达式的列表（链表） */
A_expList A_ExpList(A_exp head, A_expList tail)
{
    A_expList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

/* 函数声明 */
A_fundec A_Fundec(A_pos pos, S_symbol name, A_fieldList params, S_symbol result, A_exp body)
{
    A_fundec p = checked_malloc(sizeof(*p));
    p->pos = pos;
    p->name = name;
    p->params = params;
    p->result = result;
    p->body = body;
    return p;
}

/* 函数生成列表 */
A_fundecList A_FundecList(A_fundec head, A_fundecList tail)
{
    A_fundecList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

/* 声明列表 */
A_decList A_DecList(A_dec head, A_decList tail)
{
    A_decList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

/*  */
A_namety A_Namety(S_symbol name, A_ty ty)
{
    A_namety p = checked_malloc(sizeof(*p));
    p->name = name;
    p->ty = ty;
    return p;
}

/* A_Namety 列表 */
A_nametyList A_NametyList(A_namety head, A_nametyList tail)
{
    A_nametyList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

/* 有name的表达式 */
A_efield A_Efield(S_symbol name, A_exp exp)
{
    A_efield p = checked_malloc(sizeof(*p));
    p->name = name;
    p->exp = exp;
    return p;
}

/* 有name的表达式 列表 */
A_efieldList A_EfieldList(A_efield head, A_efieldList tail)
{
    A_efieldList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

void printExp(A_exp exp)
{
    const char *enums[16] = {
        "A_varExp", "A_nilExp", "A_intExp", "A_doubleExp", "A_stringExp", "A_callExp", "A_opExp", "A_recordExp",
        "A_seqExp", "A_assignExp", "A_ifExp", "A_whileExp", "A_forExp", "A_breakExp", "A_letExp", "A_arrayExp"};
    printf(" ---------->>>>> Exp info, kind: %s ", enums[exp->kind]);
    if (exp->kind == 0)
    {
        printf(" (%s)\n", exp->u.stringg);
    }
    if (exp->kind == 2)
    {
        printf("( %d )\n", exp->u.intt);
    }
    printf("\n");
}

void printDec(A_dec dec)
{
    const char *enums[16] = {"A_functionDec", "A_varDec", "A_typeDec"};
    printf("    | \n");
    printf("     ---------->>>>> Declare info, kind: %s  ", enums[dec->kind]);

    if (dec->kind == 0)
    {
        printf(" function: %s. \n", S_name(dec->u.function->head->name));
    }

    if (dec->kind == 1)
    {
        printf(" var: %s. \n", S_name(dec->u.var.var));
    }
}

/*
    面向对象 object tiger
*/
A_dec A_ClassDec(A_pos pos, A_clsdec class)
{  
    A_dec p = checked_malloc(sizeof(*p));
    p->kind=A_clsDec;
    p->pos=pos;
    p->u.cls = class;
    return p;
}
A_clsdec A_Clsdec(A_pos pos, S_symbol name, S_symbol parent, A_clsFieldList fields)
{
    A_clsdec p = checked_malloc(sizeof(*p));
    p->pos = pos;
    p->clsName = name;
    p->parentName = parent;
    p->fileds = fields;
    return p;
}

A_clsFieldList A_ClsFieldList(A_clsField head, A_clsFieldList list)
{
    A_clsFieldList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = list;
    return p;
}

// 声明类属性
A_clsField A_ClsVarField(A_pos pos, S_symbol var, S_symbol type, A_exp init)
{
    A_clsField p = checked_malloc(sizeof(*p));
    p->pos = pos;
    p->kind = clsField;
    p->u.var.var = var;
    p->u.var.typ = type;
    p->u.var.init = init;
    return p;
}

// 声明类方法
A_clsField A_ClsMethod(A_pos pos, S_symbol name, A_fieldList params, S_symbol result, A_exp body)
{
    A_clsField p = checked_malloc(sizeof(*p));
    p->pos = pos;
    p->kind = clsMethod;
    p->u.method.name = name;
    p->u.method.params = params;
    p->u.method.result = result;
    p->u.method.body = body;
    return p;
}

A_dec A_ObjDec(A_pos pos,  S_symbol var, S_symbol typ, S_symbol class)
{
    A_dec p = checked_malloc(sizeof(*p));
    p->kind = A_obj;
    p->pos = pos;
    p->u.obj.var = var;
    p->u.obj.typ = typ;
    p->u.obj.class = class;
    return p;
}

#endif
