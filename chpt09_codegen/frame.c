/*
 * CS5161 Assignment 5
 * 
 * Ming Zhou 4465225
 * 
 * Activation Record Module - MIPS architecture
 */

#include <stdio.h>
#include <stdlib.h>
#include "table.h"
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "printtree.h"
#include "translate.h"

#define PARA_REG_NUM 4
#define SAVE_REG_NUM 8
#define TEMP_REG_NUM 10

const int F_wordSize = 4;

/**
 * 1. 初始化寄存器 ，如返回值 rv, 栈帧 fp, 栈指针 sp 等
 */
//Get the frame pointer
static Temp_temp fp = NULL;
Temp_temp F_FP(){
	if(fp==NULL){
		fp = Temp_newtemp();
		printf("FP pointer initialized: %s\n", Temp_look(Temp_name(), fp));
	}
	return fp;
}

//Get the return value register
static Temp_temp ra = NULL;
Temp_temp F_RA(){
	if(ra==NULL){
		ra = Temp_newtemp();
		//printf("RA pointer initialized: %s\n", Temp_look(Temp_name(), ra));
	}
	return ra;
}

//Get the return value register
static Temp_temp rv = NULL;
Temp_temp F_RV(){
	if(rv==NULL){
		rv = F_VN(0);
		//printf("RV pointer initialized: %s\n", Temp_look(Temp_name(), rv));
	}
	return rv;
}

//Get the stack register
static Temp_temp sp = NULL;
Temp_temp F_SP(){
	if(sp==NULL){
		sp = Temp_newtemp();
		//printf("SP pointer initialized: %s\n", Temp_look(Temp_name(), sp));
	}
	return sp;
}

//Get the zero constant register
static Temp_temp zero = NULL;
Temp_temp F_ZERO(){
	if(zero==NULL){
		zero = Temp_newtemp();
		//printf("ZERO pointer initialized: %s\n", Temp_look(Temp_name(), zero));
	}
	return zero;
}

/**
 * 2. SN、TN、AN 和 VN 通常代表不同的编译器内部符号或标识符
 */
