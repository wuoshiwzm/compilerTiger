/**
    语义分析，类型检查

    类型定义
    tydec   →   type type-id = ty
    ty      →   type_id
            →   { tyfields }
            →   array of type-id
            如：
            type mytype = {a:int, b:string}
            type arrtype = array of int (数组的个数不限制)
            var row := arrtype[10] of 0
            var row2 := arrtype[11] of 1

    record
    自定义type-id -> , 定义 type-id 的 record 实例
    每个实例 和 数组 数据都是唯一的，即使另一个数据的每个属性都与他相同
    type-id {id1=exp {, id2=exp}}  创建一个 type-id 类型的 record 实例， 且顺序必须和给定的 record 一致 （类似于 dict）
    record  →
            type intlist = {hd: int, tl: intlist}
            let var record := intlist{ hd = 3, tl = intlist{ hd = 5 } }


    变量定义
    vardec  →   var id := exp
            →   var id : type-id := exp
            如： var b : int := 300;  ...

    函数定义
    fundec  →    function id ( tyfields) = exp
            →    function id ( tyfields) : type-id = exp
            如：function treeLeaves(t : tree) : int = xxx...

    field 定义
    tyfields    →   ϵ
                →   id : type-id {, id : type-id}
                如：{hd: tree, tl: treelist}

    scope 定义
    scope   →   let ...vardec... in exp end
            →   let ...fundec... in exp end
            →   let ...typedec... in exp end
            如：let var v := 6
                in
                    exps...
                end




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

/*
 * 函数体声明
 */
/* 将变量 转义成表达式类型 */
static struct expty transVar(S_table venv, S_table tenv, A_var v);

/* 将表达式 转义成表达式类型 */
static struct expty transExp(S_table venv, S_table tenv, A_exp e);

static void transDec(S_table venv, S_table tenv, A_dec d);

static Ty_ty transTy(S_table tenv, A_ty t);

static Ty_tyList makeFormalTyList(S_table t, A_fieldList p);

static Ty_ty actual_ty(Ty_ty ty);

static bool args_match(S_table v, S_table t, A_expList el, Ty_tyList fl, A_exp fun);

static bool ty_match(Ty_ty t1, Ty_ty t2);

static bool efields_match(S_table v, S_table t, Ty_ty ty, A_exp e);

static Ty_fieldList makeFieldTys(S_table t, A_fieldList fs);


/* 核心函数，类型检查 */
void SEM_transProg(A_exp exp) {
    struct expty et;
}

/* 获取 ty 的原生类型 */
static Ty_ty actual_ty(Ty_ty ty) {
    if (!ty) return ty;
    // ty->kind == Ty_name 说明 ty 的类型名是是重命名后的，如： a:int , b: string  等
    if (ty->kind == Ty_name) return actual_ty(ty->u.name.ty);
    else return ty;
}


