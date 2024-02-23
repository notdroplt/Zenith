#include "config.h"

//struct config_option_t * read_config_file(allocator_param(pool_t pool, char * filename)) {
//	FILE * fp;
//	size_t file_size;
//
//	fp = fopen(filename, "r");
//	if (!fp)
//		goto destructor_0;
//
//	fseek(fp, 0, SEEK_END);
//	file_size = ftell(fp);
//	fseek(fp, 0, SEEK_SET);
//
//	*file_content = create_arena(file_size * 2);
//	if (!*file_content)
//		goto destructor_1;
//
//destructor_1:
//	fclose(fp);
//destructor_0:
//	*size = 0;
//	*file_content = 0;
//	return NULL;
//}

