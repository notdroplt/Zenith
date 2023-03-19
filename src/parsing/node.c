#include <string.h>
#include <stdio.h>
#include "types.h"
#include "nodes.h"
#include "optimizing.h"

struct Node create_node(const enum NodeTypes type, const bool isconst) {
    return (struct Node){
        .type = type,
        .isconst = isconst
    };
}

struct Node * create_intnode(const uint64_t value) {
    struct NumberNode * node = malloc(sizeof(struct NumberNode));
    if (!node) return NULL;
    *node = (struct NumberNode) {
        .base = create_node(Integer, true),
        .value = 0,
        .number = value
    };

    return (struct Node *)node;
}

struct Node * create_doublenode(const double value) {
    struct NumberNode * node = malloc(sizeof(struct NumberNode));
    if (!node) return NULL;
    *node = (struct NumberNode) {
        .base = create_node(Double, true),
        .number = 0,
        .value = value
    };

    return (struct Node *)node;
}

struct Node * create_stringnode(struct string_t value, const enum NodeTypes type) {
	struct StringNode * node = malloc(sizeof(struct StringNode));
	if (!node) return NULL;
	
	*node = (struct StringNode) {
		.base = create_node(type, false),
		.value = value
    	};

	return (struct Node *)node;
}

struct Node * create_unarynode(struct Node * value, const enum TokenTypes token){
	
	struct Node * node = optimized_unarynode(value, token);
    if (node) return node;
    
    node = malloc(sizeof(struct UnaryNode));
	if (!node) return NULL;
	
	*(struct UnaryNode *)node = (struct UnaryNode) {
		.base = create_node(Unary, value->isconst),
		.token = token,
        .value = value
	};

	return node;
}

struct Node * create_binarynode(struct Node * left, const enum TokenTypes token, struct Node * right){
    struct Node * node = optimized_binarynode(left, token, right);
    if (node) return node;

    node = malloc(sizeof(struct BinaryNode));
    if (!node) return NULL;
    
    *(struct BinaryNode *)node = (struct BinaryNode) {
        .base = create_node(Binary, right->isconst && left->isconst),
        .left = left,
        .right = right,
        .token = token
    };

    return node;
}

struct Node * create_ternarynode(struct Node * condition, struct Node * true_op, struct Node * false_op){
    struct Node * node = optimized_ternarynode(condition, true_op, false_op);

    if (node) return node;
    node = malloc(sizeof(struct TernaryNode));
    
    if(!node) return NULL;
    *(struct TernaryNode *)node = (struct TernaryNode) {
        .base = create_node(Ternary, condition->isconst),
        .condition = condition,
        .trueop = true_op,
        .falseop = false_op
    };

    return (struct Node *)node;
}

struct Node * create_expressionnode(struct string_t name, struct Node * value) {
    struct ExpressionNode * node = malloc(sizeof(struct ExpressionNode));
    if(!node) return NULL;
    *node = (struct ExpressionNode) {
        .base = create_node(Expression, value->isconst),
        .name = name,
        .value = value
    };

    return (struct Node *)node;
}

struct Node * create_listnode(struct Array *node_array){
    struct ListNode * node = malloc(sizeof(struct ListNode));
    if(!node) return NULL;
    *node = (struct ListNode) {
        .base = create_node(List, false), /* actually true but not now */
        .nodes = node_array
    };

    return (struct Node *)node;
}

struct Node * create_callnode(struct Node * expr, struct Array *arg_array, const enum NodeTypes type){
    struct CallNode * node = malloc(sizeof(struct CallNode));
    if(!node) return NULL;
    *node = (struct CallNode ) {
        .base = create_node(type, false),
        .expr = expr,
        .arguments = arg_array,
    };

    return (struct Node *)node;
}

struct Node * create_switchnode(struct Node * expr, struct Array *cases){ 
    struct SwitchNode * node = malloc(sizeof(struct SwitchNode));
    if(!node) return NULL;
    *node = (struct SwitchNode ) {
        .base = create_node(Switch, expr->isconst),
        .expr = expr,
        .cases = cases
    };

