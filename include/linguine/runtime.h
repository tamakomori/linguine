/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Linguine
 * Copyright (c) 2025, Tamako Mori. All rights reserved.
 */

/*
 * RT: Language Runtime
 */

#ifndef LINGUINE_RUNTIME_H
#define LINGUINE_RUNTIME_H

#include "linguine/compat.h"
#include "linguine/linguine.h"

/* Maximum arguments of a call. */
#define RT_ARG_MAX	32

/* Forward declaration */
struct lir_func;
struct rt_env;
struct rt_env;
struct rt_frame;
struct rt_value;
struct rt_func;
struct rt_string;
struct rt_array;
struct rt_dict;
struct rt_bindglobal;
struct rt_bindlocal;

/* Value type. */
enum rt_value_type {
	RT_VALUE_INT,
	RT_VALUE_FLOAT,
	RT_VALUE_STRING,
	RT_VALUE_ARRAY,
	RT_VALUE_DICT,
	RT_VALUE_FUNC,
};

enum rt_bytecode {
	ROP_NOP,		/* 0x00: nop */
	ROP_ASSIGN,		/* 0x01: dst = src */
	ROP_ICONST,		/* 0x02: dst = integer constant */
	ROP_FCONST,		/* 0x03: dst = floating-point constant */
	ROP_SCONST,		/* 0x04: dst = string constant */
	ROP_ACONST,		/* 0x05: dst = empty array */
	ROP_DCONST,		/* 0x06: dst = empty dictionary */
	ROP_INC,		/* 0x07: dst = src + 1 */
	ROP_NEG,		/* 0x08: dst = ~src */
	ROP_ADD,		/* 0x09: dst = src1 + src2 */
	ROP_SUB,		/* 0x0a: dst = src1 - src2 */
	ROP_MUL,		/* 0x0b: dst = src1 * src2 */
	ROP_DIV,		/* 0x0c: dst = src1 / src2 */
	ROP_MOD,		/* 0x0d: dst = src1 % src2 */
	ROP_AND,		/* 0x0e: dst = src1 & src2 */
	ROP_OR,			/* 0x0f: dst = src1 | src2 */
	ROP_XOR,		/* 0x10: dst = src1 ^ src2 */
	ROP_LT,			/* 0x11: dst = src1 <  src2 [0 or 1] */
	ROP_LTE,		/* 0x12: dst = src1 <= src2 [0 or 1] */
	ROP_GT,			/* 0x13: dst = src1 >  src2 [0 or 1] */
	ROP_GTE,		/* 0x14: dst = src1 >= src2 [0 or 1] */
	ROP_EQ,			/* 0x15: dst = src1 == src2 [0 or 1] */
	ROP_NEQ,		/* 0x16: dst = src1 != src2 [0 or 1] */
	ROP_EQI,		/* 0x17: dst = src1 == src2 [0 or 1], integers */
	ROP_LOADARRAY,		/* 0x18: dst = src1[src2] */
	ROP_STOREARRAY,		/* 0x19: opr1[opr2] = op3 */
	ROP_LEN,		/* 0x1a: dst = len(src) */
	ROP_GETDICTKEYBYINDEX,	/* 0x1b: dst = src1.keyAt(src2) */
	ROP_GETDICTVALBYINDEX,	/* 0x1c: dst = src1.valAt(src2) */
	ROP_STOREDOT,		/* 0x1d: obj.access = src */
	ROP_LOADDOT,		/* 0x1e: dst = obj.access */
	ROP_STORESYMBOL,	/* 0x1f: setSymbol(dst, src) */
	ROP_LOADSYMBOL,		/* 0x20: dst = getSymbol(src) */
	ROP_CALL,		/* 0x21: func(arg1, ...) */
	ROP_THISCALL,		/* 0x22: obj->func(arg1, ...) */
	ROP_JMP,		/* 0x23: PC = src */
	ROP_JMPIFTRUE,		/* 0x24: PC = src1 if src2 == 1 */
	ROP_JMPIFFALSE,		/* 0x25: PC = src1 if src2 != 1 */
	ROP_JMPIFEQ,		/* 0x25: PC = src1 if src2 indicates eq */
	ROP_LINEINFO,		/* 0x26: setDebugLine(src) */
};

