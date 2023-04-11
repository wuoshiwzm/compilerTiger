#include <stdlib.h>

/**
 * 解释 stm 对应数据结构
 */
typedef struct table *Table_;
// 定义结构体 table 和 指向结构体 table的指针 "Table_"
struct table {
    string id;
    int value;
    Table_ tail;
};
Table_ Table(string id, int value, struct table *tail);


/**
 * 解释 exp 对应的数据结构
 */
typedef struct intAndTable *IntAndTable_;
struct intAndTable {
    int i;
    Table_ t;
};
IntAndTable_ IntAndTable(int i, Table_ t);


// 解释语句
void interp(A_stm stm);

// 解释 print , assign 语句
Table_ interpStm(A_stm s, Table_ t);

IntAndTable_ interpExp(A_exp e, Table_ t);

IntAndTable_ interpExpList(A_expList expList, Table_ t);

// 更新 赋值的table
Table_ update(Table_ t, string id, int value);
Table_ doUpdate(Table_ t, string id, int value);

int lookup(Table_ t, string key);


void printTable(Table_ t);

