/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Linguine
 * Copyright (c) 2025, Tamako Mori. All rights reserved.
 */

/*
 * JIT (mips64): Just-In-Time native code generation
 */

#include "linguine/compat.h"		/* ARCH_MIPS64 */

#if defined(ARCH_MIPS64) && defined(USE_JIT)

#include "linguine/runtime.h"
#include "jit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/mman.h>		/* mmap(), mprotect(), munmap() */

/* False asseretion */
#define JIT_OP_NOT_IMPLEMENTED	0
#define NEVER_COME_HERE		0

/* Branch patch type */
#define PATCH_BAL		0
#define PATCH_BEQ		1
#define PATCH_BNE		2

/* Generated code. */
static uint32_t *jit_code_region;
static uint32_t *jit_code_region_cur;
static uint32_t *jit_code_region_tail;

/* Forward declaration */
static bool jit_visit_bytecode(struct jit_context *ctx);
static bool jit_patch_branch(struct jit_context *ctx, int patch_index);

/*
 * Generate a JIT-compiled code for a function.
 */
bool
jit_build(
	  struct rt_env *rt,
	  struct rt_func *func)
{
	struct jit_context ctx;
	int i;

	/* If the first call, map a memory region for the generated code. */
	if (jit_code_region == NULL) {
		if (!jit_map_memory_region((void **)&jit_code_region, JIT_CODE_MAX)) {
			rt_error(rt, _("Memory mapping failed."));
			return false;
		}
		jit_code_region_cur = jit_code_region;
		jit_code_region_tail = jit_code_region + JIT_CODE_MAX / 4;
	}

	/* Make a context. */
	memset(&ctx, 0, sizeof(struct jit_context));
	ctx.code_top = jit_code_region_cur;
	ctx.code_end = jit_code_region_tail;
	ctx.code = ctx.code_top;
	ctx.rt = rt;
	ctx.func = func;

	/* Make code writable and non-executable. */
	jit_map_writable(jit_code_region, JIT_CODE_MAX);

	/* Visit over the bytecode. */
	if (!jit_visit_bytecode(&ctx))
		return false;

	jit_code_region_cur = ctx.code;

	/* Patch branches. */
	for (i = 0; i < ctx.branch_patch_count; i++) {
		if (!jit_patch_branch(&ctx, i))
			return false;
	}

	/* Make code executable and non-writable. */
	jit_map_executable(jit_code_region, JIT_CODE_MAX);

	func->jit_code = (bool (*)(struct rt_env *))ctx.code_top;

	return true;
}

/*
 * Free a JIT-compiled code for a function.
 */
void
jit_free(
	 struct rt_env *rt,
	 struct rt_func *func)
{
	UNUSED_PARAMETER(rt);
	UNUSED_PARAMETER(func);

	/* XXX: */
}

/*
 * Assembler output functions
 */

/* Decoration */
#define ASM

/* Registers */
#define REG_ZERO	0
#define REG_AT		1
#define REG_V0		2
#define REG_V1		3
#define REG_A0		4
#define REG_A1		5
#define REG_A2		6
#define REG_A3		7
#define REG_T0		8
#define REG_T1		9
#define REG_T2		10
#define REG_T3		11
#define REG_T4		12
#define REG_T5		13
#define REG_T6		14
#define REG_T7		15
#define REG_S0		16
#define REG_S1		17
#define REG_S2		18
#define REG_S3		19
#define REG_S4		20
#define REG_S5		21
#define REG_S6		22
#define REG_S7		23
#define REG_T8		24
#define REG_T9		25
#define REG_K0		26
#define REG_K1		27
#define REG_GP		28
#define REG_SP		29
#define REG_FP		30
#define REG_RA		31

/* Put a instruction word. */
#define IW(w)				if (!jit_put_word(ctx, w)) return false
static INLINE bool
jit_put_word(
	struct jit_context *ctx,
	uint32_t word)
{
	if (ctx->code >= ctx->code_end) {
		rt_error(ctx->rt, "Code too big.");
		return false;
	}

	*(uint32_t *)ctx->code = word;
	ctx->code = (uint32_t *)ctx->code + 1;

	return true;
}

/*
 * Templates
 */

static INLINE uint32_t hihi16(uint64_t d)
{
	return (uint32_t)((d >> 48) & 0xffff);
}

static INLINE uint32_t hilo16(uint64_t d)
{
	return (uint32_t)((d >> 32) & 0xffff);
}

static INLINE uint32_t lohi16(uint64_t d)
{
	return (uint32_t)((d >> 16) & 0xffff);
}

static INLINE uint32_t lolo16(uint64_t d)
{
	return (uint32_t)(d & 0xffff);
}

static INLINE uint32_t hi16(uint32_t d)
{
	return (d >> 16) & 0xffff;
}

static INLINE uint32_t lo16(uint32_t d)
{
	return d & 0xffff;
}

static INLINE uint32_t tvar16(int d)
{
	return (uint32_t)d & 0xffff;
}

#define EXC()	exc((uint64_t)ctx->exception_code, (uint64_t)ctx->code)
static INLINE uint32_t exc(uint64_t handler, uint64_t cur)
{
	return (uint32_t)(((handler - cur - 4) / 4) & 0xffff);
}

#define ASM_BINARY_OP(f)												\
	ASM { 														\
		/* $s0: rt */												\
		/* $s1: &rt->frame->tmpvar[0] */									\
															\
		/* Arg1 $a0 = rt */											\
		/* move $a0, $s0 */		IW(0x02002025);								\
															\
		/* Arg2 $a1 = dst */											\
		/* li $a1, dst */		IW(0x24050000 | lo16((uint32_t)dst));					\
															\
		/* Arg3 $a2 = src1 */											\
		/* li $a2, src1 */		IW(0x24060000 | tvar16(src1)); 						\
															\
		/* Arg4 $a3: src2 */											\
		/* li $a3, src2 */		IW(0x24070000 | tvar16(src2)); 						\
															\
		/* Call f(). */												\
		/* lui  $t9, f@hh */		IW(0x3c190000 | hihi16((uint64_t)f));					\
		/* ori  $t9, f@hl */		IW(0x37390000 | hilo16((uint64_t)f));					\
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);								\
		/* ori  $t9, f@lh */		IW(0x37390000 | lohi16((uint64_t)f));					\
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);								\
		/* ori  $t9, f@ll */		IW(0x37390000 | lolo16((uint64_t)f));					\
		/* move $s2, $ra */		IW(0x03e09025);								\
		/* jalr $t9 */			IW(0x0320f809);								\
		/* nop */			IW(0x00000000);								\
		/* move $ra, $s2 */		IW(0x0240f825);								\
															\
		/* If failed: */											\
		/* beqz $v0, $zero, exc */	IW(0x10400000 | EXC());							\
		/* nop */			IW(0x00000000);								\
	}

