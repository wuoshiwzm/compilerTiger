#ifndef _TABLE_H_
#define _TABLE_H_
/**
    符号表
*/

typedef struct TAB_table_ *TAB_table;

/* 创建新的空符号表 */
TAB_table TAB_empty(void);

/* 将 key -> value 绑定添加进符号表 t，隐藏 key 之前的绑定 */
void TAB_enter(TAB_table t, void *key, void *value);

/* 在符号表 t 中查找 绑定键 key 最新的 绑定 */
void *TAB_look(TAB_table t, void *key);

/* 符号表弹出一个元素（绑定），并返回这个绑定的 key, 有可能暴露出 key 之前的绑定。 */
void *TAB_pop(TAB_table t);

/* Call "show" on every "key"->"value" pair in the table, */
void  TAB_dump(TAB_table t, void(*show)(void *key, void *value));

void TAB_show_tenv(TAB_table);

void TAB_show_venv(TAB_table);

#endif