    return (struct Node *)node;
}

struct Node * create_lambdanode(struct string_t name, struct Node * expression, struct Array *params_arr) {
    struct LambdaNode * node = malloc(sizeof(struct LambdaNode));
    if (!node) return NULL;
    *node = (struct LambdaNode) {
        .base = create_node(Lambda, false),
        .name = name,
        .expression = expression,
        .params = params_arr
    };

    return (struct Node *)node;
}

struct Node * create_definenode(struct string_t name, struct Node * cast){
    struct DefineNode * node = malloc(sizeof(struct DefineNode));
    if(!node) return NULL;
    *node = (struct DefineNode) {
        .base = create_node(Define, true),
        .name = name,
        .cast = cast
    };

    return (struct Node *)node;
}

struct Node * create_includenode(struct string_t fname, bool binary){
    struct IncludeNode * node = malloc(sizeof(struct IncludeNode));
    if(!node) return NULL;
    *node = (struct IncludeNode) {
        .base = create_node(Include, !binary),
        .fname = fname,
        .binary = binary
    };

    return (struct Node *)node;
}

struct Node * create_scopenode(struct Node * parent, struct Node * child){
    struct ScopeNode * node = malloc(sizeof(struct ScopeNode));
    if(!node) return NULL;
    *node = (struct ScopeNode) {
        .base = create_node(Scope, parent->isconst),
        .parent = parent,
        .child = child
    };

    return (struct Node *)node;
}

struct Node * create_tasknode(struct Array *array) {
    struct TaskNode * node = malloc(sizeof(struct TaskNode));
    if(!node) return NULL;
    *node = (struct TaskNode) {
        .base = create_node(Task, false),
        .tasks = array
    };

    return (struct Node *)node;
}

static void delete_numbernode(struct NumberNode * node) {
    free(node);
}

static void delete_stringnode(struct StringNode * node) {
    free(node);
}

static void delete_unarynode(struct UnaryNode * node) {
    if (node->value)
        delete_node(node->value);
    free(node);
}

static void delete_binarynode(struct BinaryNode * node) {
    delete_node(node->left);
    delete_node(node->right);
    free(node);
}

static void delete_ternarynode(struct TernaryNode* node) {
    delete_node(node->condition);
    delete_node(node->falseop);
    delete_node(node->trueop);
    free(node);
}

static void delete_expressionnode(struct ExpressionNode* node) {
    if (node->value)
        delete_node(node->value);
    free(node);
}

static void delete_listnode(struct ListNode * node) {
    delete_array(node->nodes, (deleter_func)delete_node);
    free(node);
}

static void delete_callnode(struct CallNode * node) {
    delete_array(node->arguments, (deleter_func)delete_node);
    delete_node(node->expr);
    free(node);
}

static void delete_switchnode(struct SwitchNode * node) {
    for (size_t i = 0; i < array_size(node->cases); i++)
    {
        struct pair_t * pair = (struct pair_t*)array_index(node->cases, i);
        if (pair->first)
            delete_node(pair->first);
        delete_node(pair->second);
    }
    delete_array(node->cases, NULL);
    delete_node(node->expr);
    free(node);
}

static void delete_lambdanode(struct LambdaNode * node) {
    delete_array(node->params, (deleter_func)delete_node);
    delete_node(node->expression);
    free(node);
}

static void delete_definenode(struct DefineNode * node) {
    delete_node(node->cast);
    free(node);
}

static void delete_includenode(struct IncludeNode * node){
    free(node);
}

static void delete_scopenode(struct ScopeNode * node) {
    delete_node(node->parent);
    delete_node(node->child);
    free(node);
}

static void delete_tasknode(struct TaskNode * node) {
    delete_array(node->tasks, (deleter_func)delete_node);
    free(node);
}