#define ASM_UNARY_OP(f)													\
	ASM { 														\
		/* $s0: rt */												\
		/* $s1: &rt->frame->tmpvar[0] */									\
															\
		/* Arg1 $a0 = rt */											\
		/* move $a0, $s0 */		IW(0x02002025);								\
															\
		/* Arg2 $a1 = dst */											\
		/* li $a1, dst */		IW(0x24050000 | lo16((uint32_t)dst));					\
															\
		/* Arg3 $a2 = src */											\
		/* li $a2, src */		IW(0x24060000 | tvar16(src)); 						\
															\
		/* Call f(). */												\
		/* lui  $t9, f@hh */		IW(0x3c190000 | hihi16((uint64_t)f));					\
		/* ori  $t9, f@hl */		IW(0x37390000 | hilo16((uint64_t)f));					\
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);								\
		/* ori  $t9, f@lh */		IW(0x37390000 | lohi16((uint64_t)f));					\
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);								\
		/* ori  $t9, f@ll */		IW(0x37390000 | lolo16((uint64_t)f));					\
		/* move $s2, $ra */		IW(0x03e09025);								\
		/* jalr $t9 */			IW(0x0320f809);								\
		/* nop */			IW(0x00000000);								\
		/* move $ra, $s2 */		IW(0x0240f825);								\
															\
		/* If failed: */											\
		/* beqz $v0, $zero, exc */	IW(0x10400000 | EXC());							\
		/* nop */			IW(0x00000000);								\
	}

/*
 * Bytecode visitors
 */

/* Visit a ROP_LINEINFO instruction. */
static INLINE bool
jit_visit_lineinfo_op(
	struct jit_context *ctx)
{
	uint32_t line;

	CONSUME_IMM32(line);

	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* rt->line = line; */
		/* li $t0, line */	IW(0x24080000 | lo16(line));
		/* sw $t0, 8($s0) */	IW(0xae080008);
	}

	return true;
}

/* Visit a ROP_ASSIGN instruction. */
static INLINE bool
jit_visit_assign_op(
	struct jit_context *ctx)
{
	int dst;
	int src;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src);

	dst *= (int)sizeof(struct rt_value);
	src *= (int)sizeof(struct rt_value);

	/* rt->frame->tmpvar[dst] = rt->frame->tmpvar[src]; */
	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* $t0 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li    $t0, dst */		IW(0x240c0000 | lo16((uint32_t)dst));
		/* daddu $t0, $t0, $s1 */	IW(0x0191602d);

		/* $t1 = src_addr = &rt->frame->tmpvar[src] */
		/* li   $t1, src */		IW(0x240d0000 | lo16((uint32_t)src));
		/* daddu $t1, $t1, $s1 */	IW(0x01b1682d);

		/* *dst_addr = *src_addr */
		/* ld $t2, 0($t1) */		IW(0xddae0000);
		/* ld $t3, 8($t1) */		IW(0xddaf0008);
		/* sd $t2, 0($t0) */		IW(0xfd8e0000);
		/* sd $t3, 8($t0) */		IW(0xfd8f0008);
	}

	return true;
}

/* Visit a ROP_ICONST instruction. */
static INLINE bool
jit_visit_iconst_op(
	struct jit_context *ctx)
{
	int dst;
	uint32_t val;

	CONSUME_TMPVAR(dst);
	CONSUME_IMM32(val);

	dst *= (int)sizeof(struct rt_value);

	/* Set an integer constant. */
	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* $t0 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li    $t0, dst */		IW(0x240c0000 | lo16((uint32_t)dst));
		/* daddu $t0, $t0, $s1 */	IW(0x0191602d);

		/* rt->frame->tmpvar[dst].type = RT_VALUE_INT */
		/* li $t1, 0 */			IW(0x240d0000);
		/* sw $t1, 0($t0) */		IW(0xad8d0000);

		/* rt->frame->tmpvar[dst].val.i = val */
		/* lui $t1, val@h */		IW(0x3c0d0000 | hi16(val));
		/* ori $t1, $t1, val@l */	IW(0x35ad0000 | lo16(val));
		/* sw  $t1, 8($t0) */		IW(0xad8d0008);
	}

	return true;
}

/* Visit a ROP_FCONST instruction. */
static INLINE bool
jit_visit_fconst_op(
	struct jit_context *ctx)
{
	int dst;
	uint32_t val;

	CONSUME_TMPVAR(dst);
	CONSUME_IMM32(val);

	dst *= (int)sizeof(struct rt_value);

	/* Set a floating-point constant. */
	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* $t0 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li    $t0, dst */		IW(0x240c0000 | lo16((uint32_t)dst));
		/* daddu $t0, $t0, $s1 */	IW(0x0191602d);

		/* rt->frame->tmpvar[dst].type = RT_VALUE_FLOAT */
		/* li $t1, 1 */			IW(0x240d0001);
		/* sw $t1, 0($t0) */		IW(0xad8d0000);

		/* rt->frame->tmpvar[dst].val.i = val */
		/* lui $t1, val@h */		IW(0x3c0d0000 | hi16(val));
		/* ori $t1, $t1, val@l */	IW(0x35ad0000 | lo16(val));
		/* sw  $t1, 8($t0) */		IW(0xad8d0008);
	}

	return true;
}

/* Visit a ROP_SCONST instruction. */
static INLINE bool
jit_visit_sconst_op(
	struct jit_context *ctx)
{
	int dst;
	const char *val;
	uint64_t f;

	CONSUME_TMPVAR(dst);
	CONSUME_STRING(val);

	f = (uint64_t)rt_make_string;
	dst *= (int)sizeof(struct rt_value);

	/* rt_make_string(rt, &rt->frame->tmpvar[dst], val); */
	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* Arg1 $a0 = rt */
		/* move $a0, $s0 */		IW(0x02002025);

		/* Arg2 $a1 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li    $a1, dst */		IW(0x24050000 | tvar16(dst));
		/* daddu $a1, $a1, $s1 */	IW(0x00b1282d);

