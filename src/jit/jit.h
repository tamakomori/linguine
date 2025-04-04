/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Linguine
 * Copyright (c) 2025, Tamako Mori. All rights reserved.
 */

/*
 * JIT (mips32): Just-In-Time native code generation
 */

#ifndef LINGUINE_JIT_H
#define LINGUINE_JIT_H

#if defined(USE_JIT)

#include "linguine/compat.h"

#include <string.h>

/* Error message */
#define BROKEN_BYTECODE		_("Broken bytecode.")

/* Code size. */
#define JIT_CODE_MAX	(16 * 1024 * 1024)

/* PC entry size. */
#define PC_ENTRY_MAX		2048

/* Branch pathch size. */
#define BRANCH_PATCH_MAX	2048

/*
 * JIT codegen context
 */
struct jit_context {
	struct rt_env *rt;
	struct rt_func *func;

	/* Mappd code area top. */
	void *code_top;

	/* Mapped code area end. */
	void *code_end;

	/* Current code position. */
	void *code;

	/* Exception handler. */
	void *exception_code;

	/* Current code LIR PC. */
	int lpc;

	/* Table to represent LIR-PC to Arm64-code map. */
	struct pc_entry {
		uint32_t lpc;
		uint32_t *code;
	} pc_entry[PC_ENTRY_MAX];
	int pc_entry_count;

	/* Table to represent branch patching entries. */
	struct branch_patch {
		uint32_t *code;
		uint32_t lpc;
		int type;
	} branch_patch[BRANCH_PATCH_MAX];
	int branch_patch_count;
};

/* Map a region. */
bool jit_map_memory_region(void **region, size_t size);

/* Make a region writable. */
void jit_map_writable(void *region, size_t size);

/* Make a region executable. */
void jit_map_executable(void * region, size_t size);

/*
 * Get an opcode.
 */
#define CONSUME_OPCODE(d)	if (!jit_get_opcode(ctx, &d)) return false
static INLINE bool
jit_get_opcode(
	struct jit_context *ctx,
	uint8_t *opcode)
{
	if (ctx->lpc + 1 > ctx->func->bytecode_size) {
		rt_error(ctx->rt, BROKEN_BYTECODE);
		return false;
	}

	*opcode = ctx->func->bytecode[ctx->lpc];

	ctx->lpc++;

	return true;
}

/*
 * Get an imm32 operand.
 */
#define CONSUME_IMM32(d)	if (!jit_get_opr_imm32(ctx, &d)) return false
static INLINE bool
jit_get_opr_imm32(
	struct jit_context *ctx,
	uint32_t *d)
{
	if (ctx->lpc + 4 > ctx->func->bytecode_size) {
		rt_error(ctx->rt, BROKEN_BYTECODE);
		return false;
	}

	*d = ((uint32_t)ctx->func->bytecode[ctx->lpc] << 24) |
	     (uint32_t)(ctx->func->bytecode[ctx->lpc + 1] << 16) |
	     (uint32_t)(ctx->func->bytecode[ctx->lpc + 2] << 8) |
	     (uint32_t)ctx->func->bytecode[ctx->lpc + 3];

	ctx->lpc += 4;

	return true;
}

/*
 * Get an imm16 operand that represents tmpvar index.
 */
#define CONSUME_TMPVAR(d)	if (!jit_get_opr_tmpvar(ctx, &d)) return false
static INLINE bool
jit_get_opr_tmpvar(
	struct jit_context *ctx,
	int *d)
{
	if (ctx->lpc + 2 > ctx->func->bytecode_size) {
		rt_error(ctx->rt, BROKEN_BYTECODE);
		return false;
	}

	*d = (ctx->func->bytecode[ctx->lpc] << 8) |
	      ctx->func->bytecode[ctx->lpc + 1];
	if (*d >= ctx->func->tmpvar_size) {
		rt_error(ctx->rt, BROKEN_BYTECODE);
		return false;
	}

	ctx->lpc += 2;

	return true;
}

/*
 * Get an imm8 operand.
 */
#define CONSUME_IMM8(d)		if (!jit_get_imm8(ctx, &d)) return false
static INLINE bool
jit_get_imm8(
	struct jit_context *ctx,
	int *imm8)
{
	if (ctx->lpc + 1 > ctx->func->bytecode_size) {
		rt_error(ctx->rt, BROKEN_BYTECODE);
		return false;
	}

	*imm8 = ctx->func->bytecode[ctx->lpc];

	ctx->lpc++;

	return true;
}

/*
 * Get a string operand.
 */
#define CONSUME_STRING(d)	if (!jit_get_opr_string(ctx, &d)) return false
static INLINE bool
jit_get_opr_string(
	struct jit_context *ctx,
	const char **d)
{
	int len;

	len = (int)strlen((const char *)&ctx->func->bytecode[ctx->lpc]);
	if (ctx->lpc + len + 1 > ctx->func->bytecode_size) {
		rt_error(ctx->rt, BROKEN_BYTECODE);
		return false;
	}

	*d = (const char *)&ctx->func->bytecode[ctx->lpc];

	ctx->lpc += len + 1;

	return true;
}

#endif /* defined(USE_JIT) */

#endif
