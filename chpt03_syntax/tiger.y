/* language=CPP */

%{
#include <stdio.h>
#include "util.h"
#include "errormsg.h"

int yylex(void); /* function prototype */

void yyerror(char *s)
{
 EM_error(EM_tokPos, "%s", s);
}
%}

/* token 的数据结构 */
%union {
	int pos;
	int ival;   /* int 值 */
	string sval;    /* 字符串值 */
}

%token <sval> ID STRING /* ID STRING  关联 sval */
%token <ival> INT /* INT 关联 ival */

/* 指定 token  和对应优先级*/
%token
  COMMA COLON SEMICOLON
  LPAREN RPAREN LBRACK RBRACK LBRACE RBRACE DOT
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF 
  BREAK NIL
  FUNCTION VAR TYPE PRINT


// 起始标记为 program
%start program


/* %nonassoc 表示,没有结合律 对应的标识符后面不能再跟同一个标识符 */
%nonassoc DO OF
%nonassoc THEN
%nonassoc ELSE
%left SEMICOLON
%left ASSIGN
%left OR
%left AND
%nonassoc EQ NEQ GT LT GE LE
%left MINUS PLUS
%left TIMES DIVIDE
%nonassoc UMINUS /* 没有结合律 */

%%


/* 从 program 开始 */
/* exp 表达式，表达式必须有一个返回值 */
program:	exp

exps:       exp
    |       exps COMMA exp
    ;

expq:       exp
    |       expq SEMICOLON exp
    ;

exp :   INT         /* exp 不能为空 */
    |   STRING
    |   BREAK
    |   NIL
    |   printfun
    |   lval        /* 左值 */
    |   nval
    |   tyd
    |   opexp       /* 运算表达式，如 a+b,  a>=b, ... 加减乘除 大于小于 等 <>... */
    |   assign      /* 赋值, 与声明不同， lval 已经声明过了 */
    |   func_call   /* 函数调用 */
    |   ifexp       /* if...then...else... */
    |   whileexp    /* while...then... */
    |   forexp      /* for id:=exp1 to exp2 do exp3 */
    |   letexp      /* let decs in exps end */
    |   parenexp    /* 圆括号 (exp) */
    ;

/* 左值 */
lval:       ID lval_ext                         {printf("this is lval: %s\n",$1)}
lval_ext:   /* empty */
    |       DOT ID lval_ext                     /* a.b.c..... */
    |       LBRACK exp RBRACK lval_ext          /* a[b[c[...]]] */
    ;

nval:   MINUS lval %prec UMINUS      /* 负数 */

/* any{ key=1,bey=2 } */
tyd:    ID LBRACE field_assignments RBRACE
field_assignments
        :   /* empty */
        |   field_assignment
        |   field_assignments COMMA field_assignment
field_assignment
        : ID EQ exp
        ;

/* 双目运算 */
opexp
    :       exp PLUS exp
    |       exp MINUS exp
    |       exp TIMES exp
    |       exp DIVIDE exp
    |       exp EQ exp
    |       exp NEQ exp
    |       exp LT exp
    |       exp LE exp
    |       exp GT exp
    |       exp GE exp
    |       exp AND exp
    |       exp OR exp
;
/*

opexp:      exp op exp
    ;
op:         PLUS
    |       MINUS
    |       TIMES
    |       DIVIDE
    |       EQ
    |       NEQ
    |       LT
    |       LE
    |       GT
    |       GE
    |       AND
    |       OR
;
*/



/* if 最多 if...then...else...*/
ifexp:      IF exp THEN exp ELSE exp
    |       IF exp THEN exp
    ;

/* while ... do ... */
whileexp:   WHILE exp DO exp
    ;


/* for id:=exp1 to exp2 do exp3 */
forexp:     FOR ID ASSIGN exp TO exp DO exp
    ;

/*
expq:       exp
    |       expq SEMICOLON exp
    ;
 * */
letexp:     LET decs IN expq END
    |       LET decs IN END
    ;

/* 圆括号 执行多个语句  exp1;exp2;... */
parenexp:   LPAREN expq RPAREN                   {printf("paren exp found \n")}
    ;


/* >>>>>>>>>>>>>>>>>声明<<<<<<<<<<<<<<<<<<< */
decs:   /* empty */
    |   dec decs
    ;

dec:    tydec       /* type 声明 */
    |   vardec      /* 变量声明 */
    |   fundec      /* 函数声明 */
    ;


/* 1.1  type 声明 */

/* 如：
 * type intlist = {hd:int, tl:intlist} ,
 * */
tydec:  TYPE type_id EQ ty
    ;


ty:     type_id
    |   LBRACE tyfields RBRACE
    |   ARRAY OF type_id  /* ??? 数组创建 */
    ;

/* 有两个内建类型 int string */
type_id:    ID
    ;

/*
 * tyfields 声明
 * type intlist = {hd:int, {tl:intlist, ...}}
 * */
tyfields:   /* empty */
    |       tyfield
    |       tyfields COMMA tyfield
tyfield:    ID COLON type_id
    |       ty
    ;

/* 1.2  变量声明 */
vardec:     VAR ID ASSIGN exp    /* var a : my_record := nil     a :=  nil*/
    |       VAR ID COLON type_id ASSIGN exp
    ;

/* 1.3  函数声明
 * function treeleaves(t:tree): int =
 *          exps....
 * */
fundec:     FUNCTION ID LPAREN tyfields RPAREN EQ exp                       {printf("func dec 1\n");}
    |       FUNCTION ID LPAREN tyfields RPAREN COLON type_id EQ exp         {printf("func dec 2\n");}
    ;


/* print 函数 */
printfun:   PRINT LPAREN exp RPAREN      { printf("print call  ...\n"); }
    ;

/* 2.   函数调用 */
func_call:  ID LPAREN RPAREN        { printf("calling function:.. \n"); }
    |       ID LPAREN exps RPAREN
    ;

/* 3.   赋值*/
assign:     lval ASSIGN exp
    ;


/* 错误恢复 */

