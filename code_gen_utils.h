#ifndef CODE_GEN_UTILS_H
#define CODE_GEN_UTILS_H

// A register name should be interpreted as the suffix to be given to %r to reference a 64-bit register.
// Examples: "ax", "bx", "10", "di"

// Allocates a register for scratch space.
// Currently aborts if no scratch registers are free.
// Returns a register id used for other scratch commands.
int scratch_alloc();
// Returns the name of a register from its id.
// The names returned ARE NOT dynamically allocated and SHOULD NOT be freed.
char *scratch_name(int reg);
// Marks the register whose id is given as free.
// This allow it be reused via re-allocation in future scratch_alloc calls.
void scratch_free(int reg);

// Returns the name of the register into which the arg_num'th argument is placed.
// The names returned ARE NOT dynamically allocated and SHOULD NOT be freed.
extern char *func_arg_regs[];
extern int num_func_arg_regs;
extern char *caller_saved_regs[];
extern int num_caller_saved_regs;
extern char *callee_saved_regs[];
extern int num_callee_saved_regs;

// Returns a unique label.
// The labels returned ARE dynamically allocated and MUST be freed.
char *label_create();

#endif
