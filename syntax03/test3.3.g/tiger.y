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

// expp: 代 {} 的 exps
// expn: 不代 {} 的 exps
program:	 { exps }

exps	:       exp
    	|       exps ";" exp
    	;

exp	:       /* empty */
	|       expp
	|       expn
    	;

expp 	:	"{" ex "}"
	;
expn	:	ex
	;

/* 实际语句 */
ex	:	/* empty */
	|	stat ";"
	;

stat	: A