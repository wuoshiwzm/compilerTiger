%{

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"

/*  token 位置标记， 从字符串开头 以0开始， */
int charPos=1;

/* comment 字符串栈 */
char *stringBuff = "";
char *commentBuff = "";


/**
 * Make sure the scanner terminates by supplying our own yywrap() function and
 * making it return 1.  See also `man flex` for more information.
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


/*读取文件的字符串 */

/* We need these options to avoid some gcc compiler warnings. */
%option nounput
%option noinput

%x COMMENT

%%



    /* 制表符 tab \t， 回车 \r, 空格  */
[ \r\t]         {adjust(); continue;}

\n              {
                    adjust();
                    EM_newline();
                    continue;
                }
    /*标点符号*/
","             {adjust(); return COMMA;}
":"             {adjust(); return COLON;}
";"             {adjust(); return SEMICOLON;}
"("             {adjust(); return LPAREN;}
")"             {adjust(); return RPAREN;}
"["             {adjust(); return LBRACK;}
"]"             {adjust(); return RBRACK;}
"{"             {adjust(); return LBRACE;}
"}"             {adjust(); return RBRACE;}
"."             {adjust(); return DOT;}
"+"             {adjust(); return PLUS;}
"-"             {adjust(); return MINUS;}
"*"             {adjust(); return TIMES;}
"/"             {adjust(); return DIVIDE;}
"="             {adjust(); return EQ;}
"<>"            {adjust(); return NEQ;}
"<"             {adjust(); return LT;}
"<="            {adjust(); return LE;}
">"             {adjust(); return GT;}
">="            {adjust(); return GE;}
"&"             {adjust(); return AND;}
"|"             {adjust(); return OR;}
":="            {adjust(); return ASSIGN;}

    /*保留字*/
for  	        {adjust();return FOR;}
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

    /*identifier*/
[a-zA-Z][0-9a-zA-Z_]*    {
                            adjust();
                            yylval.sval=yytext; // yylval 的值为字符串
                            return ID;
                        }

    /*数字*/
[0-9]+	    {
                    adjust();
                    yylval.ival=atoi(yytext); // 转化为数字
                    return INT;
                }

    /*注释结束  \*\/ 没有开始，只有结束 直接报错*/
"*/"            {
                    adjust();
                    EM_error(EM_tokPos,"illegal comment \n ");
                }


    /* 字符串开始或结束 */
\"[^"]*\"       {
                    if(yytext[yyleng-2]=='\\'){  /* 右边引号之前是 \ ， 如果是説明匹配到 \", 不是结束， 而是字符串中包含一个 " 引号 */
                        yyless(yyleng-1);  /*  推回 yyleng-1 个字符，返回最后的引号 " ，这个引号是字符串中间的一部分 */
                        yymore(); 	/*  添加下一个字符串 下一个记号是以这个回退的引号作为开始的被 引起起字符串的剩作部分，这样整个字符串都在 yytext 中  */
                    }
                    printf("string found::: %s \n ",yytext);
                }


    /* 注释 */
"/*"            {
                    printf("comment found \n ");
                    adjust();
                    BEGIN(COMMENT);
                }


    /* 最后什么都匹配不上， 报错 */
.	            {
                    adjust();
                    EM_error(EM_tokPos,"illegal token :  %s\n ", yytext);
                }


<COMMENT>{

    "/*"        {
                    adjust();
                    EM_error(EM_tokPos,"illegal comment, comment can not be nested\n ");
                }
    /* 注释结束 */
    "*/"        {
                    printf("comment closed, back to initial\n");
                    adjust();
                    BEGIN(INITIAL);
                }

    <<EOF>>     {
                    adjust();
                    EM_error(EM_tokPos,"illegal comment, comment not closed\n ");
                }
    /* 其他字符直接往下走 */
    .           {   adjust(); }

}


