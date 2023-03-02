#include <lex.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct lex_t * init_lex(char * string) {
    struct lex_t * lex = malloc(sizeof(struct lex_t));

    if(!lex) {
        fputs("out of memory\n", stderr);
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

int test_int_tok() {

    uint32_t number = rand();
    char * code_string = calloc(1, 16);

    if (!code_string) {
        fputs("out of memory\n", stderr);
        return 1;
    }

    sprintf(code_string, "%d\n", number);


    struct lex_t *lex = init_lex(code_string);
    if (!lex) return 1;
    struct token_t tok = getNextToken(lex);

    free(lex);
    free(code_string);

    if (tok.type != TT_Int) {
        fprintf(stderr, "[lex int] expected token of type %d (TT_Int) but got token of type %d\n", TT_Int, tok.type);
        return 1;
    }

    if (tok.val.integer != number) {
        fprintf(stderr, "[lex int] expected value to be %d but lexer returned %d\n", number,(uint32_t)tok.val.integer);
        return 1;
    }

    return 0;
}

double dont_zero(double n) {
    if (n == 0)
        ++n;
    return n;
}

int test_double_tok() {
    // should work
    double number = (double) rand() / (double)  dont_zero(rand());

    char * code_string = calloc(1, 20);

    if (!code_string) {
        fputs("out of memory\n", stderr);
        return 1;
    }

    sprintf(code_string, "%lf\n", number);

    struct lex_t *lex = init_lex(code_string);
    if (!lex) return 1;
    struct token_t tok = getNextToken(lex);

    free(lex);
    free(code_string);

    if (tok.type != TT_Double) {
        fprintf(stderr, "[lex double] expected token of type %d (TT_Double) but got token of type %d\n", TT_Double, tok.type);
        return 1;
    }
    // junky "near equal" for decimal point numbers
    if ((int64_t)(tok.val.number/1) != (int64_t)(number/1)) {
        fprintf(stderr, "[lex double] expected value to be %lf but lexer returned %lf\n", number, tok.val.number);
        return 1;
    }

    return 0;
}

int test_str_tok() {
    size_t size = rand() % 32;
    char * str = calloc(1, size + 1);
    static char charset[] = "qwertyuiopasdfghjklzxcvbnm_QWERTYUIOPASDFGHJKLZXCVBNM";

    if (!str) {
        fputs("out of memory", stderr);
        return 1;
    }

    char * code_string = calloc(1, 40);

    if (!code_string) {
        fputs("out of memory\n", stderr);
        free(str);
        return 1;
    }

    --size;
    for (size_t n = 0; n < size; n++) {
        int key = rand() % (int) (sizeof charset - 1);
        str[n] = charset[key];  
    }
    str[size] = '\0';
    
    sprintf(code_string, "\"%s\"\n", str);

    struct lex_t *lex = init_lex(code_string);
    if (!lex) return 1;
    struct token_t tok = getNextToken(lex);

    free(lex);
    if (tok.type != TT_String) {
        fprintf(stderr, "[lex string] expected token of type %d (TT_String) but got token of type %d\n", TT_String, tok.type);
        return 1;
    }

    if (strvcmp(tok.string, str)) {
        fprintf(stderr, "[lex string] expected value to be %s but lexer returned %.*s\n", str, (int)tok.string.size, tok.string.string);
        return 1;
    }

    free(code_string);

    free(str);
    return 0;
}

int main(int argc, char ** argv ) {
    srand(time(NULL));

    if (argc < 2) return 1;

    if(strcmp(argv[1], "int") == 0) return test_int_tok();
    if(strcmp(argv[1], "float") == 0) return test_double_tok();
    if(strcmp(argv[1], "str") == 0) return test_str_tok();
    
    return 1;
}