		/* Arg3 $a2 = val */
		/* lui  $a2, val@hh */		IW(0x3c060000 | hihi16((uint64_t)val));
		/* ori  $a2, val@hl */		IW(0x34c60000 | hilo16((uint64_t)val));
		/* dsll $a2, $a2, 16 */		IW(0x00063438);
		/* ori  $a2, val@lh */		IW(0x34c60000 | lohi16((uint64_t)val));
		/* dsll $a2, $a2, 16 */		IW(0x00063438);
		/* ori  $a2, val@ll */		IW(0x34c60000 | lolo16((uint64_t)val));

		/* Call rt_make_string(). */
		/* lui  $t9, f@hh */		IW(0x3c190000 | hihi16(f));
		/* ori  $t9, f@hl */		IW(0x37390000 | hilo16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@lh */		IW(0x37390000 | lohi16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@ll */		IW(0x37390000 | lolo16(f));
		/* move $s2, $ra */		IW(0x03e09025);
		/* jalr $t9 */			IW(0x0320f809);
		/* nop */			IW(0x00000000);
		/* move $ra, $s2 */		IW(0x0240f825);

		/* If failed: */
		/* beqz $v0, $zero, exc */	IW(0x10400000 | EXC());
		/* nop */			IW(0x00000000);
	}

	return true;
}

/* Visit a ROP_ACONST instruction. */
static INLINE bool
jit_visit_aconst_op(
	struct jit_context *ctx)
{
	int dst;
	uint64_t f;

	CONSUME_TMPVAR(dst);

	f = (uint64_t)rt_make_empty_array;
	dst *= (int)sizeof(struct rt_value);

	/* rt_make_empty_array(rt, &rt->frame->tmpvar[dst]); */
	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* Arg1 $a0 = rt */
		/* move $a0, $s0 */		IW(0x02002025);

		/* Arg2 $a1 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li    $a1, dst */		IW(0x24050000 | lo16((uint32_t)dst));
		/* daddu $a1, $a1, $s1 */	IW(0x00b1282d);

		/* Call rt_make_empty_array(). */
		/* lui  $t9, f@hh */		IW(0x3c190000 | hihi16(f));
		/* ori  $t9, f@hl */		IW(0x37390000 | hilo16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@lh */		IW(0x37390000 | lohi16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@ll */		IW(0x37390000 | lolo16(f));
		/* move $s2, $ra */		IW(0x03e09025);
		/* jalr $t9 */			IW(0x0320f809);
		/* nop */			IW(0x00000000);
		/* move $ra, $s2 */		IW(0x0240f825);

		/* If failed: */
		/* beqz $v0, $zero, exc */	IW(0x10400000 | EXC());
		/* nop */			IW(0x00000000);
	}

	return true;
}

/* Visit a ROP_DCONST instruction. */
static INLINE bool
jit_visit_dconst_op(
	struct jit_context *ctx)
{
	int dst;
	uint64_t f;

	CONSUME_TMPVAR(dst);

	f = (uint64_t)rt_make_empty_dict;
	dst *= (int)sizeof(struct rt_value);

	/* rt_make_empty_dict(rt, &rt->frame->tmpvar[dst]); */
	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* Arg1 $a0 = rt */
		/* move $a0, $s0 */		IW(0x02002025);

		/* Arg2 $a1 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li    $a1, dst */		IW(0x24050000 | lo16((uint32_t)dst));
		/* daddu $a1, $a1, $s1 */	IW(0x00b1282d);

		/* Call rt_make_empty_dict(). */
		/* lui  $t9, f@hh */		IW(0x3c190000 | hihi16(f));
		/* ori  $t9, f@hl */		IW(0x37390000 | hilo16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@lh */		IW(0x37390000 | lohi16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@ll */		IW(0x37390000 | lolo16(f));
		/* move $s2, $ra */		IW(0x03e09025);
		/* jalr $t9 */			IW(0x0320f809);
		/* nop */			IW(0x00000000);
		/* move $ra, $s2 */		IW(0x0240f825);

		/* If failed: */
		/* beqz $v0, $zero, exc */	IW(0x10400000 | EXC());
		/* nop */			IW(0x00000000);
	}

	return true;
}

/* Visit a ROP_INC instruction. */
static INLINE bool
jit_visit_inc_op(
	struct jit_context *ctx)
{
	int dst;

	CONSUME_TMPVAR(dst);

	dst *= (int)sizeof(struct rt_value);

	/* Increment an integer. */
	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* $t0 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li    $t0, dst */		IW(0x240c0000 | lo16((uint32_t)dst));
		/* daddu $t0, $t0, $s1 */	IW(0x0191602d);

		/* rt->frame->tmpvar[dst].val.i++ */
		/* lw    $t1, 8($t0) */		IW(0x8d8d0008);
		/* addiu $t1, $t1, 1 */		IW(0x25ad0001);
		/* sw    $t1, 8($t0) */		IW(0xad8d0008);
	}

	return true;
}

/* Visit a ROP_ADD instruction. */
static INLINE bool
jit_visit_add_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_add_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_add_helper);

	return true;
}

/* Visit a ROP_SUB instruction. */
static INLINE bool
jit_visit_sub_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_sub_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_sub_helper);

	return true;
}

/* Visit a ROP_MUL instruction. */
static INLINE bool
jit_visit_mul_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_mul_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_mul_helper);

	return true;
}

/* Visit a ROP_DIV instruction. */
static INLINE bool
jit_visit_div_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_div_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_div_helper);

	return true;
}

/* Visit a ROP_MOD instruction. */
static INLINE bool
jit_visit_mod_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_mod_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_mod_helper);

	return true;
}

/* Visit a ROP_AND instruction. */
static INLINE bool
jit_visit_and_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_and_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_and_helper);

	return true;
}

/* Visit a ROP_OR instruction. */
static INLINE bool
jit_visit_or_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_or_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_or_helper);

	return true;
}

/* Visit a ROP_XOR instruction. */
static INLINE bool
jit_visit_xor_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_xor_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_xor_helper);

	return true;
}

/* Visit a ROP_XOR instruction. */
static INLINE bool
jit_visit_neg_op(
	struct jit_context *ctx)
{
	int dst;
	int src;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src);

	/* if (!rt_neg_helper(rt, dst, src)) return false; */
	ASM_UNARY_OP(rt_xor_helper);

	return true;
}

