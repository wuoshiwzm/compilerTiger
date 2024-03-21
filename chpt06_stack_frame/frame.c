
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "frame.h"
#include "escape.h"
#include "translate.h"



/*  什么时候创建栈帧？

    frame.h: 存储器变量
    temp.h:  寄存器变量
*/


/*  什么变量会进栈帧？
    寄存器： 不进栈帧
    存储： 进栈帧
    函数的参数， 返回地址，结果  很多局部变量 都由寄存器处理，
    只有一些特殊情况会将变量的值写入存储，也就是栈帧中

    1.  传址变量：变量为一地址  ( &p )
    2.  该变量被嵌套在当前过程（函数）内的过程（其他函数）访问
    3.  变量值太大，不能放在单个寄存器中 （ 32位，4字节 或 64位, 8字节 ）
    4.  变量是一个数组，元素需要地址运算（C语言）
    5.  需要使用存放该变量的寄存器
    6.  太多局部变量 临时变量， 无法全部放入寄存器中
    7.  函数参数大于k个 （k一般为 4 到 5个）
*/

struct F_frame_
{
    // 函数的栈帧标识符
    S_symbol name;

    // 父帧指针，调用它的函数的栈帧
    F_frame parentFp;

    // 局部变量
    F_accessList locals;

    // 实参..., 形参已经在抽象语法中定义了，栈帧只需要定义实参，实参保证用连续地址
    F_accessList params;

    // 返回地址 调用函数f的 call指令地址+1
    void *retAddr;

    // 临时变量
    Temp_tempList temps;

    // 静态链
    int staticLink;

    // 是否逃逸 （暂时所有参数都当成逃逸的，入栈帧）
    U_boolList escapes;
    // 保护的寄存器 (暂时没用)


};

struct F_access_ 
{
    enum {inFrame, inReg} kind; // 一个调用 是在栈帧中， 还是在 寄存器中 
    union{
        int offset; // 栈帧中， 则看偏移量 offset
        Temp_temp reg;  // 寄存器，则看临时寄存器变量 Temp_temp
    } u;
};


// 文件外不可见，F_access 不对外暴露 
static F_access InFrame(int offset); // 指出一个相对帧指针 FP 偏移量 offset 的存储位置
static F_access InReg(Temp_temp reg); // 指出使用寄存器 reg

 

// 创建一个栈帧
F_frame F_newFrame(Temp_label funname, U_boolList formals){

    F_frame f = checked_malloc(sizeof(*f));
    f->name = funname;

    // 获取调用函数的信息

    // 目前所有参数都当成逃逸的，全都进栈帧
    f->escapes = formals;

    return f;
}



// 请求在一个栈帧中分配一个局部变量，escape=true 非逃逸，返回 InFrame(-4) 则表示相对 FP 的位移
// 如果 escape=false，则有可能返回 InReg(txx) 表示在寄存器中，不占用栈帧
F_access F_allocLocal(F_frame f, bool escape){




}




 