static Temp_temp sn[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static Temp_temp tn[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static Temp_temp an[4] = {NULL, NULL, NULL, NULL};
static Temp_temp vn[2] = {NULL, NULL};
Temp_temp F_SN(int i){
	if(0<=i && i<=7) {
		if(sn[i]==NULL){
			sn[i] = Temp_newtemp();
		}
		return sn[i];
	} else {
		printf("Internal error: using a pointer that is not available for MIPS.");
		exit(2);
	}
}
Temp_temp F_TN(int i){
	if(0<=i && i<=9) {
		if(tn[i]==NULL){
			tn[i] = Temp_newtemp();
		}
		return tn[i];
	} else {
		printf("Internal error: using a pointer that is not available for MIPS.");
		exit(2);
	}
}
Temp_temp F_AN(int i){
	if(0<=i && i<=3) {
		if(an[i]==NULL){
			an[i] = Temp_newtemp();
		}
		return an[i];
	} else {
		printf("Internal error: using a pointer that is not available for MIPS.");
		exit(2);
	}
}
Temp_temp F_VN(int i){
	if(0<=i && i<=1) {
		if(vn[i]==NULL){
			vn[i] = Temp_newtemp();
		}
		return vn[i];
	} else {
		printf("Internal error: using a pointer that is not available for MIPS.");
		exit(2);
	}
}

Temp_tempList F_SAVES(){
  static Temp_tempList saves = NULL;
  if (!saves){
    Temp_tempList rear;
    saves = rear = Temp_TempList(Temp_newtemp(), NULL);
    for (int i = 0; i < SAVE_REG_NUM; ++i) {
      rear = rear->tail = Temp_TempList(Temp_newtemp(), NULL);
    }
  }
  return saves;
}


/**
 * 3. 将之前生成的寄存器地址 映射对应的字符串，如 F_FP() -> String("$fp"))
 */
static Temp_map _F_tempMap = NULL;
Temp_map F_tempMap(){
	if(_F_tempMap==NULL){
		_F_tempMap = Temp_empty();
		Temp_enter(_F_tempMap, F_RV(), String("$v0"));	//$v0 and $v1 ($2 and $3) are for returned values
		Temp_enter(_F_tempMap, F_RA(), String("$ra"));	//$31
		Temp_enter(_F_tempMap, F_FP(), String("$fp"));	//$30
		Temp_enter(_F_tempMap, F_SP(), String("$sp"));	//$29
		Temp_enter(_F_tempMap, F_ZERO(), String("$zero"));	//$0, constant 0
		int i;
    // sn
		for(i=0;i<8;i++){
			char* s = checked_malloc(8*sizeof(char)); 
			sprintf(s, "$s%d", i);
			Temp_enter(_F_tempMap, F_SN(i), s);
		}
    // tn
		for(i=0;i<10;i++){
			char* s = checked_malloc(8*sizeof(char)); 
			sprintf(s, "$t%d", i);
			Temp_enter(_F_tempMap, F_TN(i), s);
		}
    // an
		for(i=0;i<4;i++){
			char* s = checked_malloc(8*sizeof(char)); 
			sprintf(s, "$a%d", i);
			Temp_enter(_F_tempMap, F_AN(i), s);
		}
	}
	return _F_tempMap;
}
static Temp_tempList regLists[4] = {NULL, NULL, NULL, NULL};

/**
 * 4. 生成临时寄存器变量表，一共10个(TEMP_REG_NUM)， 和8个(SAVE_REG_NUM)
 */
Temp_tempList F_TEMP(){
  static Temp_tempList temps = 0;
  if (!temps){
    Temp_tempList tail;
    temps = tail = Temp_TempList(Temp_newtemp(), 0);
    for (int i = 1; i < TEMP_REG_NUM; ++i) {
      tail = tail->tail = Temp_TempList(Temp_newtemp(), NULL);
    }
  }
}

/**
 * 5. 函数调用相关寄存器
 */
Temp_tempList F_getRegList(RL_type type){
	int tp = (int)type;
	if(regLists[tp] == NULL){
		switch(type){

    // 特殊寄存器 rv, ra, fp, sp
		case specialregs:
			regLists[tp] = Temp_TempList(
							F_RV(), Temp_TempList(
							F_RA(), Temp_TempList(
							F_FP(), Temp_TempList(
							F_SP(), NULL
							))));
			break;
    // 参数寄存器 (前4个)
		case argregs:
			regLists[tp] = Temp_TempList(
							F_AN(0), Temp_TempList(
							F_AN(1), Temp_TempList(
							F_AN(2), Temp_TempList(
							F_AN(3), NULL
							))));
			break;
    // 被调用者数据寄存器
		case calleesaves:
			regLists[tp] = Temp_TempList(
							F_SN(0), Temp_TempList(
							F_SN(1), Temp_TempList(
							F_SN(2), Temp_TempList(
							F_SN(3), Temp_TempList(
							F_SN(4), Temp_TempList(
							F_SN(5), Temp_TempList(
							F_SN(6), Temp_TempList(
							F_SN(7), NULL
							))))))));
			break;	
		case callersaves:
			regLists[tp] = Temp_TempList(
							F_TN(0), Temp_TempList(
							F_TN(1), Temp_TempList(
							F_TN(2), Temp_TempList(
							F_TN(3), Temp_TempList(
							F_TN(4), Temp_TempList(
							F_TN(5), Temp_TempList(
							F_TN(6), Temp_TempList(
							F_TN(7), Temp_TempList(
							F_TN(8), Temp_TempList(
							F_TN(9), NULL
							))))))))));
			break;		
		}
	}

	return regLists[tp];
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

F_frag F_StringFrag(Temp_label label, string str){
	F_frag frag = (F_frag)checked_malloc(sizeof(struct F_frag_));
	frag->kind = F_stringFrag;
	frag->u.stringg.label = label;
	frag->u.stringg.str = str;
	return frag;
}

F_frag F_ProcFrag(T_stm body, F_frame frame){
	F_frag frag = (F_frag)checked_malloc(sizeof(struct F_frag_));
	frag->kind = F_procFrag;
	frag->u.proc.body = body;
	frag->u.proc.frame = frame;
	return frag;
}

F_fragList F_FragList(F_frag head, F_fragList tail){
	F_fragList fragList = (F_fragList)checked_malloc(sizeof(struct F_fragList_));
	fragList->head = head;
	fragList->tail = tail;
	return fragList;
}

static F_fragList *fragList = NULL;
static F_fragList fragList_head = NULL;

static F_frag* extendFragList(){
	if(fragList==NULL){
		fragList = (F_fragList*)checked_malloc(sizeof(F_fragList*));
	}

	*fragList = (F_fragList)checked_malloc(sizeof(struct F_fragList_));

	if(fragList_head==NULL){
		//Remember the head of frag list
		fragList_head = *fragList;
	}
	F_frag *currentFrag = &((*fragList)->head);
	fragList = &((*fragList)->tail);
	*fragList=NULL; 
    
	return currentFrag;
}

void F_String(Temp_label label, string str){
	F_frag *currentFrag = extendFragList();
	*currentFrag = F_StringFrag(label, str);
}

void F_Proc(T_stm body, F_frame frame){
	F_frag *currentFrag = extendFragList();
	*currentFrag = F_ProcFrag(body, frame);
//printf("New proc added to frag list\n");fflush(stdout);
}

F_fragList F_getFragList(){
	return fragList_head;
}

//Generate an IRT expression to access target address
T_exp F_Exp(F_access access, T_exp framePtr){
	if(access->kind==inFrame){
		return T_Mem(T_Binop(T_plus, framePtr, T_Const(access->u.offset)));
	} else {//access->kind==inReg
		return T_Temp(access->u.reg);
	}
}

static F_access InFrame(int offset){
	F_access acc = (F_access)checked_malloc(sizeof(struct F_access_));
	acc->kind = inFrame;
	acc->u.offset = offset;
	return acc;
}

static F_access InReg(Temp_temp reg){
	F_access acc = (F_access)checked_malloc(sizeof(struct F_access_));
	acc->kind = inReg;
	acc->u.reg = reg;
	return acc;
}

static int getNextLoc(F_frame frame){
	return frame->offset;
}

static Temp_temp getNextReg(F_frame frame){
	frame->locals++;
	return Temp_newtemp();
}

// 静态链
static F_access static_acc = NULL;
F_access F_staticLink(){
	if(static_acc==NULL){
		static_acc = InFrame(0);
	}
	return static_acc;
}

F_frame F_newFrame(Temp_label name, U_boolList formals){
	//1) Allocate mem for the new frame
	F_frame frame = (F_frame)checked_malloc(sizeof(struct F_frame_));
	frame->offset = 0;
	frame->locals = 0;
	frame->begin_label = name;

/* Problematic on 64-bit machine
	//2) Calculate the number of formals
	int i = 0;
	U_boolList bl;
	for(bl=formals; bl!=NULL; bl=bl->tail){
		i++;
	}

	if(i==0){
		frame->formals = NULL;
	} else {
		//3) Allocate mem for the formals in the frame
		int acc_size = sizeof(struct F_access_);
		F_accessList accList = (F_accessList)checked_malloc((i+1) * acc_size);
		F_accessList accList_head = accList;
		//Construct a list in the allocated mem
		for(;formals!=NULL;formals=formals->tail){
			accList->head = (F_access)accList;
			F_access *acc = &accList->head;
			if(formals->head==TRUE){
			//Escaping: go to memory (stack)
				*acc = InFrame(frame->offset);
				frame->offset -= F_wordSize;
			} else {//FALSE
				//Not escaping: go to register
				*acc = InReg(getNextReg(frame));
			}
			accList->tail = accList + acc_size/8;
			accList = accList->tail;
		}
		accList->tail = NULL;
		frame->formals = accList_head->tail;
	}
	return frame;
*/

	F_accessList accList = NULL;
	F_accessList accList_head = NULL;

	for(;formals!=NULL;formals=formals->tail){
		if(accList_head==NULL){
			accList = (F_accessList)checked_malloc(sizeof(struct F_accessList_));
			accList_head = accList;
		} else {
			accList->tail = (F_accessList)checked_malloc(sizeof(struct F_accessList_));
			accList = accList->tail;
		}

		if(formals->head==TRUE){
		//Escaping: go to memory (stack)
			accList->head = InFrame(frame->offset);
			frame->offset -= F_wordSize;
		} else {//FALSE
			//Not escaping: go to register
			accList->head = InReg(getNextReg(frame));
		}
	}

	if(accList!=NULL){
		accList->tail == NULL;
	}

	frame->formals = accList_head;
	return frame;
}

Temp_label F_name(F_frame frame){
	return frame->begin_label;
}

F_accessList F_formals(F_frame frame){
	return frame->formals;
}

F_access F_allocLocal(F_frame frame, bool escape){
	F_access acc;
	if(escape==TRUE){
	//Escaping: go to memory (stack)
		acc = InFrame(frame->offset);
		frame->offset -= F_wordSize;
	} else {//FALSE
		//Not escaping: go to register
		acc = InReg(getNextReg(frame));
	}
	return acc;
}

T_exp F_externalCall(string s, T_expList args){
	return T_Call(T_Name(Temp_namedlabel(s)), args);
}

static void createEscapeList(U_boolList *formal_escs, int i){
	int node_size = (sizeof(struct U_boolList_))/8;
	U_boolList escs = (U_boolList)checked_malloc(i*sizeof(struct U_boolList_));
	U_boolList escs_head = escs;
	//escs->head = TRUE;
	for(;i>0;i--){
		escs->head = TRUE; // = fList->head->escape;
		if(i>1){
			escs->tail = escs + node_size;
			escs = escs->tail; //!!!
		} else {
			escs->tail = NULL;
		}			
	}

	//

	*formal_escs = escs_head;
}


/**
 * F_procEntryExit 函数退出时要执行的最后指令
 */
static Temp_tempList returnSink = NULL;
// 给函数体添加一个 “下沉” 的指令，告诉寄存器分配器 那些寄存器在流程结束时还是生效的。（即 0寄存器，返回值，返回地址，调用者要保存的临时变量）
AS_instrList F_procEntryExit2(AS_instrList body) {
  // 保留 0寄存器，返回值，栈指针 和 被调用函数要保存的数据
  if (!returnSink){
    returnSink = Temp_TempList(F_ZERO(),
                               Temp_TempList(F_RA(),
                                             Temp_TempList(F_SP(), F_SAVES())));
  }
  // 将对应的 Temp_TempList 添加到 body 的后面，说明这些数据寄存器在函数执行后依然是活跃的
  return AS_splice(body,
                   AS_InstrList(AS_Oper("", NULL, returnSink, NULL), NULL) );
}

// 本章的代码生成只处理函数体，函数的进入 和 退出业务由此函数处理
AS_proc F_procEntryExit3(F_frame frame, AS_instrList body){
  char buf[100];
  sprintf(buf, "procedure  %s \n", S_name(frame->name));
  return AS_Proc(String(buf),body,"END \n");
}

