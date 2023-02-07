#include <dotenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define set_without_overwrite(key, value) \
	if (!getenv(key))                     \
	setenv(key, value, 0)

static int do_nothing_function(const char * format __attribute_maybe_unused__, ...) {
    return 0;
}

static int verbose_printf_function(const char * format_string, ...) {
    va_list args;
    int ret = 0;
    va_start(args, format_string);

    ret = vfprintf(stderr, format_string, args);

    va_end(args);
    
    return ret;
}


char *strip(char *str)
{
	char *end = str + strlen(str) - 1;

	if (end < str)
		return str;

	while (end > str && isspace(*end))
		--end;

	end[1] = 0;
	while (isspace(*str))
		++str;

	return str;
}

void set_default()
{
	set_without_overwrite(env_output, "output.zvm");
	set_without_overwrite(env_input, "source.znh");
	set_without_overwrite(env_debug, "1");
	set_without_overwrite(env_verbose, "0");
	set_without_overwrite(env_compile_virtmac, "1");	
	set_without_overwrite(env_compile_ihex, "0");		
	set_without_overwrite(env_dump_json, "0");		
	set_without_overwrite(env_print_disassemble, "0");	
}

int load_dotenv()
{
	char key[64], value[64], *skey, *svalue;
	FILE *fp;
	char *line = NULL;
	size_t len = 0;

	set_default();
	fp = fopen(".env", "r");
	if (!fp)
		return 0; //.env's are cool but not actually necessary

	while (getline(&line, &len, fp) != -1)
	{

		sscanf(line, "%[^=]=%[^\n;#]", key, value);
		skey = strip(key);
		svalue = strip(value);

		if (strlen(strip(line)) == 0 || *strip(line) == '#' || *strip(line) == ';')
			continue;
		else if (strcasecmp(svalue, "true") == 0 || strcasecmp(svalue, "yes") == 0)
			setenv(skey, "1", 1);
		else if (strcasecmp(svalue, "false") == 0 || strcasecmp(svalue, "no") == 0)
			setenv(skey, "0", 1);
		else
			setenv(skey, svalue, 1);
	}

	fclose(fp);

	if (*getenv(env_verbose) == '1')
        vrprintf = verbose_printf_function;
    else {
        vrprintf = do_nothing_function;
	}
	
	return 0;
}


verbose_printf vrprintf;

