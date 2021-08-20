/* Scratch register management */
bool  scratch_in_use[7] = { 0 };
char *scratch_names[7]  = { "bx", "10", "11", "12", "13", "14", "15" }
int scratch_alloc(){
    for(int i = 0; i < (sizeof scratch_in_use)/(sizeof scratch_in_use[0]); i++){
        if( !scratch_in_use[i] ) return i;
    }

    // all registers in use
    printf("Unable to allocate new register. Aborting...");
    abort()
    return -1;
}
void scratch_free(int r){
    reg_in_use[r] = 0;
}
char *scratch_name(int r){
    return scratch_names[r];
}

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
