#include "plugin_loader.h"
#include <ctype.h>
#include <math.h>
#include <limits.h>
enum state
{
    cs_single_line,
    cs_multi_line,
    cs_asterisk_in_multiline
};

string_view *skip_comment(__attribute__((unused)) struct lexer_plugin_t * lexer, string_view *str)
{
    enum state currentState;
    char currentChar;

    if (view_start(str) != '/' && (view_index(char, str, 1) != '/' && view_index(char, str, 1) != '*'))
        return str;

    view_walk(char, str);

    currentState = view_start(str) == '/' ? cs_single_line : cs_multi_line;

    while ((currentChar = view_start(str)))
    {
        switch (currentState)
        {
        case cs_single_line:
            if (currentChar == '\n' || currentChar == '\0')
                return str;
            break;
        case cs_multi_line:
            if (currentChar == '*')
                currentState = cs_asterisk_in_multiline;

            break;

        case cs_asterisk_in_multiline:
            if (currentChar == '/')
            {
                return str;
            }
            else if (currentChar != '*')
            {
                currentState = cs_multi_line;
            }
            break;
        }
        view_walk(char, str);
    }

    return NULL;
}

enum prefixes
{
    int_decimal = 10,
    int_binary = 2,
    int_octal = 8,
    int_hex = 16
};

static int is_digit(char c, enum prefixes prefix) {
	switch (prefix) {
		case int_binary:
			return c == '0' || c == '1';
		case int_octal:
			return c >= '0' && c <= '7';
		case int_hex:
			return (c >= '0' && c <= '9') || ((c | 32) >= 'a' && (c | 32) <= 'f');
		default:
			return c >= '0' && c <= '9';
	}
}

static int get_digit(string_view * str, enum prefixes prefix) {
	while(view_start(str) == '_')
		view_walk(char, str);

	char c = view_start(str);
	view_walk(char, str);

	switch (prefix) {
		case int_hex:
		if (isdigit(c))
				return c - '0';
			return (c | 32) - 'a' + 10;
		default:
			return c - '0';
	}
}

static int64_t ipow(int64_t base, int64_t exp)
{
/* random joke why not*/
	int64_t res = base;
	if (exp <= 0) return 1;

	for (int i = 0; i < exp - 1; ++i)
		res *= base;

	return res;
}

/**
 * @brief parses an integer from content
 *
 * all non decimal formats require a prefix, prefixing only a 0 does not parse into octal
 *
 * @param[in] content content of file to parse
 * @param[out] value value of parsed integer
 * @return the view after tokenizing a number
 */
int default_num_reader(__attribute__((unused)) struct lexer_plugin_t * lexer, string_view *str, uint64_t *integer, double* decimal)
{
    enum prefixes prefix = int_decimal;
	int64_t exp = 1;
	uint64_t multiplier;
	int64_t divider = -1;
	int8_t signal = 1;
	int is_int = 1;
    uint64_t local_integer = 0;
	double local_decimal = 0.0;

    if (view_start(str) == '0')
    {
        view_walk(char, str);

        if ((view_start(str) | 32) == 'b')
            prefix = int_binary;
        else if ((view_start(str) | 32) == 'x' || (view_start(str) | 32) == 'h')
            prefix = int_hex;
        else if ((view_start(str) | 32) == 'o')
            prefix = int_octal;
        else if ((view_start(str) | 32) == 'd')
            prefix = int_decimal;
		else if (!isdigit(view_start(str)))
			return 1;
		else
			return ZLP_UNKNOWN_NUMBER_BASE;
		view_walk(char, str);
    }

	multiplier = 1;

	while(is_digit(view_start(str), prefix)) {
		local_integer += get_digit(str, prefix) * multiplier;
		multiplier *= prefix;
	}

	if (!is_digit(view_start(str), prefix) && isalpha(view_start(str)) && (view_start(str) | 32) != 'e')
		return -1;

	if (view_start(str) != '.' && (view_start(str) | 32) != 'e') {
		*integer = local_integer;
		return 1;
	}

	local_decimal = local_integer * 1.0l;

	if (view_start(str) == '.') {
		is_int = 0;
		view_walk(char, str);
		while (is_digit(view_start(str), prefix)) {
			local_decimal = fmal(get_digit(str, prefix), powl(prefix, divider), local_decimal);
			--divider;
			view_walk(char, str);
		}
		*decimal = local_decimal;
	}

	if ((view_start(str) | 32) != 'e')
		return is_int;

	view_walk(char, str);

	signal = view_start(str) == '-' ? -1 : 1;

	if (signal == -1) view_walk(char, str);

	multiplier = prefix;
	while(isdigit(view_start(str))) {
		// exponents only as decimals, its convenient
		exp += get_digit(str, int_decimal) * multiplier;
		multiplier *= prefix;
	}

	if (signal >= 0 && is_int) {
		*integer = local_integer * ipow(prefix, exp);
		return 1;
	}

	*decimal = local_decimal * powl(prefix, signal * exp);
	return 0;

}

