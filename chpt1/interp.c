#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "util.h"
#include "prog1.h"
#include "interp.h"


/**
 * interpreter 解释器
 * 语句的解释器
 * 表达式的解释器：表达式返回整型 且 有副作用
 * 解释器本身模拟 直线语言 时不产生副作用。（print 由副作用实现）
 * 本程序采用链表存储元素和他们对应的值
 * 解释器就是要更新出这样一个最终的链表
 */

/**
 * Table 构造函数
 * 把树转化成这样的table, id | value | *tail(指向 下一个 table 结构体)
 * tail 为指向一个  struct table 的指针
 */
Table_ Table(string id, int value, struct table *tail) {
    Table_ t = malloc(sizeof(*t));
    t->id = id;
    t->value = value;
    t->tail = tail;
    return t;
}

/**
 * IntAntTable 构造
 * i: 当前的值
 * t: 赋值表
 */
IntAndTable_ IntAndTable(int i , Table_ t){
    IntAndTable_ it = checked_malloc(sizeof(*it));
    it->i = i;
    it->t = t;
    return it;
}

/**
 * 解释表达式
 */
IntAndTable_ interpExp(A_exp e, Table_ t) {
    if(e->kind == A_idExp){
        printf("A_idExp %s \n", e->u.id);
        return IntAndTable(lookup(t,e->u.id),t);
    }

    if(e->kind == A_numExp){
        printf("A_numExp:  %d \n",e->u.num);
        return IntAndTable(e->u.num,t);
    }

    if(e->kind == A_eseqExp){
        t = interpStm(e->u.eseq.stm,t);
        printf("A_eseqExp \n");
        return interpExp(e->u.eseq.exp, t);
    }

    // 运算操作表达式，这里有 A_exp left; A_binop oper; A_exp right 3 个元素
    if(e->kind == A_opExp){
        IntAndTable_ itableTemp;
        printf("A_opExp %d \n", e->u.op.oper);

        int lval, rval;
        // 计算左表达式的值
        itableTemp = interpExp(e->u.op.left, t);
        lval = itableTemp->i;
        printf("calc lval : %d \n",lval);

        // 计算右表达式的值
        itableTemp = interpExp(e->u.op.right, itableTemp->t);
        rval = itableTemp->i;
        printf("calc rval : %d \n",rval);

        // 左侧值，右侧值都有了， 这里就可以计算他们的  +-*/ 了
        int value;
        switch (e->u.op.oper) {
            case A_plus:
                value = lval + rval; break;
            case A_minus:
                value = lval - rval; break;
            case A_times:
                value = lval * rval; break;
            case A_div:
                if (rval == 0){
                    printf("div by 0 !");
                    value = 9999;
                }else{
                    value = lval / rval;
                }
                break;
            default:
                assert(!"doing operation A_exp->u.op.oper failed !");
        }
        return IntAndTable(value,itableTemp->t);
        printf("start to calculate, the oper is :: %d, the value is: %d \n", e->u.op.oper,itableTemp->i);
    }
    printf("no oper found ...");
    return NULL;
}

/**
 * 解释 explist
 */
IntAndTable_ interpExpList(A_expList eList, Table_ t) {
    IntAndTable_ itable;

    // 递归更新 itable
    if(eList->kind == A_pairExpList){
        itable = interpExp(eList->u.pair.head, t);
        itable = interpExpList(eList->u.pair.tail, itable->t);
    }

    if(eList->kind == A_lastExpList){
        itable = interpExp(eList->u.last, t);
    }
    return itable;
}

// 解释语句, 递归更新赋值表 table
Table_ interpStm(A_stm stm, Table_ table) {
    printf("interpStm called \n");
    IntAndTable_ itable;

    // 赋值语句 id:= exp, 更新赋值表
    if(stm->kind == A_assignStm){
        itable = interpExp(stm->u.assign.exp,table);
        // id:= itable->i 对应的最新值
        printf("assign to id: %s , value is %d \n", stm->u.assign.id, itable->i);
        table = update(itable->t,stm->u.assign.id,itable->i);
    }

    // print 语句
    if(stm->kind == A_printStm){
        itable = interpExpList(stm->u.print.exps,table);
        table = itable->t;
        printf("print statement , value : %d \n", itable->i);
    }

    if(stm->kind == A_compoundStm){
        printf("compound stm  \n");
        // 更新 table
        table = interpStm(stm->u.compound.stm1,table);
        table = interpStm(stm->u.compound.stm2,table);
    }
    return table;
}


// 更新一个表中的某个结点的值 （比如把 a 的值从4 更新为7）,从头往下一个个找，找到就更新, 找不到就添加
// 并返回更新后的 table
Table_ update(Table_ t, string id, int value){
//    printf("calling update , lookup res: %s \n", t->id);
    // 如果能找到就更新
    if(lookup(t,id) != -1){
        t = doUpdate(t,id,value);
        return t;
    }else{
        // 找不到就在开头添加
        Table_  table = Table(id,value,t);
        table->tail = t;
        return table;
    }
}
Table_ doUpdate(Table_ t, string id, int value){
    if(t->id == id){
        t->value = value;
    }

    if(t->tail != NULL){
        t->tail = doUpdate(t->tail,id,value);
    }
    return t;
}


// 查询 Table_ 表中有没有 key 的对应的值，有就返回他的 value，没有就返回 -1，
// 这里只查询， 和 update 没关系
int lookup(Table_ t, string key){
    printf("calling lookup \n");
    // 表无内容，不需要判断
    if(t == NULL){
        return -1;
    }
    if(t->id == key){
        return t->value;
    }
    if(t->tail != NULL){
        return lookup(t->tail,key);
    }
    return -1;
}

// 入口解释 stm,
void interp(A_stm stm) {
    printf("interp start \n");
    Table_ table = NULL;
    // 递归更新 table 数据结构
    table = interpStm(stm, table);

    printf("the final table: \n");
    printTable(table);
    printf("done");

}


void printTable(Table_ t){
    if( t != NULL){
        printf("id: %s, value : %d \n",t->id, t->value);
        if(t->tail != NULL){
            printTable(t->tail);
        }
    }
}
