/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Linguine
 * Copyright (c) 2025, Tamako Mori. All rights reserved.
 */

/*
 * Intrinsics
 */

#include "linguine/runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define NEVER_COME_HERE		0

static bool rt_intrin_length(struct rt_env *rt);
static bool rt_intrin_push(struct rt_env *rt);
static bool rt_intrin_unset(struct rt_env *rt);
static bool rt_intrin_resize(struct rt_env *rt);
static bool rt_intrin_substring(struct rt_env *rt);

bool
rt_register_intrinsics(
	struct rt_env *rt)
{
	struct item {
		const char *name;
		int param_count;
		const char *param[RT_ARG_MAX];
		bool (*cfunc)(struct rt_env *rt);
	} items[] = {
		{"length", 1, {"val"}, rt_intrin_length},
		{"push", 2, {"arr", "val"}, rt_intrin_push},
		{"unset", 2, {"dict", "key"}, rt_intrin_unset},
		{"resize", 2, {"arr", "size"}, rt_intrin_resize},
		{"substring", 3, {"str", "start", "len"}, rt_intrin_substring},
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
}

/* length() */
static bool
rt_intrin_length(
	struct rt_env *rt)
{
	struct rt_value val, ret;

	if (!rt_get_arg(rt, 0, &val))
		return false;

	switch (val.type) {
	case RT_VALUE_INT:
	case RT_VALUE_FLOAT:
	case RT_VALUE_FUNC:
		ret.type = RT_VALUE_INT;
		ret.val.i = 0;
		break;
	case RT_VALUE_STRING:
		ret.type = RT_VALUE_INT;
		ret.val.i = (int)strlen(val.val.str->s);
		break;
	case RT_VALUE_ARRAY:
		ret.type = RT_VALUE_INT;
		ret.val.i = val.val.arr->size;
		break;
	case RT_VALUE_DICT:
		ret.type = RT_VALUE_INT;
		ret.val.i = val.val.dict->size;
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	if (!rt_set_return(rt, &ret))
		return false;

	return true;
}

/* push() */
static bool
rt_intrin_push(
	struct rt_env *rt)
{
	struct rt_value arr, val;

	if (!rt_get_arg(rt, 0, &arr))
		return false;
	if (!rt_get_arg(rt, 1, &val))
		return false;

	switch (arr.type) {
	case RT_VALUE_INT:
	case RT_VALUE_FLOAT:
	case RT_VALUE_FUNC:
	case RT_VALUE_STRING:
	case RT_VALUE_DICT:
		rt_error(rt, "Not an array.");
		break;
	case RT_VALUE_ARRAY:
		if (!rt_set_array_elem(rt, &arr, arr.val.arr->size, &val))
			return false;
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	if (!rt_set_return(rt, &arr))
		return false;

	return true;
}

/* unset() */
static bool
rt_intrin_unset(
	struct rt_env *rt)
{
	struct rt_value arr, val;

	if (!rt_get_arg(rt, 0, &arr))
		return false;
	if (!rt_get_arg(rt, 1, &val))
		return false;

	switch (arr.type) {
	case RT_VALUE_INT:
	case RT_VALUE_FLOAT:
	case RT_VALUE_FUNC:
	case RT_VALUE_STRING:
	case RT_VALUE_DICT:
		rt_error(rt, _("Not a dictionary."));
		break;
	case RT_VALUE_ARRAY:
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	if (val.type != RT_VALUE_STRING) {
		rt_error(rt, _("Subscript not a string."));
		return false;
	}

	if (!rt_remove_dict_elem(rt, &arr, val.val.str->s))
		return false;

	return true;
}

/* resize() */
static bool
rt_intrin_resize(
	struct rt_env *rt)
{
	struct rt_value arr, size;

	if (!rt_get_arg(rt, 0, &arr))
		return false;
	if (!rt_get_arg(rt, 1, &size))
		return false;

	switch (arr.type) {
	case RT_VALUE_INT:
	case RT_VALUE_FLOAT:
	case RT_VALUE_FUNC:
	case RT_VALUE_STRING:
	case RT_VALUE_DICT:
		rt_error(rt, _("Not an array."));
		break;
	case RT_VALUE_ARRAY:
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	if (size.type != RT_VALUE_INT) {
		rt_error(rt, _("Value is not an integer."));
		return false;
	}

	if (!rt_resize_array(rt, &arr, size.val.i))
		return false;

	return true;
}

/* substring() */
static bool
rt_intrin_substring(
	struct rt_env *rt)
{
	struct rt_value str_v, start_v, len_v, ret_v;
	int start_i, len_i, slen;
	char *s;

	if (!rt_get_arg(rt, 0, &str_v))
		return false;
	if (!rt_get_arg(rt, 1, &start_v))
		return false;
	if (!rt_get_arg(rt, 2, &len_v))
		return false;

	if (str_v.type != RT_VALUE_STRING) {
		rt_error(rt, "Not a string.");
		return false;
	}
	if (start_v.type != RT_VALUE_INT) {
		rt_error(rt, "Not an integer.");
		return false;
	}
	if (len_v.type != RT_VALUE_INT) {
		rt_error(rt, "Not an integer.");
		return false;
	}

	start_i = start_v.val.i;
	if (start_i == -1)
		start_i = 0;

	slen = (int)strlen(str_v.val.str->s);
	len_i = len_v.val.i;
	if (len_i < 0)
		len_i = slen;
	if (start_i + len_i > slen)
		len_i = slen - start_i;

	s = malloc((size_t)(len_i + 1));
	if (s == NULL) {
		rt_out_of_memory(rt);
		return false;
	}

	strncpy(s, str_v.val.str->s + start_i, (size_t)len_i);

	if (!rt_make_string(rt, &ret_v, s))
		return false;

	free(s);

	if (!rt_set_return(rt, &ret_v))
		return false;

	return true;
}
