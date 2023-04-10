#include "util.h"
#include "slp.h"


//  A_stm ：指向 A_stm_ 数据结构基类的指针


A_stm A_CompoundStm(A_stm stm1, A_stm stm2) {
    // 获取地址指针
    A_stm s = checked_malloc(sizeof *s);
    // 指针 kind 赋值
    s->kind = A_compoundStm;
    // 指针 union 赋值，
    // 复合语句，每个子语句都是一个语句
    s->u.compound.stm1 = stm1;
    s->u.compound.stm2 = stm2;
    return s;
}

// 赋值语句 id := exp, id 是变量， exp 是要赋的值，也是一个表达式
A_stm A_AssignStm(string id, A_exp exp) {
    A_stm s = checked_malloc(sizeof *s);
    s->kind = A_assignStm;
    s->u.assign.id = id;
    s->u.assign.exp = exp;
    return s;
}

// 打印语句 print(explist) , 打印内容是多个表达式
A_stm A_PrintStm(A_expList exps) {
    A_stm s = checked_malloc(sizeof *s);
    s->kind = A_printStm;
    s->u.print.exps = exps;
    return s;
}

// 标识符
A_exp A_IdExp(string id){
    A_exp  e = checked_malloc(sizeof *e);
    e->kind = A_idExp;
    e->u.id = id;
    return e
}


A_exp A_NumExp(int num) {
    A_exp e = checked_malloc(sizeof *e);
    e->kind = A_numExp;
    e->u.num = num;
    return e;
}

// 操作符表达式，左侧，表达式本式，右侧
A_exp A_OpExp(A_exp left, A_binop oper, A_exp right) {
    A_exp e = checked_malloc(sizeof *e);
    e->kind = A_opExp;
    e->u.op.left = left;
    e->u.op.oper = oper;
    e->u.op.right = right;
    return e;
}

A_exp A_EseqExp(A_stm stm, A_exp exp) {
    A_exp e = checked_malloc(sizeof *e);
    e->kind = A_eseqExp;
    e->u.eseq.stm = stm;
    e->u.eseq.exp = exp;
    return e;
}

// "exp, explist"
A_expList A_PairExpList(A_exp head, A_expList tail) {
    A_expList e = checked_malloc(sizeof *e);
    e->kind = A_pairExpList;
    e->u.pair.head = head;
    e->u.pair.tail = tail;
    return e;
}

A_expList A_LastExpList(A_exp last) {
    A_expList e = checked_malloc(sizeof *e);
    e->kind = A_lastExpList;
    e->u.last = last;
    return e;
}
