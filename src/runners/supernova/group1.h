#include "supernova_private.h"

#define sninstr_func(instruction) \
void supernova_##instruction##_function(register struct thread_t *thread, union instruction_t instruction)

/**
 * @brief set `rd` to the bitwise `and` between `r1` and `r2`
 * 
 * ```
 * rd <- r1 & r2 
 * ```
 * 
 * @param thread current working thread
 * @param instruction instruction elements
*/
sninstr_func(andr_instrc);

/**
 * @brief set `rd` to the bitwise `and` between `r1` and an immediate value
 * 
 * ```
 * rd <- r1 & imm
 * 
 * @param thread current working thread
 * @param instruction instruction elements
*/
sninstr_func(andi_instrc);