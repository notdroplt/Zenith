#include <lex.h>
#include <stdio.h>

static char lexNext(struct lex_t *lex)
{
	if (lex->position.index >= lex->file_size)
		return '\0';
	
	++lex->position.index;
	++lex->position.column;

	lex->current_char = lex->position.index < lex->file_size ? lex->content[lex->position.index] : '\0';

	if (lex->current_char == '\n')
	{
		lex->position.column = 1;
		lex->position.last_line_pos = lex->position.index;
		++lex->position.line;
	}
	return lex->current_char;
}

static struct token_t lexString(struct lex_t *lex)
{
	size_t start = lex->position.index + 1; /* skip the first " */
	struct token_t tok;
	lexNext(lex);
	while (lex->current_char != '"' && lex->current_char)
		lexNext(lex);
	
	lexNext(lex);

	if (!lex->current_char)
	{
		tok.type = TT_Unknown;
		tok.string.size = 0;
		tok.string.string = NULL;
		return tok; /* lexer silently dies*/
	}

	tok.string.string = lex->content + start;
	tok.string.size = lex->position.index - start - 1;

	tok.type = TT_String;

	return tok;
}

static uint64_t strtoi(struct string_t string)
{
	uint64_t value = 0;
	int i = 1;

	while (string.size--){
		value += (string.string[string.size] - '0') * i;
		i *= 10;
	}
	
	return value;
}

static double strtod(struct string_t str)
{
	double value = 0.0;
	size_t i = 0, div = 10;

	for (; str.string[i] != '.'; ++i)
		value = value * 10.0 + str.string[i] - '0';
	
	++i;

	for (; i < str.size; ++i)
	{
		value += (double)(str.string[i] - '0') / (double)div;
		div *= 10;
	}

	return value;
}

static struct token_t lexNumber(struct lex_t *lex)
{
	struct token_t tok;
	int dot = 0;
	tok.string.string = lex->content + lex->position.index;
	tok.string.size = 0;

	tok.type = TT_Unknown;
	do
	{
		if (lex->current_char == '.' && dot){
			tok.type = TT_Unknown;
			tok.val.integer = 0;
		}
		else if (lex->current_char == '.')
			dot = 1;

		++tok.string.size;

	} while (lexNext(lex) && (isdigit(lex->current_char) || lex->current_char == '.'));

	if (dot)
	{
		tok.type = TT_Double;
		tok.val.number = strtod(tok.string);
	}
	else
	{
		tok.type = TT_Int;
		tok.val.integer = strtoi(tok.string);
	}
	return tok;
}

static struct token_t lexNewToken(struct lex_t *lex, enum TokenTypes val)
{
	struct token_t tok;
	tok.type = val;
	tok.val.keyword = KW_Unknown;
	tok.string.size = 0;
	tok.string.string = NULL;
	lexNext(lex);
	return tok;
}
int strvcmp(const struct string_t s1, const char *s2)
{
	size_t i = 0;
	if (s1.size != strlen(s2))
		return 1;

	for (; i < s1.size; ++i)
		if (s1.string[i] != s2[i])
			return 1;
	
	return 0;
}

static struct token_t lexIdentifier(struct lex_t *lex)
{
	struct token_t tok;
	uint64_t offset = 0;
	uint64_t start = lex->position.index;
	int i = 0;

	static const char *keywords[] = {"var", "function", "as", "do", "switch", "default", "if", "then", "else", "end", "return", "include", ""};
	static const char *domains[] = {"byte", "hword", "word", "dword", "char", "short", "int", "long"};

	do
		lexNext(lex);
	while (isalnum(lex->current_char) || lex->current_char == '_');

	offset = lex->position.index - start;

	tok.type = TT_Identifier;
	tok.string.string = lex->content + start;
	tok.string.size = offset;

	for (; i < KW_Unknown; ++i)
		if (strvcmp(tok.string, keywords[i]) == 0)
		{
			tok.type = TT_Keyword;
			tok.val.keyword = i;
			return tok;
		}

