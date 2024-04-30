
#include <assert.h>
#include "util.h"
#include "tree.h"
#include "translate.h"
#include "translate.c"

/*
struct Tr_exp_ {
    enum {Tr_ex, Tr_nx, Tr_cx} kind;
    union {
        T_exp ex;
        T_stm nx;
        struct Cx cx;
    } u;
};
 */
// 条件结构体 struct Cx {patchList trues; patchList falses; T_stm stm};
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
            T_exp er = T_Temp(r) // 返回值

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
static T_stm unNx(Tr_exp exp){
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



// struct Cx {patchList trues; patchList falses; T_stm stm}
// T_stm T_Cjump(T_relOp op, T_exp left, T_exp right, Temp_label true, Temp_label false) 为真时跳转 true，否则 false
static struct Cx unCx(Tr_exp exp){
    struct Cx cx = (Cx)checked_malloc(sizeof (struct Cx));
    Temp_label t = Temp_newlabel(), f = Temp_newlabel();

    struct Cx cx;
    switch (exp->kind) {
        case Tr_ex:
            // cx.stm = T_Exp(exp.u.ex);
            cx.stm = T_Cjump(T_ne, exp->u.ex, T_Const(0),NULL, NULL);
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


/*
 * patchList 函数
 */
// 将标记 label 填充到 真/假值回填表中, 把patchList 中每一个标号都换成 label
void doPatch(patchList list, Temp_label label) {
    for (; list; list = list->tail) {
        *(list->head) = label;
    }
}

// 连接两个 patchList
patchList joinPatch(patchList first, patchList second) {
    if (!first) return second;
    for (; first->tail; first = first->tail); // 找到 first 的尾端
    first->tail = second;
    return first;
}




