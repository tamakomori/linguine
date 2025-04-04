/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Linguine
 * Copyright (c) 2025, Tamako Mori. All rights reserved.
 */

/*
 * JIT (ppc32): Just-In-Time native code generation
 */

#include "linguine/compat.h"		/* ARCH_PPC32 */

#if defined(ARCH_PPC32) && defined(USE_JIT)

#include "linguine/runtime.h"
#include "jit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* False asseretion */
#define JIT_OP_NOT_IMPLEMENTED	0
#define NEVER_COME_HERE		0

/* PC entry size. */
#define PC_ENTRY_MAX		2048

/* Branch pathch size. */
#define BRANCH_PATCH_MAX	2048

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
#define REG_R0		0	/* volatile */
#define REG_R1		1	/* stack pointer */
#define REG_R2		2	/* (TOC pointer) */
#define REG_R3		3	/* volatile, parameter, return */
#define REG_R4		4	/* volatile, parameter */
#define REG_R5		5	/* volatile, parameter */
#define REG_R6		6	/* volatile, parameter */
#define REG_R7		7	/* volatile, parameter */
#define REG_R8		8	/* volatile, parameter */
#define REG_R9		9	/* volatile, parameter */
#define REG_R10		10	/* volatile, parameter */
#define REG_R11		11	/* (volatile, environment pointer) */
#define REG_R12		12	/* (exception handling, glink) */
#define REG_R13		13	/* (thread ID) */
#define REG_R14		14	/* rt, non-volatile, local */
#define REG_R15		15	/* rt->frame->tmpvar[0], non-volatile, local */
#define REG_R16		16	/* exception_handler, non-volatile, local */
#define REG_R17		17	/* (non-volatile, local) */
#define REG_R18		18	/* (non-volatile, local) */
#define REG_R19		19	/* (non-volatile, local) */
#define REG_R20		20	/* (non-volatile, local) */
#define REG_R21		21	/* (non-volatile, local) */
#define REG_R22		22	/* (non-volatile, local) */
#define REG_R23		23	/* (non-volatile, local) */
#define REG_R24		24	/* (non-volatile, local) */
#define REG_R25		25	/* (non-volatile, local) */
#define REG_R26		26	/* (non-volatile, local) */
#define REG_R27		27	/* (non-volatile, local) */
#define REG_R28		28	/* (non-volatile, local) */
#define REG_R29		29	/* (non-volatile, local) */
#define REG_R30		30	/* (non-volatile, local) */
#define REG_R31		31	/* (non-volatile, local) */

/* Put a instruction word. */
#define IW(w)				if (!jit_put_word(ctx, w)) return false
static INLINE bool
jit_put_word(
	struct jit_context *ctx,
	uint32_t word)
{
	uint32_t tmp;

	if ((uint32_t *)ctx->code >= (uint32_t *)ctx->code_end) {
		rt_error(ctx->rt, "Code too big.");
		return false;
	}

	tmp = ((word & 0xff) << 24) |
	      (((word >> 8) & 0xff) << 16) |
	      (((word >> 16) & 0xff) << 8) |
	      ((word >> 24) & 0xff);

	*(uint32_t *)ctx->code = tmp;
	ctx->code = (uint32_t *)ctx->code + 1;

	return true;
}

/*
 * Templates
 */

static INLINE uint32_t hi16(uint32_t d)
{
	uint32_t b2 = (d >> 16) & 0xff;
	uint32_t b3 = (d >> 24) & 0xff;
	return (b2 << 24) | (b3 << 16);
}

static INLINE uint32_t lo16(uint32_t d)
{
	uint32_t b0 = d & 0xff;
	uint32_t b1 = (d >> 8) & 0xff;
	return (b0 << 24) | (b1 << 16);
}

static INLINE uint32_t tvar16(int d)
{
	uint32_t b0 = d & 0xff;
	uint32_t b1 = (d >> 8) & 0xff;
	return (b0 << 24) | (b1 << 16);
}

