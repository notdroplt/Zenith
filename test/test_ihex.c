#include <formats.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char * expected_string = 
":10000000AAAAAAAAAAAAAAAAABABABABABABABAB48\n"
":10001000ABCDABCDABCDABCDABCD0123ABCD0123C8\n"
":100020000123456789ABCDEF000000000000000010\n"
":00FFFF0101";



int main(void) {
    uint64_t array1 [] = {
        0xAAAAAAAAAAAAAAAA, 0xABABABABABABABAB,
        0xABCDABCDABCDABCD, 0xABCD0123ABCD0123,
        0x0123456789ABCDEF,        
    };
    
    FILE * fp = NULL;
    size_t size = 0, status = 0; 
    const char * string = NULL;

    ihex_create_file(array1, sizeof(array1), "test.hex");

    fp = fopen("test.hex", "r");

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    string = calloc(sizeof(char), size + 1);
    
    if (!string) {
        fclose(fp);
        fputs("could not allocate enough space to compare strings", stderr);
        return 1;
    }

    status = 0;
    if ((status = fread((void*) string, 1, size, fp)) != size) {
        fclose(fp);
        free((void *)string);
        fprintf(stderr, "invalid read on file \"test.hex\" returned %ld and expected %ld", status, size);
        return 1;
    }


    if(strcmp(expected_string, string)) {
        fclose(fp);
        fprintf(stderr, "expected result and current result don't match\nexpected string:\n%s\ncurrent result:\n%s\n", expected_string, string);
        free((void*)string);
        return 1;
    }

    free((void *)string);

    fclose(fp);
    remove("test.hex");

    return 0;
}
