#include "compiler.h"

#include <stdint.h>
#include <stdio.h>

struct table_entry {
    struct string_t name;
    struct Array *names;
    uint64_t allocation_point;
    uint8_t
        arg_size; /* although possible, please reconsider what you are coding before writing a function with more than 255 arguments */
    bool is_function;
};

typedef uint64_t return_t;
static return_t assemble(struct Assembler *assembler, const struct Node *node);

struct Assembler *create_assembler(struct Array *parsed_nodes) {
    struct Assembler *assembler = calloc(1, sizeof(struct Assembler));
    if (!assembler) goto destructor_final;

    assembler->instructions = create_list();
    if (!assembler->instructions) goto destructor_1;

    assembler->table = create_map(0);
    if (!assembler->table) goto destructor_2;

    assembler->parsed_nodes = parsed_nodes;
    assembler->dot = 0x1000;
    assembler->entry_point = 0;
    assembler->root_index = 0;
    return assembler;
destructor_2:
    delete_list(assembler->instructions, NULL);
destructor_1:
    free(assembler);
destructor_final:
    ZenithOutOfMemory;
    return NULL;
}
/* memory allocate -> malloc */
/* register allocate -> ralloc */
static int32_t ralloc(struct Assembler *assembler, bool descending, int32_t set_next) {
    static int32_t next_reg = -1;
    int i;

    if (set_next != -1) {
        next_reg = set_next;
        return assembler->registers & (1 << set_next);
    }

    if (next_reg != -1 && assembler->registers & (1 << next_reg)) {
        assembler->registers |= 1 << next_reg;
        return next_reg;
    }

    if (!descending) {
        for (i = 1; i < 32; ++i)
            if (!(assembler->registers & (1 << i))) break;
    } else {
        for (i = 31; i > 0; --i)
            if (!(assembler->registers & (1 << i))) break;
    }

    if (i != -1) {
        assembler->registers |= 1 << i;
        return i;
    }

    return -1;
}

static void clear_register(struct Assembler *assembler, uint32_t index) {
    assembler->registers &= ~(1 << index);
}

static int append_instruction(struct Assembler *assembler, const union instruction_t instruction) {
    if (!list_append(assembler->instructions, (void *)instruction.value)) return 0;
    assembler->dot += 8;
    return 1;
}

static struct string_t get_current_function_name(struct Assembler *assembler) {
    return ((struct LambdaNode *)array_index(assembler->parsed_nodes, assembler->root_index))->name;
}

static return_t assemble_number(struct Assembler *assembler, const struct NumberNode *node) {
    uint32_t reg_index = 0;
    if (node->value == 0.0 && node->number == 0) return 0;

    reg_index = ralloc(assembler, false, -1);
    if (reg_index == -1U) return -1U;

    if (node->number > 0x400000000000) append_instruction(assembler, LInstruction(lui_instrc, reg_index, node->number >> 18));

    append_instruction(assembler, SInstruction(addi_instrc, 0, reg_index, node->number & ((1LLU << 46) - 1)));

    return reg_index;
}

static return_t assemble_unary(struct Assembler *assembler, const struct UnaryNode *node) {
    return_t used_reg = assemble(assembler, node->value);
    union instruction_t inst;
    if (used_reg == -1U) return used_reg;

    switch (node->token) {
        case TT_Minus:
            inst = RInstruction(subi_instrc, used_reg, 0, used_reg);
            break;
        case TT_Not:
            inst = SInstruction(xori_instrc, used_reg, used_reg, -1);
            break;
        case TT_Increment:
            inst = SInstruction(addi_instrc, used_reg, used_reg, 1);
            break;
        case TT_Decrement:
            inst = SInstruction(subi_instrc, used_reg, used_reg, 1);
            break;
        default:
            return used_reg;
    }

    append_instruction(assembler, inst);
    return used_reg;
}

