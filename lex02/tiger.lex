%{

#include <string.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"

//  token 位置标记， 从字符串开头 以0开始，
int charPos=1;

// comment 字符串栈
char *stringBuff = "";
char *commentBuff = "";


/**
 * Make sure the scanner terminates by supplying our own yywrap() function and
 * making it return 1.  See also `man flex` for more information.
 *
 */
int yywrap(void)
{
    charPos=1;
    return 1;
}

// 指针后移
void adjust(void)
{
    // 之前一步的指针
    EM_tokPos=charPos;
    // yyleng 长底已经匹配过，charPos 指针后移到 yyleng 个字符
    charPos+=yyleng;
}

%}


/**读取文件的字符串 */

/* Helper variables for building up strings from characters. */
const int INITIAL_BUFFER_LENGTH = 32;
char *string_buffer;
unsigned int string_buffer_capacity;



// 制表符 tab \t， 回车 \r, 空格
[ \r\t]            {adjust();continue;}

%%
" "             {adjust(); continue;}
\n              {adjust(); EM_newline(); continue;}

//标点符号
","             {adjust(); return COMMA;}
":"             {adjust();return ;}
";"             {adjust();return ;}
"("             {adjust();return LPAREN;}
")"             {adjust();return RPAREN;}
"{"             {adjust();return LBRACK;}
"}"             {adjust();return RBRACK;}
"."             {adjust();return DOT;}
"+"             {adjust();return PLUS;}

//  保留字
for  	        {adjust(); return FOR;}
while           {adjust();return WHILE;}
to              {adjust();return TO;}
break           {adjust();return BREAK;}
let             {adjust();return LET;}
in              {adjust();return IN;}
end             {adjust();return END;}
function        {adjust();return FUNCTION;}
var             {adjust();return VAR;}
type            {adjust();return TYPE;}
array           {adjust();return ARRAY;}
if              {adjust();return IF;}
then            {adjust();return THEN;}
else            {adjust();return ELSE;}
do              {adjust();return DO;}
of              {adjust();return OF;}
nil             {adjust();return NIL;}

// identifier
[a-zA-Z][0-9a-zA-Z]*    {
                            adjust();
                            yylval.sval=yytext; // yylval 的值为字符串
                            return ID;
                        }

// 数字
[0-9]+	        {
                    adjust();
                    yylval.ival=atoi(yytext); // 转化为数字
                    return INT;
                }



// 注释开始   /*   commentBuff 开始写入
/\*          {adjust();  BEGIN(COMMENT_START);}


// 注释结束   */    
\*/              {adjust(); BEGIN(COMMENT_END);}

// 字符串开始或结束
\"             {adjust(); BEGIN(STRING_TAG}



// 最后什么都匹配不上， 报错
.	            {
                    adjust();
                    EM_error(EM_tokPos,"illegal token");
                }
%%


//  Start Conditions
<STRING_TAG>



