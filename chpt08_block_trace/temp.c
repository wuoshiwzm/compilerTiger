/*
 * temp.c - functions to create and manipulate temporary variables which are
 *          used in the IR tree representation before it has been determined
 *          which variables are to go into registers.
 *
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"

// num ： 栈的位置？
struct Temp_temp_ {
    int num;
};

// symbol 的 name
string Temp_labelstring(Temp_label s) { return S_name(s); }

// label
static int labels = 0;
Temp_label Temp_newlabel(void) {
    char buf[100];
    sprintf(buf, "L%d", labels++);
    return Temp_namedlabel(String(buf));
}

/* The label will be created only if it is not found. */
Temp_label Temp_namedlabel(string s) {
    return S_Symbol(s);
}

// 初始 temps 为100
static int temps = 100;

// 新 temp 入栈
Temp_temp Temp_newtemp(void) {
    Temp_temp p = (Temp_temp) checked_malloc(sizeof(*p));
    p->num = temps++;
    {
        char r[16];
        sprintf(r, "%d", p->num);
        // Temp_enter(Temp_map m, Temp_temp t, string s)
        Temp_enter(Temp_name(), p, String(r));
    }
    return p;
}

struct Temp_map_ {
    TAB_table tab; // 符号表
    Temp_map under; //
};

// 获取最新的 Temp_map
Temp_map Temp_name(void) {
    // 静态值，只会初始化一次， 后面只会修改
    static Temp_map m = NULL;
    if (!m) m = Temp_empty();
    return m;
}

Temp_map newMap(TAB_table tab, Temp_map under) {
    Temp_map m = checked_malloc(sizeof(*m));
    m->tab = tab;
    m->under = under;
    return m;
}

Temp_map Temp_empty(void) {
    return newMap(TAB_empty(), NULL);
}

// 把两个 Temp_map 首尾相连
Temp_map Temp_layerMap(Temp_map over, Temp_map under) {
    if (over == NULL)
        return under;
    else return newMap(over->tab, Temp_layerMap(over->under, under));
}

// 将temp　添加进 Temp_map 的符号表, 绑定 t -> s
void Temp_enter(Temp_map m, Temp_temp t, string s) {
    assert(m && m->tab);
    TAB_enter(m->tab, t, s);
}

// 在 Temp_map 的符号表 tab 中查找 Temp_temp， 返回其值（string）
string Temp_look(Temp_map m, Temp_temp t) {
    string s;
    assert(m && m->tab);
    s = TAB_look(m->tab, t);
    if (s) return s;
    else if (m->under) return Temp_look(m->under, t);
    else return NULL;
}

Temp_tempList Temp_TempList(Temp_temp h, Temp_tempList t) {
    Temp_tempList p = (Temp_tempList) checked_malloc(sizeof(*p));
    p->head = h;
    p->tail = t;
    return p;
}

// Temp symbol 链表
Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t) {
    Temp_labelList p = (Temp_labelList) checked_malloc(sizeof(*p));
    p->head = h;
    p->tail = t;
    return p;
}

static FILE *outfile;

static void showit(Temp_temp t, string r) {
    fprintf(outfile, "t%d -> %s\n", t->num, r);
}

void Temp_dumpMap(FILE *out, Temp_map m) {
    outfile = out;
    TAB_dump(m->tab, (void (*)(void *, void *)) showit);
    if (m->under) {
        fprintf(out, "---------\n");
        Temp_dumpMap(out, m->under);
    }
}
