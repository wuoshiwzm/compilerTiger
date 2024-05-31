#ifndef _TRANSLATE_H_
#define _TRANSLATE_H_


/*
 * 结果体
 */
// translate 相对于 frame 模块， 增加了 Level 概念，对应函数嵌套
typedef struct Tr_exp_ *Tr_exp;

//  --------------------------- 栈帧部分 ---------------------------  //
typedef struct Tr_level_ *Tr_level;

// 带 level 版的 accessList, formals, allocLocal
typedef struct Tr_access_ *Tr_access;
typedef struct Tr_accessList_ *Tr_accessList;
struct Tr_accessList_{
    Tr_access head;
    Tr_accessList tail;
};

struct Tr_exp_ {
    enum {Tr_ex, Tr_nx, Tr_cx} kind;
    union {
        T_exp ex;
        T_stm nx;
        struct Cx cx;
    } u;
};

struct Cx {
  patchList trues;
  patchList falses;
  T_stm stm;
};

// 真/值 标号回填表
typedef struct patchList_ *patchList;
struct patchList_{ Temp_label *head; patchList tail;};
static patchList PatchList(Temp_label *head, patchList tail);

// 构造函数
Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

// 最外层的函数
Tr_level Tr_outermost(void);

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);

Tr_level Tr_getParent(Tr_level level);

void Tr_printLevel(Tr_level level);

// access
Tr_accessList Tr_formals(Tr_level level);

//Tr_access Tr_allocLocal(Tr_level level, bool escape); escape 默认为 TRUE, 直接进栈帧
Tr_access Tr_allocLocal(Tr_level level);

void Tr_printAccess(Tr_access access);

/*
 * 翻译中间代码
 */
// 构造函数，转换为 Tr_...
static Tr_exp Tr_Ex(T_exp exp);
static Tr_exp Tr_Nx(T_stm nx);
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm);

// 反构造，剥离 Tr_...
static T_exp unEx(Tr_exp exp);
static T_stm unNx(Tr_exp exp);
static struct Cx unCx(Tr_exp exp);

// 更新片段表， 记住一个过程的片段 ProcFrag
void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals);



// 下面就是本章主要实现的内容： 将不同类型的表达式 翻译成 Tr_exp
Tr_exp Tr_nilExp();

// 值
Tr_exp Tr_stringExp(string str);
Tr_exp Tr_intExp(int val);
Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init);
Tr_exp Tr_voidExp(void);

// record 类型
Tr_exp Tr_recordExp(int cnt);
void Tr_recordExp_app(Tr_exp te, Tr_exp init, bool last);

// 运算
Tr_exp Tr_arithExp(A_oper oper, Tr_exp left, Tr_exp right); // 二元算术运算 +-*/
Tr_exp Tr_logicExp(A_oper oper, Tr_exp left, Tr_exp right, bool isStrCompare); // 二元不等运算，<, >, <= ...

// 变量
Tr_exp Tr_simpleVar(Tr_access access, Tr_level level); // 简单变量
Tr_exp Tr_fieldVar(Tr_exp base, int field_offset);  // 字段变量 a, 通常与 struct , class 相关，a.b, cls.fld, ...
Tr_exp Tr_subscriptVar(Tr_exp base, Tr_exp offset_exp); // a[name], name 不一定是数字

// 声明 & 赋值
Tr_exp Tr_assignExp(Tr_exp lval, Tr_exp rval);

// 表达式
Tr_exp Tr_seqExp(Tr_exp* array, int size);
Tr_exp Tr_callExp();

// 循环 & 判断
Tr_exp Tr_forExp();
Tr_exp Tr_whileExp();
Tr_exp Tr_breakExp();
Tr_exp Tr_ifExp(Tr_exp cond, Tr_exp trues, Tr_exp falses);



Tr_exp Tr_letExp();


Tr_exp Tr_();
Tr_exp Tr_();
Tr_exp Tr_();
Tr_exp Tr_();

// ？？？
void Tr_genLoopDoneLabel();
void Tr_recordExp_app(Tr_exp te, Tr_exp init, bool last);
void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals, Temp_label label);

// 函数返回值的片段， 存在 Translate 的 片段表中
F_fragList Tr_getResult(void);

// 调试
void Tr_printTree(Tr_exp exp);
void Tr_printTrExp(Tr_exp exp);
void Tr_printResult();

#endif