/* Runtime environment. */
struct rt_env {
	/* Stack. (Do not move. JIT assumes the offset 0.) */
	struct rt_frame *frame;

	/* Execution line. (Do not move. JIT assumes the offset 8.) */
	int line;

	/* Global symbols. */
	struct rt_bindglobal *global;

	/* Function list. */
	struct rt_func *func_list;

	/* Heap usage in bytes. */
	size_t heap_usage;

	/* Deep object list. */
	struct rt_string *deep_str_list;
	struct rt_array *deep_arr_list;
	struct rt_dict *deep_dict_list;

	/* Garbage object list. */
	struct rt_string *garbage_str_list;
	struct rt_array *garbage_arr_list;
	struct rt_dict *garbage_dict_list;

	/* Execution file. */
	char file_name[1024];

	/* Error message. */
	char error_message[4096];

#if defined(CONF_DEBUGGER)
	/* Last file and line. */
	char dbg_last_file_name[1024];
	int dbg_last_line;

	/* Stop flag. */
	volatile bool dbg_stop_flag;

	/* Single step flag. */
	bool dbg_single_step_flag;

	/* Error flag. */
	bool dbg_error_flag;
#endif
};

/* Calling frame. */
struct rt_frame {
	/* tmpvar (Do not move. JIT assumes the offset 0.) */
	struct rt_value *tmpvar;
	int tmpvar_size;

	/* function */
	struct rt_func *func;

	/* bindlocal. */
	struct rt_bindlocal *local;

	/* Shallow string list. */
	struct rt_string *shallow_str_list;

	/* Shallow array list. */
	struct rt_array *shallow_arr_list;

	/* Shallow dictionary list. */
	struct rt_dict *shallow_dict_list;

	/* Next frame. */
	struct rt_frame *next;
};

/*
 * Variable value.
 *  - If a value is zero-cleared, it shows an integer zero.
 *  - This struct has a 16-byte size.
 */
struct rt_value {
	/* Offset 0: */
	int type;

#if defined(ARCH_ARM64) || defined(ARCH_X86_64) || defined(ARCH_PPC64)
	int padding;
#endif

	/* Offset 4 or 8: */
	union {
		int i;
		float f;
		struct rt_string *str;
		struct rt_array *arr;
		struct rt_dict *dict;
		struct rt_func *func;
	} val;
};

/* String object. */
struct rt_string {
	char *s;

	/* String list (shallow or deep). */
	struct rt_string *prev;
	struct rt_string *next;
	bool is_deep;

	/* Is marked? (for mark-and-sweep GC). */
	bool is_marked;
};

/* Array object */
struct rt_array {
	int alloc_size;
	int size;
	struct rt_value *table;

	/* Array list (shallow or deep). */
	struct rt_array *prev;
	struct rt_array *next;
	bool is_deep;

	/* Is marked? (for mark-and-sweep GC). */
	bool is_marked;
};

/* Dictionary object. */
struct rt_dict {
	int alloc_size;
	int size;
	char **key;
	struct rt_value *value;

	/* Dict list (shallow or deep). */
	struct rt_dict *prev;
	struct rt_dict *next;
	bool is_deep;

	/* Is marked? (for mark-and-sweep GC). */
	bool is_marked;
};

/* Function object. */
struct rt_func {
	char *name;
	int param_count;
	char *param_name[RT_ARG_MAX];

	char *file_name;

	/* Bytecode for a function. (if not a cfunc) */
	int bytecode_size;
	uint8_t *bytecode;
	int tmpvar_size;

	/* JIT-generated code. */
	bool (*jit_code)(struct rt_env *env);

	/* Function pointer. (if a cfunc) */
	bool (*cfunc)(struct rt_env *env);

	/* Next. */
	struct rt_func *next;
};

/* Global variable entry. */
struct rt_bindglobal {
	char *name;
	struct rt_value val;