	for (i = 0; i < DM_unknown; ++i)
		if (strvcmp(tok.string, domains[i]) == 0)
		{
			tok.type = TT_Domain;
			tok.val.keyword = i;
			break;
		}
	

	tok.val.integer = 0;
	return tok;
}

static struct token_t lexCompare(struct lex_t *lex, char c, enum TokenTypes value)
{
	struct token_t tok;
	tok.type = value;
	lexNext(lex);
	if (lex->current_char == '=')
	{
		tok.type = value + 1;
		lexNext(lex);
	}
	else if (lex->current_char == c)
	{
		tok.type = value + 2;
		lexNext(lex);
	}
	return tok;
}

static struct token_t lexEqual(struct lex_t *lex)
{
	struct token_t tok;
	lexNext(lex);
	tok.type = TT_Unknown;
	if (lex->current_char == '=')
	{
		tok.type = TT_CompareEqual;
		lexNext(lex);
	}
	else if (lex->current_char == '>')
	{
		tok.type = TT_Arrow;
		lexNext(lex);
	}
	return tok;
}

static struct token_t lexNot(struct lex_t *lex)
{
	struct token_t tok;

	lexNext(lex);
	tok.type = TT_Not;
	if (lex->current_char == '=')
	{
		tok.type = TT_NotEqual;
		lexNext(lex);
	}

	return tok;
}

static struct token_t lexRepeat(struct lex_t *lex, enum TokenTypes type, char c)
{
	struct token_t tok;
	tok.type = type;
	lexNext(lex);
	if (lex->current_char == c)
	{
		tok.type = type + 1;
		lexNext(lex);
	}
	return tok;
}

struct token_t getNextToken(struct lex_t *lex)
{
	struct token_t tok = UNDEFINED_TOK;
	switch (lex->current_char)
	{
	case '\0':
		tok.type = TT_Unknown;
		return tok;
	case '\n':
	case '\t':
	case '\v':
	case '\r':
	case ' ':
		lexNext(lex);
		return getNextToken(lex);
	case '"':
		return lexString(lex);
	case '#':
		while (lex->current_char && lex->current_char != '\n')
			lexNext(lex);

		return getNextToken(lex);
	case ';':
		return lexNewToken(lex, TT_Semicolon);
	case '+':
		return lexRepeat(lex, TT_Plus, '+');
	case '-':
		return lexRepeat(lex, TT_Minus, '-');
	case '/':
		return lexNewToken(lex, TT_Divide);
	case '*':
		return lexNewToken(lex, TT_Multiply);
	case '=':
		return lexEqual(lex);
	case '!':
		return lexNot(lex);
	case '?':
		return lexNewToken(lex, TT_Question);
	case ':':
		return lexNewToken(lex, TT_Colon);
	case '>':
		return lexCompare(lex, '<', TT_GreaterThan);
	case '<':
		return lexCompare(lex, '>', TT_LessThan);
	case '(':
		return lexNewToken(lex, TT_LeftParentesis);
	case ')':
		return lexNewToken(lex, TT_RightParentesis);
	case '[':
		return lexNewToken(lex, TT_LeftSquareBracket);
	case ']':
		return lexNewToken(lex, TT_RightSquareBracket);
	case '{':
		return lexNewToken(lex, TT_LeftSquareBracket);
	case '}':
		return lexNewToken(lex, TT_RightSquareBracket);
	case ',':
		return lexNewToken(lex, TT_Comma);
	case '.':
		return lexNewToken(lex, TT_Dot);
	case '@':
		return lexNewToken(lex, TT_At);
	case '|':
		return lexRepeat(lex, TT_BitwiseOr, '|');
	case '&':
		return lexRepeat(lex, TT_BitwiseAnd, '&');
	case '^':
		return lexNewToken(lex, TT_BitWiseXor);
	case '~':
		return lexNewToken(lex, TT_Tilda);
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return lexNumber(lex);
	default:
		return lexIdentifier(lex);
	}
}
