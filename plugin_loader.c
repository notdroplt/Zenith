#include "plugin_loader.h"

#ifdef __linux__
#include <dlfcn.h>

struct plugin_t *load_plugin(char *plugin_name)
{
    void *handle;
    struct plugin_t *plugin;
    char *error;

    handle = dlopen(plugin_name, RTLD_LAZY);
    if (!handle)
    {
        return NULL;
    }

    dlerror();
    plugin = dlsym(handle, "plugin");

    if ((error = dlerror()) != NULL)
        return NULL;

    if (plugin->on_load)
        plugin->on_load(plugin->data);

    return plugin;
}

int unload_plugin(struct plugin_t *plugin)
{
    if (plugin->on_unload)
        plugin->on_unload(plugin->data);

    dlclose(plugin->native_handler);

    return 0;
}

#endif