void delete_node(struct Node * node) {
    if (!node) return;
    switch(node->type){
        case Integer:
        case Double:
            delete_numbernode((struct NumberNode*)node);
            break;
        case String:
        case Identifier:
            delete_stringnode((struct StringNode*)node);
            break;
        case Unary:
            delete_unarynode((struct UnaryNode*)node);
            break;
        case Binary:
            delete_binarynode((struct BinaryNode*)node);
            break;
        case Ternary:
            delete_ternarynode((struct TernaryNode*)node);
            break;
        case Expression:
            delete_expressionnode((struct ExpressionNode*)node);
            break;
        case List:
            delete_listnode((struct ListNode*)node);
            break;
        case Index:
        case Call:
            delete_callnode((struct CallNode*)node);
            break;
        case Lambda:
            delete_lambdanode((struct LambdaNode*)node);
            break;
        case Switch:
            delete_switchnode((struct SwitchNode*)node);
            break;
        case Define:
            delete_definenode((struct DefineNode*)node);
            break;
        case Include:
            delete_includenode((struct IncludeNode*)node);
            break;
        case Scope:
            delete_scopenode((struct ScopeNode*)node);
            break;
        case Task:
            delete_tasknode((struct TaskNode*)node);
            break;
        case Unused:
            break;
    }
}

#define typecast(type, val) ((type)val)
int node_equals(const struct Node * left, const struct Node * right) {
    if (left->type != right->type) return false;

    switch (left->type) {
        case Unused:
            return false;
        case Integer:
            return typecast(struct NumberNode*, left)->number == typecast(struct NumberNode*, right)->number;
        case Double:
            return typecast(struct NumberNode*, left)->value == typecast(struct NumberNode*, right)->value;
        case String:
        case Identifier:
            if (typecast(struct StringNode*, left)->value.size != typecast(struct StringNode*, right)->value.size) return false;
            return strncmp(
                typecast(struct StringNode*, left)->value.string,
                typecast(struct StringNode*, right)->value.string,
                typecast(struct StringNode*, left)->value.size
            ) == 0;
        case Unary:
            return typecast(struct UnaryNode *, left)->token == typecast(struct UnaryNode *, right)->token &&
                node_equals(
                    typecast(struct UnaryNode *, left)->value,
                    typecast(struct UnaryNode *, right)->value
                );
        case Binary:
            return typecast(struct BinaryNode *, left)->token == typecast(struct BinaryNode *, right)->token &&
                node_equals(
                    typecast(struct BinaryNode *, left)->left,
                    typecast(struct BinaryNode *, right)->left
                ) && 
                node_equals(
                    typecast(struct BinaryNode *, left)->right,
                    typecast(struct BinaryNode *, right)->right
                );
        case Ternary:
            return node_equals(
                    typecast(struct TernaryNode *, left)->condition,
                    typecast(struct TernaryNode *, right)->condition
                ) &&
                node_equals(
                    typecast(struct TernaryNode *, left)->trueop,
                    typecast(struct TernaryNode *, right)->trueop
                ) &&
                node_equals(
                    typecast(struct TernaryNode *, left)->falseop,
                    typecast(struct TernaryNode *, right)->falseop
                );
        case Expression:
            if (typecast(struct ExpressionNode*, left)->name.size != typecast(struct ExpressionNode*, right)->name.size) return false;
            return strncmp(
                typecast(struct ExpressionNode*, left)->name.string,
                typecast(struct ExpressionNode*, right)->name.string,
                typecast(struct ExpressionNode*, left)->name.size
            ) == 0 && 
            node_equals(
                typecast(struct ExpressionNode*, left)->value,
                typecast(struct ExpressionNode*, right)->value
            );
        case List:
            if (array_size(typecast(struct ListNode*, left)->nodes) != array_size(typecast(struct ListNode*, right)->nodes)) return false;
            return array_compare(
                typecast(struct ListNode*, left)->nodes,
                typecast(struct ListNode*, right)->nodes,
                (comparer_func)node_equals);
        case Call:
        case Index:
            return node_equals(typecast(struct CallNode*, left)->expr, typecast(struct CallNode*, right)->expr) &&
            array_compare(
                typecast(struct CallNode*, left)->arguments, 
                typecast(struct CallNode*, left)->arguments, 
                (comparer_func)node_equals
            );
        case Lambda:
            if (typecast(struct LambdaNode*, left)->name.size != typecast(struct LambdaNode*, right)->name.size) return false;
            return strncmp(
                typecast(struct LambdaNode*, left)->name.string,
                typecast(struct LambdaNode*, right)->name.string,
                typecast(struct LambdaNode*, left)->name.size
            ) == 0 && node_equals(
                typecast(struct LambdaNode*, left)->expression,
                typecast(struct LambdaNode*, left)->expression
            ) && array_compare(
                typecast(struct LambdaNode*, left)->params, 
                typecast(struct LambdaNode*, right)->params, 
                (comparer_func)node_equals
            );
        case Switch:
            return node_equals(
                typecast(struct SwitchNode *, left)->expr,
                typecast(struct SwitchNode *, right)->expr
            ) && array_compare(
                typecast(struct SwitchNode *, left)->cases, 
                typecast(struct SwitchNode *, right)->cases,
                (comparer_func)node_equals
            );
        case Define:
            if (typecast(struct DefineNode *, left)->name.size != typecast(struct DefineNode *, right)->name.size) return false;
            return strncmp(
                typecast(struct DefineNode *, left)->name.string,
                typecast(struct DefineNode *, right)->name.string,
                typecast(struct DefineNode *, left)->name.size
            ) == 0 && node_equals(
                typecast(struct DefineNode * , left)->cast, 
                typecast(struct DefineNode * , right)->cast
            );
        case Include:
            if (typecast(struct IncludeNode *, left)->fname.size != typecast(struct IncludeNode *, right)->fname.size) return false;
            return strncmp(
                typecast(struct IncludeNode *, left)->fname.string,
                typecast(struct IncludeNode *, right)->fname.string,
                typecast(struct IncludeNode *, left)->fname.size
            ) == 0;
        case Scope:
            return node_equals(
                typecast(struct ScopeNode *, left)->parent, 
                typecast(struct ScopeNode *, right)->parent
            ) && node_equals(
                typecast(struct ScopeNode *, left)->child, 
                typecast(struct ScopeNode *, right)->child
            );
        case Task:
            return array_compare(
                typecast(struct TaskNode *, left)->tasks, 
                typecast(struct TaskNode *, right)->tasks,
                (comparer_func)node_equals
            );
            break;
    }
    return false;
}