/* Visit a ROP_LT instruction. */
static INLINE bool
jit_visit_lt_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_lt_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_lt_helper);

	return true;
}

/* Visit a ROP_LTE instruction. */
static INLINE bool
jit_visit_lte_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_lte_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_lte_helper);

	return true;
}

/* Visit a ROP_EQ instruction. */
static INLINE bool
jit_visit_eq_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_eq_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_eq_helper);

	return true;
}

/* Visit a ROP_NEQ instruction. */
static INLINE bool
jit_visit_neq_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_neq_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_neq_helper);

	return true;
}

/* Visit a ROP_GTE instruction. */
static INLINE bool
jit_visit_gte_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_gte_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_gte_helper);

	return true;
}

/* Visit a ROP_GT instruction. */
static INLINE bool
jit_visit_gt_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_gt_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_gt_helper);

	return true;
}

/* Visit a ROP_EQI instruction. */
static INLINE bool
jit_visit_eqi_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	dst *= (int)sizeof(struct rt_value);
	src1 *= (int)sizeof(struct rt_value);
	src2 *= (int)sizeof(struct rt_value);

	/* src1 == src2 */
	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* $t0 = rt->frame->tmpvar[src1].val.i */
		/* li    $t0, src1 */		IW(0x240c0000 | lo16((uint32_t)src1));
		/* daddu $t0, $t0, $s1 */	IW(0x0191602d);
		/* lw    $t0, 8($t0) */		IW(0x8d8c0008);

		/* $t1 = rt->frame->tmpvar[src2].val.i */
		/* li    $t1, src2 */		IW(0x240d0000 | lo16((uint32_t)src2));
		/* daddu $t1, $t1, $s1 */	IW(0x01b1682d);
		/* lw    $t1, 8($t1) */		IW(0x8dad0008);

		/* src1 == src2 */
		/* dsubu $at, $t0, $t1 */	IW(0x018d082f);
	}

	return true;
}

/* Visit a ROP_LOADARRAY instruction. */
static INLINE bool
jit_visit_loadarray_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_loadarray_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_loadarray_helper);

	return true;
}

/* Visit a ROP_STOREARRAY instruction. */
static INLINE bool
jit_visit_storearray_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!jit_storearray_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_storearray_helper);

	return true;
}

/* Visit a ROP_LEN instruction. */
static INLINE bool
jit_visit_len_op(
	struct jit_context *ctx)
{
	int dst;
	int src;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src);

	/* if (!jit_len_helper(rt, dst, src)) return false; */
	ASM_UNARY_OP(rt_len_helper);

	return true;
}

/* Visit a ROP_GETDICTKEYBYINDEX instruction. */
static INLINE bool
jit_visit_getdictkeybyindex_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!jit_getdictkeybyindex_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_getdictkeybyindex_helper);

	return true;
}

/* Visit a ROP_GETDICTVALBYINDEX instruction. */
static INLINE bool
jit_visit_getdictvalbyindex_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!jit_getdictvalbyindex_helper(rt, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_getdictvalbyindex_helper);

	return true;
}

/* Visit a ROP_LOADSYMBOL instruction. */
static INLINE bool
jit_visit_loadsymbol_op(
	struct jit_context *ctx)
{
	int dst;
	const char *src_s;
	uint64_t src;
	uint64_t f;

	CONSUME_TMPVAR(dst);
	CONSUME_STRING(src_s);

	src = (uint64_t)(intptr_t)src_s;
	f = (uint64_t)rt_loadsymbol_helper;

	/* if (!jit_loadsymbol_helper(rt, dst, src)) return false; */
	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* Arg1 $a0 = rt */
		/* move $a0, $s0 */		IW(0x02002025);

		/* Arg2 $a1 = dst */
		/* li $a1, dst */		IW(0x24050000 | tvar16(dst));

		/* Arg3 $a2 = src */
		/* lui  $a2, src@hh */		IW(0x3c060000 | hihi16(src));
		/* ori  $a2, src@hl */		IW(0x34c60000 | hilo16(src));
		/* dsll $a2, $a2, 16 */		IW(0x00063438);
		/* ori  $a2, src@lh */		IW(0x34c60000 | lohi16(src));
		/* dsll $a2, $a2, 16 */		IW(0x00063438);
		/* ori  $a2, src@ll */		IW(0x34c60000 | lolo16(src));

		/* Call rt_loadsymbol_helper(). */
		/* lui  $t9, f@hh */		IW(0x3c190000 | hihi16(f));
		/* ori  $t9, f@hl */		IW(0x37390000 | hilo16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@lh */		IW(0x37390000 | lohi16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@ll */		IW(0x37390000 | lolo16(f));
		/* move $s2, $ra */		IW(0x03e09025);
		/* jalr $t9 */			IW(0x0320f809);
		/* nop */			IW(0x00000000);
		/* move $ra, $s2 */		IW(0x0240f825);

		/* If failed: */
		/* beqz $v0, $zero, exc */	IW(0x10400000 | EXC());
		/* nop */			IW(0x00000000);
	}

	return true;
}

/* Visit a ROP_STORESYMBOL instruction. */
static INLINE bool
jit_visit_storesymbol_op(
	struct jit_context *ctx)
{
	const char *dst_s;
	uint64_t dst;
	int src;
	uint64_t f;

	CONSUME_STRING(dst_s);
	CONSUME_TMPVAR(src);

	dst = (uint64_t)(intptr_t)dst_s;
	f = (uint64_t)rt_storesymbol_helper;

	/* if (!rt_storesymbol_helper(rt, dst, src)) return false; */
	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* Arg1 $a0 = rt */
		/* move $a0, $s0 */		IW(0x02002025);

		/* Arg2 $a1 = dst */
		/* lui  $a1, dst@hh */		IW(0x3c050000 | hihi16(dst));
		/* ori  $a1, dst@hl */		IW(0x34a50000 | hilo16(dst));
		/* dsll $a1, $a1, 16 */		IW(0x00052c38);
		/* ori  $a1, dst@lh */		IW(0x34a50000 | lohi16(dst));
		/* dsll $a1, $a1, 16 */		IW(0x00052c38);
		/* ori  $a1, dst@ll */		IW(0x34a50000 | lolo16(dst));

		/* Arg3 $a2 = src */
		/* li $a2, src */		IW(0x24060000 | tvar16(src));

