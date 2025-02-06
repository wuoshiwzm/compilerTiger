%{
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "errormsg.h"

int yylex(void); /* function prototype */

A_exp absyn_root;

void yyerror(char *s)
{
 EM_error(EM_tokPos, "%s", s);
}

%}


%union {
	int		      ival;
	string 		   sval;
	A_var 		   var;
	A_exp 		   exp;
	A_dec 		   dec;
	A_ty  		   ty;
	A_decList 	   declist;
	A_expList 	   expList;
	A_fieldList	   fieldList;
	A_fundec	      fundec;
	A_fundecList   fundecList;
	A_namety	      namety;
	A_nametyList   nametyList;
	A_efield	      efield;
	A_efieldList   efieldList;
	S_symbol	      symbol;

   // 面向对象
   A_clsdec       classdec;
   A_clsFieldList clsfieldList; 
   A_clsField     clsfield;

}

%token <sval> ID STRING
%token <ival> INT

%token
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK
  LBRACE RBRACE DOT
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF
  BREAK NIL
  FUNCTION VAR TYPE CLASS METHOD EXTENDS
  NEW

%type <symbol> 	   symbol
%type <var>    	   lvalue
%type <exp>    	   exp program
%type <dec>    	   dec tydecs fundecs clsdec
%type <ty>     	   ty
%type <declist>      decs
%type <expList>      expseq args
%type <fieldList>    tyfieldlist
%type <fundec> 	   fundec
%type <namety> 	   tydec
%type <efield> 	   refield
%type <efieldList>   refieldlist

// object tiger 
%type <classdec>     classdec
%type <clsfieldList> clsfieldlist
%type <clsfield>     clsfield


%left 		    SEMICOLON
%right 		    ASSIGN
%left 		    OR
%left 		    AND
%nonassoc 	    EQ NEQ LT LE GT GE
%left 		    PLUS MINUS
%left 		    TIMES DIVIDE
%right 		    UMINUS


%start program

%%

program:	exp{absyn_root = $1;}
		;

exp:		lvalue	{$$ = A_VarExp(EM_tokPos, $1);}
   |		NIL	{$$ =A_NilExp(EM_tokPos);}
   |		LPAREN expseq RPAREN	{$$ =A_SeqExp(EM_tokPos, $2);}
   |		INT     {$$ =A_IntExp(EM_tokPos, $1);}
   |		STRING	{$$ =A_StringExp(EM_tokPos, $1);}
   |		MINUS exp {$$ =A_IntExp(EM_tokPos, -($2->u.intt));} %prec UMINUS
   |		symbol LPAREN args RPAREN {$$ =A_CallExp(EM_tokPos, $1 ,$3);}
   |		exp PLUS exp  {$$ =A_OpExp(EM_tokPos, A_plusOp , $1, $3);}
   |		exp MINUS exp {$$ =A_OpExp(EM_tokPos, A_minusOp, $1, $3);}
   |		exp TIMES exp {$$ =A_OpExp(EM_tokPos, A_timesOp, $1, $3);}
   |		exp DIVIDE exp {$$ =A_OpExp(EM_tokPos, A_divideOp, $1, $3);}
   |		exp EQ exp {$$ =A_OpExp(EM_tokPos, A_eqOp, $1, $3);}
   |		exp NEQ exp {$$ =A_OpExp(EM_tokPos, A_neqOp, $1, $3);}
   |		exp GT exp {$$ =A_OpExp(EM_tokPos, A_gtOp, $1, $3);}
   |		exp LT exp {$$ =A_OpExp(EM_tokPos, A_ltOp, $1, $3);}
   |		exp GE exp {$$ =A_OpExp(EM_tokPos, A_geOp, $1, $3);}
   |		exp LE exp {$$ =A_OpExp(EM_tokPos, A_leOp, $1, $3);}
   |		exp AND exp {$$ =A_IfExp(EM_tokPos,$1,$3,A_IntExp(EM_tokPos,0));}
   | 		exp OR exp {$$ =A_IfExp(EM_tokPos,$1,A_IntExp(EM_tokPos,1),$3);}
   |		symbol LBRACE refieldlist RBRACE {$$ =A_RecordExp(EM_tokPos, $1, $3);}
   |		lvalue LBRACK exp RBRACK OF exp
   		       {
			if($1->kind != A_simpleVar)
		    		    exit(0);
			$$ =A_ArrayExp(EM_tokPos,$1->u.simple,$3,$6);
   }
   /*careful for this ,about lvalue,the lvalue must be symbol ,the next step will verify this*/
   | 	     	lvalue ASSIGN exp{$$ =A_AssignExp(EM_tokPos, $1, $3);}
   |		IF exp THEN exp ELSE exp{$$ =A_IfExp(EM_tokPos, $2 , $4, $6);}
   |		IF exp THEN exp{$$ =A_IfExp(EM_tokPos,$2,$4,NULL);}
   |		WHILE exp DO exp{$$ =A_WhileExp(EM_tokPos,$2, $4);}
   |		FOR symbol ASSIGN exp TO exp DO exp{$$ =A_ForExp(EM_tokPos,$2,$4,$6,$8);}
   | 		BREAK {$$ =A_BreakExp(EM_tokPos);}
   |		LET decs IN expseq END {$$ =A_LetExp(EM_tokPos,$2,A_SeqExp(EM_tokPos,$4));}
   ;