void node_format(FILE * file, const struct Node * root) {
    switch (root->type) {
        case Integer:
            fprintf(file, "{\"type\":\"int\",\"value\":%ld}", ((struct NumberNode *)root)->number);
            break;
        case Double:
            fprintf(file, "{\"type\":\"double\",\"value\":%lf}", ((struct NumberNode *)root)->value);
            break;
        case String:
        case Identifier:
            fprintf(file, "{\"type\":\"%s\",\"value\":%*s}", root->type == String ? "str" : "id", (int)((struct StringNode *)root)->value.size, ((struct StringNode *)root)->value.string);
            break;
        case Unary:
            fprintf(file, "{\"type\":\"unary\",\"token\":%d,\"value\":", ((struct UnaryNode *)root)->token);
            node_format(file, ((struct UnaryNode *)root)->value);
            putc('}', file);
            break;
        case Binary:
            fprintf(file, "{\"type\":\"binary\",\"token\":%d,\"left\":", ((struct BinaryNode *)root)->token);
            node_format(file, ((struct BinaryNode *)root)->left);
            fputs(",\"right\":", file);
            node_format(file, ((struct BinaryNode *)root)->right);
            putc('}', file);
            break;
        case Ternary:
            fputs("{\"type\":\"ternary\",\"condition\":", file);
            node_format(file, ((struct BinaryNode *)root)->right);
            break;
        case Expression:
            fprintf(file, "{\"type\":\"expression\",\"name\":%*s,\"value\":", (int)((struct ExpressionNode *)root)->name.size, ((struct ExpressionNode *)root)->name.string);
            node_format(file, ((struct ExpressionNode *)root)->value);
        case List:
        case Call:
        case Lambda:
        case Switch:
        case Define:
        case Index:
        case Include:
        case Scope:
        case Task:
        default:
            break;
    }
}
