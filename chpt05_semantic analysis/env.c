/**
    Created by l30025288 on 2023/12/26.
*/
#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"
#include "env.h"

/* 变量类型环境对象 可以添加到外层的 类型 环境中 */
E_enventry E_VarEntry(Ty_ty ty){
    E_enventry venv;
    venv = checked_malloc(sizeof(*venv));
    venv->kind = E_varEntry;
    venv->u.var.ty = ty;
    return venv;
}

/* 生成一个函数环境对象，可以添加到外层的 值 环境中 */
E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result){
    E_enventry  fenv;
    fenv = checked_malloc(sizeof(*fenv));

    fenv->kind = E_funEntry;
    fenv->u.fun.formals = formals;
    fenv->u.fun.result = result;

    return fenv;
}

/* 初始化类型环境 Ty_ ty environment 的符号表 */
S_table E_base_tenv(void){
    /* 初始化一个空符号表 */
    S_table init_t = S_empty();
    S_enter(init_t, S_Symbol("int"), Ty_Int());
    S_enter(init_t, S_Symbol("string"), Ty_String());
    S_enter(init_t, S_Symbol("double"), Ty_Double());
    return init_t;
}


/**
将符号 sym 添加到符号表 t 中
void S_enter(S_table t, S_symbol sym, void *value)
*/

/*
初始化值环境 val environment 的符号表
初始化时全局只有 tiger 的标准函数 在值环境中 
*/ 
S_table E_base_venv(void){
    S_table t = S_empty();

    /*
    function print(s : string)
        Print s on standard output.
    */
    S_enter(
            t,
            S_Symbol("print"),
            // E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result)
            // Ty_tyList Ty_TyList(Ty_ty head, Ty_tyList tail)
            /**
                形参类型： Ty_TyList()
                返回值类型：  Ty_void()
            */
            E_FunEntry(Ty_TyList(Ty_String(), NULL), Ty_Void())
        );

    /*
    function flush()
        Flush the standard output buffer.
    */
    S_enter(
        t, 
        S_Symbol("flush"), 
        E_FunEntry(NULL, Ty_Void())
        );

    /*
    function getchar() : string
        Read a character from standard input; return empty string on end of file.
    */
    S_enter(
        t,
        S_Symbol("getchar"),
        E_FunEntry(NULL, Ty_String())
    );

    /*
    function ord(s: string) : int
        Give ASCII value of first character of s; yields -1 if s is empty string. ord 函数， 获取字符（不是字符串）对应的 ASC 码
    */
    S_enter(
        t,
        S_Symbol("ord"),
        E_FunEntry(
            Ty_TyList(Ty_String(), NULL), 
            Ty_Int()
            )
    );

    /*
    function chr(i: int) : string
        Single-character string from ASCII value i; halt program if i out of range.
    */
    S_enter(
        t,
        S_Symbol("chr"),
        E_FunEntry(
            Ty_TyList(Ty_Int(), NULL), 
            Ty_String()
            )
    );

    /*
    function size(s: string) : int
        Number of characters in s.
    */
    S_enter(
        t,
        S_Symbol("size"),
        E_FunEntry(
            Ty_TyList(Ty_String(), NULL), 
            Ty_Int()
            )
    );

    /*
    function substring(s:string, first:int, n:int) : string
        Substring of string s, starting with character first, n characters long. Char-acters are numbered starting at 0.
    */
    S_enter(
        t,
        S_Symbol("substring"),
        E_FunEntry(
            Ty_TyList(Ty_String(), Ty_TyList(Ty_Int(), Ty_TyList(Ty_Int(), NULL))), 
            Ty_String()
            )
    );

    /*
    function concat (s1: string, s2: string) : string
        Concatenation of s1 and s2.
    */
    S_enter(
        t,
        S_Symbol("concat"),
        E_FunEntry(
            Ty_TyList(Ty_String(), Ty_TyList(Ty_String(), NULL)), 
            Ty_String()
            )
    );    


    /*
    function not(i : integer) : integer
        Return (i=0).
    */
    S_enter(
        t,
        S_Symbol("not"),
        E_FunEntry(
            Ty_TyList(Ty_Int(), NULL), 
            Ty_Int()
            )
    ); 

    /*
    function exit(i: int)
        Terminate execution with code i.
    */
    S_enter(
        t,
        S_Symbol("exit"),
        E_FunEntry(
            Ty_TyList(Ty_Int(), NULL), 
            Ty_Void()
            )
    ); 


}