#define EXC()	exc((uint32_t)ctx->exception_code, (uint32_t)ctx->code)
static INLINE uint32_t exc(uint32_t handler, uint32_t cur)
{
	uint32_t tmp = handler - cur;
	uint32_t b0 = tmp & 0xff;
	uint32_t b1 = (tmp >> 8) & 0xff;
	return (b0 << 24) | (b1 << 16);
}

#define ASM_BINARY_OP(f)												\
	ASM { 														\
		/* Arg1 R3: rt */											\
		/* mr r3, r14 */		IW(0x7873c37d);								\
															\
		/* Arg2 R4: dst */											\
		/* li r4, dst */		IW(0x00008038 | tvar16(dst)); 						\
															\
		/* Arg3 R5: src1 */											\
		/* li r5, src1 */		IW(0x0000a038 | tvar16(src1)); 						\
															\
		/* Arg4 R6: src2 */											\
		/* li r6, src2 */		IW(0x0000c038 | tvar16(src2)); 						\
															\
		/* Call f(). */												\
		/* lis r12, f[31:16] */		IW(0x0000803d | hi16((uint32_t)f));					\
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | lo16((uint32_t)f));					\
		/* mflr r31 */			IW(0xa602e87f);								\
		/* mtctr r12 */			IW(0xa603897d);								\
		/* bctrl */ 			IW(0x2104804e);								\
		/* mtlr r31 */			IW(0xa603e87f);								\
															\
		/* If failed: */											\
		/* cmpwi r3, 0 */		IW(0x0000032c);								\
		/* beq exception_handler */	IW(0x00008241 | EXC());							\
	}

#define ASM_UNARY_OP(f)													\
	ASM { 														\
		/* Arg1 R3: rt */											\
		/* mr r3, r14 */		IW(0x7873c37d);								\
															\
		/* Arg2 R4: dst */											\
		/* li r4, dst */		IW(0x00008038 | tvar16(dst)); 						\
															\
		/* Arg3 R5: src */											\
		/* li r5, src */		IW(0x0000a038 | tvar16(src)); 						\
															\
		/* Call f(). */												\
		/* lis r12, f[31:16] */		IW(0x0000803d | hi16((uint32_t)f));					\
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | lo16((uint32_t)f));					\
		/* mflr r31 */			IW(0xa602e87f);								\
		/* mtctr r12 */			IW(0xa603897d);								\
		/* bctrl */ 			IW(0x2104804e);								\
		/* mtlr r31 */			IW(0xa603e87f);								\
															\
		/* If failed: */											\
		/* cmpwi r3, 0 */		IW(0x0000032c);								\
		/* beq exception_handler */	IW(0x00008241 | EXC());							\
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
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* rt->line = line; */
		/* li r0, line */	IW(0x00000038 | lo16(line));
		/* stw r0, 4(r14) */	IW(0x04000e90);
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
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* R3 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li r3, dst */	IW(0x00006038 | lo16((uint32_t)dst));
		/* add r3, r3, r15 */	IW(0x147a637c);

		/* R4 = src_addr = &rt->frame->tmpvar[src] */
		/* li r4, src */	IW(0x00008038 | lo16((uint32_t)src));
		/* add r4, r4, r15 */	IW(0x147a847c);

		/* *dst_addr = *src_addr */
		/* lwz r5, 0(r4) */	IW(0x0000a480);
		/* lwz r6, 4(r4) */	IW(0x0400c480);
		/* stw r5, 0(r3) */	IW(0x0000a390);
		/* stw r6, 4(r3) */	IW(0x0400c390);
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
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* R3 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li r3, dst */	IW(0x00006038 | tvar16(dst));
		/* add r3, r3, r15 */	IW(0x147a637c);

		/* rt->frame->tmpvar[dst].type = RT_VALUE_INT */
		/* li r4, 0 */		IW(0x00008038);
		/* stw r4, 0(r3) */	IW(0x00008390);

		/* rt->frame->tmpvar[dst].val.i = val */
		/* lis r4, val@h */	IW(0x0000803c | hi16(val));
		/* ori r4, r4, val@l */	IW(0x00008460 | lo16(val));
		/* stw r4, 4(r3) */	IW(0x04008390);
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
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* R3 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li r3, dst */	IW(0x00006038 | lo16((uint32_t)dst));
		/* add r3, r3, r15 */	IW(0x147a637c);

		/* rt->frame->tmpvar[dst].type = RT_VALUE_FLOAT */
		/* li r4, 1 */		IW(0x01008038);
		/* stw r4, 0(r3) */	IW(0x00008390);

		/* rt->frame->tmpvar[dst].val.i = val */
		/* lis r4, val@h */		IW(0x0000803c | hi16(val));
		/* ori r4, r4, val@l */		IW(0x00008460 | lo16(val));
		/* stw r4, 4(r3) */		IW(0x04008390);
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
	uint32_t f;

	CONSUME_TMPVAR(dst);
	CONSUME_STRING(val);

	f = (uint32_t)rt_make_string;
	dst *= (int)sizeof(struct rt_value);

	/* rt_make_string(rt, &rt->frame->tmpvar[dst], val); */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3: rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li r4, dst */		IW(0x00008038 | lo16((uint32_t)dst));
		/* add r4, r4, r15 */		IW(0x147a847c);

		/* Arg3: R5 = val */
		/* lis  r5, val[31:16] */	IW(0x0000a03c | hi16((uint32_t)val));
		/* ori  r5, r5, val[15:0] */	IW(0x0000a560 | lo16((uint32_t)val));

		/* Call rt_make_string(). */
		/* lis  r12, f[31:16] */	IW(0x0000803d | hi16(f));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | lo16(f));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | EXC());
	}

	return true;
}

