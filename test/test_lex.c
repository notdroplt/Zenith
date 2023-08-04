#include <parsing/lex.h>
#include <stdio.h>
#include <time.h>

#define code_string_size 128

struct lex_t *init_lex(char *string)
{
    struct lex_t *lex = malloc(sizeof(struct lex_t));

    if (!lex)
    {
        (void)fputs("out of memory\n", stderr);
        return NULL;
    }

    lex->position.column = 1;
    lex->position.line = 1;
    lex->position.last_line_pos = 0;
    lex->position.index = 0;

    lex->file_size = strlen(string);
    lex->content = string;
    lex->current_char = *string;

    return lex;
}

int test_int_tok(void)
{
    struct lex_t *lex = NULL;
    struct token_t tok;
    uint32_t number = rand();
    char *code_string = calloc(1, code_string_size);

    if (!code_string)
    {
        (void)fputs("out of memory\n", stderr);
        return 1;
    }

    (void)sprintf(code_string, "%d\n", number);

    lex = init_lex(code_string);
    if (!lex)
    {
        free(code_string);
        return 1;
    }
    tok = getNextToken(lex);

    free(lex);
    free(code_string);

    if (tok.type != TT_Int)
    {
        return fprintf(stderr, "[lex int] expected token of type %d (TT_Int) but got token of type %d\n", TT_Int, tok.type);
    }

    if (tok.val.integer != number)
    {
        return fprintf(stderr, "[lex int] expected value to be %d but lexer returned %d\n", number, (uint32_t)tok.val.integer);
    }

    return 0;
}

double dont_zero(double n)
{
    if (n == 0)
    {
        ++n;
    }
    return n;
}

int test_double_tok(void)
{
    struct lex_t *lex = NULL;
    struct token_t tok;
    double number = (double)rand() / dont_zero(rand());

    char *code_string = calloc(1, code_string_size);

    if (!code_string)
    {
        return fputs("out of memory\n", stderr);
    }

    (void)sprintf(code_string, "%lf\n", number);

    lex = init_lex(code_string);
    if (!lex)
    {
        free(code_string);
        return 1;
    }
    tok = getNextToken(lex);

    free(lex);
    free(code_string);

    if (tok.type != TT_Double)
    {
        return fprintf(stderr, "[lex double] expected token of type %d (TT_Double) but got token of type %d\n", TT_Double, tok.type);
    }
    // junky "near equal" for decimal point numbers
    if ((int64_t)(tok.val.number / 1) != (int64_t)(number / 1))
    {
        return fprintf(stderr, "[lex double] expected value to be %lf but lexer returned %lf\n", number, tok.val.number);
    }

    return 0;
}

int test_str_tok(void)
{
    size_t size = rand() % code_string_size;
    char *str = calloc(1, size + 1);
    static char charset[] = "qwertyuiopasdfghjklzxcvbnm_QWERTYUIOPASDFGHJKLZXCVBNM";
    char *code_string = NULL;
    struct lex_t *lex = NULL;
    struct token_t tok;

    if (!str)
    {
        return fputs("out of memory", stderr);
    }

    code_string = calloc(1, 40);

    if (!code_string)
    {
        free(str);
        return fputs("out of memory\n", stderr);
    }

    --size;
    for (size_t n = 0; n < size; n++)
    {
        int key = rand() % (int)(sizeof charset - 1);
        str[n] = charset[key];
    }
    str[size] = '\0';

    (void)sprintf(code_string, "\"%s\"\n", str);

    lex = init_lex(code_string);
    if (!lex)
    {
        free(code_string);
        return 1;
    }
    tok = getNextToken(lex);

    free(lex);
    if (tok.type != TT_String)
    {
        return fprintf(stderr, "[lex string] expected token of type %d (TT_String) but got token of type %d\n", TT_String, tok.type);
    }
    
    free(code_string);

    free(str);
    return 0;
}

int main(int argc, char **argv)
{
    srand(time(NULL));

    if (argc < 2)
    {
        return 1;
    }

    if (strcmp(argv[1], "int") == 0)
    {
        return test_int_tok();
    }
    if (strcmp(argv[1], "float") == 0)
    {
        return test_double_tok();
    }
    if (strcmp(argv[1], "str") == 0)
    {
        return test_str_tok();
    }

    return 1;
}
