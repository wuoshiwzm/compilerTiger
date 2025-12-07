/**
    环境
*/
typedef struct E_enventry_ *E_enventry;

/* 类型环境元素 */
struct E_enventry_ {
    enum {E_varEntry, E_funEntry} kind;
    union {
        struct {Ty_ty ty;} var; /* 变量类型 */
        struct {Ty_tyList formals; Ty_ty result;} fun; /* 函数参数类型，返回值类型 */
    } u;
};

E_enventry E_VarEntry(Ty_ty ty);
E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result);

S_table E_base_tenv(void);  /* 类型环境 Ty_ ty environment */
S_table E_base_venv(void);  /* 值环境  E_ enventry environment */


static S_table E_classenv(); /* 全局类表 */
static S_table E_objenv();  /* 全局对象表 */