	/* XXX: */
	struct rt_bindglobal *next;
};

/* Local variable entry. */
struct rt_bindlocal {
	char *name;
	struct rt_value val;

	/* XXX: */
	struct rt_bindlocal *next;
};

/* Create a runtime environment. */
bool
rt_create(
	struct rt_env **rt);

/* Destroy a runtime environment. */
bool
rt_destroy(
	struct rt_env *rt);

/* Get a file name. */
const char *
rt_get_error_file(
	struct rt_env *rt);

/* Get an error line number. */
int
rt_get_error_line(
	struct rt_env *rt);

/* Get an error message. */
const char *
rt_get_error_message(
	struct rt_env *rt);

/* Output an error message. */
void
rt_error(
	struct rt_env *rt,
	const char *msg,
	...);

/* Output an out-of-memory message. */
void
rt_out_of_memory(
	struct rt_env *rt);

/* Register functions from a souce text. */
bool
rt_register_source(
	struct rt_env *rt,
	const char *file_name,
	const char *source_text);

/* Register functions from bytecode data. */
bool
rt_register_bytecode(
	struct rt_env *rt,
	uint32_t size,
	uint8_t *data);

/* Register a C function.. */
bool
rt_register_cfunc(
	struct rt_env *rt,
	const char *name,
	int param_count,
	const char *param_name[],
	bool (*cfunc)(struct rt_env *env));

/* Call a function. */
bool
rt_call(
	struct rt_env *rt,
	struct rt_func *func,
	struct rt_value *thisptr,
	int arg_count,
	struct rt_value *arg,
	struct rt_value *ret);

/* Call a function with a name. */
bool
rt_call_with_name(
	struct rt_env *rt,
	const char *func_name,
	struct rt_value *thisptr,
	int arg_count,
	struct rt_value *arg,
	struct rt_value *ret);

/* Make an integer value. */
void
rt_make_int(
	struct rt_value *val,
	int i);

/* Make a floating-point value. */
void
rt_make_float(
	struct rt_value *val,
	float f);

/* Make a string value. */
bool
rt_make_string(
	struct rt_env *rt,
	struct rt_value *val,
	const char *s);

/* Make a string value with a format. */
bool
rt_make_string_format(
	struct rt_env *rt,
	struct rt_value *val,
	const char *s,
	...);

/* Make an empty array value. */
bool
rt_make_empty_array(
	struct rt_env *rt,
	struct rt_value *val);

/* Make an empty dictionary value */
bool
rt_make_empty_dict(
	struct rt_env *rt,
	struct rt_value *val);

/* Clone a value. */
bool
rt_copy_value(
	struct rt_env *rt,
	struct rt_value *dst,
	struct rt_value *src);

/* Get a value type. */
bool
rt_get_value_type(
	struct rt_env *rt,
	struct rt_value *val,
	int *type);

/* Get an integer value. */
bool
rt_get_int(
	struct rt_env *rt,
	struct rt_value *val,
	int *ret);

/* Get a floating-point value. */
bool
rt_get_float(
	struct rt_env *rt,
	struct rt_value *val,
	float *ret);

/* Get a string value. */
bool
rt_get_string(
	struct rt_env *rt,
	struct rt_value *val,
	const char **ret);

/* Get a function value. */
bool
rt_get_func(
	struct rt_env *rt,
	struct rt_value *val,
	struct rt_func **ret);

/* Get an array size. */
bool
rt_get_array_size(
	struct rt_env *rt,
	struct rt_value *val,
	int *size);

/* Get an array element. */
bool
rt_get_array_elem(
	struct rt_env *rt,
	struct rt_value *array,
	int index,
	struct rt_value *val);

/* Set an array element. */
bool
rt_set_array_elem(
	struct rt_env *rt,
	struct rt_value *array,
	int index,
	struct rt_value *val);

/* Resize an array. */
bool
rt_resize_array(
	struct rt_env *rt,
	struct rt_value *arr,
	int size);

/* Get a dictionary size. */
bool
rt_get_dict_size(
	struct rt_env *rt,
	struct rt_value *dict,
	int *size);

