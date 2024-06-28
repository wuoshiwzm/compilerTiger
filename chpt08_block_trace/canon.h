#ifndef WORKSPACE_CANON_H
#define WORKSPACE_CANON_H

#include "tree.h"
#include "temp.h"

typedef struct C_stmListList_ *C_stmListList;
struct C_block { C_stmListList stmLists; Temp_label label;};
struct C_stmListList_ {T_stmList head; C_stmListList tail; };

// 将 T_stm 转化为 T_stmList 链表
T_stmList C_linearize(T_stm stm);
// 将 T_stmList 转化为 基本块
struct C_block C_basicBlocks(T_stmList stmList);
// 将基本块转换为 轨迹
T_stmList C_traceSchedule(struct C_block b);

#endif //WORKSPACE_CANON_H
