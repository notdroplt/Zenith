#include <parser.h>
#include <stdlib.h>
#include <stdio.h>

static void parse_next(struct Parser * parser);
static node_pointer parse_ternary(struct Parser * parser);

struct Parser* create_parser(const char * filename){
	struct Parser * parser = NULL;
	size_t size = 0;
	char * content = NULL;
	FILE * fp = fopen(filename, "r");

	if (!fp) {
		fprintf(stderr, "file '%s' does not exist", filename);
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	content = malloc(size);
	if (!content) {
		fclose(fp);
		return NULL;
	}

	if(fread(content, sizeof(char), size, fp) != size) {
		fprintf(stderr, "could not read '%s'\n", filename);
		fclose(fp);
		goto parser_destructor_content;
	}

	fclose(fp);

	parser = calloc(1, sizeof(struct Parser));
	if (!parser) goto parser_destructor_content;

	parser->strings = create_list();

	if (!parser->strings) goto parser_destructor_mem;

	parser->symbols = create_list();
	if (!parser->symbols) {
		delete_list(parser->strings, NULL);
parser_destructor_mem:
		free(parser);
parser_destructor_content:
		free(content);
		return NULL;
	}

	parser->lexer.content = content;
	parser->lexer.current_char = *content;
	parser->lexer.file_size = size;
	parser->lexer.position.column = 1;
	parser->lexer.position.index = 0;
	parser->lexer.position.last_line_pos = 0;
	parser->lexer.position.line = 1;
	parser->sucessfull = true;
	parser->filename = filename;
	parse_next(parser);   
	return parser; 
}


void parser_note(struct Parser * parser, char * note, char * details)
{
	fprintf(stderr, "%s:%d:%d: \033[36m%s:\033[0m %s\n", parser->filename, parser->lexer.position.line, parser->lexer.position.column, note, details);
}

void parser_warn(struct Parser * parser, char * warn, char * details)
{
	fprintf(stderr, "%s:%d:%d: \033[33m%s:\033[0m %s\n", parser->filename, parser->lexer.position.line, parser->lexer.position.column, warn, details);
}

void parser_error(struct Parser * parser, char * error, char * details)
{
	fprintf(stderr, "%s:%d:%d:\033[31m Parser (%s) error:\033[0m %s\n", parser->filename, parser->lexer.position.line, parser->lexer.position.column, error, details);
	parser->sucessfull = 0;
}

static void parse_next(struct Parser * parser) {
	parser->current_token = getNextToken(&parser->lexer);
}

static node_pointer parse_number(struct Parser * parser) {
	if (!parser) return NULL;

	struct token_t token = parser->current_token;
	if (token.type == TT_Unknown) return NULL;

	parse_next(parser);

	if (token.type == TT_Int)
		return create_intnode(token.val.integer);
	return create_doublenode(token.val.number);
}

static node_pointer parse_string(struct Parser * parser, enum TokenTypes type) {
	if (!parser) return NULL;
	struct token_t token = parser->current_token;
	parse_next(parser);
	
	if (token.type == TT_Unknown) return NULL;

	
	struct string_t *str = malloc(sizeof(struct string_t));

	if (!str) return NULL;
	*str = token.string;

	if (list_append(type == TT_String ? parser->strings : parser->symbols, str)) {
		free(str);
		return NULL;
	}
	
	
	return create_stringnode(token.string, (enum NodeTypes) type);

}

static struct Array * parse_comma(struct Parser * parser, enum TokenTypes end_token) {
	struct List *  list = create_list();
	node_pointer value = NULL;
	if (!list) return NULL;

	while(parser->current_token.type != end_token) {
		value = parse_ternary(parser);
		if (!value) goto comma_destructor_list;

		if(list_append(list, value)) goto comma_destructor_all;

		if(parser->current_token.type != TT_Comma || parser->current_token.type == end_token) break;

		parse_next(parser);
	}

