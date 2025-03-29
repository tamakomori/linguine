/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Linguine
 * Copyright (c) 2025, Tamako Mori. All rights reserved.
 */

/*
 * The 'linguine' Command
 */

#include "linguine/linguine.h"
#include "linguine/runtime.h"
#include "linguine/ast.h"
#include "linguine/hir.h"
#include "linguine/lir.h"
#include "linguine/cback.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <locale.h>
#include <unistd.h>
#include <stdbool.h>

#if defined(TARGET_WINDOWS)
#include <windows.h>
#endif

const char version[] =
	"Linguine CLI Version 0.0.2\n";

const char usage[] =
	"Usage:\n"
	"  Run program:\n"
	"    linguine <source files and/or bytecode files>\n"
	"  Run program (safe mode):\n"
	"    linguine --safe-mode <source files and/or bytecode files>\n"
	"  Compile to a bytecode file:\n"
	"    linguine --bytecode <source files>\n"
	"  Compile to an application C source:\n"
	"    linguine --app <source files>\n"
	"  Compile to a DLL C source:\n"
	"    linguine --dll <source files>\n"
	"  Show this help:\n"
	"    linguine --help\n"
	"  Show version:\n"
	"    linguine --version\n";

int opt_index;
bool opt_compile;
bool opt_compile_to_lsc;
bool opt_compile_to_app;
bool opt_compile_to_dll;
const char *opt_output;

/* Config */
extern bool linguine_conf_use_jit;

static char source_data[1 * 1024 * 1024];
static int source_size;

static void parse_options(int argc, char *argv[]);
static bool run_interpreter(int argc, char *argv[], int *ret);
static bool run_source_compiler(int argc, char *argv[]);
static bool run_binary_compiler(int argc, char *argv[]);
static bool load_file(char *fname);
static void init_lang_code(void);
static void print_error(struct rt_env *rt);
static bool register_ffi(struct rt_env *rt);
static int wide_printf(const char *format, ...);

int main(int argc, char *argv[])
{
	int ret;

	init_lang_code();

	/* Parse command line options. */
	parse_options(argc, argv);

	/* Run. */
	ret = 0;
	if (opt_compile_to_dll || opt_compile_to_app) {
		/* Translate to a C source file. */
		if (!run_source_compiler(argc, argv))
			return 1;
	} else if (opt_compile_to_lsc) {
		/* Translate to a bytecode file. */
		if (!run_binary_compiler(argc, argv))
			return 1;
	} else {
		/* Run by the interpreter. */
		if (!run_interpreter(argc, argv, &ret))
			return 1;
	}

	return ret;
}

static void parse_options(int argc, char *argv[])
{
	int index;

	index = 1;
	while (index < argc) {
		/* --help */
		if (strcmp(argv[index], "--help") == 0) {
			wide_printf("%s", usage);
			exit(0);
		}

		/* --version */
		if (strcmp(argv[index], "--version") == 0) {
			wide_printf("%s", version);
			exit(0);
		}

		/* --safe-mode */
		if (strcmp(argv[index], "--safe-mode") == 0) {
			linguine_conf_use_jit = false;
			index++;
			continue;
		}

		/* --bytecode */
		if (strcmp(argv[index], "--bytecode") == 0) {
			if (index + 1 >= argc) {
				wide_printf("%s", usage);
				exit(1);
			}

			opt_compile = true;
			opt_compile_to_lsc = true;
			opt_output = argv[index + 1];

			index++;
			continue;
		}

		/* --app */
		if (strcmp(argv[index], "--app") == 0) {
			if (index + 1 >= argc) {
				wide_printf("%s", usage);
				exit(1);
			}

			opt_compile = true;
			opt_compile_to_app = true;
			opt_output = argv[index + 1];

			index += 2;
			continue;
		}

		/* --dll */
		if (strcmp(argv[index], "--dll") == 0) {
			if (index + 1 >= argc) {
				wide_printf("%s", usage);
				exit(1);
			}

			opt_compile = true;
			opt_compile_to_dll = true;
			opt_output = argv[index + 1];

			index += 2;
			continue;
		}

		break;
	}

	if (index >= argc) {
		wide_printf("%s", usage);
		exit(1);
	}

	opt_index  = index;
}

static bool run_interpreter(int argc, char *argv[], int *retval)
{
	struct rt_env *rt;
	struct rt_value ret;
	int i;

	/* Create a runtime. */
	if (!rt_create(&rt))
		return false;

	/* Register FFI functions. */
	if (!register_ffi(rt))
		return false;

	for (i = opt_index; i < argc; i++) {
		/* Load a file. */
		if (!load_file(argv[i]))
			return false;

		if (strstr(argv[i], ".lsc") != NULL) {
			/* Load a bytecode file. */
			if (!rt_register_bytecode(rt, (uint32_t)source_size, (uint8_t *)source_data)) {
				print_error(rt);
				return false;
			}
		} else {
			/* Compile a source code. */
			if (!rt_register_source(rt, argv[i], source_data)) {
				print_error(rt);
				return false;
			}
		}
	}

#if defined(CONF_DEBUGGER)
	rt->dbg_stop_flag = true;
#endif

	/* Run the main function. */
	if (!rt_call_with_name(rt, "main", NULL, 0, NULL, &ret)) {
		print_error(rt);
		return false;
	}

	/* Destroy a runtime. */
	if (!rt_destroy(rt))
		return false;

	/* Return a result value. */
	*retval = ret.val.i;
	return true;
}