		/* Call rt_storesymbol_helper(). */
		/* lui  $t9, f@hh */		IW(0x3c190000 | hihi16(f));
		/* ori  $t9, f@hl */		IW(0x37390000 | hilo16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@lh */		IW(0x37390000 | lohi16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@ll */		IW(0x37390000 | lolo16(f));
		/* move $s2, $ra */		IW(0x03e09025);
		/* jalr $t9 */			IW(0x0320f809);
		/* nop */			IW(0x00000000);
		/* move $ra, $s2 */		IW(0x0240f825);

		/* If failed: */
		/* beqz $v0, $zero, exc */	IW(0x10400000 | EXC());
		/* nop */			IW(0x00000000);
	}

	return true;
}

/* Visit a ROP_LOADDOT instruction. */
static INLINE bool
jit_visit_loaddot_op(
	struct jit_context *ctx)
{
	int dst;
	int dict;
	const char *field_s;
	uint64_t field;
	uint64_t f;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(dict);
	CONSUME_STRING(field_s);

	field = (uint64_t)(intptr_t)field_s;
	f = (uint64_t)rt_loaddot_helper;

	/* if (!rt_loaddot_helper(rt, dst, dict, field)) return false; */
	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* Arg1 $a0 = rt */
		/* move $a0, $s0 */		IW(0x02002025);

		/* Arg2 $a1 = dst */
		/* li $a1, dst */		IW(0x24050000 | tvar16(dst));

		/* Arg3 $a2 = dict */
		/* li $a2, dict */		IW(0x24060000 | tvar16(dict));

		/* Arg4 $a3 = field */
		/* lui  $a3, field@hh */	IW(0x3c070000 | hihi16(field));
		/* ori  $a3, field@hl */	IW(0x34e70000 | hilo16(field));
		/* dsll $a3, $a3, 16 */		IW(0x00073c38);
		/* ori  $a3, field@lh */	IW(0x34e70000 | lohi16(field));
		/* dsll $a3, $a3, 16 */		IW(0x00073c38);
		/* ori  $a3, field@ll */	IW(0x34e70000 | lolo16(field));

		/* Call rt_loaddot_helper(). */
		/* lui  $t9, f@hh */		IW(0x3c190000 | hihi16(f));
		/* ori  $t9, f@hl */		IW(0x37390000 | hilo16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@lh */		IW(0x37390000 | lohi16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@ll */		IW(0x37390000 | lolo16(f));
		/* move $s2, $ra */		IW(0x03e09025);
		/* jalr $t9 */			IW(0x0320f809);
		/* nop */			IW(0x00000000);
		/* move $ra, $s2 */		IW(0x0240f825);

		/* If failed: */
		/* beqz $v0, $zero, exc */	IW(0x10400000 | EXC());
		/* nop */			IW(0x00000000);
	}

	return true;
}

/* Visit a ROP_STOREDOT instruction. */
static INLINE bool
jit_visit_storedot_op(
	struct jit_context *ctx)
{
	int dict;
	const char *field_s;
	uint64_t field;
	int src;
	uint64_t f;

	CONSUME_TMPVAR(dict);
	CONSUME_STRING(field_s);
	CONSUME_TMPVAR(src);

	field = (uint64_t)(intptr_t)field_s;
	f = (uint64_t)rt_storedot_helper;

	/* if (!jit_storedot_helper(rt, dst, dict, field)) return false; */
	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* Arg1 $a0 = rt */
		/* move $a0, $s0 */		IW(0x02002025);

		/* Arg2 $a1 = dict */
		/* li $a1, dict */		IW(0x24050000 | tvar16(dict));

		/* Arg3 $a2 = field */
		/* lui  $a2, field@hh */	IW(0x3c060000 | hihi16(field));
		/* ori  $a2, field@hl */	IW(0x34c60000 | hilo16(field));
		/* dsll $a2, $a2, 16 */		IW(0x00063438);
		/* ori  $a2, field@lh */	IW(0x34c60000 | lohi16(field));
		/* dsll $a2, $a2, 16 */		IW(0x00063438);
		/* ori  $a2, field@ll */	IW(0x34c60000 | lolo16(field));

		/* Arg4 $a3 = src */
		/* li $a3, src */		IW(0x24070000 | tvar16(src));

		/* Call rt_storedot_helper(). */
		/* lui  $t9, f@hh */		IW(0x3c190000 | hihi16(f));
		/* ori  $t9, f@hl */		IW(0x37390000 | hilo16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@lh */		IW(0x37390000 | lohi16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@ll */		IW(0x37390000 | lolo16(f));
		/* move $s2, $ra */		IW(0x03e09025);
		/* jalr $t9 */			IW(0x0320f809);
		/* nop */			IW(0x00000000);
		/* move $ra, $s2 */		IW(0x0240f825);

		/* If failed: */
		/* beqz $v0, $zero, exc */	IW(0x10400000 | EXC());
		/* nop */			IW(0x00000000);
	}

	return true;
}

/* Visit a ROP_CALL instruction. */
static inline bool
jit_visit_call_op(
	struct jit_context *ctx)
{
	int dst;
	int func;
	int arg_count;
	int arg_tmp;
	int arg[RT_ARG_MAX];
	uint32_t tmp;
	uint64_t arg_addr;
	int i;
	uint64_t f;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(func);
	CONSUME_IMM8(arg_count);
	for (i = 0; i < arg_count; i++) {
		CONSUME_TMPVAR(arg_tmp);
		arg[i] = arg_tmp;
	}

	if (arg_count > 0) {
		/* Embed arguments to the code. */
		tmp = (uint32_t)((8 + 4 * arg_count - 4) / 4);
		ASM {
			/* b */		IW(0x10000000 | tmp);
			/* nop */	IW(0x00000000);
		}
		arg_addr = (uint64_t)(intptr_t)ctx->code;
		for (i = 0; i < arg_count; i++) {
			*(uint32_t *)ctx->code = (uint32_t)arg[i];
			ctx->code = (uint32_t *)ctx->code + 1;
		}
	} else {
		arg_addr = 0;
	}

	f = (uint64_t)rt_call_helper;

	/* if (!rt_call_helper(rt, dst, func, arg_count, arg)) return false; */
	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* Arg1 $a0 = rt */
		/* move $a0, $s0 */		IW(0x02002025);

		/* Arg2 $a1 = dst */
		/* li $a1, dst */		IW(0x24050000 | tvar16(dst));

