#include <stdio.h>
#include "util.h"
#include "prog1.h"
#include "interp.h"

/*
    绪论：直线式程序解释器
*/
/* This file is intentionally empty.  You should fill it in with your
   solution to the programming exercise. */

// gcc -Wall -g  .\test_interp.c .\interp.c .\prog1.c .\slp.c .\util.c

int main(){
    printf("calling main\n");
    A_stm  stm = prog();
    interp(stm);
}




