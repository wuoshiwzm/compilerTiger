/**
    语义分析，类型检查
*/
#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "translate.h"
#include "env.h"
#include "semant.h"


/*
 * 函数体声明
 */
/* 将变量 转义成表达式类型 */
static struct expty transVar(S_table venv, S_table tenv, A_var v);

/* 将表达式 转义成表达式类型 */
static struct expty transExp(S_table venv, S_table tenv, A_exp e);

/* 核心函数，类型检查 */
void SEM_transProg(A_exp exp)



/*
 struct E_enventry_ {
    enum {E_varEntry, E_funEntry} kind;
    union {
        struct {Ty_ty ty;} var;
        struct {Ty_tyList formals; Ty_ty result;} fun;
    } u;
 };
 */

/*
 struct A_var_
   {enum {A_simpleVar, A_fieldVar, A_subscriptVar} kind;
    A_pos pos;
union {S_symbol simple; // 简单变量的符号
       struct {A_var var;
           S_symbol sym;} field;
       struct {A_var var;
           A_exp exp;} subscript;
     } u;
  };
 */

/* struct Ty_ty_ {
    enum {
        // 基础类型
        Ty_record, Ty_nil, Ty_int, Ty_string, Ty_array, Ty_name, Ty_void, Ty_double
    } kind;
    union {
        Ty_fieldList record;
        Ty_ty array;
        struct {
            S_symbol sym;
            Ty_ty ty;
        } name;
    } u;
}; */


/* 获取 ty 的原生类型 */
static Ty_ty actual_ty(Ty_ty ty){
    if(!ty) return ty;
    //  ty->kind == Ty_name 说明是重命名后的 type
    if(ty->kind == Ty_name) return actual_ty(ty->u.name.ty)
    else return ty;
}


/* 解析变量的类型 */
static struct expty transVar(S_table venv, S_table tenv, A_var v){

    /* 声明 环境， 类型变量， 域链表变量 */
    E_enventry x; // 环境
    struct expty et,et2;
    Ty_fieldList fl;

    /* A_simpleVar, A_fieldVar, A_subscriptVar} */
    switch (v->kind) {
        case A_simpleVar : // 简单变量

            /* 全局符号表 t 中查找符号 sym,返回绑定的 value.  v->u.simple: 简单变量的符号 */
            x = S_look(venv, v->u.simple);

            /* 符号 */
            if(v_val && x != E_varEntry){

                /*
                 值环境中不存在
                 或者存在 ，但绑定的值不是简单变量，说明一个符号应该是简单变量，但对应的值不是简单变量，报语义错误
                 */
                EM_error(v->pos, " undefined var %s or the type is not simple var", S_name(v->u.simple));

                // 固定返回 int, 让编译器继续往下走。
                return expTy(NULL, Ty_Int());
            }else{

                /*
                 返回一个转义的表达式的对象 expty {Tr_exp exp; Ty_ty ty;}
                 属性为表达式 Tr_exp exp 和 对应的类型 Ty_ty ty
                 return expTy(v->u.simple, actual_ty(x->u.var.ty));
                 */
                return expTy(NULL, actual_ty(x->u.var.ty));
            }
            break;


        /*
         * var record (a.b) 域变量
         * 如 type intlist = {hd: int, tl: intlist}  intlist.hd
         * 其中 intlist.hd   intlist 对应 A_var var, hd 对应 S_symbol sym
         * */
        case A_fieldVar :

            /* 拿到 a.b 对应的类型，则应该走 simpleVar 分支，为 record 类型，*/
            struct expty et = transVar(venv, tenv, v->u.field.var);

            /* 在 {hd: int, tl: intlist} 中 找hd 对应的类型 */
            Ty_ty basety = actual_ty(et.ty);

            /* Confirm that the actual type is a record. */
            if(basety->kind != Ty_record){
                /* 域变量，其类型应该是 Ty_record */
                EM_error(v->pos, " it is field var, but not a record type." );
                return expTy(null, Ty_Record(NULL));
            }

            /* 遍历当前的 Ty_record 环境 的所有 field , 找到v的符号( hd )对应的那个field: {hd: int}
             * 他的类型就是对应的 intlist.hd 的类型*/
            Ty_fieldList fields = basety->u.record;
            for (fl = et.ty->u.record; fl; fl = fl->tail) {
                if (fl->head->name == v->u.field.sym) {
                    return expTy(NULL, actual_ty(fl->head->ty));
                }
            }

            /* 没找到， 说明当前的自定义域中没有 ｖ 对应的 field */
            EM_error(v->pos, " there is no such field in record: %s", S_name(v->u.field.sym));

            /* 返回空 Ty_Record, 让编译继续类型检查 */
            return expTy(NULL, Ty_Record(NULL));

        /*
         * arr[b]
         * 其中  arr 对应 A_var var,
         * b 对应 A_exp exp, 不一定是个数字 ，也可以是个表达式，如变量名，或计算表达式
         * */
        case A_subscriptVar:

            /* 获取数组 arr */
            struct expty et = transVar(venv, tenv, v->u.subscript.var);

            if( actual_ty(et.ty)->kind != Ty_array){
                EM_error(v->pos, "it is not a array");
            }

            /* 获取下标，下标的最总类型应该是 int */
            struct expty exp_et = transExp(venv, tenv, v->u.subscript.exp);
            if(actual_ty(exp_et.ty)->kind != Ty_int){
                EM_error(v->pos, " the subscript of arr must be int !");
            }

            /* 下标是整型，则返回数组的类型，就是 arr[b] 的类型 */
            return expTy(NULL, actual_ty(et.ty->u.array));
    }
}



