#include <stdio.h>
#include <string.h>
#include "util.h"

// tree 的构造函数，返回 tree 的指针 T_tree
typedef struct tree *T_tree;
struct tree {
    T_tree  left;
    string key;
    T_tree  right;
};
T_tree Tree(T_tree l, string k, T_tree r);

// binding
typedef struct binding * T_binding;
struct binding{
    string key,
    void *value;
};
T_binding Binding(string key, void *value);

T_tree Tree(T_tree l, string k, T_tree r);

T_tree insert(string key, T_tree t);

bool member(string key, T_tree t);

T_tree insert(string key, void *binding, T_tree t);

// 无类型指针 void *
void * lookup(string key, T_tree);
