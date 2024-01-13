/**
   符号表
*/

#include <stdio.h>
#include "util.h"
#include "table.h"

// 为什么符号表中 table属性 数组的长度是 127???????
#define TABSIZE 127

/**
 * 声明结构体
 */
typedef struct binder_ *binder;
struct binder_ {
    void *key;
    void *value;
    binder next;
    void *prevtop;
};


/**
 * table: binder 的表
 * top: 表顶的 *key
 */
struct TAB_table_{
    binder table[TABSIZE]; // 名为 table 的数组,元素类型为 binder
    void *top;
};


/**
 * binder 绑定对象的构造函数
 *
 * @param key
 * @param value
 * @param next     binder key 之前的绑定
 * @param prevtop  添加 binder 时, table 的 top
 * @return
 */
static binder Binder(void *key, void *value, binder next, void *prevtop){
    binder b = checked_malloc(sizeof(*b));
    b->key = key;
    b->value = value;
    b->next = next;
    b->prevtop = prevtop;
    return b;
}

/* 空符号表 构造函数 */
TAB_table TAB_empty(void){
    TAB_table t = checked_malloc(sizeof(*t));
    t->top = NULL;
    int i;
    for (int i = 0; i < TABSIZE; i++) {
        t->table[i] = NULL;
    }
    return t;
}


/**
 *  将 key -> value 绑定添加进符号表 t，顶掉 key 之前的绑定
 */
void TAB_enter(TAB_table t, void *key, void *value) {
    int index;
    binder b;
    assert(t && key); /* 非空判断 */

    /* 计算 key 的 hash */
    index = ((unsigned) key) % TABSIZE;

    /* t->table[index] 为 table[index] 之前的绑定, 因为key的binder会替换前一个binder,这里将前一个 binder 保留下来.
        t->top 为 符号表的顶端, 保留不变
        t->table[index] 表示 b 的前一个 binder
        prevtop : 添加 binder 后这时 table 的 top */

    b = Binder(key,value,t->table[index],t->top);
    t->table[index] = b; /* 将新生成的绑定 替换原来的 table[index] 上的绑定 */
    t->top = key;
}


/**
 * 在符号表 t 中查找 key 对应的所有 Binder (一个key 有可能有多个binder, 新的 binder 会替换掉之前的), 返回对应的value
 * @param t
 * @param key
 * @return
 */
void *TAB_look(TAB_table t, void *key) {
    int index;
    binder b;
    assert(t && key);
    index = ((unsigned)key) % TABSIZE;

    for (b = t->table[index]; b; b = b->next) {
        if(b->key == key) return b->value;
    }
    return NULL;
}

/**
 * 从符号表 t 弹出最上面的 key (t->top)
 *
 * @param t
 * @return
 */
void *TAB_pop(TAB_table t) {
    void *topkey;
    binder b;
    int index;
    assert(t);

    topkey = t->top;
    assert(topkey);

    /* 获取top 的 key 对应的 index */
    int topkeyIndex = ((unsigned)topkey) % TABSIZE;
    b = t->table[topkeyIndex];
    assert(b);

    /* 将 b 的上一个 binder 替换为当前的 binder */
    t->table[index] = b->next;
    return b->key;
}


/**
 * show方法调用表中所有的 key->value 绑定
 * @param t
 * @param show
 */
void  TAB_dump(TAB_table t, void(*show)(void *key, void *value)){

    void *topKey = t->top;

    int index = ((unsigned)topKey) % TABSIZE;
    binder b = t->table[index];

    /* top 的 binder 为NULL, 说明是空表 */
    if(b == NULL) return;

    /* 将 topKey 对应的 binder 替换为上一个 binder ， 进行 遍历 */
    t->table[index] = b->next;
    t->top = b->prevtop;

    show(b->key, b->value);
    /* 递归执行 */
    TAB_dump(t, show);

    /* 对 key　对应的 binder 都遍历完了 回复 table 的 top 的 table[index] 对应的 binder */
    t->top = topKey;
    t->table[index] = b;
}


/* 展示所有的类型环境 */
//void TAB_show_tenv(TAB_table){
//    if(!t) {
//        printf("empty table");
//        return;
//    }
//}




















