#include "persist_tree.h"

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