lvalue:		symbol {$$ =A_SimpleVar(EM_tokPos,$1);}
   |		lvalue DOT symbol {$$ =A_FieldVar(EM_tokPos,$1,$3);}
   |		lvalue LBRACK exp RBRACK {$$ =A_SubscriptVar(EM_tokPos,$1,$3);}
   ;

symbol:		ID {$$ =S_Symbol($1);}
   ;

expseq:		exp SEMICOLON expseq {$$ =A_ExpList($1,$3);}
   | 		exp{$$ =A_ExpList($1,NULL);}
   |		{$$ =NULL;}
   ;

args:		exp COMMA args {$$ =A_ExpList($1,$3);}
   |  		exp {$$ =A_ExpList($1,NULL);}
   |		{$$ =NULL;}
   ;



refieldlist:	refield COMMA refieldlist {$$ =A_EfieldList($1,$3);}
   | 		refield {$$ =A_EfieldList($1,NULL);}
   ;

refield:	symbol EQ exp {$$ =A_Efield($1,$3);}
   |		{$$ =NULL;}
   ;

decs:		dec decs {$$ =A_DecList($1,$2);}
   |		{$$ =NULL;}
   ;

dec:		tydecs {$$=$1;}
   | 		fundecs {$$=$1;}
   | 		clsdec { $$=$1; } // 新建类
   |     VAR symbol ASSIGN NEW symbol {$$ =A_ObjDec(EM_tokPos,$2,$4,$7);} // 新建对象
   |     VAR symbol COLON symbol ASSIGN NEW symbol {$$ =A_ObjDec(EM_tokPos,$2,$4,$7);} // 新建对象
   |     VAR symbol ASSIGN NEW symbol {$$ =A_ObjDec(EM_tokPos,$2,NULL,$5);}
   | 		VAR symbol ASSIGN exp     { $$ =A_VarDec(EM_tokPos,$2,NULL,$4);}
   | 		VAR symbol COLON symbol ASSIGN exp {$$ =A_VarDec(EM_tokPos,$2,$4,$6);}
   ;

// object_tiger
clsdec:  classdec { $$=A_ClassDec(EM_tokPos, $1); }
   ;

classdec:   CLASS symbol LBRACE clsfieldlist RBRACE {$$=A_Clsdec(EM_tokPos,$2,NULL,$4);}
   |     CLASS symbol EXTENDS symbol LBRACE clsfieldlist RBRACE {$$=A_Clsdec(EM_tokPos,$2,$4,$6);}
   ;

clsfieldlist: clsfield clsfieldlist {$$=A_ClsFieldList($1, $2);}
   |     clsfield  {$$=A_ClsFieldList($1, NULL);}
   ;

// 类声明属性 及 方法
clsfield:   VAR symbol ASSIGN exp {$$ =A_ClsVarField(EM_tokPos,$2,NULL,$4);}
   |        VAR symbol COLON symbol ASSIGN exp {$$ =A_ClsVarField(EM_tokPos,$2,$4,$6);} 
   |        METHOD symbol LPAREN tyfieldlist RPAREN EQ exp {$$ =A_ClsMethod(EM_tokPos,$2,$4,NULL,$7);}
   |        METHOD symbol LPAREN tyfieldlist RPAREN COLON symbol EQ exp {$$ =A_ClsMethod(EM_tokPos,$2,$4,$7,$9);}
   ;

fundecs:	fundec fundecs {$$=A_FunctionDec(EM_tokPos,A_FundecList($1,$2->u.function));}
   |		fundec {$$=A_FunctionDec(EM_tokPos,A_FundecList($1,NULL));}
   ;

tydecs:		tydec tydecs {$$ = A_TypeDec(EM_tokPos,A_NametyList($1,$2->u.type));}
   |	 	tydec {$$ = A_TypeDec(EM_tokPos,A_NametyList($1,NULL));}
   ;

tydec:		TYPE symbol EQ ty {$$ =A_Namety($2,$4);}
   ;

ty:		symbol{$$ =A_NameTy(EM_tokPos,$1);}
   | 		LBRACE tyfieldlist RBRACE{$$ =A_RecordTy(EM_tokPos,$2);}
   |  		ARRAY OF symbol{$$ =A_ArrayTy(EM_tokPos,$3);}
   ;

tyfieldlist:	symbol COLON symbol COMMA tyfieldlist {$$ =A_FieldList(A_Field(EM_tokPos,$1,$3),$5);}
   |		symbol COLON symbol {$$ =A_FieldList(A_Field(EM_tokPos,$1,$3),NULL);}
   |		{$$ = NULL;}
   ;




fundec:		FUNCTION symbol LPAREN tyfieldlist RPAREN EQ exp {$$ =A_Fundec(EM_tokPos,$2,$4,NULL,$7);}
   | 		FUNCTION symbol LPAREN tyfieldlist RPAREN COLON symbol EQ exp{$$ =A_Fundec(EM_tokPos,$2,$4,$7,$9);}
   ;


tydecs:		tydec tydecs {$$ = A_TypeDec(EM_tokPos,A_NametyList($1,$2->u.type));}
   |	 	tydec {$$ = A_TypeDec(EM_tokPos,A_NametyList($1,NULL));}
   ;

tydec:		TYPE symbol EQ ty {$$ =A_Namety($2,$4);}
   ;