		/* Arg3 $a2 = func */
		/* li $a2, func */		IW(0x24060000 | tvar16(func));

		/* Arg4 $a3 = arg_count */
		/* li $a3, arg_count */		IW(0x24070000 | lo16((uint32_t)arg_count));

		/* Arg5 $a4 = arg */
		/* lui  $a4, arg@hh */		IW(0x3c080000 | hihi16(arg_addr));
		/* ori  $a4, arg@hl */		IW(0x35080000 | hilo16(arg_addr));
		/* dsll $a4, $a4, 16 */		IW(0x00084438);
		/* ori  $a4, arg@lh */		IW(0x35080000 | lohi16(arg_addr));
		/* dsll $a4, $a4, 16 */		IW(0x00084438);
		/* ori  $a4, arg@ll */		IW(0x35080000 | lolo16(arg_addr));

		/* Call rt_call_helper(). */
		/* lui  $t9, f@hh */		IW(0x3c190000 | hihi16(f));
		/* ori  $t9, f@hl */		IW(0x37390000 | hilo16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@lh */		IW(0x37390000 | lohi16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@ll */		IW(0x37390000 | lolo16(f));
		/* move $s2, $ra */		IW(0x03e09025);
		/* jalr $t9 */			IW(0x0320f809);
		/* nop */			IW(0x00000000);
		/* move $ra, $s2 */		IW(0x0240f825);

		/* If failed: */
		/* beqz $v0, $zero, exc */	IW(0x10400000 | EXC());
		/* nop */			IW(0x00000000);
	}

	return true;
}

/* Visit a ROP_THISCALL instruction. */
static inline bool
jit_visit_thiscall_op(
	struct jit_context *ctx)
{
	int dst;
	int obj;
	const char *symbol;
	int arg_count;
	int arg_tmp;
	int arg[RT_ARG_MAX];
	uint32_t tmp;
	uint64_t arg_addr;
	int i;
	uint64_t f;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(obj);
	CONSUME_STRING(symbol);
	CONSUME_IMM8(arg_count);
	for (i = 0; i < arg_count; i++) {
		CONSUME_TMPVAR(arg_tmp);
		arg[i] = arg_tmp;
	}

	if (arg_count > 0) {
		/* Embed arguments to the code. */
		tmp = (uint32_t)((4 + 4 * arg_count + 4) / 4);
		ASM {
			/* b */		IW(0x10000000 | tmp);
			/* nop */	IW(0x00000000);
		}
		arg_addr = (uint64_t)(intptr_t)ctx->code;
		for (i = 0; i < arg_count; i++) {
			*(uint32_t *)ctx->code = (uint32_t)arg[i];
			ctx->code = (uint32_t *)ctx->code + 1;
		}
	} else {
		arg_addr = 0;
	}

	f = (uint64_t)rt_thiscall_helper;

	/* if (!rt_thiscall_helper(rt, dst, obj, symbol, arg_count, arg)) return false; */
	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* Arg1 $a0 = rt */
		/* move $a0, $s0 */		IW(0x02002025);

		/* Arg2 $a1 = dst */
		/* li $a1, dst */		IW(0x24050000 | tvar16(dst));

		/* Arg3 $a2 = obj */
		/* li $a2, obj */		IW(0x24060000 | tvar16(obj));

		/* Arg4 $a3 symbol */
		/* lui  $a3, symbol@hh */	IW(0x3c070000 | hihi16((uint64_t)symbol));
		/* ori  $a3, symbol@hl */	IW(0x34e70000 | hilo16((uint64_t)symbol));
		/* dsll $a3, $a3, 16 */		IW(0x00073c38);
		/* ori  $a3, symbol@lh */	IW(0x34e70000 | lohi16((uint64_t)symbol));
		/* dsll $a3, $a3, 16 */		IW(0x00073c38);
		/* ori  $a3, symbol@ll */	IW(0x34e70000 | lolo16((uint64_t)symbol));

		/* Arg5 arg */
		/* lui  $t0, arg@hh */		IW(0x3c0c0000 | hihi16(arg_addr));
		/* ori  $t0, arg@hl */		IW(0x358c0000 | hilo16(arg_addr));
		/* dsll $t0, $a2, 16 */		IW(0x000c6438);
		/* ori  $t0, arg@lh */		IW(0x358c0000 | lohi16(arg_addr));
		/* dsll $t0, $a2, 16 */		IW(0x000c6438);
		/* ori  $t0, arg@ll */		IW(0x358c0000 | lolo16(arg_addr));
		/* daddiu $sp, $sp, -40 */	IW(0xffac0020);
		/* sd $t0, 32($sp) */		IW(0xafa80010);

		/* Call rt_thiscall_helper(). */
		/* lui  $t9, f@hh */		IW(0x3c190000 | hihi16(f));
		/* ori  $t9, f@hl */		IW(0x37390000 | hilo16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@lh */		IW(0x37390000 | lohi16(f));
		/* dsll $t9, $t9, 16 */		IW(0x0019cc38);
		/* ori  $t9, f@ll */		IW(0x37390000 | lolo16(f));
		/* move $s2, $ra */		IW(0x03e09025);
		/* jalr $t9 */			IW(0x0320f809);
		/* nop */			IW(0x00000000);
		/* move $ra, $s2 */		IW(0x0240f825);
		/* daddiu $sp, $sp, 40 */	IW(0x67bd0028);

		/* If failed: */
		/* beqz $v0, $zero, exc */	IW(0x10400000 | EXC());
		/* nop */			IW(0x00000000);
	}

	return true;
}

/* Visit a ROP_JMP instruction. */
static inline bool
jit_visit_jmp_op(
	struct jit_context *ctx)
{
	uint32_t target_lpc;

	CONSUME_IMM32(target_lpc);
	if (target_lpc >= (uint32_t)(ctx->func->bytecode_size + 1)) {
		rt_error(ctx->rt, BROKEN_BYTECODE);
		return false;
	}

	/* Patch later. */
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BAL;
	ctx->branch_patch_count++;

	ASM {
		/* Patched later. */
		/* b 0 */	IW(0x10000000);
		/* nop */	IW(0x00000000);
	}

	return true;
}

/* Visit a ROP_JMPIFTRUE instruction. */
static inline bool
jit_visit_jmpiftrue_op(
	struct jit_context *ctx)
{
	int src;
	uint32_t target_lpc;