static bool ty_match(Ty_ty a, Ty_ty_b) {

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

    return (
            ((tk == Ty_record || tk == Ty_array) && t == e) ||
            (tk == Ty_record && ek == Ty_nil) ||
            (ek == Ty_record && tk == Ty_nil) ||
            (tk != Ty_record && tk != Ty_array && tk == ek)
    );
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
static bool args_match(S_table v, S_table tt, A_expList ell, Ty_tyList fll, A_exp fun) {
    /* 表达式类型	*/
    struct expty et;
    A_expList el = ell; /* 传 */
    Ty_tyList fl = fll;

    /* 判断当前的第 i 个实参 与 第 i 个形参 类型是否匹配 */
    while (el && fl) {
        /* 转义函数表达式 第一个 */
        et = transExp(v, tt, el->head);

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
static bool efields_match(S_table v, S_table t, Ty_ty tyy, A_exp e) {
    struct expty et;
    Ty_fieldList ty = tyy->u.record; /* 定义的fields 类型 */
    A_efieldList fl = e->u.record.fields; /* 表达式的 fields*/

    /* 比较 field list 里面的属性的类型是否一致 */
    while (ty && fl) {
        et = transExp(v, t, fl->head->exp);
        if (
            (ty->head->name != fl->head->name) ||
            !ty_match(ty->head->ty, et.ty)
        ) {
            EM_error(e->pos, "unmatched name: type in %s", S_name(e->u.record.typ));
            return FALSE;
        }
        ty = ty->tail;
        fl = fl->tail;
    }

    /* ty 和 fl 有一个不是NULL, 说明个数不匹配 */
    if (ty || fl) {
        EM_error(e->pos, "the records define un match, either one of them is none.");
        return FALSE;
    }
    return TRUE;
}


/* 解析变量的类型 */
static struct expty transVar(S_table venv, S_table tenv, A_var v) {

    /* 声明 环境， 类型变量， 域链表变量 */
    E_enventry x; // 环境
    struct expty et, et2;
    Ty_fieldList fl;

    /* A_simpleVar, A_fieldVar, A_subscriptVar} */
    switch (v->kind) {
        case A_simpleVar : // 简单变量

            /* 全局符号表 t 中查找符号 sym,返回绑定的 value.  v->u.simple: 简单变量的符号 */
            x = S_look(venv, v->u.simple);

            /* 符号 */
            if (x && x->kind != E_varEntry) {

                /*
                 值环境中不存在
                 或者存在 ，但绑定的值不是简单变量，说明一个符号应该是简单变量，但对应的值不是简单变量，报语义错误
                 */
                EM_error(v->pos, " undefined var %s or the type is not simple var", S_name(v->u.simple));

                // 固定返回 int, 让编译器继续往下走。
                return expTy(NULL, Ty_Int());
            } else {

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
            et = transVar(venv, tenv, v->u.field.var);

            /* 在 {hd: int, tl: intlist} 中 找hd 对应的类型 */
            Ty_ty basety = actual_ty(et.ty);

            /* Confirm that the actual type is a record. */
            if (basety->kind != Ty_record) {
                /* 域变量，其类型应该是 Ty_record */
                EM_error(v->pos, " it is field var, but not a record type.");
                return expTy(NULL, Ty_Record(NULL));
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
            et = transVar(venv, tenv, v->u.subscript.var);

            if (actual_ty(et.ty)->kind != Ty_array) {
                EM_error(v->pos, "it is not a array");
            }

            /* 获取下标，下标的最总类型应该是 int */
            struct expty exp_et = transExp(venv, tenv, v->u.subscript.exp);
            if (actual_ty(exp_et.ty)->kind != Ty_int) {
                EM_error(v->pos, " the subscript of arr must be int !");
            }

            /* 下标是整型，则返回数组的类型，就是 arr[b] 的类型 */
            return expTy(NULL, actual_ty(et.ty->u.array));
    }
}

/**
 * 解析表达式类型
 * @param v 值环境 venv
 * @param t 类型环境 tenv
 * @param e 表达式
 * @return
 */
static struct expty transExp(S_table v, S_table t, A_exp e) {

    A_oper oper;

    E_enventry callinfo; /* 函数调用: funx(a,b) */
    Ty_ty recordty, arrayty;
    A_expList list;

    struct expty left, right, final, final2, final3, final4, final5, lo, hi;

    switch (e->kind) {

        /* 变量表达式 */
        case A_VarExp:
            return transVar(v, t, e->u.var);
            break;

            /* NIL 表达式 */
        case A_NilExp:
            return expTy(NULL, Ty_Nil());
            break;

            /* 调用表达式 */
        case A_CallExp:

            /* 获取调用信息 在值环境 v 中查找函数 func 的参数，返回值 */
            callinfo = S_look(v, e->u.call.func);

            /* 表达式形式为 函数名funx(参数params)的形式，否则表示函数 funx 未定义 */
            if (callinfo && callinfo->kind == E_funEntry) {

                /* 判断实参(e->u.call.args) 和 形参(callinfo->u.fun.formals)类型是否一致 */
                if (args_match(v, t, e->u.call.args, callinfo->u.fun.formals, e)) {
                    /* 参数类型正确，返回值类型为 定义的或 void */
                    if (callinfo->u.fun.result) {
                        return expTy(NULL, actual_ty(callinfo->u.fun.result));
                    } else {
                        return expTy(NULL, Ty_Void());
                    }
                } else {
                    EM_error(e->pos, "args match failed.\n");
                    return expTy(NULL, Ty_Void());
                }
            } else {
                EM_error(e->pos, "func: %s undefined.\n", S_name(e->u.call.func));
            }
            return expTy(NULL, Ty_Void());

            /* record 表达式 */
        case  A_recordExp:
            /* 在类型环境 tenv 中查找符号 e->u.record.typ 绑定的类型  */
            recordty = actual_ty(S_look(t, e->u.record.typ));

            if (!recordty) { /* 找不到 record 绑定的类型 */
                EM_error(e->pos, "can not find type def of record name: %s \n", S_name(e->u.record.typ));
            } else {
                if (recordty->!= Ty_record) {
                    EM_error(e->pos, " %s should be a record type, but in fact it is not \n", S_name(e->u.record.typ));
                }

                // 判断 record 中的 fields 的定义是否匹配
                if (efields_match(v,t,recordty, e)) {
                    return expTy(NULL, recordty);
                }
            }
            return expTy(NULL, Ty_Record(NULL));

        case A_arrayExp:
            arrayty = actual_ty(S_look(t, e->u.array.typ));

            if((!arrayty) || (arrayty->kind != Ty_array)){
                EM_error(e->pos, "the exp should be array but it is not or undefined : %s \n", S_name(e->u.array.typ));
            }

            /* 判断数组大小，默认值类型 是否与数组定义一致*/
            final2 = transExp(v,t,e->u.array.size);
            final3 = transExp(v,t,e->u.array.init); /* 数组的默认值 如 int a[2]='test' */

            if(final2.ty->kind != Ty_int){
                EM_error(e->pos,"array size should be int \n");
            } else if( !ty_match(final3.ty, arrayty->u.array)){
                EM_error(e->pos,"array type unmatch \n");
            }else{
                return expTy(NULL, arrayty);
            }
            return expTy(NULL, Ty_Array(NULL));

        /* 表达式列表：按每一个表达式转义 */
        case A_seqExp:
            list = e->u.seq;

            if(!list){
                return expTy(NULL, Ty_Void());
            }

            /* 遍历list */
            while(list->tail){
                return transExp(v,t,list->head);
                list = list->tail;
            }
            return transExp(v,t,list->head);

        /* while 表达式 */
        case A_whileExp:

            /* while() 中止条件判断，应该是 int (0, 1) */
            final = transExp(v,t,e->u.whilee.test);
            if(final.ty->kind!= Ty_int){
                EM_error(e->pos, "while() condition should be int \n");
            }

            /* while 函数体则没有要求 */
            transExp(v,t,e->u.whilee.body);
            return expTy(NULL, Ty_Void());


        /* 赋值表达式  var := exp */
        case A_assignExp:
            struct expty v = transVar(v,t,e->u.assign.var);
            struct expty ex = transVar(v,t,e->u.assign.exp);

            /* 判断赋值类型与定义类型是否一致 */
            if(!ty_match(v.ty, ex.ty)){
                EM_error(e->pos, "assign type is not same as defined. \n");
            }
            /* 赋值表达式没有返回值 */
            return expTy(NULL, Ty_Void());


        /* break表达式 */
        case A_breakExp:
            return expTy(NULL, Ty_Void());


        /* for表达式 */
        case A_forExp:
            struct expty from = transVar(v,t,e->u.forr.lo);
            struct expty to = transVar(v,t,e->u.forr.hi);
            struct expty body = transExp(v,t,e->u.forr.body);

            if((from.ty->kind != Ty_int) || (to.ty->kind != Ty_int)){
                EM_error(e->pos, "the from , to is not number. \n");
            }

            /* 进入值环境 v */
            S_beginScope(v);
            /* 声明 int i 的初始化 */
            transDec(v,t, A_VarDec(e->pos, e->u.forr.var, S_Symbol("int"), e->u.forr.lo));
            /* 函数体 */
            body = transExp(v,t,e->u.forr.body);
            S_endScope(v);

            return expTy(NULL, Ty_Void());


        /* let 声明表达式
         * let ...decs... in ...exp_body... end */
        case A_letExp:

            /* 声明只在域scope内有效 */
            S_beginScope(v);
            S_beginScope(t);

            /* 转义所有的 let 的声明语句 */
            for (A_decList decs = e->u.let.decs; decs ; decs = decs->tail) {
                transDec(v,t,decs->head);
            }

            /* 转义 let 的函数体 */
            struct expty final = transExp(v,t,e->u.let.body);

            S_endScope(t);
            S_endScope(v);
            return final;

        /* 双目表达式
         typedef enum {A_plusOp, A_minusOp, A_timesOp, A_divideOp,
	     A_eqOp, A_neqOp, A_ltOp, A_leOp, A_gtOp, A_geOp} A_oper; */
        case A_opExp:

            A_oper op = e->u.op.oper;  /* 符号 */
            struct expty lexp = transExp(v,t,e->u.op.left);
            struct expty rexp = transExp(v,t,e->u.op.right);
            A_exp rr = e->u.op.right;

            /* 加减乘除 +,-,*,/ */

            /* typedef enum {A_plusOp, A_minusOp, A_timesOp, A_divideOp, ...} A_oper; */
            if(1 <= oper && oper < 4){

                if(lexp.ty->kind != Ty_int && lexp.ty->kind != Ty_double){
                    EM_error(e->pos, "left num need to be int or double for oper: + - * / \n");
                }

                if(rexp.ty->kind != Ty_int && rexp.ty->kind != Ty_double){
                    EM_error(e->pos, "right num need to be int or double for oper: + - * / \n");
                }

                /* 除法除0时报错 */
                if(oper == 3 && rr->u.intt == 0){
                    EM_error(e->pos, "no!!!!!! devide by zero !!!!!!!!!!!! \n");
                }

                if(lexp.ty->kind == Ty_int && rexp.ty->kind == Ty_int && oper != 3){
                    return expTy(NULL, Ty_Int());
                }
            } else if(3 < oper && oper < 10){
                /* 比较等式 A_eqOp, A_neqOp, A_ltOp, A_leOp, A_gtOp, A_geOp */

                /* 等于/不等于 record 可以为 nil, 也就是説  type　record　和 nil 可以相等或不等 */
                if (oper == 4 || oper == 5) {
                    if ((left.ty->kind == Ty_record || left.ty->kind == Ty_nil) &&
                        (right.ty->kind == Ty_record || right.ty->kind == Ty_nil)) {
                        return expTy(NULL, Ty_Int());
                    }
                }

                /* 比较式左右的类型应该相等 */
                if(left.ty->kind != right.ty->kind){
                    EM_error(e->pos, " the left exp type is not the same with right exp type, can not compare! \n")
                }

                return expTy(NULL, Ty_Int());
            } else {
                /* 未定义的运算符 */
                assert(0);
            }

        /* if表达式 */
        case A_ifExp:
            struct expty ifexp = e->u.iff.test;
            struct expty thenexp = e->u.iff.then
            struct expty elseexp = e->u.iff.elsee;

            if (ifexp.ty->kind != Ty_int){
                EM_error(e->pos, "the exp in if() is not 1/0\n");
            }

            /* 包含 else 部分，返回类型要和 then 部分一致 */
            if (elseexp){
                if (!ty_match(thenexp.ty, elseexp.ty)){
                    EM_error(e->pos, "return type of then and else should be the same.\n");
                }
            }
            return expTy(NULL, thenexp.ty);


        case A_stringExp:
            return expTy(NULL, Ty_String());
        case A_intExp:
            return expTy(NULL, Ty_Int());
        case A_doubleExp:
            return expTy(NULL, Ty_Double());

        default:
            /* 应该走不到这里，否则就是漏掉的类型 */
            assert(0);
    }
}



/*
 struct A_dec_
    {enum {A_functionDec, A_varDec, A_typeDec} kind;
     A_pos pos;
     union {A_fundecList function;
	    // escape may change after the initial declaration
    struct {S_symbol var; S_symbol typ; A_exp init; bool escape;} var;
        A_nametyList type;
    } u;
};
 */
/* 转义声明表达式 */
static void transDec(S_table v, S_table t, A_dec d){

}



/* 转义类型 */
static Ty_ty transTy(S_table tb, A_ty ty){

}

/* 域列表构造函数 */
static Ty_fieldList makeFieldTys(S_table t, A_fieldList fs){

}

/* 形参列表构造函数 */
static Ty_tyList makeFormalTyList(S_table t, A_fieldList fl){

}
