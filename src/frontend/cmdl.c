#include "frontend.h"
enum
{
    string_size = 32,
    last_item = (string_size - 1)
};



struct HashMap *parse_commandline(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    return NULL;
}

// {
//     struct HashMap *map = create_map(0);
//     struct HashMap *host = NULL;
//     const char *color_env = NULL;
//     if (!map)
//     {
//         goto destructor_1;
//     }

//     host = create_map(0);
//     if (!host)
//     {
//         goto destructor_2;
//     }

//     color_env = getenv("TERM");

//     map_addst_key(map, "color-support", json_bool, (void *)(long)(color_env && strstr(color_env, "color")));
//     map_addst_key(map, "host", json_object, host);
//     map_addst_key(host, "bus-size", json_number_int, (void *)sizeof(void *));
//     map_addst_key(host, "name", json_string, HOSTNAME);

//     for (int i = 1; i < argc; ++i)
//     {
//         /**
//          * there are only 2 types of flags
//          *
//          * - double dash/no value (--debug, --version, --help)
//          * - double dash/valued (--input=file)
//          *
//          */
//         char session[string_size];
//         char key[string_size];
//         char value[string_size];
//         memset(session, 0, string_size);
//         memset(key, 0, string_size);
//         memset(value, 0, string_size);
//         if (!sscanf(argv[i], "--%s", argv[i]))
//         {
//             continue;
//         }

//         if (sscanf(argv[i], "%15s.%15s=%15s", session, key, value) == 3 ||
//             sscanf(argv[i], "%15s=%15s", key, value) == 2)
//         {
//             goto set_key;
//         }

//         memcpy(key, argv[i], string_size);
//         key[last_item] = 0;
//     set_key:
//         if (strlen(session))
//         {
//             struct pair_t *block = map_getkey_s(map, session);
//             struct HashMap *session_map = !block->first ? create_map(0) : block->first;

//             if (block->first != session_map)
//             {
//                 map_addst_key(map, session, json_object, session_map);
//             }

//             if (strlen(value))
//             {
//                 map_addst_key(session_map, session, json_string, strdup(value));
//             }
//             else
//             {
//                 map_addst_key(session_map, session, json_bool, NULL);
//             }
//         }
//     }

// destructor_2:
//     free(map);
// destructor_1:
//     return NULL;
// }
