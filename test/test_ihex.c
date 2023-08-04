#include <output/formats.h>


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
    
    FILE * fpointer = NULL;
    size_t size = 0;
    size_t status = 0; 
    const char * string = NULL;

    ihex_create_file(array1, sizeof(array1), "test.hex");

    fpointer = fopen("test.hex", "re");

    (void)fseek(fpointer, 0, SEEK_END);
    size = ftell(fpointer);
    (void)fseek(fpointer, 0, SEEK_SET);

    string = calloc(sizeof(char), size + 1);
    
    if (!string) {
        (void)fclose(fpointer);
        (void)fputs("could not allocate enough space to compare strings", stderr);
        return 1;
    }

    status = fread((void*) string, 1, size, fpointer);
    if (status != size) {
        (void)fclose(fpointer);
        free((void *)string);
        (void)fprintf(stderr, "invalid read on file \"test.hex\" returned %ld and expected %ld", status, size);
        return 1;
    }


    if(strcmp(expected_string, string) != 0) {
        (void)fclose(fpointer);
        (void)fprintf(stderr, "expected result and current result don't match\nexpected string:\n%s\ncurrent result:\n%s\n", expected_string, string);
        free((void*)string);
        return 1;
    }

    free((void *)string);

    (void)fclose(fpointer);
    (void)remove("test.hex");

    return 0;
}
