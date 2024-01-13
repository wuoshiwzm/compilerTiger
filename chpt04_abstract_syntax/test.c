//
// Created by l30025288 on 2023/10/11.
//




A_var A_SimpleVar(A_pos pos, S)


typedef  struct A_var_ *A_var;
struct A_var_
{
    enum {A_simpleVar, A_fieldVar, A_subscriptVar} kind;
    A_pos pos;
    union {
        S_symbol simple;
        struct {
            A_var var;
            S_symbol sym;
        } field;
        struct {
            A_var var;
            A_exp exp;
        } bscript;
    } u;
};


// 用到的抽象语法：
// 表达式序列 A_SeqExp 构造:      A_exp A_SeqExp(A_pos pos, A_expList seq)
// 表达式链表:                   A_ExpList(A_exp head, A_expList tail)
// 赋值表达式:                   A_AssignExp()
// 运算表达式:                   A_OpExp()
// 变量表达式:                   A_VarExp( A_pos pos, A_var var )
A_SeqExp(
        2, // 位置
        A_ExpList( // 表达式列 A_expList seq
                A_AssignExp( // expList head; 赋值表达式  "a := 5"
                        4,//pos 位置
                        A_SimpleVar(2,S_Symbol("a")), // 位置 2 上的 a
                        A_IntExp(7,5) // 位置 7 上的 5
                        ),
                A_ExpList(  // expList tail
                        A_OpExp( // 表达式 "a+1"
                                11, // pos 位置
                                A_plusOp, // 加号  "+"
                                A_VarExp( A_SimpleVar(10,S_Symbol("a")) ), // 变量"a"
                                A_IntExp(12, 1) // 数字 “1”
                                ),
                        NULL // explist 这里已经到 explist 最后，tail 为 null
                        )
                ),
        )
