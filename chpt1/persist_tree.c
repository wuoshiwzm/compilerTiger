#include "persist_tree.h"


// tree 的构造函数，返回 tree 的指针 T_tree
T_tree Tree(T_tree l, string k, T_tree r){
    // *t 指针t 的访问值
    T_tree t = checked_malloc(sizeof(*t));
    t->left = l;
    t->right = r;
    t->key = k;
    return t;
}

/**
 *  长效二叉搜索树，如果 tree2 = insert(x,tree1) ，
 *  则 tree2 可以被使用的同时， tree1 仍可以继续用于查找
 */

T_tree insert(string key, T_tree t){
    if(t ==NULL) return Tree(NULL, key, NULL); // 找到一个空节点，插入key, 生成一个小子树，返回
    else if(strcmp(key, t->key) < 0) // key < t->key，在左子树尝试
        return Tree(insert(key, t->left), t->key, t->right);
    else if(strcmp(key,t->key) > 0)  //  key > t->key，在左子树尝试，在右子树插入
        return Tree(t->left, t->key, insert(key,t->right));
    else return Tree(t->left,key,t->right); // key 存在，则返回这个节点为根的子树
}

bool member(string key, T_tree t){
    if(t ==NULL) return FALSE; // 没找到
    else if(strcmp(key, t->key) < 0) // 在左子树找
        return member(key, t->left);
    else if(strcmp(key,t->key) > 0)  // 在右子树找
        return member(key,t->right);
    else return TRUE; // 找到
}

// Binding 关联绑定
T_binding Binding(string key, void *value){
    T_binding b = checked_malloc(sizeof( *b));
    b->key = key;
    b->*value = value;
    return b;
}

T_tree insert(string key, void *binding, T_tree){

}


void * lookup(string key, T_tree){

}


int main(){

    T_tree base = Tree(NULL,String("a"),NULL);

    base = insert(String("c"),base);
    base = insert(String("g"),base);
    base = insert(String("d"),base);
    base = insert(String("a"),base);


    printf("test member function:: %d \n", member(String("a"),base));
    printf("test member function:: %d \n", member(String("b"),base));
    printf("test member function:: %d \n", member(String("g"),base));

}
