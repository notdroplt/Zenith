#include <nodes.h>
#include <stdlib.h>

struct Node create_node(const enum NodeTypes type, const bool isconst) {
    return (struct Node){
        .type = type,
        .isconst = isconst
    };
}

node_pointer create_intnode(const uint64_t value) {
    struct NumberNode * node = malloc(sizeof(struct NumberNode));
    if (!node) return NULL;
    *node = (struct NumberNode) {
        .base = create_node(Integer, true),
        .value = 0,
        .number = value
    };

    return (node_pointer)node;
}

node_pointer create_doublenode(const double value) {
    struct NumberNode * node = malloc(sizeof(struct NumberNode));
    if (!node) return NULL;
    *node = (struct NumberNode) {
        .base = create_node(Double, true),
        .number = 0,
        .value = value
    };

    return (node_pointer)node;
}

node_pointer create_stringnode(struct string_t value, const enum NodeTypes type) {
    struct StringNode * node = malloc(sizeof(struct StringNode));
    if (!node) return NULL;
    *node = (struct StringNode) {
        .base = create_node(type, false),
        .value = value
    };

    return (node_pointer)node;
}

node_pointer create_unarynode(const node_pointer value, const enum TokenTypes token){
    struct UnaryNode * node = malloc(sizeof(struct UnaryNode));
    if (!node) return NULL;
    *node = (struct UnaryNode) {
        .base = create_node(Unary, value->isconst),
        .token = token,
        .value = value
    };

    return (node_pointer)node;
}

node_pointer create_binarynode(node_pointer left, const enum TokenTypes token, node_pointer right){
    struct BinaryNode * node = malloc(sizeof(struct BinaryNode));
    if (!node) return NULL;
    *node = (struct BinaryNode) {
        .base = create_node(Binary, right->isconst && left->isconst),
        .left = left,
        .right = right,
        .token = token
    };

    return (node_pointer)node;
}

node_pointer create_ternarynode(node_pointer condition, node_pointer true_op, node_pointer false_op){
    struct TernaryNode * node = malloc(sizeof(struct TernaryNode));
    if(!node) return NULL;
    *node = (struct TernaryNode) {
        .base = create_node(Ternary, condition->isconst),
        .condition = condition,
        .trueop = true_op,
        .falseop = false_op
    };

    return (node_pointer)node;
}

node_pointer create_expressionnode(struct string_t name, node_pointer value) {
    struct ExpressionNode * node = malloc(sizeof(struct ExpressionNode));
    if(!node) return NULL;
    *node = (struct ExpressionNode) {
        .base = create_node(Expression, value->isconst),
        .name = name,
        .value = value
    };

    return (node_pointer)node;
}

node_pointer create_listnode(struct Array *node_array){
    struct ListNode * node = malloc(sizeof(struct ListNode));
    if(!node) return NULL;
    *node = (struct ListNode) {
        .base = create_node(List, false), /* actually true but not now */
        .nodes = node_array
    };

    return (node_pointer)node;
}

node_pointer create_callnode(node_pointer expr, struct Array *arg_array, const enum NodeTypes type){
    struct CallNode * node = malloc(sizeof(struct CallNode));
    if(!node) return NULL;
    *node = (struct CallNode ) {
        .base = create_node(type, false),
        .expr = expr,
        .arguments = arg_array,
    };

    return (node_pointer)node;
}

node_pointer create_switchnode(node_pointer expr, struct Array *cases){ 
    struct SwitchNode * node = malloc(sizeof(struct SwitchNode));
    if(!node) return NULL;
    *node = (struct SwitchNode ) {
        .base = create_node(Switch, expr->isconst),
        .expr = expr,
        .cases = cases
    };

    return (node_pointer)node;
}

node_pointer create_lambdanode(struct string_t name, node_pointer expression, struct Array *params_arr) {
    struct LambdaNode * node = malloc(sizeof(struct LambdaNode));
    if (!node) return NULL;
    *node = (struct LambdaNode) {
        .base = create_node(Lambda, false),
        .name = name,
        .expression = expression,
        .params = params_arr
    };

    return (node_pointer)node;
}

node_pointer create_definenode(struct string_t name, node_pointer cast){
    struct DefineNode * node = malloc(sizeof(struct DefineNode));
    if(!node) return NULL;
    *node = (struct DefineNode) {
        .base = create_node(Define, true),
        .name = name,
        .cast = cast
    };

    return (node_pointer)node;
}

node_pointer create_includenode(struct string_t fname, bool binary){
    struct IncludeNode * node = malloc(sizeof(struct IncludeNode));
    if(!node) return NULL;
    *node = (struct IncludeNode) {
        .base = create_node(Include, !binary),
        .fname = fname,
        .binary = binary
    };

    return (node_pointer)node;
}

node_pointer create_scopenode(node_pointer parent, node_pointer child){
    struct ScopeNode * node = malloc(sizeof(struct ScopeNode));
    if(!node) return NULL;
    *node = (struct ScopeNode) {
        .base = create_node(Scope, parent->isconst),
        .parent = parent,
        .child = child
    };

    return (node_pointer)node;
}

node_pointer create_tasknode(struct Array *array) {
    struct TaskNode * node = malloc(sizeof(struct TaskNode));
    if(!node) return NULL;
    *node = (struct TaskNode) {
        .base = create_node(Task, false),
        .tasks = array
    };

    return (node_pointer)node;
}

static void delete_numbernode(struct NumberNode * node) {
    free(node);
}

static void delete_stringnode(struct StringNode * node) {
    free(node);
}

static void delete_unarynode(struct UnaryNode * node) {
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