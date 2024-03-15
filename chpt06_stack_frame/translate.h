// 相对于 frame 模块， 增加了 Level 相关数据
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

typedef struct Tr_access_ *Tr_access;

typedef struct Tr_accessList_ *Tr_accessList;

// 构造函数
Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

/*
	带 level 版的 accessList, formals, allocLocal 
*/

typedef struct Tr_level_ *Tr_level;

// 最外层的函数
Tr_level Tr_outermost(void);
Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);

Tr_accessList Tr_formals(Tr_level level);
Tr_access Tr_allocLocal(Tr_level level, bool escape);
