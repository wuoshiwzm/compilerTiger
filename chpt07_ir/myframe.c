#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 抽象语法
#include "table.h"
#include "util.h"
#include "symbol.h"
#include "types.h"

// tree
#include "tree.h"

// 栈帧
#include "temp.h"
#include "frame.h"
#include "myframe.h"
#include "translate.h"

// 帧指针 FP

// 全局帧指针（最新的栈帧）  (a.k.a. Base Pointer).
static Temp_temp fp = NULL;
// 获取当前 帧指针
Temp_temp F_FP(void){
    if(fp == NULL){
        // 初始化为 Temp_newtemp()
        fp = Temp_newtemp();
    }
    return fp;
}

// 栈指针
static Temp_temp sp = NULL;
Temp_temp F_SP(){
    if(sp == NULL){
        // 初始化为 Temp_newtemp()
        sp = Temp_newtemp();
    }
    return sp;
}

// 将数据添加进 frame 中， 生成这个访问 access，
static F_access InFrame(int offset){
    F_access acc = (F_access) checked_malloc(sizeof (struct F_access_));
    acc->kind = inFrame;
    acc->u.offset = offset;
    return acc;
}
// access 存于 reg 中
static F_access  InReg(Temp_temp reg){
    F_access acc = (F_access) checked_malloc(sizeof (struct F_access_));
    acc->kind = inReg;
    acc->u.reg = reg;
    return acc;
}

// 获取下一个位置，即 offset
static int getNextLoc(F_frame frame){
    return frame->offset;
}

static  Temp_temp  getNextReg(F_frame frame){
    // 局部变量个数++
    frame->locals++;
    return Temp_newtemp();
}

/*
 * 静态链
 */
static F_access static_link = NULL;

F_access F_staticLink(){
    // 静态链默认在栈帧中
    if (static_link == NULL){
        static_link = InFrame(0);
    }
    return static_link;
}

// ***frame 构造, 从形参开始***
F_frame F_newFrame(Temp_label funname, U_boolList formals){
//    printf(">>>new frame \n");

    // 局部变量都存在 栈帧中
    F_frame  f = (F_frame) checked_malloc(sizeof (struct F_frame_));
    f->begin_label = funname;
    f->locals = 0;
    f->offset = 0; // 为offset 为负数，在 frame pointer 基础上

    // f->formals 形参都存在栈帧中，不存入 register
    // access, accessList : 对应  F_formals  F_allocLocal  F_staticLink
    F_accessList accList = NULL;
    F_accessList accList_head = NULL;

    if (formals != NULL){
        for (;formals!= NULL;formals=formals->tail) {
            // 不断更新 tail 为当前的 accList
            // 给每个 formal 一个 F_accessList, head 是当前的 formal 是在 栈帧中， 还是寄存器中
            if (accList_head == NULL){
                accList = (F_accessList) checked_malloc(sizeof (struct F_accessList_));
                accList_head = accList;
            }else{
                accList->tail = (F_accessList) checked_malloc(sizeof (struct F_accessList_));
                accList = accList->tail;
            }

            // 更新 head
            // 形参 默认是在栈帧中，暂时不考虑在 寄存器存
            if(formals->head == TRUE){
                accList->head = InFrame(f->offset); // 当前的偏移量
                f->offset -= F_wordSize; // 形参进入栈帧，offset 后移一个单位 wordsize
            }else{
                // 寄存器中的情况...
            }
        }
        accList->tail = NULL;
    }
    f->formals = accList_head;
    return f;
}

// 返回栈帧的符号
Temp_label F_name(F_frame f){
    return f->begin_label;
}

F_accessList F_formals(F_frame f){
    return f->formals;
}

// 局部变量
// 请求在一个栈帧中分配一个局部变量，escape=true 非逃逸，返回 InFrame(-4) 则表示相对 FP 的位移
// 如果 escape=false，则有可能返回 InReg(txx) 表示在寄存器中，不占用栈帧
F_access F_allocLocal(F_frame f){
    F_access  acc;
    acc = InFrame(f->offset);
    f->offset -= F_wordSize;
    return acc;
}

T_stm F_procEntryExit1(F_frame frame, T_stm stm){
  return stm;
}



/**
 * 中间代码
 */

// 调用外部函数,如 printf, checked_malloc 等
T_exp F_externalCall(string funName, T_expList args){
  return T_Call(T_Name(Temp_namedlabel(funName)), args);
}

// 生成 access 的*访问地址*
T_exp F_Exp(F_access acc, T_exp framePtr){
  // 判断目标 access 在栈帧中 / 寄存器中
  if (acc->kind == inFrame){
    // 地址 = 栈帧地址 + offset(位移)
    return T_Mem(T_Binop(T_plus, framePtr, T_Const(acc->u.offset)));
  }else{
    // 寄存器
    return T_Temp(acc->u.reg);
  }
}

// 全局片段表 *fragList 为多个 F_fragList 的链表， 也就是说 全局变量*fragList 是链表的链表
static F_fragList *fragList = NULL;
static F_fragList fragList_head = NULL;

// 字符串片段
F_frag F_StringFrag(Temp_label label, string str){
  F_frag ff = = (F_frag) checked_malloc(sizeof (struct *ff));
  ff->kind = F_stringFrag;
  ff->u.stringg.label = label;
  ff->u.stringg.str = str;
  return ff;
}

// 流程片段
F_frag F_ProcFrag(T_stm body, F_frame frame){
  F_frag ff = = (F_frag) checked_malloc(sizeof (struct *ff));
  ff->kind = F_procFrag;
  ff->u.proc.body = stm;
  ff->u.proc.frame = frame;
  return ff;
}

F_fragList F_FragList(F_frag head, F_fragList tail){
  F_fragList fl = (F_fragList)checked_malloc(sizeof(*fl));
  fl->head = head;
  fl->tail = tail;
  return fl;
}

// 重置全局片段表, 返回 head
static F_frag* extendFragList(){
  //　创建指针变量
  if (fragList == NULL){
    fragList = (F_fragList*) checked_malloc(sizeof(F_fragList*));
  }
  // 指针变量赋值为一个 fragList 对象
  *fragList = (F_fragList) checked_malloc(sizeof (struct F_fragList_));

  if (fragList_head == NULL){
    fragList_head = *fragList;
  }

  // 把全局片段表的 head 拿出来返回，然后全局片段片更新为 NULL
  F_frag  *curf = &((*fragList)->head);

  // 除了 head 外剩于的片段表设为 NULL
  fragList = &((*fragList)->tail);
  *fragList = NULL;

  return curf;
}

// 更新当前片段为 string 片段
void F_String(Temp_label label, string str){
  F_frag *currentFrag = extendFragList();
  *currentFrag = F_StringFrag(label, str);
}



























// ******************************************************************************* //


/*  什么时候创建栈帧？

    frame.h: 存储器变量
    temp.h:  寄存器变量 ???
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