	CONSUME_TMPVAR(src);
	CONSUME_IMM32(target_lpc);
	if (target_lpc >= (uint32_t)(ctx->func->bytecode_size + 1)) {
		rt_error(ctx->rt, BROKEN_BYTECODE);
		return false;
	}

	src *= (int)sizeof(struct rt_value);

	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* $at = rt->frame->tmpvar[src].val.i */
		/* li    $t0, src */		IW(0x240c0000 | tvar16(src));
		/* daddu $t0, $t0, $s1 */	IW(0x0191602d);
		/* lw    $at, 8($t0) */		IW(0x8d810008);
	}

	/* Patch later. */
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BNE;
	ctx->branch_patch_count++;

	ASM {
		/* Patched later. */
		/* bne $at, 0, taget */		IW(0x14200000);
		/* nop */			IW(0x00000000);
	}

	return true;
}

/* Visit a ROP_JMPIFFALSE instruction. */
static inline bool
jit_visit_jmpiffalse_op(
	struct jit_context *ctx)
{
	int src;
	uint32_t target_lpc;

	CONSUME_TMPVAR(src);
	CONSUME_IMM32(target_lpc);
	if (target_lpc >= (uint32_t)(ctx->func->bytecode_size + 1)) {
		rt_error(ctx->rt, BROKEN_BYTECODE);
		return false;
	}

	src *= (int)sizeof(struct rt_value);

	ASM {
		/* $s0: rt */
		/* $s1: &rt->frame->tmpvar[0] */

		/* $at = rt->frame->tmpvar[src].val.i */
		/* li    $t0, src */		IW(0x240c0000 | tvar16(src));
		/* daddu $t0, $t0, $s1 */	IW(0x0191602d);
		/* lw    $at, 8($t0) */		IW(0x8d810008);
	}
	
	/* Patch later. */
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BEQ;
	ctx->branch_patch_count++;

	ASM {
		/* Patched later. */
		/* beq $at, 0, taget */		IW(0x10200000);
		/* nop */			IW(0x00000000);
	}

	return true;
}

/* Visit a ROP_JMPIFEQ instruction. */
static inline bool
jit_visit_jmpifeq_op(
	struct jit_context *ctx)
{
	int src;
	uint32_t target_lpc;

	CONSUME_TMPVAR(src);
	CONSUME_IMM32(target_lpc);
	if (target_lpc >= (uint32_t)(ctx->func->bytecode_size + 1)) {
		rt_error(ctx->rt, BROKEN_BYTECODE);
		return false;
	}

	/* Patch later. */
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BEQ;
	ctx->branch_patch_count++;

	ASM {
		/* Patched later. */
		/* beq $at, 0, taget */		IW(0x10200000);
		/* nop */			IW(0x00000000);
	}

	return true;
}

/* Visit a bytecode of a function. */
bool
jit_visit_bytecode(
	struct jit_context *ctx)
{
	uint8_t opcode;

	/* Put a prologue. */
	ASM {
		/* s0: rt */
		/* s1: &rt->frame->tmpvar[0] */

		/* Push the general-purpose registers. */
		/* daddiu $sp, $sp, -64 */	IW(0x67bdffc0);
		/* sd $s0, 56($sp) */		IW(0xffb00038);
		/* sd $s1, 48($sp) */		IW(0xffb10030);
		/* sd $s2, 40($sp) */		IW(0xffb20028);
		/* sd $s3, 32($sp) */		IW(0xffb30020);
		/* sd $s4, 24($sp) */		IW(0xffb40018);
		/* sd $s5, 16($sp) */		IW(0xffb50010);
		/* sd $s6, 8($sp) */		IW(0xffb60008);
		/* sd $s7, 0($sp) */		IW(0xffb70000);

		/* s0 = rt */
		/* move $s0, $a0 */		IW(0x00808025);

		/* s1 = *rt->frame = &rt->frame->tmpvar[0] */
		/* ld $s1, 0($a0) */		IW(0xdc910000);
		/* nop */			IW(0x00000000);
		/* ld $s1, 0($s1) */		IW(0xde310000);
		/* nop */			IW(0x00000000);

		/* Skip an exception handler. */
		/* b body */			IW(0x1000000d);
		/* nop */			IW(0x00000000);
	}

	/* Put an exception handler. */
	ctx->exception_code = ctx->code;
	ASM {
	/* EXCEPTION: */
		/* ld $s7, 0($sp) */		IW(0xdfb70000);
		/* ld $s6, 8($sp) */		IW(0xdfb60008);
		/* ld $s5, 16($sp) */		IW(0xdfb50010);
		/* ld $s4, 24($sp) */		IW(0xdfb40018);
		/* ld $s3, 32($sp) */		IW(0xdfb30020);
		/* ld $s2, 40($sp) */		IW(0xdfb20028);
		/* ld $s0, 48($sp) */		IW(0xdfb00030);
		/* ld $s1, 56($sp) */		IW(0xdfb10038);
		/* daddiu $sp, $sp, 64 */	IW(0x67bd0040);
		/* li $v0, 0 */			IW(0x34020000);
		/* jr $ra */			IW(0x03e00008);
		/* nop */			IW(0x00000000);
	}

