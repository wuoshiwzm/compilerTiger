#ifndef COMPILER_TIGER_MASTER_FRAME_H
#define COMPILER_TIGER_MASTER_FRAME_H

/*
    栈帧 + 片段
    栈帧存什么？    实参， 局部变量
    栈帧在哪里？    SP + offset
    什么时候决定放寄存器/栈帧？ 一开始统一存放在 临时位置，晚些再决定哪些变量放在寄存器中，哪些放在栈帧中

    定义类型
    F_frame: 栈帧
    F_access: "访问"， 这个访问可以是形参，也可以是局部变量
        access to formal: 形参
        access to alloclocal: 局部变量
*/
typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access;
typedef struct F_accessList_ *F_accessList;

// 定义+声明
struct F_accessList_ {
    F_access head;
    F_accessList tail;
};

// 构造新栈帧
F_frame F_newFrame(Temp_label funname, U_boolList formals);

// 生成栈帧的标识符
Temp_label F_name(F_frame f);

// 获取函数形参的访问 （注意这里是 list， 因为有多个形参）
F_accessList F_formals(F_frame f);

// 获取对某个局部变量的访问 escape表示是否逃逸 bool escape 暂时默认为 true, 存入 frame
// F_access F_allocLocal(F_frame f, bool escape);
F_access F_allocLocal(F_frame f);

// 静态链
F_access F_staticLink();

/*
 * IR 中间代码
 */
// 调用外部函数
T_exp F_externalCall(string s, T_expList args);

// 生成 access 的*访问地址*
T_exp F_Exp(F_access acc, T_exp framePtr);

// 帧指针寄存器
Temp_temp F_FP(void);
// 一个值是机器字大小的常数
extern const int F_wordSize = 4;

// 片段
typedef struct F_frag_ *F_frag;
struct F_frag_{
  enum { F_stringFrag, F_procFrag } kind; // 字符 / 语句
  union {
    struct { Temp_label label; string str; } stringg; // 符号 + 字符体
    struct { T_stm body; F_frame  frame; } proc; // 栈帧 + 函数体
  }u;
};

// 片段 构造函数
F_frag F_stringFrag(Temp_label label, string str);
F_frag F_procFrag(T_stm body, F_frame frame);

// 片段 list
typedef struct F_fragList_ *F_fragList;
struct F_fragList_ { F_frag head; F_fragList tail };
F_fragList F_FragList( F_frag head, F_fragList tail );

// 字符串栈帧
void F_String(Temp_label label, string str);


#endif //COMPILER_TIGER_MASTER_FRAME_H