	return list_to_array(list);
comma_destructor_all:
	delete_node(value);
comma_destructor_list:
	delete_list(list, (deleter_func)delete_node);
	return NULL;
}

static node_pointer parse_array(struct Parser * parser) {
	struct Array * arr = parse_comma(parser, TT_RightSquareBracket);
	if (!arr) return NULL;
	else if (parser->current_token.type != TT_RightSquareBracket) {
		parser_error(parser, "array", "expected ']'");
		delete_array(arr, (deleter_func)delete_node);
		return NULL;
	}

	parse_next(parser);
	return create_listnode(arr);
}

static void delete_case_pair(struct pair_t * pair) {
	if (pair->first)
		delete_node(pair->first);
	delete_node(pair->second);
}

static node_pointer parse_switch (struct Parser * parser) {
	node_pointer switch_expr = NULL;
	struct List * cases = NULL;

	if (!cases) return NULL;
	parse_next(parser);
	
	if (parser->current_token.type == TT_Unknown) {
		parser_error(parser, "switch", "expected a switch expression");
		return NULL;
	}

	switch_expr = parse_ternary(parser);

	if (!switch_expr) return NULL;

	cases = create_list();
	if (!cases) {
		delete_node(switch_expr);
		return NULL;
	}

	while(parser->current_token.val.keyword != KW_end) {
		node_pointer case_expr, case_return;
		if (parser->current_token.type != TT_Colon && parser->current_token.val.keyword != KW_default) {
			parser_error(parser, "case", "':' or \"default\" to initiate a case expression.");
			goto delete_xcase;
		}

		if (parser->current_token.type == TT_Colon) {
			parse_next(parser);
			case_expr = parse_ternary(parser);
			if (!case_expr) {
				parser_error(parser, "case", "expected a case expression");
				goto delete_xcase_ret;
			}
		} else {
			parse_next(parser);
			case_expr = NULL;
		}

		if (parser->current_token.type != TT_Equal) {
			parser_error(parser, "case", "expected '=' after case expression");
			goto delete_xcase_ret;
		}

		parse_next(parser);

		case_return = parse_ternary(parser);

		if (!case_return) goto delete_xcase_ret;

		struct pair_t * pair = malloc(sizeof(struct pair_t));

		if (!pair) goto delete_xpair;
		
		pair->first = case_expr;
		pair->second = case_return;

		if (list_append(cases, pair))
		{
			free(pair);
delete_xpair:
			delete_node(case_return);
delete_xcase_ret:
			delete_node(case_expr);
			goto delete_xcase;
		}
	}

	if (!parser->current_token.type || parser->current_token.val.keyword != KW_end) {
		parser_error(parser, "switch", "expected 'end' to close switch expression");
delete_xcase:
		delete_node(switch_expr);
		delete_list(cases, (deleter_func)delete_case_pair);
		return NULL;
	}

	return create_switchnode(switch_expr, list_to_array(cases));
}

static node_pointer parse_task(struct Parser * parser) {
	parser_error(parser, "unimplemented", "tasks are yet to be implemented");
	return NULL;
}

static node_pointer parse_atom(struct Parser * parser) {
	node_pointer node;
	if (parser->current_token.type == TT_Unknown) return NULL;

	switch (parser->current_token.type) {
		case TT_Int:
		case TT_Double:
			return parse_number(parser);
		case TT_String:
		case TT_Identifier:
			return parse_string(parser, parser->current_token.type);
		case TT_Keyword:
			if (parser->current_token.val.keyword == KW_switch)
				return parse_switch(parser);
			else if (parser->current_token.val.keyword == KW_do)
				return parse_task(parser);
			return NULL;
		case TT_LeftParentesis:
			parse_next(parser);

			node = parse_ternary(parser);
			if (!node) return NULL;
			if (parser->current_token.type != TT_RightParentesis) {
				parser_error(parser, "atom", "expected ')'");
				delete_node(node);
				return NULL;
			}
			parse_next(parser);
			return node;
		case TT_LeftSquareBracket:
			parse_next(parser);
			return parse_array(parser);
		default:
			parser_error(parser, "atom", "unknown case");
			return NULL;
	}
}

static node_pointer parse_prefix(struct Parser * parser) {
	enum TokenTypes type = parser->current_token.type;
	node_pointer node;

	/* the unary plus has some uses on C, but because*/
	if (type == TT_Plus) {
		parser_warn(parser, "discouraged uses", "use of unary plus operator `+(value)` is discouraged");
		parse_next(parser);
		return parse_atom(parser);
	}

	if (type != TT_Minus && type != TT_Not) return parse_atom(parser);

	parse_next(parser);

	node = parse_ternary(parser);
	if (!node) return NULL;

	return create_unarynode(node, type);
}

static node_pointer parse_postfix(struct Parser * parser) {
	node_pointer node = parse_prefix(parser);
	if (!node) return NULL;
	if (parser->current_token.type == TT_Dot) {
		node_pointer child;
		parse_next(parser);
		child = parse_postfix(parser);
		if (!child) {
			delete_node(node);
			return NULL;
		}

		return create_scopenode(node, child);
	}
	if (parser->current_token.type == TT_LeftSquareBracket) {
		struct Array * index = create_array(1);
		if (!index) goto index_destructor_node;
		parse_next(parser);
		array_set_index(index, 0, parse_ternary(parser));
		if (!array_index(index, 0)) {
			parser_error(parser, "indexing", "expected an expression for an index");
			goto index_destructor_full;
		}
		else if (parser->current_token.type != TT_RightSquareBracket) {
			parser_error(parser, "indexing", "expected a ']'to close an index marker");
			goto index_destructor_full;
		}

		parse_next(parser);
		return create_callnode(node, index, Index);
index_destructor_full:
		delete_array(index, (deleter_func)delete_node);
index_destructor_node:
		delete_node(node);
		return NULL;
	}
	if (parser->current_token.type == TT_LeftParentesis) {
		struct Array * arguments;
		parse_next(parser);
		arguments = parse_comma(parser, TT_RightParentesis);

		if (!arguments) {
			delete_node(node);
			return NULL;
		}

		if (parser->current_token.type != TT_RightParentesis) {
			parser_error(parser, "function call", "expected a ')' after function call");
			delete_array(arguments, (deleter_func)delete_node);
			delete_node(node);
			return NULL;
		}

		parse_next(parser);
		return create_callnode(node, arguments, Call);
	}
	return node;
}

static node_pointer parse_term(struct Parser * parser) {
	node_pointer lside = parse_postfix(parser), rside;
	if (parser->current_token.type == TT_Unknown) return lside;

	while (parser->current_token.type == TT_Multiply || parser->current_token.type == TT_Divide) {
		enum TokenTypes tok = parser->current_token.type;
		parse_next(parser);

		rside = parse_postfix(parser);
		if (!rside) goto term_destructor_lside;

		if (lside->type == String || rside->type == String) {
			parser_error(parser, "binary", "cannot '*' or '/' a string");
			goto term_destructor_all;
		}

		lside = create_binarynode(lside, tok, rside);
		if (parser->current_token.type == TT_Unknown) break;
	}

	return lside;
term_destructor_all:
	delete_node(rside);
term_destructor_lside:
	delete_node(lside);
	return NULL;
}

static node_pointer parse_factor(struct Parser * parser) {
	node_pointer lside = parse_term(parser), rside;
	if (parser->current_token.type == TT_Unknown) return lside;

	while (parser->current_token.type == TT_Plus || parser->current_token.type == TT_Minus) {
		enum TokenTypes tok = parser->current_token.type;
		parse_next(parser);

		rside = parse_term(parser);
		if (!rside) goto factor_destructor_lside;

		if (lside->type == String || rside->type == String) {
			parser_error(parser, "binary", "cannot '+' or '-' a string");
			goto factor_destructor_all;
		}

		lside = create_binarynode(lside, tok, rside);
		if (parser->current_token.type == TT_Unknown) break;
	}

	return lside;
factor_destructor_all:
	delete_node(rside);
factor_destructor_lside:
	delete_node(lside);
	return NULL;
}

static node_pointer parse_comparisions(struct Parser * parser) {
	node_pointer lside = parse_factor(parser), rside;
	if (parser->current_token.type == TT_Unknown) return lside;

	while (parser->current_token.type >= TT_LessThan && parser->current_token.type <= TT_GreaterThanEqual) {
		enum TokenTypes tok = parser->current_token.type;
		parse_next(parser);

		rside = parse_factor(parser);
		if (!rside) goto comp_destructor_lside;

		if (lside->type == String || rside->type == String) {
			parser_error(parser, "binary", "cannot '==', '!=', '>', '>=', '<', or '<=' a string");
			goto comp_destructor_all;
		}

		lside = create_binarynode(lside, tok, rside);
		if (parser->current_token.type == TT_Unknown) break;
	}

	return lside;
comp_destructor_all:
	delete_node(rside);
comp_destructor_lside:
	delete_node(lside);
	return NULL;
}

static node_pointer parse_ternary(struct Parser * parser) {
	node_pointer condition = parse_comparisions(parser), trueop, falseop;

	if (!condition || parser->current_token.type != TT_Question || parser->current_token.type == TT_Unknown) {
		return condition;
	}

	parse_next(parser);

	if (parser->current_token.type == TT_Unknown) {
		parser_error(parser, "ternary", "expected expression after '?'");
		goto ter_destructor_cond;
	} else if (!(trueop = parse_comparisions(parser))) {
		goto ter_destructor_cond;
	} else if (parser->current_token.type == TT_Unknown || 
				parser->current_token.type != TT_Colon) {
		parser_error(parser, "ternary", "expected expression before ':'");
		goto ter_destructor_true;
	}

	parse_next(parser);

	falseop = parse_comparisions(parser);
	if (!falseop) goto ter_destructor_true;

	return create_ternarynode(condition, trueop, falseop);

ter_destructor_true:
	delete_node(trueop);
ter_destructor_cond:
	delete_node(condition);
	return NULL;
}

static struct string_t parse_name(struct Parser * parser) {
	if (parser->current_token.type != TT_Identifier) {
		parser_error(parser, "name", "token cannot be a name");
	}
	struct string_t string = parser->current_token.string;
	parse_next(parser);
	return string;
}

static node_pointer parse_lambda(struct Parser * parser, struct string_t name, bool arrowed) {	
	if (arrowed) {
		node_pointer ptr = parse_ternary(parser);
		if (!ptr) return NULL;
		return create_expressionnode(name, ptr);
	}

	struct List * list = create_list();

	if (!list) return NULL;

	do {
		if (parser->current_token.type == TT_Comma) {
			parse_next(parser);
			continue;
		} else if (parser->current_token.type != TT_Identifier)
			break;
		
		if (list_append(list, create_stringnode(parser->current_token.string, Identifier))) 
			goto lambda_destructor;
		parse_next(parser);
	} while(parser->current_token.type);

	if (!arrowed && parser->current_token.type != TT_RightParentesis) {
		parser_error(parser, "lambda", "expected ')' to close parameter list");
		goto lambda_destructor;
	} else if (!arrowed)
		parse_next(parser);

	if (parser->current_token.type != TT_Arrow) {
		parser_error(parser, "lambda", "expected '=>' after parameter list");
		goto lambda_destructor;
	}
	parse_next(parser);

	node_pointer expr = parse_ternary(parser);

	if (!expr) {
lambda_destructor:
		delete_list(list, (deleter_func)delete_node);
		return NULL;
	}

	return create_lambdanode(name, expr, list_to_array(list));
}

static node_pointer parse_define(struct Parser * parser, struct string_t name) {
	node_pointer cast = parse_string(parser, parser->current_token.type);
	if (!cast) 
		return NULL;
	return create_definenode(name, cast);
}

static node_pointer parse_toplevel(struct Parser * parser) {
	if (parser->current_token.val.keyword == KW_include) {
		parse_next(parser);
		struct string_t str = parser->current_token.string;
		parse_next(parser);
		return create_includenode(str, false);
	}

	struct string_t name = parse_name(parser);
	if (!name.size) {
		parser_error(parser, "top-level expression", "a name is necessary for every expression assigned");
		return NULL;
	}

	struct token_t tok = parser->current_token;
	parse_next(parser);

	if (tok.type == TT_LeftParentesis || tok.type == TT_Arrow) 
		return parse_lambda(parser, name, tok.type == TT_Arrow);
	else if (tok.type == TT_Colon || tok.val.keyword == KW_as) {
		parser_warn(parser, "discouraged uses", "use of `defines` is not yet completed, consider removing for now");
		return parse_define(parser, name);
	} else if (tok.type == TT_Equal) {
		node_pointer ptr = parse_ternary(parser);
		if (!ptr) return NULL;
		return create_expressionnode(name, ptr);
	}

	parser_error(parser, "top-level expression", "expected either '(', '=>', '=', ':', or \"as\" after a name");
	return NULL;
}

struct Array * translate_unit(struct Parser * parser) {
	if (!parser->lexer.content) return NULL;

	struct List * list = create_list();
	
	while(parser->current_token.type) {
		node_pointer node = parse_toplevel(parser);
		if (!node || !parser->sucessfull) break;
		list_append(list, node);
	}

	bool success = parser->sucessfull;

	delete_list(parser->symbols, free);
	delete_list(parser->strings, free);
	free(parser->lexer.content);
	free(parser);

	if (success) 
		return list_to_array(list);
	delete_list(list, (deleter_func)delete_node);
	return NULL;
}
