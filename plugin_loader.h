/**
 * @file plugin_loader.h
 * @author notdroplt
 * @brief making the entire codebase modular and somewhat easier to maintain
 * @version 0.1
 * @date 2023-12-30
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once
#ifndef ZENITH_PLUGIN_LOADER_H
#define ZENITH_PLUGIN_LOADER_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "allocator.h"

/*! define pointer parameters that input sources */
#define IN

/*! define pointer parameters that are secondary return values */
#define OUT

#if defined(_WIN32) || defined(doxygen)
#define plugin_export __declspec(dllexport)
#else
#define plugin_export
#endif

/**
 * @brief triggers for plugins
 *
 * those plugins also represent compiler phases
 *
 * normally, nothing should happen between after_[...] and before_[...] events that could
 * modify the pipeline, so most of the time it will be only plugin (de)allocation and nothing more
 *
 * all on_[...] events should be also flagged with the `replace_default` flag, normally there is only
 * one plugin working at those at one time, as those completely modify the pipeline data
 *
 * and it normally is
 *
 * - on start:
 * - - in: argc and argv
 * - - out: parsed array of command line arguments
 * - before token:
 * - - in/out: command line arguments
 * - on token:
 * - - in: command line arguments, file name and content,
 * - - out: one token
 * - after token:
 * - - in/out: a token
 *
 */
enum plugin_triggers
{
    on_start,     /*!< run before everything */
    before_token, /*!< run before lexing */
    on_token,     /*!< be the lexer */
    after_token,  /*!< run after lexing */
    before_parse, /*!< run before parse */
    on_parse,     /*!< be the parser */
    after_parse,  /*!< run after parsing */
    on_end,       /*!< plugin should be run after after everything */
    replace_default = (sizeof(void *) * 4) - 1
};

#define plugin_trigger(trigger) (1 << (trigger))

/*! semantic naming, opaque pointer */
struct plugin_data;

/*! on_load function signature */
typedef void (*zp_on_load_t)(IN struct plugin_data *);

/*! on_unload function signature */
typedef void (*zp_on_unload_t)(IN struct plugin_data *);

/**
 * @brief plugin handler
 *
 * this struct is returned to the plugin loader with some values blank, as they are
 * written by the driver itself
 */
struct plugin_t
{
    enum plugin_triggers trigger;	/*!< specify trigger */
    struct plugin_data *data;		/*!< pointer to data used around inside the plugin */
    void *native_handler;			/*!< native shared library handler */
    void *specific_data;			/*!< pointer to values that may vary between different types of plugins */
    zp_on_load_t on_load;			/*!< constructor if plugin needs state */
    zp_on_unload_t on_unload;		/*!< destructor if plugin needs state */
    char * author;					/*!< author name */
};

extern struct plugin_t default_lexer_plugin;
///TODO: actually make the parser a plugin because this api is going to be so painful
//extern struct plugin_t default_parser_plugin;

#include "lexer_plugin.h"

struct plugin_t *load_plugin(char *plugin_name);

int unload_plugin(struct plugin_t *plugin);

#endif
