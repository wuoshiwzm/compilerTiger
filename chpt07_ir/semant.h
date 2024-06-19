#ifndef _SYMBOL_H_
#define _SYMBOL_H_
/**
    符号类
*/
typedef struct S_symbol_ *S_symbol;

/* 获取 string 对应的 Symbol， 相同的字符串会获取到相同的 symbol */
S_symbol S_Symbol(string);

string S_name(S_symbol); /* 符号 S_symbol 对应 名字 */

/* 符号表 */
typedef struct TAB_table_ * S_table;

/* 空符号表 */
S_table S_empty(void);

/* 将符号添加进符号表 S_table 中添加绑定: sym->value */
void S_enter(S_table t, S_symbol sym, void *value);

/* 符号表 t 中查找符号 sym */
void *S_look(S_table t, S_symbol sym);

void S_beginScope(S_table t);
void S_endScope(S_table t);

void S_show(S_table);

#endif
