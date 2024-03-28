#ifndef COMPILER_TIGER_MASTER_MYFRAME_H_
#define COMPILER_TIGER_MASTER_MYFRAME_H_

struct F_frame_ {
    int offset; // 偏移量 什么的偏移量？
    int locals; // 局部变量在栈帧中占用的空间

    F_accessList formals; // 函数形参
    Temp_label begin_label; // 函数的栈帧标识符 frame name ?

    // 返回地址， 临时变量， 静态链

};

struct F_access_ {
    enum {
        inFrame, inReg
    } kind; // 一个调用 是在栈帧中， 还是在 寄存器中
    union {
        int offset; // 栈帧中， 则看偏移量 offset
        Temp_temp reg;  // 寄存器，则看临时寄存器变量 Temp_temp
    } u;
};
#endif