static bool run_binary_compiler(int argc, char *argv[])
{
	char lsc_fname[1024];
	char *dot;
	FILE *fp;
	int i, j, k;

	for (i = opt_index; i < argc; i++) {
		int func_count;

		/* Load a file. */
		if (!load_file(argv[i]))
			return false;

		/* Do parse and build AST. */
		if (!ast_build(argv[i], source_data)) {
			wide_printf(_("Error: %s: %d: %s\n"),
				    ast_get_file_name(),
				    ast_get_error_line(),
				    ast_get_error_message());
			return false;
		}

		/* Transform AST to HIR. */
		if (!hir_build()) {
			wide_printf(_("Error: %s: %d: %s\n"),
				    hir_get_file_name(),
				    hir_get_error_line(),
				    hir_get_error_message());
			return false;
		}

		/* Open a lsc file. */
		strcpy(lsc_fname, argv[i]);
		dot = strstr(lsc_fname, ".");
		if (dot != NULL)
			strcpy(dot, ".lsc");
		else
			strcat(lsc_fname, ".lsc");
		fp = fopen(lsc_fname, "wb");
		if (fp == NULL) {
			wide_printf(_("Cannot open file \"%s\".\n"), lsc_fname);
			exit(1);
		}

		/* Put a file header. */
		func_count = hir_get_function_count();
		fprintf(fp, "Linguine Bytecode\n");
		fprintf(fp, "Source\n");
		fprintf(fp, "%s\n", argv[i]);
		fprintf(fp, "Number Of Functions\n");
		fprintf(fp, "%d\n", func_count);

		/* For each function. */
		for (j = 0; j < func_count; j++) {
			struct hir_block *hfunc;
			struct lir_func *lfunc;

			/* Transform HIR to LIR (bytecode). */
			hfunc = hir_get_function(j);
			if (!lir_build(hfunc, &lfunc)) {
				wide_printf(_("Error: %s: %d: %s\n"),
					     lir_get_file_name(),
					     lir_get_error_line(),
					     lir_get_error_message());
				return false;;
			}

			/* Put a bytcode. */
			fprintf(fp, "Begin Function\n");
			fprintf(fp, "Name\n");
			fprintf(fp, "%s\n", lfunc->func_name);
			fprintf(fp, "Parameters\n");
			fprintf(fp, "%d\n", lfunc->param_count);
			for (k = 0; k < lfunc->param_count; k++)
				fprintf(fp, "%s\n", lfunc->param_name[k]);
			fprintf(fp, "Local Size\n");
			fprintf(fp, "%d\n", lfunc->tmpvar_size);
			fprintf(fp, "Bytecode Size\n");
			fprintf(fp, "%d\n", lfunc->bytecode_size);
			fwrite(lfunc->bytecode, (size_t)lfunc->bytecode_size, 1, fp);
			fprintf(fp, "\nEnd Function\n");

			/* Free a LIR. */
			lir_free(lfunc);
		}

		fclose(fp);

		/* Free HIR. */
		hir_free();
	}


	return true;
}

static bool run_source_compiler(int argc, char *argv[])
{
	int i, j;

	if (!cback_init(opt_output))
		return false;

	for (i = opt_index; i < argc; i++) {
		int func_count;

		/* Load a file. */
		if (!load_file(argv[i]))
			return false;

		/* Do parse and build AST. */
		if (!ast_build(argv[i], source_data)) {
			wide_printf(_("Error: %s: %d: %s\n"),
				     ast_get_file_name(),
				     ast_get_error_line(),
				     ast_get_error_message());
			return false;
		}

		/* Transform AST to HIR. */
		if (!hir_build()) {
			wide_printf(_("Error: %s: %d: %s\n"),
				     hir_get_file_name(),
				     hir_get_error_line(),
				     hir_get_error_message());
			return false;
		}

		/* For each function. */
		func_count = hir_get_function_count();
		for (j = 0; j < func_count; j++) {
			struct hir_block *hfunc;
			struct lir_func *lfunc;

			/* Transform HIR to LIR (bytecode). */
			hfunc = hir_get_function(j);
			if (!lir_build(hfunc, &lfunc)) {
				wide_printf(_("Error: %s: %d: %s\n"),
					     lir_get_file_name(),
					     lir_get_error_line(),
					     lir_get_error_message());
				return false;;
			}

			/* Put C function. */
			if (!cback_translate_func(lfunc))
				return false;

			/* Free a LIR. */
			lir_free(lfunc);
		}

		/* Free HIR. */
		hir_free();
	}

	if (opt_compile_to_dll) {
		if (cback_finalize_dll())
			return false;
	} else if (opt_compile_to_app) {
		if (cback_finalize_standalone())
			return false;
	}

	return true;
}