static return_t assemble_binary(struct Assembler *assembler, const struct BinaryNode *node, bool isJumping) {
    return_t lchild = assemble(assembler, node->left), rchild = assemble(assembler, node->right);

    union instruction_t inst;

    if (rchild == -1U || lchild == -1U) return lchild;

    if (!isJumping) {
        switch (node->token) {
            case TT_Plus:
                inst = RInstruction(addr_instrc, lchild, rchild, lchild);
                break;
            case TT_Minus:
                inst = RInstruction(subr_instrc, lchild, rchild, lchild);
                break;
            case TT_Multiply:
                inst = RInstruction(smulr_instrc, lchild, rchild, lchild);
                break;
            case TT_Divide:
                inst = RInstruction(sdivr_instrc, lchild, rchild, lchild);
                break;
            case TT_NotEqual:
                /* x ^ x = 0 so for different values result is non-zero  */
                append_instruction(assembler, RInstruction(xorr_instrc, lchild, rchild, lchild));
                inst = RInstruction(setlur_instrc, 0, lchild, lchild);
                break;
            case TT_CompareEqual:
                append_instruction(assembler, RInstruction(xorr_instrc, lchild, rchild, lchild));
                inst = RInstruction(setleur_instrc, lchild, 0, lchild);
                break;
            case TT_GreaterThan:
                /* r1 = r2 < r1 (signed) */
                inst = RInstruction(setlsr_instrc, rchild, lchild, lchild);
                break;
            case TT_GreaterThanEqual:
                /* r1 = r2 <= r1 (signed) */
                inst = RInstruction(setlesr_instrc, rchild, lchild, lchild);
                break;
            case TT_LessThan:
                /* r1 = r1 < r2 (signed) */
                inst = RInstruction(setlsr_instrc, lchild, rchild, lchild);
                break;
            case TT_LessThanEqual:
                /* r1 = r1 <= r2 (signed) */
                inst = RInstruction(setlesr_instrc, lchild, rchild, lchild);
                break;
            default:
                return 1;
        }
    } else {
        /* jump offsets are set after on caller function*/
        switch (node->token) {
            case TT_NotEqual:
                inst = SInstruction(je_instrc, lchild, rchild, 0);
                break;
            case TT_CompareEqual:
                inst = SInstruction(jne_instrc, lchild, rchild, 0);
                break;
            case TT_GreaterThan:
                inst = SInstruction(jles_instrc, lchild, rchild, 0);
                break;
            case TT_GreaterThanEqual:
                inst = SInstruction(jls_instrc, lchild, rchild, 0);
                break;
            case TT_LessThan:
                inst = SInstruction(jles_instrc, rchild, lchild, 0);
                break;
            case TT_LessThanEqual:
                inst = SInstruction(jls_instrc, rchild, lchild, 0);
                break;
            default:
                return -1;
        }
    }
    clear_register(assembler, rchild);
    append_instruction(assembler, inst);
    return lchild;
}

int sstrcmp(const struct string_t *s1, const struct string_t *s2) {
    if (s1->size != s2->size) return 1;
    for (size_t i = 0; i < s1->size; i++)
        if (s1->string[i] != s2->string[i]) return 1;

    return 0;
}

static return_t assemble_identifier(struct Assembler *assembler, const struct StringNode *node) {
    struct pair_t *table_entry = map_getkey_ss(assembler->table, node->value);
    struct string_t fname;
    struct table_entry *function_table;

    if (table_entry->first) { return ((struct table_entry *)(table_entry->second))->allocation_point; }

    fname = get_current_function_name(assembler);

    function_table = map_getkey_ss(assembler->table, fname)->second;

    if (!function_table) return -1U;

    if (array_find(function_table->names, &node->value, (comparer_func)sstrcmp) != -1) {
        struct pair_t *entry = map_getkey_ss(assembler->table, get_current_function_name(assembler));
        if (!entry->first) return -1U;
        return 30 - array_find(((struct table_entry *)entry->second)->names, &node->value, (comparer_func)sstrcmp);
    }

    return -1U;
}

static return_t assemble_lambda(struct Assembler *assembler, const struct LambdaNode *node) {
    struct List *args = create_list();
    struct string_t *name = NULL;
    struct Array *arr;
    struct table_entry *entry;
    uint32_t reg_index = -1U, regs = -1U;

    for (uint32_t i = 0; i < array_size(node->params); ++i) {
        // get current param
        struct StringNode *arg = (struct StringNode *)array_index(node->params, i);
        if (arg->base.type != Identifier) goto destructor_list;

        // request register for param
        reg_index = ralloc(assembler, true, -1);
        if (reg_index == -1U) goto destructor_list;

        // allocate string for param
        name = malloc(sizeof(struct string_t));
        if (!name) goto destructor_list;

        name->size = arg->value.size;
        name->string = arg->value.string;

        if (!list_append(args, name)) {
            free(name);
            goto destructor_list;
        }
    }

    if (!(arr = list_to_array(args))) goto destructor_list;

    if (!(entry = malloc(sizeof(struct table_entry)))) goto destructor_array;

    entry->is_function = true;
    entry->allocation_point = assembler->dot;
    entry->arg_size = array_size(node->params);
    entry->name = node->name;
    entry->names = arr;

    assembler->registers = (1 << array_size(node->params)) - 1;

    if (!map_addss_key(assembler->table, node->name, entry)) goto destructor_entry;

    regs = assemble(assembler, node->expression);

    if (regs == -1U) goto destructor_entry;

    if (!append_instruction(assembler, SInstruction(jalr_instrc, 0, 31, 0))) goto destructor_entry;

    return 1;

destructor_entry:
    free(entry);
destructor_array:
    delete_array(arr, free);
    return -1U;
destructor_list:
    delete_list(args, free);
    ZenithOutOfMemory;
    return -1U;
}

