#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

/* Scratch register management */
bool  scratch_in_use[7] = { 0 };
char *scratch_names[7]  = { "bx", "10", "11", "12", "13", "14", "15" };
int scratch_alloc(){
    for(int r = 0; r < (sizeof scratch_in_use)/(sizeof scratch_in_use[0]); r++){
        if( !scratch_in_use[r] ){
            scratch_in_use[r] = true;
            return r;
        }
    }

    // all registers in use
    printf("Unable to allocate another scratch register. Aborting...");
    abort();
    return -1;
}
void scratch_free(int r){
    scratch_in_use[r] = false;
}
char *scratch_name(int r){
    return scratch_names[r];
}

/* Function stack management */
char *func_arg_regs[] = {"di", "si", "dx", "cx", "8", "9"};
int num_func_arg_regs = sizeof(func_arg_regs)/sizeof(func_arg_regs[0]);
char *caller_saved_regs[] = {"10", "11"};
int num_caller_saved_regs = sizeof(caller_saved_regs)/sizeof(caller_saved_regs[0]);
char *callee_saved_regs[] = {"bx", "12", "13", "14", "15"};
int num_callee_saved_regs = sizeof(callee_saved_regs)/sizeof(callee_saved_regs[0]);

/* Label management */
int label_counter = 0;
char *label_create(){
    int digits = 1;
    for(int place = 10; label_counter >= place; place *= 10)
        digits += 1;
    // 3 = .LC, digits, 1 = \0
    char *label = malloc(3 + digits + 1);
    // TODO: err-check malloc
    sprintf(label, ".LC%d", label_counter++);
    return label;
}
