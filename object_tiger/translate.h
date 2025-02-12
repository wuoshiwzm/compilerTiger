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
