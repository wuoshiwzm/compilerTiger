/**
    符号类
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "util.h"
#include "symbol.h"
#include "table.h"

struct S_symbol_ {string name; S_symbol next;};

/* 创建符号 （符号名， 下一个符号） */
static S_symbol mksymbol(string name, S_symbol next)
{
    S_symbol s = checked_malloc(sizeof(*s));
	
	s->name=checked_malloc(sizeof(strlen(name)) + 1);
    strcpy(s->name, name);
    s->next = next;
    return s;
}

#define SIZE 109

/* hashtable 为符号表，存储符号类 S_symbol */
static S_symbol hashtable[SIZE];

static unsigned int hash(char *s0) 
{
    unsigned int h=0;
    char *s;
    for(s=s0; *s; s++) {
        h = h * 65599 + *s;
    }
	/* my-come-true
    for(s=s0; *s; s++){
    	if (isupper(*s) | *s == '_') h -= 'A'; else h -= 'a';
        h = h + *s;
    }
	*/
    return h;
}
 
static int streq(string a, string b)
{
 return !strcmp(a,b);
}

/* S_symbol 构造函数，生成名字name 对应的符号 hashtable is a global-var save array of S_symbol*/
S_symbol S_Symbol(string name) 
{
    int index = hash(name) % SIZE;
    S_symbol syms = hashtable[index], sym;

    for(sym = syms; sym; sym = sym->next)
        if(streq(sym->name,name)) return sym;

    /* 没找到，新建符号 sym, 赋值给 hashtable[index] */
    sym = mksymbol(name,syms);
    hashtable[index]=sym;
    return sym;
}

/***********************/
void S_distable()
{
	int i;
	S_symbol t;
	for (i = 0; i < SIZE; i++){
		if (hashtable[i]) printf("%s\n", hashtable[i]->name);
	}
}
/***********************/ 
/* 获取符号的名字 */
string S_name(S_symbol sym)
{
	return sym->name;
}

/* 获取空符号表 */
S_table S_empty(void)
{ 
	return TAB_empty();
}
/* 将符号 sym 添加到符号表 t 中 */
void S_enter(S_table t, S_symbol sym, void *value){
    TAB_enter(t,sym,value);
}

/* 在类型表 t 中寻找符号 sym 的绑定 */
void *S_look(S_table t, S_symbol sym) {
    return TAB_look(t,sym);
}

/**
 * 初始化全局环境 （默认有一个全局符号表）
 */

/* 新建一个基本符号 */
static struct S_symbol_ marksym = {"<mark>",0};

/* 进入环境的 符号表 */
void S_beginScope(S_table t)
{
    /* 将基本元素添加到全局环境中， value为NULL: */
    /* S_enter(S_table t, S_symbol sym, void *value) , value */
    S_enter(t,&marksym, NULL);
}

/* 退出环境的符号表 */
void S_endScope(S_table t)
{
    S_symbol  s;
    /* 一直弹出符号，只到遇到基本符号 marksym 为止 */
    do s= TAB_pop(t); 
	while (s!= &marksym);
}

void S_dump(S_table t, void (*show)(S_symbol sym, void *binding)) {
  TAB_dump(t, (void (*)(void *, void *)) show);
}


void S_show(S_table s) {
    TAB_show_tenv(s);
}

