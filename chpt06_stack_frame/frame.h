#ifndef COMPILER_TIGER_MASTER_FRAME_H
#define COMPILER_TIGER_MASTER_FRAME_H


/*
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
//F_access F_allocLocal(F_frame f, bool escape);
F_access F_allocLocal(F_frame f);

// 静态链
F_access F_staticLink();

#endif //COMPILER_TIGER_MASTER_FRAME_H


