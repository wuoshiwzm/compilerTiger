#include "translate.h"
#include "temp.h"
#include "types.h"
/**
    环境
*/
typedef struct E_enventry_ *E_enventry;

// 带  level　的E_enventry, E_VarEntry， E_FunEntry  的类型环境元素
struct E_enventry_ {
    enum {E_varEntry, E_funEntry} kind;
    union {
        struct {Tr_access access; Ty_ty ty;} var; /* 变量 */
        struct {Tr_level level; Temp_label label; Ty_tyList formals; Ty_ty result;} fun; /* 函数 */
    } u;
};

E_enventry E_VarEntry(Tr_access access, Ty_ty ty);
E_enventry E_FunEntry(Tr_level level, Temp_label label, Ty_tyList formals, Ty_ty result)


static S_table E_base_tenv(void);  /* 类型环境 Ty_ ty environment */
static S_table E_base_venv(void);  /* 值环境  E_ enventry environment */