string_view default_str_reader(struct lexer_plugin_t * lexer, string_view * str) {
	if(!str->size || !subview_inits_view(char, str, &lexer->string_delimiter)) return null_view(char);
	string_view start;

	view_run(char, str, lexer->string_delimiter.size);
	view_copy(str, &start);

	while(!subview_inits_view(char, str, &lexer->string_delimiter)) {
		view_walk(char, str);
	}

	int diff = str->ptr - start.ptr;

	view_run(char, str, lexer->string_delimiter.size);
	return generate_subview(char, &start, 0, diff);
}

string_view default_id_reader(struct lexer_plugin_t * lexer, string_view * str) {
	if (!str->size) return null_view(char);
	if (lexer->identifier_prefix.size && !subview_inits_view(char, str, &lexer->identifier_prefix)) return null_view(char);
	string_view start;
	if (lexer->identifier_prefix.size) {
		view_run(char, str, lexer->identifier_prefix.size);
	}

	view_copy(str, &start);

	if (!isalpha(view_start(str)) && !(view_start(str) == '_')) {
		/* "you can't run a negative distance" :point_up::nerd_face: */
		view_run(char, str, -lexer->identifier_prefix.size);
		return null_view(char);
	}

	while(isalnum(view_start(str)) || view_start(str) == '_'){
		view_walk(char, str);
	}

	int diff = str->ptr - start.ptr;

	return generate_subview(char, &start, 0, diff);
}

#define null_token (struct zlp_token){.id = 0, .integer = 0}

struct zlp_token next_token(struct plugin_t * lexer, string_view * content) {
	struct lexer_plugin_t * lplug = lexer->specific_data;
	if (!lplug->number_reader) lplug->number_reader = default_num_reader;
	if (!lplug->string_reader) lplug->string_reader = default_str_reader;
	if (!lplug->identifier_reader) lplug->identifier_reader = default_id_reader;

restart:
	while (isspace(view_start(content)))
		view_walk(char, content);

	if (!content->size)
		return (struct zlp_token){
			.id = EndToken,
			.values = {0}
		};

	if (subview_inits_view(char, content, &lplug->single_line_comment_prefix)) {
		while (view_start(content) != '\n')
			view_walk(char, content);
		goto restart; // not exploding the stack
	}

	if (subview_inits_view(char, content, &lplug->multi_line_comment_endings[0])) {
		view_run(char, content, lplug->multi_line_comment_endings[0].size);
		string_view * ml_end = &lplug->multi_line_comment_endings[1];
		while (content->size >= ml_end->size)
			if (!subview_inits_view(char, content, ml_end))
				// no need to look for something that is going to be already wrong
				view_run(char, content, ml_end->size);
			else
				break;

		if (content->size < ml_end->size)
			return (struct zlp_token){
				.id = EndToken,
				.values = {ZLP_COMMENT_DID_NOT_FINISH}
			};
		goto restart; // not exploding the stack, again
	}

	if (isdigit(view_start(content))) {
		double possible_double;
		uint64_t possible_int;
		int result = lplug->number_reader(lplug, content, &possible_int, &possible_double);
		if (result < 0)
			return (struct zlp_token){
				.id = EndToken,
				.values = {result} /* possibly use token values to store errors */
			};
		if (result == 0)
			return (struct zlp_token){
				.id = lplug->tid_decimal,
				.decimal = possible_double
			};
		return (struct zlp_token){
			.id = lplug->tid_integer,
			.integer = possible_int
		};
	}

	for (unsigned i = lplug->token_count - 1; i; --i) {
		if (subview_inits_view(char, content, &lplug->tokens[i].view)) {
			view_run(char, content, lplug->tokens[i].view.size);

			return lplug->tokens[i];
		}
	}

	if (isalpha(view_start(content)) || (lplug->identifier_prefix.size && !subview_inits_view(char, content, &lplug->identifier_prefix))) {
		string_view identifier = lplug->identifier_reader(lplug, content);

		if (identifier.size)
			return (struct zlp_token){
				.id = lplug->tid_identifier,
				.view = identifier
			};
		return null_token;
	}

	if (lplug->string_delimiter.size && subview_inits_view(char, content, &lplug->string_delimiter)) {
		string_view string = lplug->string_reader(lplug, content);
		if (string.size)
			return (struct zlp_token){
				.id = lplug->tid_string,
				.view = string
			};
		return null_token;
	}

	return (struct zlp_token){ .id = 0, .view = generate_subview(char, content, 0, 1) };
}