/* Visit a ROP_ACONST instruction. */
static INLINE bool
jit_visit_aconst_op(
	struct jit_context *ctx)
{
	int dst;
	uint32_t f;

	CONSUME_TMPVAR(dst);

	f = (uint32_t)rt_make_empty_array;
	dst *= (int)sizeof(struct rt_value);

	/* rt_make_empty_array(rt, &rt->frame->tmpvar[dst]); */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3: rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li r4, dst */		IW(0x00008038 | lo16((uint32_t)dst));
		/* add r4, r4, r15 */		IW(0x147a847c);

		/* Call rt_make_empty_array(). */
		/* lis  r12, f[31:16] */	IW(0x0000803d | hi16(f));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | lo16(f));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | EXC());
	}

	return true;
}

/* Visit a ROP_DCONST instruction. */
static INLINE bool
jit_visit_dconst_op(
	struct jit_context *ctx)
{
	int dst;
	uint32_t f;

	CONSUME_TMPVAR(dst);

	f = (uint32_t)rt_make_empty_dict;
	dst *= (int)sizeof(struct rt_value);

	/* rt_make_empty_dict(rt, &rt->frame->tmpvar[dst]); */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3: rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li r4, dst */		IW(0x00008038 | lo16((uint32_t)dst));
		/* add r4, r4, r15 */		IW(0x147a847c);

		/* Call rt_make_empty_dict(). */
		/* lis  r12, f[31:16] */	IW(0x0000803d | hi16(f));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | lo16(f));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | EXC());
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
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* R3 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li r3, dst */	IW(0x00006038 | lo16((uint32_t)dst));
		/* add r3, r3, r15 */	IW(0x147a637c);

		/* rt->frame->tmpvar[dst].val.i++ */
		/* lwz r4, 4(r3) */	IW(0x04008380);
		/* addi r4, r4, 1 */	IW(0x01008438);
		/* stw r4, 4(r3) */	IW(0x04008390);
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
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* R3 = src1_addr = &rt->frame->tmpvar[src1] */
		/* li r3, src */	IW(0x00006038 | lo16((uint32_t)src1));
		/* add r3, r3, r15 */	IW(0x147a637c);
		/* lwz r3, 4(r3) */	IW(0x04006380);

		/* R4 = src2_addr = &rt->frame->tmpvar[src2] */
		/* li r4, src2 */	IW(0x00008038 | lo16((uint32_t)src2));
		/* add r4, r4, r15 */	IW(0x147a847c);
		/* lwz r4, 4(r4) */	IW(0x04008480);

		/* src1 == src2 */
		/* cmpw r3, r4 */	IW(0x0020037c);
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
	uint32_t src;
	uint32_t f;

	CONSUME_TMPVAR(dst);
	CONSUME_STRING(src_s);

	src = (uint32_t)(intptr_t)src_s;
	f = (uint32_t)rt_loadsymbol_helper;

	/* if (!jit_loadsymbol_helper(rt, dst, src)) return false; */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3 = rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dst */
		/* li r4, dst */		IW(0x00008038 | tvar16(dst));

		/* Arg3 R5 = src */
		/* lis  r5, src[31:16] */	IW(0x0000a03c | hi16(src));
		/* ori  r5, r5, src[15:0] */	IW(0x0000a560 | lo16(src));

		/* Call rt_loadsymbol_helper(). */
		/* lis  r12, f[31:16] */	IW(0x0000803d | hi16(f));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | lo16(f));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | EXC());
	}

	return true;
}

