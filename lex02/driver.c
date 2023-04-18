%{

#include <string.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"

//  token 位置标记， 从字符串开头 以0开始，
int charPos=1;

int yywrap(void)
{
    charPos=1;
    return 1;
}


void adjust(void)
{
    EM_tokPos=charPos;
    charPos+=yyleng;
}

%}

%%
" "	 {adjust(); continue;}
\n	 {adjust(); EM_newline(); continue;}
","	 {adjust(); return COMMA;}
for  	 {adjust(); return FOR;}
[0-9]+	 {adjust(); yylval.ival=atoi(yytext); return INT;}
.	 {adjust(); EM_error(EM_tokPos,"illegal token");}