static inline return_t assemble_ternary(struct Assembler *assembler, const struct TernaryNode *node) {
    uint64_t dot_v;
    uint64_t used_true, status_true;
    uint64_t used_false;

    union instruction_t *last_instr;

    if (node->condition->type == Binary)
        assemble_binary(assembler, (struct BinaryNode *)node->condition, true);
    else
        /* checks `cond != 0`, which is what c-like languages do */
        append_instruction(assembler, SInstruction(je_instrc, assemble(assembler, node->condition), 0, 0));

    last_instr = list_get_tail_value(assembler->instructions);

    dot_v = assembler->dot;

    used_true = assemble(assembler, node->trueop);

    if (dot_v == assembler->dot) {
        /* if the node is just a variable reference */
        append_instruction(assembler, RInstruction(addr_instrc, used_true, 0, used_true));
    }

    status_true = assembler->registers;

    append_instruction(assembler, SInstruction(jal_instrc, 0, 0, 0));
    last_instr->stype.immediate = assembler->dot - dot_v;

    last_instr = list_get_tail_value(assembler->instructions);

    dot_v = assembler->dot;

    used_false = assemble(assembler, node->falseop);
    if (dot_v == assembler->dot) /* if the node is just a variable */
        append_instruction(assembler, RInstruction(addr_instrc, used_false, 0, used_false));

    assembler->registers |= status_true;
    last_instr->ltype.immediate = assembler->dot - dot_v;

    return used_true;
}

static return_t assemble_call(struct Assembler *assembler, const struct CallNode *node) {
    return_t return_reg;
    uint64_t offset;
    uint32_t arg_size = array_size(node->arguments);
    uint8_t arg_counter = 30;
    struct string_t name = ((struct StringNode *)(node->expr))->value;
    struct table_entry *sym_entry;
    struct pair_t *pair;

    if (node->expr->type != Identifier) { fprintf(stderr, "expected"); }

    pair = map_getkey_ss(assembler->table, name);

    if (!pair->first) {
        ZENITH_PRINT_ERR(":%*s\n", 0x0700, (int)name.size, name.string);
        return -1LU;
    }

    sym_entry = pair->second;
    if (sym_entry->arg_size != arg_size) { ZENITH_PRINT_ERR(":%d:%d", 0x701, (int)sym_entry->arg_size, arg_size); }

    for (; arg_counter < 30 - arg_size; --arg_counter) {
        uint64_t reg = ralloc(assembler, false, arg_counter);
        if (reg == -1LU) {
            ZENITH_PRINT_ERR("", 0x702);
            return -1LU;
        }
        if (assemble(assembler, array_index(node->arguments, 30 - arg_counter)) == -1LU) { return -1LU; }
    }

    return_reg = ralloc(assembler, false, -1);

    offset = assembler->dot - sym_entry->allocation_point;

    if (offset >= 1 << 18) append_instruction(assembler, LInstruction(auipc_instrc, return_reg, (offset >> 18)));
    append_instruction(assembler, SInstruction(jalr_instrc, return_reg, return_reg, (offset & ((1 << 18) - 1))));

    return return_reg;
}

static return_t assemble(struct Assembler *assembler, const struct Node *node) {
    switch (node->type) {
        case Integer:
            return assemble_number(assembler, (const struct NumberNode *)node);
        case Identifier:
            return assemble_identifier(assembler, (const struct StringNode *)node);
        case Unary:
            return assemble_unary(assembler, (const struct UnaryNode *)node);
        case Binary:
            return assemble_binary(assembler, (const struct BinaryNode *)node, false);
        case Ternary:
            return assemble_ternary(assembler, (const struct TernaryNode *)node);
        case Lambda:
            return assemble_lambda(assembler, (const struct LambdaNode *)node);
        case Call:
            return assemble_call(assembler, (const struct CallNode *)node);
        default:
            fprintf(stderr, "did not recognize node of type %d\n", node->type);
            return -1;
    }
}

struct Array *compile_unit(struct Assembler *assembler) {
    for (size_t i = 0; i < array_size(assembler->parsed_nodes); ++i) {
        const struct Node *node = array_index(assembler->parsed_nodes, i);

        if (assemble(assembler, node) == -1U) break;

        ++assembler->root_index;
    }

    return list_to_array(assembler->instructions);
}

void delete_table_entry(struct table_entry *entry) {
    delete_array(entry->names, free);
    free(entry);
}

void delete_assembler(struct Assembler *assembler) {
    delete_array(assembler->parsed_nodes, (deleter_func)delete_node);
    delete_map(assembler->table, (deleter_func)delete_table_entry);
    free(assembler);
}