/* Visit a ROP_STORESYMBOL instruction. */
static INLINE bool
jit_visit_storesymbol_op(
	struct jit_context *ctx)
{
	const char *dst_s;
	uint32_t dst;
	int src;
	uint32_t f;

	CONSUME_STRING(dst_s);
	CONSUME_TMPVAR(src);

	dst = (uint32_t)(intptr_t)dst_s;
	f = (uint32_t)rt_storesymbol_helper;

	/* if (!rt_storesymbol_helper(rt, dst, src)) return false; */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3 = rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2: R4 = dst */
		/* lis  r4, dst[31:16] */	IW(0x0000803c | hi16(dst));
		/* ori  r4, r4, dst[15:0] */	IW(0x00008460 | lo16(dst));

		/* Arg3 R5 = src */
		/* li r5, src */		IW(0x0000a038 | tvar16(src));

		/* Call rt_storesymbol_helper(). */
		/* lis  r12, f[31:16] */	IW(0x0000803d | hi16(f));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | lo16(f));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | (uint32_t)((((uint32_t)ctx->exception_code - (uint32_t)ctx->code) & 0xff) << 24) | (uint32_t)(((((uint32_t)ctx->exception_code - (uint32_t)ctx->code) >> 8) & 0xff) << 16));
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
	uint32_t field;
	uint32_t f;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(dict);
	CONSUME_STRING(field_s);

	field = (uint32_t)(intptr_t)field_s;
	f = (uint32_t)rt_loaddot_helper;

	/* if (!rt_loaddot_helper(rt, dst, dict, field)) return false; */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3 = rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dst */
		/* li r4, dst */		IW(0x00008038 | tvar16(dst));

		/* Arg3 R5 = dict */
		/* li r5, dict */		IW(0x0000a038 | tvar16(dict));

		/* Arg4 R6 = field */
		/* lis  r6, r6, field[31:16] */	IW(0x0000c03c | hi16(field));
		/* ori  r6, r6, field[15:0] */	IW(0x0000c660 | lo16(field));

		/* Call rt_loaddot_helper(). */
		/* lis  r12, f[31:16] */	IW(0x0000803d | hi16(f));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | lo16(f));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | (uint32_t)((((uint32_t)ctx->exception_code - (uint32_t)ctx->code) & 0xff) << 24) | (uint32_t)(((((uint32_t)ctx->exception_code - (uint32_t)ctx->code) >> 8) & 0xff) << 16));
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
	uint32_t field;
	int src;
	uint32_t f;

	CONSUME_TMPVAR(dict);
	CONSUME_STRING(field_s);
	CONSUME_TMPVAR(src);

	field = (uint32_t)(intptr_t)field_s;
	f = (uint32_t)rt_storedot_helper;

	/* if (!jit_storedot_helper(rt, dst, dict, field)) return false; */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3 = rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dict */
		/* li r4, dict */		IW(0x00008038 | tvar16(dict));

		/* Arg3 R5 = field */
		/* lis  r5, field[31:16] */	IW(0x0000a03c | hi16(field));
		/* ori  r5, r5, field[15:0] */	IW(0x0000a560 | lo16(field));

		/* Arg4 R6: src */
		/* li r6, src */		IW(0x0000c038 | tvar16(src));

		/* Call rt_storedot_helper(). */
		/* lis  r12, f[31:16] */	IW(0x0000803d | hi16(f));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | lo16(f));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | EXC());
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
	uint32_t arg_addr;
	int i;
	uint32_t f;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(func);
	CONSUME_IMM8(arg_count);
	for (i = 0; i < arg_count; i++) {
		CONSUME_TMPVAR(arg_tmp);
		arg[i] = arg_tmp;
	}

	/* Embed arguments to the code. */
	if (arg_count > 0) {
		tmp = (uint32_t)(4 + 4 * arg_count);
		ASM {
			/* b tmp */
			IW(0x00000048 | lo16(tmp));
		}
		arg_addr = (uint32_t)(intptr_t)ctx->code;
		for (i = 0; i < arg_count; i++) {
			*(uint32_t *)ctx->code = (uint32_t)arg[i];
			ctx->code = (uint32_t *)ctx->code + 1;
		}
	} else {
		arg_addr = 0;
	}

	f = (uint32_t)rt_call_helper;

	/* if (!rt_call_helper(rt, dst, func, arg_count, arg)) return false; */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3 = rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dst */
		/* li r4, dst */		IW(0x00008038 | tvar16(dst));

		/* Arg3 R5 = func */
		/* li r5, func */		IW(0x0000a038 | tvar16(func));

		/* Arg4 R6: arg_count */
		/* li r6, arg_count */		IW(0x0000c038 | lo16((uint32_t)arg_count));

		/* Arg5 R7 = arg */
		/* lis  r7, arg[31:16] */	IW(0x0000e03c | hi16(arg_addr));
		/* ori  r7, r7, arg[15:0] */	IW(0x0000e760 | lo16(arg_addr));

		/* Call rt_call_helper(). */
		/* lis  r12, f[31:16] */	IW(0x0000803d | hi16(f));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | lo16(f));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | EXC());
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
	uint32_t arg_addr;
	int i;
	uint32_t f;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(obj);
	CONSUME_STRING(symbol);
	CONSUME_IMM8(arg_count);
	for (i = 0; i < arg_count; i++) {
		CONSUME_TMPVAR(arg_tmp);
		arg[i] = arg_tmp;
	}

	/* Embed arguments. */
	if (arg_count > 0) {
		tmp = (uint32_t)(4 + 4 * arg_count);
		ASM {
			/* b tmp */
			IW(0x00000048 | lo16(tmp));
		}
		arg_addr = (uint32_t)(intptr_t)ctx->code;
		for (i = 0; i < arg_count; i++) {
			*(uint32_t *)ctx->code = (uint32_t)arg[i];
			ctx->code = (uint32_t *)ctx->code + 1;
		}
	} else {
		arg_addr = 0;
	}

	f = (uint32_t)rt_thiscall_helper;

	/* if (!rt_thiscall_helper(rt, dst, obj, symbol, arg_count, arg)) return false; */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3 = rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dst */
		/* li r4, dst */		IW(0x00008038 | tvar16(dst));

		/* Arg3 R5 = obj */
		/* li r5, obj */		IW(0x0000a038 | tvar16(obj));

		/* Arg4 R6 = symbol */
		/* lis  r6, symbol[31:16] */	IW(0x0000c03c | hi16((uint32_t)symbol));
		/* ori  r6, r6, symbol[15:0] */	IW(0x0000a560 | lo16((uint32_t)symbol));

		/* Arg5 R7 = arg_count */
		/* li r7, arg_count */		IW(0x0000e038 | lo16((uint32_t)arg_count));

		/* Arg6 R8: arg */
		/* lis  r8, arg[31:16] */	IW(0x0000003d | hi16(arg_addr));
		/* ori  r8, r8, arg[15:0] */	IW(0x00000861 | lo16(arg_addr));

		/* Call rt_thiscall_helper(). */
		/* lis  r12, f[31:16] */	IW(0x0000803d | hi16(f));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | lo16(f));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | EXC());
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
		/* b 0 */	IW(0x00000048);
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
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* R3 = rt->frame->tmpvar[src].val.i */
		/* li r3, src */		IW(0x00006038 | lo16((uint32_t)src));
		/* add r3, r3, r15 */		IW(0x147a637c);
		/* lwz r3, 4(r3) */		IW(0x04006380);

		/* Compare: rt->frame->tmpvar[dst].val.i == 1 */
		/* cmpwi r3, 0 */		IW(0x0000032c);
	}

	/* Patch later. */
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BNE;
	ctx->branch_patch_count++;

	ASM {
		/* Patched later. */
		/* bne 0 */	IW(0x00008240);
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
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* R3 = rt->frame->tmpvar[src].val.i */
		/* li r3, src */		IW(0x00006038 | lo16((uint32_t)src));
		/* add r3, r3, r15 */		IW(0x147a637c);
		/* lwz r3, 4(r3) */		IW(0x04006380);

		/* Compare: rt->frame->tmpvar[dst].val.i == 1 */
		/* cmpwi r3, 0 */		IW(0x0000032c);
	}
	
	/* Patch later. */
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BEQ;
	ctx->branch_patch_count++;

	ASM {
		/* Patched later. */
		/* beq 0 */	IW(0x00008241);
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
		/* beq 0 */	IW(0x00008241);
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
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Push the general-purpose registers. */
		/* stw r14, -8(r1) */		IW(0xf8ffc191);
		/* stw r15, -16(r1) */		IW(0xf0ffe191);
		/* stw r31, -24(r1) */		IW(0xe8ffe193);
		/* addi r1, r1, -64 */		IW(0xc0ff2138);

		/* R14 = rt */
		/* mr r14, r3 */		IW(0x781b6e7c);

		/* R15 = *rt->frame = &rt->frame->tmpvar[0] */
		/* lwz r15, 0(r14) */		IW(0x0000ee81);
		/* lwz r15, 0(r15) */		IW(0x0000ef81);

		/* Skip an exception handler. */
		/* b body */			IW(0x1c000048);
	}

	/* Put an exception handler. */
	ctx->exception_code = ctx->code;
	ASM {
	/* EXCEPTION: */
		/* addi r1, r1, 64 */		IW(0x40002138);
		/* lwz r31, -24(r1) */		IW(0xe8ffe183);
		/* lwz r15, -16(r1) */		IW(0xf0ffe181);
		/* lwz r14, -8(r1) */		IW(0xf8ffc181);
		/* li r3, 0 */			IW(0x00006038);
		/* blr */			IW(0x2000804e);
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
		/* addi r1, r1, 64 */		IW(0x40002138);
		/* lwz r31, -24(r1) */		IW(0xe8ffe183);
		/* lwz r15, -16(r1) */		IW(0xf0ffe181);
		/* lwz r14, -8(r1) */		IW(0xf8ffc181);
		/* li r3, 1 */			IW(0x01006038);
		/* blr */			IW(0x2000804e);
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
	offset = (int)((intptr_t)target_code - (intptr_t)ctx->branch_patch[patch_index].code);

	/* Set the assembler cursor. */
	ctx->code = ctx->branch_patch[patch_index].code;

	/* Assemble. */
	if (ctx->branch_patch[patch_index].type == PATCH_BAL) {
		ASM {
			/* b offset */
			IW(0x00000048 |
			   (((uint32_t)offset & 0xff) << 24) |
			   ((((uint32_t)offset >> 8) & 0xff) << 16) |
			   ((((uint32_t)offset >> 16) & 0xff) << 8) |
			   (((uint32_t)offset >> 24) & 0x03));
		}
	} else if (ctx->branch_patch[patch_index].type == PATCH_BEQ) {
		ASM {
			/* beq offset */
			IW(0x00008241 |
			   (((uint32_t)offset & 0xff) << 24) |
			   ((((uint32_t)offset >> 8) & 0xff) << 16));
		}
	} else if (ctx->branch_patch[patch_index].type == PATCH_BNE) {
		ASM {
			/* bne offset */
			IW(0x00008240 |
			   (((uint32_t)offset & 0xff) << 24) |
			   ((((uint32_t)offset >> 8) & 0xff) << 16));
		}
	}

	return true;
}

#endif /* defined(ARCH_PPC32) && defined(USE_JIT) */
