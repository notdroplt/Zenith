#include <compiler.h>

struct table_entry {
    struct string_t name;
    struct Array * names;
    uint64_t allocation_point; 
    uint8_t arg_size; /* although possible, please reconsider what you are coding before writing a function with more than 255 arguments */
    bool is_function;
};

typedef uint64_t return_t;
static return_t assemble(struct Assembler * assembler, const struct Node * node);

struct Assembler * create_assembler(const struct Array * parsed_nodes){
    struct Assembler * assembler = malloc(sizeof(struct Assembler));
    if (!assembler) goto destructor_final;

    assembler->instructions = create_list();
    if (!assembler->instructions) goto destructor_1;

    assembler->table = create_map(0);
    if (!assembler->table) goto destructor_2;

    assembler->parsed_nodes = parsed_nodes;
    assembler->dot = 0x1000;
    assembler->entry_point = 0;
    assembler->root_index = 0;
destructor_2:
    delete_list(assembler->instructions, NULL);
destructor_1:
    free(assembler);
destructor_final:
    return NULL;
}

static int32_t request_register(struct Assembler * assembler, bool descending, int32_t set_next) {
    static int32_t next_reg = -1;
    int i;

    if (set_next != -1) {
        next_reg = set_next;
        // compiler can then check if register is free
        return assembler->registers & (1 << set_next);
    }
    
    if (next_reg != -1 && assembler->registers & (1 << next_reg)) {
        // get pre-reserved register
        assembler->registers |= 1 << next_reg;
        return next_reg;
    }

    if (!descending) {
        // normal register usage
        for (i = 1; i < 32; ++i)
            if (!(assembler->registers & (1 << i)))
                break;
    } else {
        // passing arguments around
        for (i = 31; i >= 0; --i)
            if (!(assembler->registers & (1 << i)))
                break;
    }
    
    if (i != -1) {
        assembler->registers |= 1 << i;
        return i;
    } 
    
    //! TODO: use stack (if target supports it)
    return -1;
}

static void clear_register(struct Assembler * assembler, uint32_t index) {
    assembler->registers &= ~(1 << index);
}

static int append_instruction(struct Assembler * assembler, const union instruction_t instruction){
    if (list_append(assembler->instructions, (void*)instruction.value))
        return 1;
    assembler->dot += 8;
    return 0;
}

static struct string_t get_current_function_name(struct Assembler * assembler) {
    return ((struct LambdaNode*)array_index(assembler->parsed_nodes, assembler->root_index))->name;
}

static return_t assemble_number(struct Assembler * assembler, const struct NumberNode * node){
    if (node->value == 0.0 && node->number == 0) return 0;

    uint32_t reg_index = request_register(assembler, false, -1);
    if (reg_index == -1U) return -1U;

    if (node->number > 0x400000000000)
        append_instruction(assembler, LInstruction(lui_instrc, reg_index, node->number >> 18));
    
    append_instruction(assembler, SInstruction(addi_instrc, 0, reg_index, node->number & ((1LLU << 46) - 1)));

    return reg_index;
}

static return_t assemble_unary(struct Assembler * assembler, const struct UnaryNode* node) {
    return_t used_reg = assemble(assembler, node->value);
    union instruction_t inst;
    if (used_reg == -1U) return used_reg;

    switch (node->token)
    {
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

static return_t assemble_binary(struct Assembler * assembler, const struct BinaryNode * node, bool isJumping) {
    return_t lchild = assemble(assembler, node->left),
             rchild = assemble(assembler, node->right);
            
    union instruction_t inst;

    if (rchild == -1U || lchild == -1U) 
        return lchild;
    

    if (!isJumping) {
        switch (node->token)
        {
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
        switch (node->token)
        {
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

static int sstrcmp(struct string_t * s1, struct string_t * s2) {
    if (s1->size != s2->size) return 1;
    for (size_t i = 0; i < s1->size; i++)
        if (s1->string[i] != s2->string[i]) return 1;
    
    return 0;    
}

static return_t assemble_identifier(struct Assembler * assembler, const struct StringNode * node) {
    struct pair_t * table_entry = map_getkey_ss(assembler->table, node->value);

    if (table_entry->first) {
        return ((struct table_entry *)(table_entry->second))->allocation_point;
    }

    if (strncmp(get_current_function_name(assembler).string, node->value.string, node->value.size) == 0) {
        struct pair_t * entry = map_getkey_ss(assembler->table, get_current_function_name(assembler));
        if (!entry->first) return -1U;
        return 30 - array_find(((struct table_entry*)entry->second)->names, &node->value, (comparer_func)sstrcmp);
    }

    return -1U;
    
}

static return_t assemble_lambda(struct Assembler * assembler, const struct LambdaNode * node) {
    struct List * args = create_list();
    for (uint32_t i = 0; i < array_size(node->params); ++i) {
        struct Node * arg = (struct Node *)array_index(node->params, i);
        if (arg->type != Identifier) goto destructor_list;
        
        uint32_t reg_index = request_register(assembler, true, -1);
        if (reg_index == -1U) goto destructor_list;

        struct table_entry * entry = map_getkey_ss(assembler->table, node->name)->second;
        if (!entry) goto destructor_list;

        struct string_t * name = malloc(sizeof(struct string_t));
        if (!name) goto destructor_list;

        if (!list_append(args, name)) {
            free(name);
            goto destructor_list;
        }        
    }

    struct Array * arr = list_to_array(args);
    if (!arr) goto destructor_list;

    struct table_entry * entry = malloc(sizeof(struct table_entry));
    if (!entry) goto destructor_array;

    entry->is_function = true;
    entry->allocation_point = assembler->dot;
    entry->arg_size = array_size(node->params);
    entry->name = node->name;
    entry->names = arr;

    assembler->registers = (1 << array_size(node->params)) - 1;

    uint32_t regs = assemble(assembler, node->expression);

    if (regs == -1U)
        goto destructor_entry;

    if (append_instruction(assembler, SInstruction(jalr_instrc, 0, 31, 0)))
        goto destructor_entry;

    if (!map_addss_key(assembler->table, node->name, entry))
        goto destructor_entry;
    
    return 1; 

destructor_entry:
    free(entry);
destructor_array:
    delete_array(arr, free);
    return -1U;
destructor_list:
    delete_list(args, free);
    return -1U;
}


static return_t assemble(struct Assembler * assembler, const struct Node * node){
    switch (node->type)
    {
    case Integer:
        return assemble_number(assembler, (const struct NumberNode*)node);
    case Identifier:
        return assemble_identifier(assembler, (const struct StringNode*)node);
    case Unary:
        return assemble_unary(assembler, (const struct UnaryNode*)node);
    case Binary:
        return assemble_binary(assembler, (const struct BinaryNode*)node, false);
    case Lambda:
        return assemble_lambda(assembler, (const struct LambdaNode*)node);    
    default:
        return -1;
    }
}

struct Array * translate_unit(struct Assembler * assembler) {
    for (size_t i = 0; i < array_size(assembler->parsed_nodes); ++i)
    {
        const struct Node * node = array_index(assembler->parsed_nodes, i);

        if (assemble(assembler, node) == -1U) break;

        ++assembler->root_index;
    }

    return list_to_array(assembler->instructions);
}