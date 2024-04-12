/**
    Created by l30025288 on 2024/4/10.
*/


#ifndef CHPT07_IR_TREE_H
#define CHPT07_IR_TREE_H

// for test
#include "temp.h"

typedef enum{T_eq, T_ne, T_lt, T_gt, T_le, T_ge, T_ult, T_ule, T_ugt, T_uge} T_relOp;  // 比较运算
typedef enum{T_plus, T_minus, T_mul, T_div, T_and, T_or, T_lshift, T_rshift, T_arshift, T_xor} T_binOp; // 二元运算

/*
 * 语句
 */
typedef struct T_stm_ *T_stm;
struct T_stm_{
    enum {T_SEQ, T_LABEL, T_JUMP, ..., T_EXP} kind;
    union {
        struct {T_stm left, right;} SEQ;
        struct {S_symbol name, ... } LABEL;
        // ...

    }u;
};
typedef struct T_stmList_ *T_stmList;
struct T_stmList_{T_stm head; T_stmList tail;};
T_stmList T_StmList(T_stm head, T_stmList tail;);

/*
 * 表达式
 */
typedef T_exp_ *T_exp;
struct T_exp_{
    enum {T_BINOP, T_MEM, T_TEMP, ..., T_CALL} kind; // 表达式类型
    union {
        struct {T_binOp op; T_exp left, right;} BINOP;
        // ...
    }u;
};
typedef struct T_expList_ *T_expList;
struct T_expList_{T_exp head; T_expList tail;};
T_expList T_ExpList(T_exp head, T_expList tail);


/*
 * 语句 stm
 */

// SEQ(s1, s2) 语句 s1后跟 s2
T_stm T_Seq(T_stm left, T_stm right);

// LABEL(name) 定义 name 的常数值为当前机器代码地址  相当于汇编中的 标号
T_stm T_Label(Temp_label label);

// 按 label 跳转
// JUMP(e, labs)  将控制转移到地址 e, 如分支语句
T_stm T_Jump(T_exp exp, Temp_labelList labels);

// 条件跳转
// CJUMP(o, e1, e2, t, f) 计算 "e1 op e2" 为真时跳转 true, 为假时跳转 false
T_stm T_Cjump(T_relOp op, T_exp left, T_exp right, Temp_label true, Temp_label false );

// MOVE(TEMP t, e): 计算e的结果，送入临时单元 t
// MOVE(T_Mem(exp1), exp2): 计算 exp1,得到地址 a, 计算exp2 并将结果存储在从地址 a开始的 wordSize个字节的存储单元中
T_exp T_Move(T_exp exp1, T_exp exp2);

// EXP(e) 计算 e 并忽略结果
T_stm T_Exp(T_exp exp);

/*
 * 表达式 exp
 */
// BINOP(op, e1, e2) 二元操作 op，先算 left, 后算 right, 最后算 "e1 op e2"
T_exp T_Binop(T_binOp op, T_exp left, T_exp right);

// MEM(e) 存储地址 e + wordSize(Frame 模块定义)个字节，  Move(T_Mem(exp1), exp2) 表示对T_Mem(exp1)存储， 其他位置表示读取
T_exp T_Mem(T_exp exp);

// 临时变量 相当于寄存器，但可以有无限多个
T_exp T_Temp(Temp_temp temp);

// exp 序列
// ESEQ(s,e) 先计算 s　作为副作用，再计算 e 做为表达式的结果
T_exp T_Eseq(T_stm stm, T_exp exp);

// 符号常数
// NAME(name)
T_exp T_Name(Temp_label name);

// 整型常数
T_exp T_Const(int num);

// CALL(f, l) 以参数表 l 调用函数 f
T_exp T_Call(T_exp head, T_expList tail);

#endif //CHPT07_IR_TREE_H
