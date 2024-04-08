#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"
#include "temp.h"
#include "frame.h"
#include "myframe.h"
#include "translate.h"
#include "env.h"


/* 变量类型环境对象 可以添加到外层的 类型 环境中 */
E_enventry E_VarEntry(Tr_access access, Ty_ty ty) {
    E_enventry venv;
    venv = checked_malloc(sizeof(*venv));
    venv->kind = E_varEntry;
    venv->u.var.ty = ty;
    venv->u.var.access = access;
    return venv;
}

/* 生成一个函数环境对象，可以添加到外层的 值 环境中 */
E_enventry E_FunEntry(Tr_level level, Temp_label label, Ty_tyList formals, Ty_ty result) {
    E_enventry fenv = checked_malloc(sizeof(*fenv));
    fenv->kind = E_funEntry;
    fenv->u.fun.formals = formals;
    fenv->u.fun.result = result;
    fenv->u.fun.level = level;
    fenv->u.fun.label = label;
    return fenv;
}