	/* Put a body. */
	while (ctx->lpc < ctx->func->bytecode_size) {
		/* Save LPC and addr. */
		if (ctx->pc_entry_count >= PC_ENTRY_MAX) {
			rt_error(ctx->rt, _("Code too big."));
			return false;
		}
		ctx->pc_entry[ctx->pc_entry_count].lpc = (uint32_t)ctx->lpc;
		ctx->pc_entry[ctx->pc_entry_count].code = ctx->code;
		ctx->pc_entry_count++;

		/* Dispatch by opcode. */
		CONSUME_OPCODE(opcode);
		switch (opcode) {
		case ROP_LINEINFO:
			if (!jit_visit_lineinfo_op(ctx))
				return false;
			break;
		case ROP_ASSIGN:
			if (!jit_visit_assign_op(ctx))
				return false;
			break;
		case ROP_ICONST:
			if (!jit_visit_iconst_op(ctx))
				return false;
			break;
		case ROP_FCONST:
			if (!jit_visit_fconst_op(ctx))
				return false;
			break;
		case ROP_SCONST:
			if (!jit_visit_sconst_op(ctx))
				return false;
			break;
		case ROP_ACONST:
			if (!jit_visit_aconst_op(ctx))
				return false;
			break;
		case ROP_DCONST:
			if (!jit_visit_dconst_op(ctx))
				return false;
			break;
		case ROP_INC:
			if (!jit_visit_inc_op(ctx))
				return false;
			break;
		case ROP_ADD:
			if (!jit_visit_add_op(ctx))
				return false;
			break;
		case ROP_SUB:
			if (!jit_visit_sub_op(ctx))
				return false;
			break;
		case ROP_MUL:
			if (!jit_visit_mul_op(ctx))
				return false;
			break;
		case ROP_DIV:
			if (!jit_visit_div_op(ctx))
				return false;
			break;
		case ROP_MOD:
			if (!jit_visit_mod_op(ctx))
				return false;
			break;
		case ROP_AND:
			if (!jit_visit_and_op(ctx))
				return false;
			break;
		case ROP_OR:
			if (!jit_visit_or_op(ctx))
				return false;
			break;
		case ROP_XOR:
			if (!jit_visit_xor_op(ctx))
				return false;
			break;
		case ROP_NEG:
			if (!jit_visit_neg_op(ctx))
				return false;
			break;
		case ROP_LT:
			if (!jit_visit_lt_op(ctx))
				return false;
			break;
		case ROP_LTE:
			if (!jit_visit_lte_op(ctx))
				return false;
			break;
		case ROP_EQ:
			if (!jit_visit_eq_op(ctx))
				return false;
			break;
		case ROP_NEQ:
			if (!jit_visit_neq_op(ctx))
				return false;
			break;
		case ROP_GTE:
			if (!jit_visit_gte_op(ctx))
				return false;
			break;
		case ROP_GT:
			if (!jit_visit_gt_op(ctx))
				return false;
			break;
		case ROP_EQI:
			if (!jit_visit_eqi_op(ctx))
				return false;
			break;
		case ROP_LOADARRAY:
			if (!jit_visit_loadarray_op(ctx))
				return false;
			break;
		case ROP_STOREARRAY:
			if (!jit_visit_storearray_op(ctx))
				return false;
			break;
		case ROP_LEN:
			if (!jit_visit_len_op(ctx))
			return false;
			break;
		case ROP_GETDICTKEYBYINDEX:
			if (!jit_visit_getdictkeybyindex_op(ctx))
			return false;
			break;
		case ROP_GETDICTVALBYINDEX:
			if (!jit_visit_getdictvalbyindex_op(ctx))
				return false;
			break;
		case ROP_LOADSYMBOL:
			if (!jit_visit_loadsymbol_op(ctx))
				return false;
			break;
		case ROP_STORESYMBOL:
			if (!jit_visit_storesymbol_op(ctx))
				return false;
			break;
		case ROP_LOADDOT:
			if (!jit_visit_loaddot_op(ctx))
				return false;
			break;
		case ROP_STOREDOT:
			if (!jit_visit_storedot_op(ctx))
				return false;
			break;
		case ROP_CALL:
			if (!jit_visit_call_op(ctx))
				return false;
			break;
		case ROP_THISCALL:
			if (!jit_visit_thiscall_op(ctx))
				return false;
			break;
		case ROP_JMP:
			if (!jit_visit_jmp_op(ctx))
				return false;
			break;
		case ROP_JMPIFTRUE:
			if (!jit_visit_jmpiftrue_op(ctx))
				return false;
			break;
		case ROP_JMPIFFALSE:
			if (!jit_visit_jmpiffalse_op(ctx))
				return false;
			break;
		case ROP_JMPIFEQ:
			if (!jit_visit_jmpifeq_op(ctx))
				return false;
			break;
		default:
			assert(JIT_OP_NOT_IMPLEMENTED);
			break;
		}
	}

	/* Add the tail PC to the table. */
	ctx->pc_entry[ctx->pc_entry_count].lpc = (uint32_t)ctx->lpc;
	ctx->pc_entry[ctx->pc_entry_count].code = ctx->code;
	ctx->pc_entry_count++;

	/* Put an epilogue. */
	ASM {
	/* EPILOGUE: */
		/* ld $s7, 0($sp) */		IW(0xdfb70000);
		/* ld $s6, 8($sp) */		IW(0xdfb60008);
		/* ld $s5, 16($sp) */		IW(0xdfb50010);
		/* ld $s4, 24($sp) */		IW(0xdfb40018);
		/* ld $s3, 32($sp) */		IW(0xdfb30020);
		/* ld $s2, 40($sp) */		IW(0xdfb20028);
		/* ld $s1, 48($sp) */		IW(0xdfb10030);
		/* ld $s0, 56($sp) */		IW(0xdfb00038);
		/* daddiu $sp, $sp, 64 */	IW(0x67bd0040);
		/* li $v0, 1 */			IW(0x34020001);
		/* jr $ra */			IW(0x03e00008);
		/* nop */			IW(0x00000000);
	}

	return true;
}

static bool
jit_patch_branch(
    struct jit_context *ctx,
    int patch_index)
{
	uint32_t *target_code;
	int offset;
	int i;

	if (ctx->pc_entry_count == 0)
		return true;

	/* Search a code addr at lpc. */
	target_code = NULL;
	for (i = 0; i < ctx->pc_entry_count; i++) {
		if (ctx->pc_entry[i].lpc == ctx->branch_patch[patch_index].lpc) {
			target_code = ctx->pc_entry[i].code;
			break;
		}
			
	}
	if (target_code == NULL) {
		rt_error(ctx->rt, _("Branch target not found."));
		return false;
	}

	/* Calc a branch offset. */
	offset = (int)((intptr_t)target_code - (intptr_t)ctx->branch_patch[patch_index].code - 4) / 4;

	/* Set the assembler cursor. */
	ctx->code = ctx->branch_patch[patch_index].code;

	/* Assemble. */
	if (ctx->branch_patch[patch_index].type == PATCH_BAL) {
		ASM {
			/* b offset */		IW(0x10000000 | lo16((uint32_t)offset));
		}
	} else if (ctx->branch_patch[patch_index].type == PATCH_BEQ) {
		ASM {
			/* beq offset */	IW(0x10200000 | lo16((uint32_t)offset));
		}
	} else if (ctx->branch_patch[patch_index].type == PATCH_BNE) {
		ASM {
			/* bne offset */	IW(0x14200000 | lo16((uint32_t)offset));
		}
	}

	return true;
}

#endif /* defined(ARCH_PPC32) && defined(USE_JIT) */
