#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "types.h"
#include "frame.h"
#include "myframe.h"
#include "translate.h"

struct Tr_level_{
    // 层次值
    int depth;
    Tr_level parent;
    F_frame frame;
};

struct Tr_access_{
    Tr_level level;
    F_access access;
};

/*
 * Level
 */
// 全局只有一个 outermost 生成一个最外层的 level,最外层不包含栈帧和形参数据
static Tr_level outermost = NULL;
Tr_level Tr_outermost(void){
    if(outermost == NULL){
        Tr_level outermost = checked_malloc(sizeof (struct Tr_level_));
        outermost->depth=0;
        outermost->frame = NULL;
        outermost->parent = NULL;
    }
    return outermost;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals){
    F_frame frame = F_newFrame(name,  U_BoolList(TRUE, formals));// 在 formals 基础上添加一个 为 TRUE 的做为 静态态
    Tr_level level = checked_malloc(sizeof (struct Tr_level_));
    level->parent=parent;
    level->frame=frame;
    level->depth = parent->depth+1;
    return level;
}

Tr_level Tr_getParent(Tr_level level){
    if(level == outermost){
        return outermost;
    }else{
        return level->parent;
    }
}

void printLevel(Tr_level level){
    printf("Level: depth = %d; name = %s \\n", level->depth, S_name(F_name(level->frame)));
}

/*
 * Access
 */
// 带 level 的 formals, 获取这一级的 formal 的所有访问 Tr_accessList
Tr_accessList Tr_formals(Tr_level level){
    F_accessList f_accessList = F_formals(level->frame);
    Tr_accessList tr_list = NULL;
    Tr_accessList tr_list_head = NULL; // 第一个 tr_list


    for (; f_accessList != NULL; f_accessList = f_accessList->tail) {
        if (tr_list_head == NULL){
            tr_list = (Tr_accessList) checked_malloc(sizeof (struct Tr_accessList_));
            tr_list_head = tr_list;
        }else{
             tr_list->tail = (Tr_accessList) checked_malloc(sizeof (struct Tr_accessList_));
             tr_list = tr_list->tail;
        }

        tr_list->head = (Tr_access) checked_malloc(sizeof (struct Tr_access_));
        // 对应这一级的所有形参， 所以Tr_accessList 所有的 Tr_access 节点的 level 都是一样的
        tr_list->head->level = level;
        tr_list->head->access = f_accessList->head;
    }

    if (tr_list != NULL){
        tr_list->tail = NULL;
    }
    return tr_list_head;
}

// escape 默认为 TRUE, 进栈帧
Tr_access Tr_allocLocal(Tr_level level){
    Tr_access tr_acc = checked_malloc(sizeof (struct Tr_access_));
    tr_acc->level = level;
    // 获取 level 的 frame 的 allocLocal， F_access
    tr_acc->access = F_allocLocal(level->frame);
    return tr_acc;
}

void Tr_printLevel(Tr_level level){
    printf("--------print Level--------");
	printf("Level: depth = %d \n", level->depth);
//	printf("Level: depth = %d; name = %s\n", level->depth, S_name(F_name(level->frame)));
}
