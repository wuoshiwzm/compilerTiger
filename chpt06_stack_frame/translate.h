// translate 相对于 frame 模块， 增加了 Level 概念，对应函数嵌套
typedef void * Tr_exp;

// 表达式类型
struct expty {Tr_exp exp; Ty_ty ty;};

// 构造函数
struct expty expTy(Tr_exp e, Ty_ty t) {
	struct expty et;
	et.exp = e;
	et.ty  = t;
	return et;
}

//  --------------------------- 栈帧部分 ---------------------------  //
extern const int F_wordSize;

typedef struct Tr_level_ *Tr_level;

// 带 level 版的 accessList, formals, allocLocal
typedef struct Tr_access_ *Tr_access;

typedef struct Tr_accessList_ *Tr_accessList;
struct Tr_accessList_{
    Tr_access head;
    Tr_accessList tail;
};

// 构造函数
Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

// 最外层的函数
Tr_level Tr_outermost(void);

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);

Tr_level Tr_getParent(Tr_level level);

void Tr_printLevel(Tr_level level);

// access
Tr_accessList Tr_formals(Tr_level level);

//Tr_access Tr_allocLocal(Tr_level level, bool escape); escape 默认为 TRUE, 进栈帧
Tr_access Tr_allocLocal(Tr_level level);


/*
 * 中间代码
 */
typedef struct Tr_exp_ *Tr_exp;
typedef struct Tr_expList_ *Tr_expList;
struct Tr_expList_{
    Tr_exp head;
    Tr_expList tail;
};
Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail);