static bool load_file(char *fname)
{
	FILE *fp;
	size_t len;

	fp = fopen(fname, "rb");
	if (fp == NULL) {
		wide_printf(_("Cannot open file \"%s\".\n"), fname);
		return false;
	}

	len = fread(source_data, 1, sizeof(source_data) - 1, fp);
	if (len == 0) {
		wide_printf(_("Cannot read file \"%s\".\n"), fname);
		return false;
	}
	source_size = (int)len;

	/* Terminate the string. */
	source_data[len] = '\0';

	fclose(fp);

	return true;
}

static void init_lang_code(void)
{
	extern char *lang_code;
	const char *locale;

	locale = setlocale(LC_ALL, "");
	setlocale(LC_ALL, locale);
	if (locale == NULL)
		lang_code = "en";
	else if (locale[0] == '\0' || locale[1] == '\0')
		lang_code = "en";
	else if (strncasecmp(locale, "en", 2) == 0)
		lang_code = "en";
	else if (strncasecmp(locale, "fr", 2) == 0)
		lang_code = "fr";
	else if (strncasecmp(locale, "de", 2) == 0)
		lang_code = "de";
	else if (strncasecmp(locale, "it", 2) == 0)
		lang_code = "it";
	else if (strncasecmp(locale, "es", 2) == 0)
		lang_code = "es";
	else if (strncasecmp(locale, "el", 2) == 0)
		lang_code = "el";
	else if (strncasecmp(locale, "ru", 2) == 0)
		lang_code = "ru";
	else if (strncasecmp(locale, "zh_CN", 5) == 0)
		lang_code = "zh";
	else if (strncasecmp(locale, "zh_TW", 5) == 0)
		lang_code = "tw";
	else if (strncasecmp(locale, "ja", 2) == 0)
		lang_code = "ja";
	else
		lang_code = "en";
}

static void print_error(struct rt_env *rt)
{
	wide_printf(_("%s:%d: error: %s\n"),
		     rt_get_error_file(rt),
		     rt_get_error_line(rt),
		     rt_get_error_message(rt));
}

/*
 * FFI
 */

static bool cfunc_print(struct rt_env *rt);
static bool cfunc_readline(struct rt_env *rt);
static bool cfunc_readint(struct rt_env *rt);

static bool
register_ffi(
	struct rt_env *rt)
{

	struct item {
		const char *name;
		int param_count;
		const char *param[RT_ARG_MAX];
		bool (*cfunc)(struct rt_env *rt);
	} items[] = {
		{"print", 1, {"msg"}, cfunc_print},
		{"readline", 0, {}, cfunc_readline},
		{"readint", 0, {}, cfunc_readint},
	};
	int i;

	for (i = 0; i < (int)(sizeof(items) / sizeof(struct item)); i++) {
		if (!rt_register_cfunc(rt,
				       items[i].name,
				       items[i].param_count,
				       items[i].param,
				       items[i].cfunc))
			return false;
	}

	return true;

	return true;
}

static bool
cfunc_print(
	struct rt_env *rt)
{
	struct rt_value msg;
	const char *s;
	float f;
	int i;
	int type;

	if (!rt_get_local(rt, "msg", &msg))
		return false;

	if (!rt_get_value_type(rt, &msg, &type))
		return false;

	switch (type) {
	case RT_VALUE_INT:
		if (!rt_get_int(rt, &msg, &i))
			return false;
		wide_printf("%i\n", i);
		break;
	case RT_VALUE_FLOAT:
		if (!rt_get_float(rt, &msg, &f))
			return false;
		wide_printf("%f\n", f);
		break;
	case RT_VALUE_STRING:
		if (!rt_get_string(rt, &msg, &s))
			return false;
		wide_printf("%s\n", s);
		break;
	default:
		wide_printf("[object]\n");
		break;
	}

	return true;
}

static bool
cfunc_readline(
	struct rt_env *rt)
{
	struct rt_value ret;
	char buf[1024];
	int len;

	memset(buf, 0, sizeof(buf));

	fgets(buf, sizeof(buf) - 1, stdin);

	len = (int)strlen(buf);
	if (len > 0)
		buf[len - 1] = '\0';
	
	if (!rt_make_string(rt, &ret, buf))
		return false;
	if (!rt_set_local(rt, "$return", &ret))
		return false;

	return true;
}

static bool cfunc_readint(struct rt_env *rt)
{
	struct rt_value ret;
	char buf[1024];

	memset(buf, 0, sizeof(buf));

	fgets(buf, sizeof(buf) - 1, stdin);
	
	rt_make_int(&ret, atoi(buf));
	if (!rt_set_local(rt, "$return", &ret))
		return false;

	return true;
}

static int wide_printf(const char *format, ...)
{
	static char buf[4096];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

#if defined(TARGET_WINDOWS)
	static wchar_t wbuf[4096];
	MultiByteToWideChar(CP_UTF8, 0, buf, -1, wbuf, sizeof(wbuf));
	return wprintf(L"%ls", wbuf);
#else
	return printf("%s", buf);
#endif
}
