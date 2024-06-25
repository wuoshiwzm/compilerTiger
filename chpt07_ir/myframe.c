#include <stdio.h>
// 抽象语法
#include "util.h"
#include "symbol.h"

// 栈帧
#include "temp.h"

// tree
#include "tree.h"
#include "printtree.h"

#include "frame.h"
#include "myframe.h"

int F_wordSize = 4;

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

// 静态链，初始化时就是全局的栈帧（最新的）
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


/************* 中间代码， 片段 frag *************/

// 调用外部函数,如 printf, checked_malloc 等
T_exp F_externalCall(string funName, T_expList args){
  return T_Call(T_Name(Temp_namedlabel(funName)), args);
}

// 生成 access 的*访问地址*
T_exp F_Exp(F_access acc, T_exp framePtr){
  // 判断目标 access 在栈帧中 / 寄存器中, 栈帧中则在存储中找对应地址，寄存器没有地址，直接找
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
  F_frag ff = (F_frag) checked_malloc(sizeof (struct F_frag_));
  ff->kind = F_stringFrag;
  ff->u.stringg.label = label;
  ff->u.stringg.str = str;
  return ff;
}

// 流程片段
F_frag F_ProcFrag(T_stm body, F_frame frame){
  F_frag ff = (F_frag) checked_malloc(sizeof (struct F_frag_));
  ff->kind = F_procFrag;
  ff->u.proc.body = body;
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

// 返回全局片段链表
F_fragList F_getFragList(){
  return fragList_head;
}

// 将对应栈帧添加进全局片段表
T_stm F_procEntryExit1(F_frame frame, T_stm stm){
  // 先不做更新， 后面补上 todo ...
  return stm;
}

void F_printFrag(F_frag frag){
  if(frag->kind == F_stringFrag){
    printf("String: label = %s; content = \"%s\"\n", S_name(frag->u.stringg.label), frag->u.stringg.str);
  } else {
    printf("Proc: locals = %d\n", frag->u.proc.frame->locals);
    printf("      offset = %d\n", frag->u.proc.frame->offset);
    printStmList(stdout, T_StmList(frag->u.proc.body, NULL));
  }
}