/* Get a dictionary value by an index. */
bool
rt_get_dict_value_by_index(
	struct rt_env *rt,
	struct rt_value *dict,
	int index,
	struct rt_value *val);

/* Get a dictionary key by an index. */
bool
rt_get_dict_key_by_index(
	struct rt_env *rt,
	struct rt_value *dict,
	int index,
	const char **key);

/* Get a dictionary element. */
bool
rt_get_dict_elem(
	struct rt_env *rt,
	struct rt_value *dict,
	const char *key,
	struct rt_value *val);

/* Set a dictionary element. */
bool
rt_set_dict_elem(
	struct rt_env *rt,
	struct rt_value *dict,
	const char *key,
	struct rt_value *val);

/* Remove a dictionary element. */
bool
rt_remove_dict_elem(
	struct rt_env *rt,
	struct rt_value *dict,
	const char *key);

/* Get a call argument. (For C func implementation) */
bool
rt_get_arg(
	struct rt_env *rt,
	int index,
	struct rt_value *val);

/* Set a return value. (For C func implementation) */
bool
rt_set_return(
	struct rt_env *rt,
	struct rt_value *val);

/* Get a global variable. */
bool
rt_get_global(
	struct rt_env *rt,
	const char *name,
	struct rt_value *val);

/* Set a global variable. */
bool
rt_set_global(
	struct rt_env *rt,
	const char *name,
	struct rt_value *val);

/* Do a shallow GC for nursery space. */
bool
rt_shallow_gc(
	struct rt_env *rt);

/* Do a deep GC for tenured space. */
bool
rt_deep_gc(
	struct rt_env *rt);

/* Get an approximate memory usage in bytes. */
bool
rt_get_heap_usage(
	struct rt_env *rt,
	size_t *ret);

/*
 * Execution helpers
 */

/* Do assign. */
bool
rt_assign_helper(
	struct rt_env *rt,
	int dst,
	int src);

/* Do add. */
bool
rt_add_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

/* Do sub. */
bool
rt_sub_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_mul_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_div_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_mod_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_and_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_or_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_xor_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_neg_helper(
	struct rt_env *rt,
	int dst,
	int src);

bool
rt_lt_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_lte_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_eq_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_neq_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_gte_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_gt_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_storearray_helper(
	struct rt_env *rt,
	int arr,
	int subscr,
	int val);

bool
rt_loadarray_helper(
	struct rt_env *rt,
	int dst,
	int arr,
	int subscr);

bool
rt_len_helper(
	struct rt_env *rt,
	int dst,
	int src);

bool
rt_getdictkeybyindex_helper(
	struct rt_env *rt,
	int dst,
	int dict,
	int subscr);

bool
rt_getdictvalbyindex_helper(
	struct rt_env *rt,
	int dst,
	int dict,
	int subscr);

bool
rt_loadsymbol_helper(
	struct rt_env *rt,
	int dst,
	const char *symbol);

bool
rt_storesymbol_helper(
	struct rt_env *rt,
	const char *symbol,
	int src);

bool
rt_loaddot_helper(
	struct rt_env *rt,
	int dst,
	int dict,
	const char *field);

bool
rt_storedot_helper(
	struct rt_env *rt,
	int dict,
	const char *field,
	int src);

bool
rt_call_helper(
	struct rt_env *rt,
	int dst,
	int func,
	int arg_count,
	int *arg);

bool
rt_thiscall_helper(
	struct rt_env *rt,
	int dst,
	int obj,
	const char *name,
	int arg_count,
	int *arg);

/* Generate a JIT-compiled code for a function. */
bool
jit_build(
	struct rt_env *rt,
	struct rt_func *func);

/* Free a JIT-compiled code for a function. */
void
jit_free(
	struct rt_env *rt,
	struct rt_func *func);

/* Visit bytecode. */
bool
rt_visit_bytecode(struct rt_env *rt, struct rt_func *func);

/* Register intrinsics. */
bool
rt_register_intrinsics(
	struct rt_env *rt);

#endif
