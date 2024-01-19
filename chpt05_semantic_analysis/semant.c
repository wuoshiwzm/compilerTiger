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
    S_table t = E_base_tenv(); /* 类型环境 */
    S_table v = E_base_venv(); /* 值环境 */
    et = transExp(v, t, exp);
    printf("============================= done =============================...");

    /* 基础类型 */
//    string* enums[16] = {"Ty_record", "Ty_nil", "Ty_int", "Ty_string", "Ty_array", "Ty_name", "Ty_void", "Ty_double"};
//    printf("@this expr return %s \n", enums[et.ty->kind]);
}

/* 获取 ty 的原生类型 */
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

/* 解析变量的类型 */
static struct expty transVar(S_table venv, S_table tenv, A_var v){

    /* 声明 环境， 类型变量， 域链表变量 */
    E_enventry x; /* 环境 */
    struct expty et, et2;
    Ty_fieldList fl;

    /* A_simpleVar, A_fieldVar, A_subscriptVar} */
    switch (v->kind) {
        case A_simpleVar : /* 简单变量 */

            /* 全局符号表 t 中查找符号 sym,返回绑定的 value.  v->u.simple: 简单变量的符号 */
            x = S_look(venv, v->u.simple);

            if(x == NULL){
                printf(" opps, S_look find no defined type for var : %s \n.", S_name(v->u.simple));
            }


            /* 符号 */
            if (x && x->kind == E_varEntry) {
				/*
                 返回一个转义的表达式的对象 expty {Tr_exp exp; Ty_ty ty;}
                 属性为表达式 Tr_exp exp 和 对应的类型 Ty_ty ty
                 return expTy(v->u.simple, actual_ty(x->u.var.ty));
                */ 
				return expTy(NULL, actual_ty(x->u.var.ty));
            } else {
                /*
                 值环境中不存在
                 或者存在 ，但绑定的值不是简单变量，说明一个符号应该是简单变量，但对应的值不是简单变量，报语义错误
                 */
                EM_error(v->pos, " undefined var %s", S_name(v->u.simple));

                /* 固定返回 int, 让编译器继续往下走。 */                
				return expTy(NULL, Ty_Int());
            }
            break;

            /*
             * var record (a.b) 域变量
             * 如 type intlist = {hd: int, tl: intlist}  intlist.hd
             * 其中 intlist.hd   intlist 对应 A_var var, hd 对应 S_symbol sym
             * */
        case A_fieldVar :
            printf(" field symbol : %s \n", S_name(v->u.field.sym));
            printf(" field var : %s \n", S_name(v->u.field.var->u.simple));

            /* 解析 var.symbol */
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

            printf(" sub script exp type : %d \n", v->u.subscript.exp->kind);

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
 * @return 返回结构体struct expty  {Tr_exp exp; Ty_ty ty;};
 */
static struct expty transExp(S_table v, S_table t, A_exp e) {

    /* 打印当前的表达式类型 */
    string* enums[16] = {"A_varExp", "A_nilExp", "A_intExp", "A_doubleExp", "A_stringExp", "A_callExp",
                     "A_opExp", "A_recordExp", "A_seqExp", "A_assignExp", "A_ifExp",
                     "A_whileExp", "A_forExp", "A_breakExp", "A_letExp", "A_arrayExp"};
    EM_pfun(e->pos,"=========================> transExp : %s \n", enums[e->kind]);

    E_enventry callinfo; /* 函数调用: funx(a,b) */
    Ty_ty recordty, arrayty;
    A_expList list;

    struct expty left, right, final, final2, final3, final4, final5, lo, hi;
    struct expty assVar, assExp;

    struct expty for_from, for_to, for_body;
    struct expty let_body;

    struct expty if_if, if_then, if_else;

    A_oper op;

    switch (e->kind) {

        /* 变量表达式 */
        case A_varExp:
            EM_pfun(e->pos,"====> A_varExp\n");
            return transVar(v, t, e->u.var);

            /* NIL 表达式 */
        case A_nilExp:
            EM_pfun(e->pos,"====> A_nilExp\n");
            return expTy(NULL, Ty_Nil());

            /* 调用表达式 */
        case A_callExp:
            EM_pfun(e->pos,"====> A_callExp\n");
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
        case A_recordExp:
            EM_pfun(e->pos,"====> A_recordExp\n");

            /* 在类型环境 tenv 中查找符号 e->u.record.typ 绑定的类型  */
            recordty = actual_ty(S_look(t, e->u.record.typ));

            if (!recordty) { /* 找不到 record 绑定的类型 */
                EM_error(e->pos, "can not find type def of record name: %s \n", S_name(e->u.record.typ));
            } else {
                if (recordty->kind != Ty_record) {
                    EM_error(e->pos, " %s should be a record type, but in fact it is not \n", S_name(e->u.record.typ));
                }

                /* 判断 record 中的 fields 的定义是否匹配 */
                if (efields_match(v, t, recordty, e)) {
                    return expTy(NULL, recordty);
                }
            }
            return expTy(NULL, Ty_Record(NULL));

        case A_arrayExp:
            EM_pfun(e->pos,"====> A_arrayExp\n");

            arrayty = actual_ty(S_look(t, e->u.array.typ));

            if ((!arrayty) || (arrayty->kind != Ty_array)) {
                EM_error(e->pos, "the exp should be array but it is not or undefined : %s \n", S_name(e->u.array.typ));
            }

            /* 判断数组大小，默认值类型 是否与数组定义一致*/
            final2 = transExp(v, t, e->u.array.size);
            final3 = transExp(v, t, e->u.array.init); /* 数组的默认值 如 int a[2]='test' */

            if (final2.ty->kind != Ty_int) {
                EM_error(e->pos, "array size should be int \n");
            } else if (!ty_match(final3.ty, arrayty->u.array)) {
                EM_error(e->pos, "array type unmatch \n");
            } else {
                return expTy(NULL, arrayty);
            }
            return expTy(NULL, Ty_Array(NULL));

            /* 表达式列表：按每一个表达式转义 */
        case A_seqExp:
            EM_pfun(e->pos,"====> A_seqExp\n");

            list = e->u.seq;

            if (!list) {
                return expTy(NULL, Ty_Void());
            }

            /* 遍历list, 排队添加入全局环境 */
            while (list->tail) {
                transExp(v, t, list->head);
                list = list->tail;
            }
            return transExp(v, t, list->head);

            /* while 表达式 */
        case A_whileExp:

            EM_pfun(e->pos,"====> A_whileExp\n");

            /* while() 中止条件判断，应该是 int (0, 1) */
            final = transExp(v, t, e->u.whilee.test);
            if (final.ty->kind != Ty_int) {
                EM_error(e->pos, "while() condition should be int 1/0 \n");
            }

            /* while 函数体则没有要求 */
            transExp(v, t, e->u.whilee.body);
            return expTy(NULL, Ty_Void());


            /* 赋值表达式  var := exp */
        case A_assignExp:

            EM_pfun(e->pos,"====> A_assignExp\n");

            assVar = transVar(v, t, e->u.assign.var);
            assExp = transExp(v, t, e->u.assign.exp);

            /* 判断赋值类型与定义类型是否一致 */
            if (!ty_match(assVar.ty, assExp.ty)) {
                EM_error(e->pos, "assign type is not same as defined. \n");
            }
            /* 赋值表达式没有返回值 */
            return expTy(NULL, Ty_Void());


            /* break表达式 */
        case A_breakExp:

            EM_pfun(e->pos,"====> A_breakExp\n");

            return expTy(NULL, Ty_Void());


            /* for表达式 */
        case A_forExp:

            EM_pfun(e->pos,"====> A_forExp\n");

            /* struct {S_symbol var; A_exp lo,hi,body; bool escape;} forr; */
            for_from = transExp(v, t, e->u.forr.lo);
            for_to = transExp(v, t, e->u.forr.hi);

            if ((for_from.ty->kind != Ty_int) || (for_to.ty->kind != Ty_int)) {
                EM_error(e->pos, "the from , to is not number. \n");
            }

            /* 进入值环境 v */
            S_beginScope(v);
            /* 声明 int i 的初始化 */
            transDec(v, t, A_VarDec(e->pos, e->u.forr.var, S_Symbol("int"), e->u.forr.lo));
            /* 函数体 */
            for_body = transExp(v, t, e->u.forr.body);
            S_endScope(v);

            return expTy(NULL, Ty_Void());


            /* let 声明表达式
             * let ...decs... in ...exp_body... end */
        case A_letExp:

            EM_pfun(e->pos,"====> A_letExp\n");

            /* 声明只在域scope内有效 */
            S_beginScope(v);
            S_beginScope(t);

            /* 转义所有的 let 的声明语句 */
            for (A_decList decs = e->u.let.decs; decs; decs = decs->tail) {
                transDec(v, t, decs->head);
            }

            /* 转义 let 的函数体 */
            let_body = transExp(v, t, e->u.let.body);

            S_endScope(t);
            S_endScope(v);
            return let_body;

            /* 双目表达式
             typedef enum {A_plusOp, A_minusOp, A_timesOp, A_divideOp,
             A_eqOp, A_neqOp, A_ltOp, A_leOp, A_gtOp, A_geOp} A_oper; */
        case A_opExp:

            EM_pfun(e->pos,"====> A_opExp\n");

            op = e->u.op.oper;  /* 符号 */
            left = transExp(v, t, e->u.op.left);
            right = transExp(v, t, e->u.op.right);
            A_exp rr = e->u.op.right;

            /* 加减乘除 +,-,*,/ */

            /* typedef enum {A_plusOp, A_minusOp, A_timesOp, A_divideOp, ...} A_oper; */
            if (1 <= op && op < 4) {

                if (left.ty->kind != Ty_int && left.ty->kind != Ty_double) {
                    EM_error(e->pos, "left num need to be int or double for oper: + - * / \n");
                }

                if (right.ty->kind != Ty_int && right.ty->kind != Ty_double) {
                    EM_error(e->pos, "right num need to be int or double for oper: + - * / \n");
                }

                /* 除法除0时报错 */
                if (op == 3 && rr->u.intt == 0) {
                    EM_error(e->pos, "no!!!!!! devide by zero !!!!!!!!!!!! \n");
                }

                if (left.ty->kind == Ty_int && right.ty->kind == Ty_int && op != 3) {
                    return expTy(NULL, Ty_Int());
                }
            } else if (3 < op && op < 10) {
                /* 比较等式 A_eqOp, A_neqOp, A_ltOp, A_leOp, A_gtOp, A_geOp */

                /* 等于/不等于 record 可以为 nil, 也就是説  type　record　和 nil 可以相等或不等 */
                if (op == 4 || op == 5) {
                    if ((left.ty->kind == Ty_record || left.ty->kind == Ty_nil) &&
                        (right.ty->kind == Ty_record || right.ty->kind == Ty_nil)) {
                        return expTy(NULL, Ty_Int());
                    }
                }

                /* 比较式左右的类型应该相等 */
                if (left.ty->kind != right.ty->kind) {
                    EM_error(e->pos, " the left exp type is not the same with right exp type, can not compare! \n");
                }

                return expTy(NULL, Ty_Int());
            } else {
                /* 未定义的运算符 */
                assert(0);
            }

            /* if表达式 */
        case A_ifExp:

            EM_pfun(e->pos,"====> A_ifExp \n");

            if_if = transExp(v, t, e->u.iff.test);
            if_then = transExp(v, t, e->u.iff.then);

            if (if_if.ty->kind != Ty_int) {
                EM_error(e->pos, "the exp in if() is not 1/0\n");
            }

            /* 包含 else 部分，返回类型要和 then 部分一致 */
            if (e->u.iff.elsee) {

                if_else = transExp(v, t, e->u.iff.elsee);
                if (!ty_match(if_then.ty, if_else.ty)) {
                    EM_error(e->pos, "return type of then and else should be the same.\n");
                }
            }
            return expTy(NULL, if_then.ty);


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
static void transDec(S_table v, S_table t, A_dec d) {

    EM_pfun(d->pos,"====> transDec\n");

    struct expty e;
    Ty_ty dec_ty;

    /* 函数声明 */
    A_fundec fdec;
    A_fundecList fdecl;
    Ty_tyList dec_fun_params_ty;
    Ty_ty fun_ty;
    Ty_tyList formalTys, ts; /* 形参 */

    A_fieldList fl;
    struct expty funbody;
    E_enventry fentry; /* 函数入口 */

    /* 类型声明 */
    A_nametyList nl;
    Ty_ty resTy, namety;
    bool useUndefype;

    switch (d->kind) {

        /* 声明变量
           vardec   →   var id := exp
                    →   var id : type-id := exp */
        case A_varDec:

            EM_pfun(d->pos,"Var name: %s \n", S_name(d->u.var.var));
            if(d->u.var.typ){
                EM_pfun(d->pos,"Var type : %s \n", S_name(d->u.var.typ));
            }
            e = transExp(v, t, d->u.var.init); /* 初始化 */
            if (!d->u.var.typ) { /* 未指明类型 var id := exp, 则 id 的类型就是 exp 的类型 */
                if (e.ty->kind == Ty_nil || e.ty->kind == Ty_void) { /* var a := void */
                    EM_error(d->pos, "no need to init to void/nil. \n");
                    /* e绑定为 var(symbol) <-> int */
                    S_enter(v, d->u.var.var, E_VarEntry(Ty_Int()));
                } else {
                    /* 将绑定 加入值环境 v, 值(符号)为d->u.var.var， 类型为E_VarEntry(e.ty) */
                    S_enter(v, d->u.var.var, E_VarEntry(e.ty));
                }
            } else { /* 指明类型 var id : type-id := exp */

                /* 在类型环境中找 type-id 对应的类型*/
                dec_ty = S_look(t, d->u.var.typ);
                if (!dec_ty) { /* 不存在 type-id 对应的类型声明 */
                    EM_error(d->pos, "undefined type %s. \n", S_name(d->u.var.typ));
                } else {
                    /* 判断 type-id 和 exp 的类型是否一样 */
                    if (!ty_match(dec_ty, e.ty)) {
                        EM_error(d->pos, "type-id and exp type is not same : %s. \n", S_name(d->u.var.typ));
                        S_enter(v, d->u.var.var, E_VarEntry(dec_ty));
                    } else {
                        S_enter(v, d->u.var.var, E_VarEntry(dec_ty));
                    }
                }
            }

            break;

            /* 声明函数
             fundec →   function id ( tyfields) = exp
                    →   function id ( tyfields) : type-id = exp
             只要判断函数返回值类型，参数类型 是否是存在的 type 即可
             */
        case A_functionDec:
            EM_pfun(d->pos,"loop functions dec.\n");

            /* 1.判断返回值 */
            for (fdecl = d->u.function; fdecl; fdecl = fdecl->tail) {

                EM_pfun(fdecl->head->pos,"func name: %s \n", S_name(fdecl->head->name));

                /* 有返回值 fdec->head->result(symbol) */
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

                /* 形参类型 */
                formalTys = makeFormalTyList(t, fdecl->head->params);

                /* 将函数参数类型 添加进全局类型环境 */
                S_enter(v, d->u.var.var, E_FunEntry(formalTys, fun_ty));
            }

            /* 2.判断形参 */
            EM_pfun(d->pos," start check the formals of function\n");

            for (fdecl = d->u.function; fdecl; fdecl = fdecl->tail) {
                fdec = fdecl->head;

                EM_pfun(fdec->pos," start check the formals of function for func: %s\n", S_name(fdec->name));

                S_beginScope(v);
                formalTys = makeFormalTyList(t, fdec->params);

                /* 遍历形参 */
                for (fl = fdec->params, ts = formalTys; fl && ts; fl = fl->tail, ts = ts->tail) {
                    /* S_enter(S_table: 值环境,
                     * S_symbol sym : fieldlist 中 field 的 name 符号,
                     * void *value : 形参); */
                    S_enter(v, fl->head->name, E_VarEntry(ts->head));
                }

                funbody = transExp(v, t, fdec->body);  /* 函数体 */
                fentry = S_look(v, fdec->name); /* 函数入口 */

                if (!ty_match(fentry->u.fun.result, funbody.ty)) {
                    EM_error(d->pos, "result type is not the same as the func body return type. \n");
                }

                S_endScope(v);
            }
            break;

        case A_typeDec: /* 声明自定义类型 */

            EM_pfun(d->pos," start A_typeDec \n");
            /* 类型定义
               type type-id = typee
               typee:
                    type-id
                    { type-fieldsopt }
                    array of type-id
                如：
                type mytype = {a:int, b:string}
                type arrtype = array of int (数组的个数不限制)
                var row := arrtype[10] of 0
                var row2 := arrtype[11] of 1 */

            /* d->u.type 为多个 namety A_nametyList， 每一个元素为一个 A_namety
             * 将多个类型声明的每一段 (nl->head) 添加进全局的 类型环境 t , */
            for (nl = d->u.type; nl; nl = nl->tail) {
                /*
                 将符号 name 添加进 类型环境 t 中，添加绑定: sym->value
                 void S_enter(S_table t, S_symbol sym, void *value);
                 这里 value 给个 NULL
                 */
                EM_pfun(d->pos," name ty:  %s \n", S_name(nl->head->name));
                S_enter(t, nl->head->name, Ty_Name(nl->head->name, NULL));
            }

            /* 假设 设置类型 {a: intt} */


            EM_pfun(d->pos," start loop namety list. \n");
            for (nl = d->u.type; nl; nl = nl->tail) {
                /* 转义 nl-> head 的类型 ， a:int 中的 int */
                resTy = transTy(t, nl->head->ty);

                /* 如果某一条声明语句对应的类型名字不存在，说明这种类型还没有声明，不能用它来声明新的类型
                 * 如   a: intt,  这里的 intt 应该已经声明过新类型名， 否则intt 不是Ty_name*/
                if (resTy->kind != Ty_name) {
                    EM_error(d->pos, " use undefined type: %s", S_name(resTy->u.name.sym));
                }

                /* 类型环境中查找 声明名字 a (符号) 对应的类型 */
                namety = S_look(t, nl->head->name);
                /* 将新建的类型名 a 的类型设置为对应的类型 intt */
                namety->u.name.ty = resTy;
            }
            break;

        default:
            printf("default of transDec");
            assert(0);
    }
}

/* 转义类型声明 a:intt ->  transTy(S_table t, A_ty int)
 * 返回类型声明部分 具体的类型 Ty_ty*/
static Ty_ty transTy(S_table tb, A_ty ty) {

    Ty_ty tt = NULL;
    Ty_fieldList fieldTys;

    switch (ty->kind) {

        case A_nameTy:  /* type mytype = intt 中的 intt, 判断是否是已经定义过的 类型*/
            tt = S_look(tb, ty->u.name);
            if (!tt) {
                EM_error(ty->pos, "undefined type : %s .\n", S_name(ty->u.name));
            }
            return tt;

        case A_recordTy:  /* type mytype = {a:intt, b:str} 中的 {a:intt, b:str}*/

            fieldTys = makeFieldTys(tb, ty->u.record);

            /* Ty_ty Ty_Record(Ty_fieldList fields); */
            return Ty_Record(fieldTys);

        case A_arrayTy:   /* type mytype = array of intt 中的 array of intt*/

            /* 查找符号 S_symbol array 对应的类型，就是要返回的类型 */
            tt = S_look(tb, ty->u.array);

            /* 如果 array of intt 中的 intt 没有定义，则报错 */
            if (!tt) {
                EM_error(ty->pos, "undefined array type: %s.\n", S_name(ty->u.array));
                return Ty_Array(tt);
            }
        default:
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
            if(tfl){
                /* 更新链表 Ty_fieldList tfl */
                tfl->tail = Ty_FieldList(tmp,NULL);
                tfl = tfl->tail;
            }else{
                /* 初始化链表 Ty_fieldList tfl， tail 为 NULL, Head 为第一个 */
                /* Ty_fieldList Ty_FieldList(Ty_field head, Ty_fieldList tail); */
                tfl = Ty_FieldList(tmp,NULL);
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

    for(; flist ; flist = flist->tail){
        /* 遍历每一个形参定义: "t: tree" */
        head = S_look(t, flist->head->typ);
        if(!head){
            EM_error(flist->head->pos, "undefined type: %s.\n", S_name(flist->head->typ));
        }
        /* 生成链表 */
        if(!listhead){
            listtail = Ty_TyList(head, NULL); /* 生成 head 类型对应的 tylist */
            listhead = listtail;
        }else{
            listtail->tail = Ty_TyList(head, NULL);
            listtail = listtail->tail;
        }
    }
    return listhead;
}


letExp (
    decList(
        /* 新建类型 type person = {name: string, age: int} */
        typeDec(
            nametyList(
                namety(
                    person,
                    recordTy(
                    fieldList(
                            field(name, string, TRUE),

                            fieldList (
                                field(age, int, TRUE),
                                fieldList() )
                            )
                    )
                ),
                nametyList()
            )
        ),

        decList (
                /* 声明 record 给变量 p1, var p1 := person {name="Joe", age=66} */
            varDec(
                p1,
                recordExp(
                   person,
                   efieldList(
                       efield(name, stringExp(Joe)),
                       efieldList(
                           efield(age, intExp(66)),
                           efieldList()
                       )
                   )
                ),
                TRUE),

            decList (
                /* 声明 record 给变量 p2, var p2: person := nil */
                varDec(p2, person, nilExp(), TRUE),
                decList()
            )
        )
    ),

    /* 执行部分 */
    seqExp(
            expList(
                /* print(p1.name); */
                callExp(
                    print,
                    expList(
                        /* 变量 p1.name */
                        varExp( fieldVar( simpleVar(p),name)),
                        expList())
                ),
                expList()
    ))
)


letExp (
decList(
        typeDec(
        nametyList(
                namety(person,
                recordTy(
                fieldList(
        field(name,
        string,
TRUE),

fieldList (
field(age,
      int,
TRUE),

fieldList()

)))),

nametyList()

)),

decList (
varDec(myrecord1,
       recordExp(person,
       efieldList(
       efield(name,
              stringExp(Bart)),
       efieldList(
       efield(age,
              intExp(

34)),

efieldList()

))),
TRUE),

decList (
varDec(myrecord2,
       recordExp(person,
       efieldList(
       efield(name,
              stringExp(Bart)),
       efieldList(
       efield(age,
              intExp(

34)),

efieldList()

))),
TRUE),

decList()

))),

seqExp(
        expList(
        opExp(
        EQUAL,
        varExp(
        fieldVar(
        simpleVar(myreqord1),
        name)),
        varExp(
        fieldVar(
        simpleVar(myrecord1),
        name)

)),

expList(
        opExp(
        NOTEQUAL,
        varExp(
        simpleVar(myreqord1)),
        varExp(
        simpleVar(myrecord2))),
        expList()

))))
