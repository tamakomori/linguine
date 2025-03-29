/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Linguine
 * Copyright (c) 2025, Tamako Mori. All rights reserved.
 */

/*
 * JIT (ppc64): Just-In-Time native code generation
 */

#include "linguine/compat.h"		/* ARCH_PPC64 */

#if defined(ARCH_PPC64) && defined(USE_JIT)

#include "linguine/runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/mman.h>		/* mmap(), mprotect(), munmap() */

/* False asseretion */
#define JIT_OP_NOT_IMPLEMENTED	0
#define NEVER_COME_HERE		0

/* Error message */
#define BROKEN_BYTECODE		_("Broken bytecode.")

/* Code size. */
#define CODE_MAX		16 * 1024 * 1024

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

/* JIT codegen context */
struct jit_context {
	struct rt_env *rt;
	struct rt_func *func;

	/* Mappd code area top. */
	uint32_t *code_top;

	/* Mapped code area end. */
	uint32_t *code_end;

	/* Current code position. */
	uint32_t *code;

	/* Exception handler. */
	uint32_t *exception_code;

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

/* Forward declaration */
static bool jit_map_memory_region(void);
static void jit_map_writable(void);
static void jit_map_executable(void);
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
		if (!jit_map_memory_region()) {
			rt_error(rt, _("Memory mapping failed."));
			return false;
		}
	}

	/* Make a context. */
	memset(&ctx, 0, sizeof(struct jit_context));
	ctx.code_top = jit_code_region_cur;
	ctx.code_end = jit_code_region_tail;
	ctx.code = ctx.code_top;
	ctx.rt = rt;
	ctx.func = func;

	/* Make code writable and non-executable. */
	jit_map_writable();

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
	jit_map_executable();

	func->jit_code = (bool (*)(struct rt_env *))ctx.code_top;

	return true;
}

/* Map a memory region for the generated code. */
static bool
jit_map_memory_region(
	void)
{
	jit_code_region = mmap(NULL, CODE_MAX, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (jit_code_region == NULL)
		return false;

	jit_code_region_cur = jit_code_region;
	jit_code_region_tail = jit_code_region + CODE_MAX / 4;

	memset(jit_code_region, 0, CODE_MAX);

	return true;
}

/* Make the region writable and non-executable. */
static void
jit_map_writable(
	void)
{
	mprotect(jit_code_region, CODE_MAX, PROT_READ | PROT_WRITE);
}

/* Make the region executable and non-writable. */
static void
jit_map_executable(
	void)
{
	mprotect(jit_code_region, CODE_MAX, PROT_EXEC | PROT_READ);
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

	if (ctx->code >= ctx->code_end) {
		rt_error(ctx->rt, "Code too big.");
		return false;
	}

#ifdef ARCH_LE
	tmp = ((word & 0xff) << 24) |
	      (((word >> 8) & 0xff) << 16) |
	      (((word >> 16) & 0xff) << 8) |
	      ((word >> 24) & 0xff);
#else
	tmp = word;
#endif

	*ctx->code++ = tmp;

	return true;
}

/*
 * Bytecode getter
 */

/* Check an opcode. */
#define CONSUME_OPCODE(d)		if (!jit_get_opcode(ctx, &d)) return false
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

/* Get a imm32 operand. */
#define CONSUME_IMM32(d)		if (!jit_get_opr_imm32(ctx, &d)) return false
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

/* Get an imm16 operand that represents tmpvar index. */
#define CONSUME_TMPVAR(d)		if (!jit_get_opr_tmpvar(ctx, &d)) return false
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

/* Check an opcode. */
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

/* Get a string operand. */
#define CONSUME_STRING(d)		if (!jit_get_opr_string(ctx, &d)) return false
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

/*
 * Templates
 */

#define ASM_BINARY_OP(f)												\
	ASM { 														\
		/* Arg1 R3: rt */											\
		/* mr r3, r14 */		IW(0x7873c37d);								\
															\
		/* Arg2 R4: dst */											\
		/* li r4, dst */		IW(0x00008038 | (((uint32_t)dst & 0xff) << 24) | ((((uint32_t)dst >> 8) & 0xff) << 16));	\
															\
		/* Arg3 R5: src1 */											\
		/* li r5, src1 */		IW(0x0000a038 | (((uint32_t)src1 & 0xff) << 24) | ((((uint32_t)src1 >> 8) & 0xff) << 16));	\
															\
		/* Arg4 R6: src2 */											\
		/* li r6, src2 */		IW(0x0000c038 | (((uint32_t)src2 & 0xff) << 24) | ((((uint32_t)src2 >> 8) & 0xff) << 16));	\
															\
		/* Call f(). */												\
		/* lis  r12, f[63:48] */	IW(0x0000803d | (uint32_t)((((uint64_t)f >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 56) & 0xff) << 16));	\
		/* ori  r12, r12, f[47:32] */	IW(0x00008c61 | (uint32_t)((((uint64_t)f >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 40) & 0xff) << 16));	\
		/* sldi r12, r12, 32 */		IW(0xc6078c79);								\
		/* oris r12, r12, f[31:16] */	IW(0x00008c65 | (uint32_t)((((uint64_t)f >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 24) & 0xff) << 16));	\
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | (uint32_t)(((uint64_t)f & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 8) & 0xff) << 16));	\
		/* mflr r31 */			IW(0xa602e87f);								\
		/* mtctr r12 */			IW(0xa603897d);								\
		/* bctrl */ 			IW(0x2104804e);								\
		/* mtlr r31 */			IW(0xa603e87f);								\
															\
		/* If failed: */											\
		/* cmpwi r3, 0 */		IW(0x0000032c);								\
		/* beq exception_handler */	IW(0x00008241 | (uint32_t)((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) & 0xff) << 24)| (uint32_t)(((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) >> 8) & 0xff) << 16)); \
	}

#define ASM_UNARY_OP(f)													\
	ASM { 														\
		/* Arg1 R3: rt */											\
		/* mr r3, r14 */		IW(0x7873c37d);								\
															\
		/* Arg2 R4: dst */											\
		/* li r4, dst */		IW(0x00008038 | (((uint32_t)dst & 0xff) << 24) | ((((uint32_t)dst >> 8) & 0xff) << 16));	\
															\
		/* Arg3 R5: src */											\
		/* li r5, src */		IW(0x0000a038 | (((uint32_t)src & 0xff) << 24) | ((((uint32_t)src >> 8) & 0xff) << 16));	\
														\
		/* Call f(). */												\
		/* lis  r7, f[63:48] */		IW(0x0000e03c | (uint32_t)((((uint64_t)f >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 56) & 0xff) << 16));	\
		/* ori  r7, r7, f[47:32] */	IW(0x0000e760 | (uint32_t)((((uint64_t)f >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 40) & 0xff) << 16));	\
		/* sldi r7, r7, 32 */		IW(0xc607e778);								\
		/* oris r7, r7, f[31:16] */	IW(0x0000e764 | (uint32_t)((((uint64_t)f >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 24) & 0xff) << 16));	\
		/* ori  r7, r7, f[15:0] */	IW(0x0000e760 | (uint32_t)(((uint64_t)f & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 8) & 0xff) << 16));	\
		/* mflr r31 */			IW(0xa602e87f);								\
		/* mtctr r7 */			IW(0xa603e97c);								\
		/* bctrl */ 			IW(0x2104804e);								\
		/* mtlr r31 */			IW(0xa603e87f);								\
															\
		/* If failed: */											\
		/* cmpwi r3, 0 */		IW(0x0000032c);								\
		/* beq exception_handler */	IW(0x00008241 | (uint32_t)((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) & 0xff) << 24)| (uint32_t)(((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) >> 8) & 0xff) << 16)); \
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
		/* li r0, line */	IW(0x00000038);
		/* std r0, 8(r14) */	IW(0x08000ef8);
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

	/* rt->frame->tmpvar[dst] = rt->frame->tmpvar[src]; */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* R3 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li r3, dst */	IW(0x00006038 | (((uint32_t)dst & 0xff) << 24) | ((((uint32_t)dst >> 8) & 0xff) << 16));
		/* sldi r3, r3, 4 */	IW(0xe4266378);
		/* add r3, r3, r15 */	IW(0x147a637c);

		/* R4 = src_addr = &rt->frame->tmpvar[src] */
		/* li r4, dst */	IW(0x00008038 | (((uint32_t)dst & 0xff) << 24) | ((((uint32_t)dst >> 8) & 0xff) << 16));
		/* sldi r4, r3, 4 */	IW(0xe4268478);
		/* add r4, r4, r15 */	IW(0x147a847c);

		/* *dst_addr = *src_addr */
		/* ld r5, 0(r4) */	IW(0x0000a4e8);
		/* ld r6, 8(r4) */	IW(0x0800c4e8);
		/* std r5, 0(r3) */	IW(0x0000a3f8);
		/* std r6, 8(r3) */	IW(0x0800c3f8);
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

	/* Set an integer constant. */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* R3 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li r3, dst */	IW(0x00006038 | (((uint32_t)dst & 0xff) << 24) | ((((uint32_t)dst >> 8) & 0xff) << 16));
		/* sldi r3, r3, 4 */	IW(0xe4266378);
		/* add r3, r3, r15 */	IW(0x147a637c);

		/* rt->frame->tmpvar[dst].type = RT_VALUE_INT */
		/* li r4, 0 */		IW(0x00008038);
		/* stw r4, 0(r3) */	IW(0x00008390);

		/* rt->frame->tmpvar[dst].val.i = val */
		/* lis r4, val@h */		IW(0x0000803c | ((((uint32_t)val >> 16) & 0xff) << 24) | ((((uint32_t)val >> 24) & 0xff) << 16));
		/* ori r4, r4, val@l */		IW(0x00008460 | (((uint32_t)val & 0xff) << 24) | ((((uint32_t)val >> 8) & 0xff) << 16));
		/* std r4, 8(r3) */		IW(0x080083f8);
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

	/* Set a floating-point constant. */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* R3 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li r3, dst */	IW(0x00006038 | (((uint32_t)dst & 0xff) << 24) | ((((uint32_t)dst >> 8) & 0xff) << 16));
		/* sldi r3, r3, 4 */	IW(0xe4266378);
		/* add r3, r3, r15 */	IW(0x147a637c);

		/* rt->frame->tmpvar[dst].type = RT_VALUE_FLOAT */
		/* li r4, 1 */		IW(0x01008038);
		/* stw r4, 0(r3) */	IW(0x00008390);

		/* rt->frame->tmpvar[dst].val.i = val */
		/* lis r4, val@h */		IW(0x0000803c | ((((uint32_t)val >> 16) & 0xff) << 24) | ((((uint32_t)val >> 24) & 0xff) << 16));
		/* ori r4, r4, val@l */		IW(0x00008460 | (((uint32_t)val & 0xff) << 24) | ((((uint32_t)val >> 8) & 0xff) << 16));
		/* std r4, 8(r3) */		IW(0x080083f8);
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

	/* rt_make_string(rt, &rt->frame->tmpvar[dst], val); */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3: rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li r4, dst */		IW(0x00008038 | (((uint32_t)dst & 0xff) << 24) | ((((uint32_t)dst >> 8) & 0xff) << 16));
		/* sldi r4, r4, 4 */		IW(0xe4268478);
		/* add r4, r4, r15 */		IW(0x147a847c);

		/* Arg3: R5 = val */
		/* lis  r5, val[63:48] */	IW(0x0000a03c | (uint32_t)((((uint64_t)val >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)val >> 56) & 0xff) << 16));
		/* ori  r5, r5, val[47:32] */	IW(0x0000a560 | (uint32_t)((((uint64_t)val >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)val >> 40) & 0xff) << 16));
		/* sldi r5, r5, 32 */		IW(0xc607a578);
		/* oris r5, r5, val[31:16] */	IW(0x0000a564 | (uint32_t)((((uint64_t)val >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)val >> 24) & 0xff) << 16));
		/* ori  r5, r5, val[15:0] */	IW(0x0000a560 | (uint32_t)(((uint64_t)val & 0xff) << 24) | (uint32_t)((((uint64_t)val >> 8) & 0xff) << 16));

		/* Call rt_make_string(). */
		/* lis  r12, f[63:48] */	IW(0x0000803d | (uint32_t)((((uint64_t)f >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 56) & 0xff) << 16));
		/* ori  r12, r12, f[47:32] */	IW(0x00008c61 | (uint32_t)((((uint64_t)f >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 40) & 0xff) << 16));
		/* sldi r12, r12, 32 */		IW(0xc6078c79);
		/* oris r12, r12, f[31:16] */	IW(0x00008c65 | (uint32_t)((((uint64_t)f >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 24) & 0xff) << 16));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | (uint32_t)(((uint64_t)f & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 8) & 0xff) << 16));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | ((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) & 0xff) << 24)| (((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) >> 8) & 0xff) << 16));
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

	/* rt_make_empty_array(rt, &rt->frame->tmpvar[dst]); */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3: rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li r4, dst */		IW(0x00008038 | (((uint32_t)dst & 0xff) << 24) | ((((uint32_t)dst >> 8) & 0xff) << 16));
		/* sldi r4, r4, 4 */		IW(0xe4268478);
		/* add r4, r4, r15 */		IW(0x147a847c);

		/* Call rt_make_empty_array(). */
		/* lis  r12, f[63:48] */	IW(0x0000803d | (uint32_t)((((uint64_t)f >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 56) & 0xff) << 16));
		/* ori  r12, r12, f[47:32] */	IW(0x00008c61 | (uint32_t)((((uint64_t)f >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 40) & 0xff) << 16));
		/* sldi r12, r12, 32 */		IW(0xc6078c79);
		/* oris r12, r12, f[31:16] */	IW(0x00008c65 | (uint32_t)((((uint64_t)f >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 24) & 0xff) << 16));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | (uint32_t)(((uint64_t)f & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 8) & 0xff) << 16));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | (uint32_t)((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) & 0xff) << 24)| (uint32_t)(((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) >> 8) & 0xff) << 16));
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

	/* rt_make_empty_dict(rt, &rt->frame->tmpvar[dst]); */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3: rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li r4, dst */		IW(0x00008038 | (((uint32_t)dst & 0xff) << 24) | ((((uint32_t)dst >> 8) & 0xff) << 16));
		/* sldi r4, r4, 4 */		IW(0xe4268478);
		/* add r4, r4, r15 */		IW(0x147a847c);

		/* Call rt_make_empty_dict(). */
		/* lis  r12, f[63:48] */	IW(0x0000803d | (uint32_t)((((uint64_t)f >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 56) & 0xff) << 16));
		/* ori  r12, r12, f[47:32] */	IW(0x00008c61 | (uint32_t)((((uint64_t)f >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 40) & 0xff) << 16));
		/* sldi r12, r12, 32 */		IW(0xc6078c79);
		/* oris r12, r12, f[31:16] */	IW(0x00008c65 | (uint32_t)((((uint64_t)f >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 24) & 0xff) << 16));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | (uint32_t)(((uint64_t)f & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 8) & 0xff) << 16));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | ((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) & 0xff) << 24)| (((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) >> 8) & 0xff) << 16));
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

	/* Increment an integer. */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* R3 = dst_addr = &rt->frame->tmpvar[dst] */
		/* li r3, dst */	IW(0x00006038 | (((uint32_t)dst & 0xff) << 24) | ((((uint32_t)dst >> 8) & 0xff) << 16));
		/* sldi r3, r3, 4 */	IW(0xe4266378);
		/* add r3, r3, r15 */	IW(0x147a637c);

		/* rt->frame->tmpvar[dst].val.i++ */
		/* ld r4, 8(r3) */	IW(0x080083e8);
		/* addi r4, r4, 1 */	IW(0x01008438);
		/* std r4, 8(r3) */	IW(0x080083f8);
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

	/* src1 == src2 */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* R3 = src1_addr = &rt->frame->tmpvar[src1] */
		/* li r3, src */	IW(0x00006038 | (((uint32_t)src1 & 0xff) << 24) | ((((uint32_t)src1 >> 8) & 0xff) << 16));
		/* sldi r3, r3, 4 */	IW(0xe4266378);
		/* add r3, r3, r15 */	IW(0x147a637c);
		/* lwz r3, 8(r3) */	IW(0x08006380);

		/* R4 = src2_addr = &rt->frame->tmpvar[src2] */
		/* li r4, src2 */	IW(0x00008038 | (((uint32_t)src2 & 0xff) << 24) | ((((uint32_t)src2 >> 8) & 0xff) << 16));
		/* sldi r4, r4, 4 */	IW(0xe4268478);
		/* add r4, r4, r15 */	IW(0x147a847c);
		/* lwz r4, 8(r4) */	IW(0x08008480);

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
	uint64_t src;
	uint64_t f;

	CONSUME_TMPVAR(dst);
	CONSUME_STRING(src_s);

	src = (uint64_t)(intptr_t)src_s;
	f = (uint64_t)rt_loadsymbol_helper;

	/* if (!jit_loadsymbol_helper(rt, dst, src)) return false; */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3 = rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dst */
		/* li r4, dst */		IW(0x00008038 | (((uint32_t)dst & 0xff) << 24) | ((((uint32_t)dst >> 8) & 0xff) << 16));

		/* Arg3 R5 = src */
		/* lis  r5, src[63:48] */	IW(0x0000a03c | (uint32_t)((((uint64_t)src >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)src >> 56) & 0xff) << 16));
		/* ori  r5, r5, src[47:32] */	IW(0x0000a560 | (uint32_t)((((uint64_t)src >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)src >> 40) & 0xff) << 16));
		/* sldi r5, r5, 32 */		IW(0xc607a578);
		/* oris r5, r5, src[31:16] */	IW(0x0000a564 | (uint32_t)((((uint64_t)src >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)src >> 24) & 0xff) << 16));
		/* ori  r5, r5, src[15:0] */	IW(0x0000a560 | (uint32_t)(((uint64_t)src & 0xff) << 24) | (uint32_t)((((uint64_t)src >> 8) & 0xff) << 16));

		/* Call rt_loadsymbol_helper(). */
		/* lis  r12, f[63:48] */	IW(0x0000803d | (uint32_t)((((uint64_t)f >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 56) & 0xff) << 16));
		/* ori  r12, r12, f[47:32] */	IW(0x00008c61 | (uint32_t)((((uint64_t)f >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 40) & 0xff) << 16));
		/* sldi r12, r12, 32 */		IW(0xc6078c79);
		/* oris r12, r12, f[31:16] */	IW(0x00008c65 | (uint32_t)((((uint64_t)f >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 24) & 0xff) << 16));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | (uint32_t)(((uint64_t)f & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 8) & 0xff) << 16));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | (uint32_t)((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) & 0xff) << 24) | (uint32_t)(((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) >> 8) & 0xff) << 16));
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
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3 = rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2: R4 = dst */
		/* lis  r4, dst[63:48] */	IW(0x0000803c | (uint32_t)((((uint64_t)dst >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)dst >> 56) & 0xff) << 16));
		/* ori  r4, r4, dst[47:32] */	IW(0x00008460 | (uint32_t)((((uint64_t)dst >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)dst >> 40) & 0xff) << 16));
		/* sldi r4, r4, 32 */		IW(0xc6078478);
		/* oris r4, r4, dst[31:16] */	IW(0x00008464 | (uint32_t)((((uint64_t)dst >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)dst >> 24) & 0xff) << 16));
		/* ori  r4, r4, dst[15:0] */	IW(0x00008460 | (uint32_t)(((uint64_t)dst & 0xff) << 24) | (uint32_t)((((uint64_t)dst >> 8) & 0xff) << 16));

		/* Arg3 R5 = src */
		/* li r5, src */		IW(0x0000a038 | (((uint32_t)src & 0xff) << 24) | ((((uint32_t)src >> 8) & 0xff) << 16));

		/* Call rt_storesymbol_helper(). */
		/* lis  r12, f[63:48] */	IW(0x0000803d | (uint32_t)((((uint64_t)f >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 56) & 0xff) << 16));
		/* ori  r12, r12, f[47:32] */	IW(0x00008c61 | (uint32_t)((((uint64_t)f >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 40) & 0xff) << 16));
		/* sldi r12, r12, 32 */		IW(0xc6078c79);
		/* oris r12, r12, f[31:16] */	IW(0x00008c65 | (uint32_t)((((uint64_t)f >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 24) & 0xff) << 16));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | (uint32_t)(((uint64_t)f & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 8) & 0xff) << 16));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | (uint32_t)((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) & 0xff) << 24) | (uint32_t)(((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) >> 8) & 0xff) << 16));
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
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3 = rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dst */
		/* li r4, dst */		IW(0x00008038 | (((uint32_t)dst & 0xff) << 24) | ((((uint32_t)dst >> 8) & 0xff) << 16));

		/* Arg3 R5 = dict */
		/* li r5, dict */		IW(0x0000a038 | (((uint32_t)dict & 0xff) << 24) | ((((uint32_t)dict >> 8) & 0xff) << 16));

		/* Arg4 R6 = field */
		/* lis  r6, field[63:48] */	IW(0x0000c03c | (uint32_t)((((uint64_t)field >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)field >> 56) & 0xff) << 16));
		/* ori  r6, r6, field[47:32] */	IW(0x0000c660 | (uint32_t)((((uint64_t)field >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)field >> 40) & 0xff) << 16));
		/* sldi r6, r6, 32 */		IW(0xc607c678);
		/* oris r6, r6, field[31:16] */	IW(0x0000c664 | (uint32_t)((((uint64_t)field >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)field >> 24) & 0xff) << 16));
		/* ori  r6, r6, field[15:0] */	IW(0x0000c660 | (uint32_t)(((uint64_t)field & 0xff) << 24) | (uint32_t)((((uint64_t)field >> 8) & 0xff) << 16));

		/* Call rt_loaddot_helper(). */
		/* lis  r12, f[63:48] */	IW(0x0000803d | (uint32_t)((((uint64_t)f >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 56) & 0xff) << 16));
		/* ori  r12, r12, f[47:32] */	IW(0x00008c61 | (uint32_t)((((uint64_t)f >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 40) & 0xff) << 16));
		/* sldi r12, r12, 32 */		IW(0xc6078c79);
		/* oris r12, r12, f[31:16] */	IW(0x00008c65 | (uint32_t)((((uint64_t)f >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 24) & 0xff) << 16));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | (uint32_t)(((uint64_t)f & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 8) & 0xff) << 16));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | (uint32_t)((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) & 0xff) << 24) | (uint32_t)(((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) >> 8) & 0xff) << 16));
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
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3 = rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dict */
		/* li r4, dict */		IW(0x00008038 | (((uint32_t)dict & 0xff) << 24) | ((((uint32_t)dict >> 8) & 0xff) << 16));

		/* Arg3 R5 = field */
		/* lis  r5, field[63:48] */	IW(0x0000a03c | (uint32_t)((((uint64_t)field >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)field >> 56) & 0xff) << 16));
		/* ori  r5, r5, field[47:32] */	IW(0x0000a560 | (uint32_t)((((uint64_t)field >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)field >> 40) & 0xff) << 16));
		/* sldi r5, r5, 32 */		IW(0xc607a578);
		/* oris r5, r5, field[31:16] */	IW(0x0000a564 | (uint32_t)((((uint64_t)field >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)field >> 24) & 0xff) << 16));
		/* ori  r5, r5, field[15:0] */	IW(0x0000a560 | (uint32_t)(((uint64_t)field & 0xff) << 24) | (uint32_t)((((uint64_t)field >> 8) & 0xff) << 16));

		/* Arg4 R6: src */
		/* li r6, src */		IW(0x0000c038 | (((uint32_t)src & 0xff) << 24) | ((((uint32_t)src >> 8) & 0xff) << 16));

		/* Call rt_storedot_helper(). */
		/* lis  r12, f[63:48] */	IW(0x0000803d | (uint32_t)((((uint64_t)f >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 56) & 0xff) << 16));
		/* ori  r12, r12, f[47:32] */	IW(0x00008c61 | (uint32_t)((((uint64_t)f >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 40) & 0xff) << 16));
		/* sldi r12, r12, 32 */		IW(0xc6078c79);
		/* oris r12, r12, f[31:16] */	IW(0x00008c65 | (uint32_t)((((uint64_t)f >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 24) & 0xff) << 16));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | (uint32_t)(((uint64_t)f & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 8) & 0xff) << 16));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | (uint32_t)((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) & 0xff) << 24) | (uint32_t)(((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) >> 8) & 0xff) << 16));
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

	/* Embed arguments to the code. */
	tmp = (uint32_t)(4 + 4 * arg_count);
	ASM {
		/* b tmp */
		IW(0x00000048 | ((tmp & 0xff) << 24)| (((tmp >> 8) & 0xff) << 16));
	}
	arg_addr = (uint64_t)(intptr_t)ctx->code;
	for (i = 0; i < arg_count; i++)
		*ctx->code++ = (uint32_t)arg[i];

	f = (uint64_t)rt_call_helper;

	/* if (!rt_call_helper(rt, dst, func, arg_count, arg)) return false; */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3 = rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dst */
		/* li r4, dst */		IW(0x00008038 | (((uint32_t)dst & 0xff) << 24) | ((((uint32_t)dst >> 8) & 0xff) << 16));

		/* Arg3 R5 = func */
		/* li r5, func */		IW(0x0000a038 | (((uint32_t)func & 0xff) << 24) | ((((uint32_t)func >> 8) & 0xff) << 16));

		/* Arg4 R6: arg_count */
		/* li r6, arg_count */		IW(0x0000c038 | (((uint32_t)arg_count & 0xff) << 24) | ((((uint32_t)arg_count >> 8) & 0xff) << 16));

		/* Arg5 R7 = arg */
		/* lis  r7, arg[63:48] */	IW(0x0000e03c | (uint32_t)((((uint64_t)arg_addr >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)arg_addr >> 56) & 0xff) << 16));
		/* ori  r7, r7, arg[47:32] */	IW(0x0000e760 | (uint32_t)((((uint64_t)arg_addr >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)arg_addr >> 40) & 0xff) << 16));
		/* sldi r7, r7, 32 */		IW(0xc607e778);
		/* oris r7, r7, arg[31:16] */	IW(0x0000e764 | (uint32_t)((((uint64_t)arg_addr >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)arg_addr >> 24) & 0xff) << 16));
		/* ori  r7, r7, arg[15:0] */	IW(0x0000e760 | (uint32_t)(((uint64_t)arg_addr & 0xff) << 24) | (uint32_t)((((uint64_t)arg_addr >> 8) & 0xff) << 16));

		/* Call rt_call_helper(). */
		/* lis  r12, f[63:48] */	IW(0x0000803d | (uint32_t)((((uint64_t)f >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 56) & 0xff) << 16));
		/* ori  r12, r12, f[47:32] */	IW(0x00008c61 | (uint32_t)((((uint64_t)f >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 40) & 0xff) << 16));
		/* sldi r12, r12, 32 */		IW(0xc6078c79);
		/* oris r12, r12, f[31:16] */	IW(0x00008c65 | (uint32_t)((((uint64_t)f >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 24) & 0xff) << 16));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | (uint32_t)(((uint64_t)f & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 8) & 0xff) << 16));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | (uint32_t)((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) & 0xff) << 24) | (uint32_t)(((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) >> 8) & 0xff) << 16));
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

	/* Embed arguments. */
	tmp = (uint32_t)(4 + 4 * arg_count);
	ASM {
		/* b tmp */
		IW(0x00000048 | ((tmp & 0xff) << 24)| (((tmp >> 8) & 0xff) << 16));
	}
	arg_addr = (uint64_t)(intptr_t)ctx->code;
	for (i = 0; i < arg_count; i++)
		*ctx->code++ = (uint32_t)arg[i];

	f = (uint64_t)rt_thiscall_helper;

	/* if (!rt_thiscall_helper(rt, dst, obj, symbol, arg_count, arg)) return false; */
	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* Arg1 R3 = rt */
		/* mr r3, r14 */		IW(0x7873c37d);

		/* Arg2 R4 = dst */
		/* li r4, dst */		IW(0x00008038 | (((uint32_t)dst & 0xff) << 24) | ((((uint32_t)dst >> 8) & 0xff) << 16));

		/* Arg3 R5 = obj */
		/* li r5, obj */		IW(0x0000a038 | (((uint32_t)dst & 0xff) << 24) | ((((uint32_t)dst >> 8) & 0xff) << 16));

		/* Arg4 R6 = symbol */
		/* lis  r6, symbol[63:48] */	IW(0x0000a03c | (uint32_t)((((uint64_t)symbol >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)symbol >> 56) & 0xff) << 16));
		/* ori  r6, r6, symbol[47:32] */	IW(0x0000a560 | (uint32_t)((((uint64_t)symbol >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)symbol >> 40) & 0xff) << 16));
		/* sldi r6, r6, 32 */		IW(0xc607a578);
		/* oris r6, r6, symbol[31:16] */	IW(0x0000a564 | (uint32_t)((((uint64_t)symbol >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)symbol >> 24) & 0xff) << 16));
		/* ori  r6, r6, symbol[15:0] */	IW(0x0000a560 | (uint32_t)(((uint64_t)symbol & 0xff) << 24) | (uint32_t)((((uint64_t)symbol >> 8) & 0xff) << 16));

		/* Arg5 R7 = arg_count */
		/* li r7, arg_count */		IW(0x0000e038 | (((uint32_t)arg_count & 0xff) << 24) | ((((uint32_t)arg_count >> 8) & 0xff) << 16));

		/* Arg6 R8: arg */
		/* lis  r8, arg[63:48] */	IW(0x0000003d | (uint32_t)((((uint64_t)arg_addr >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)arg_addr >> 56) & 0xff) << 16));
		/* ori  r8, r8, arg[47:32] */	IW(0x00000861 | (uint32_t)((((uint64_t)arg_addr >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)arg_addr >> 40) & 0xff) << 16));
		/* sldi r8, r8, 32 */		IW(0xc6070879);
		/* oris r8, r8, arg[31:16] */	IW(0x00000865 | (uint32_t)((((uint64_t)arg_addr >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)arg_addr >> 24) & 0xff) << 16));
		/* ori  r8, r8, arg[15:0] */	IW(0x00000861 | (uint32_t)(((uint64_t)arg_addr & 0xff) << 24) | (uint32_t)((((uint64_t)arg_addr >> 8) & 0xff) << 16));

		/* Call rt_thiscall_helper(). */
		/* lis  r12, f[63:48] */	IW(0x0000803d | (uint32_t)((((uint64_t)f >> 48) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 56) & 0xff) << 16));
		/* ori  r12, r12, f[47:32] */	IW(0x00008c61 | (uint32_t)((((uint64_t)f >> 32) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 40) & 0xff) << 16));
		/* sldi r12, r12, 32 */		IW(0xc6078c79);
		/* oris r12, r12, f[31:16] */	IW(0x00008c65 | (uint32_t)((((uint64_t)f >> 16) & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 24) & 0xff) << 16));
		/* ori  r12, r12, f[15:0] */	IW(0x00008c61 | (uint32_t)(((uint64_t)f & 0xff) << 24) | (uint32_t)((((uint64_t)f >> 8) & 0xff) << 16));
		/* mflr r31 */			IW(0xa602e87f);
		/* mtctr r12 */			IW(0xa603897d);
		/* bctrl */ 			IW(0x2104804e);
		/* mtlr r31 */			IW(0xa603e87f);

		/* If failed: */
		/* cmpwi r3, 0 */		IW(0x0000032c);
		/* beq exception_handler */	IW(0x00008241 | (uint32_t)((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) & 0xff) << 24) | (uint32_t)(((((uint64_t)ctx->exception_code - (uint64_t)ctx->code) >> 8) & 0xff) << 16));
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

	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* R3 = rt->frame->tmpvar[src].val.i */
		/* li r3, dst */		IW(0x00006038 | (((uint32_t)src & 0xff) << 24) | ((((uint32_t)src >> 8) & 0xff) << 16));
		/* sldi r3, r3, 4 */		IW(0xe4266378);
		/* add r3, r3, r15 */		IW(0x147a637c);
		/* lwz r3, 8(r3) */		IW(0x08006380);

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

	ASM {
		/* R14: rt */
		/* R15: &rt->frame->tmpvar[0] */
		/* R31: saved LR */

		/* R3 = rt->frame->tmpvar[src].val.i */
		/* li r3, dst */		IW(0x00006038 | (((uint32_t)src & 0xff) << 24) | ((((uint32_t)src >> 8) & 0xff) << 16));
		/* sldi r3, r3, 4 */		IW(0xe4266378);
		/* add r3, r3, r15 */		IW(0x147a637c);
		/* lwz r3, 8(r3) */		IW(0x08006380);

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
		/* std r14, -8(r1) */		IW(0xf8ffc1f9);
		/* std r15, -16(r1) */		IW(0xf0ffe1f9);
		/* std r31, -24(r1) */		IW(0xe8ffe1fb);
		/* addi r1, r1, -64 */		IW(0xc0ff2138);

		/* R14 = rt */
		/* mr r14, r3 */		IW(0x781b6e7c);

		/* R15 = *rt->frame = &rt->frame->tmpvar[0] */
		/* ld r15, 0(r14) */		IW(0x0000eee9);
		/* ld r15, 0(r15) */		IW(0x0000efe9);

		/* Skip an exception handler. */
		/* b body */			IW(0x1c000048);
	}

	/* Put an exception handler. */
	ctx->exception_code = ctx->code;
	ASM {
	/* EXCEPTION: */
		/* addi r1, r1, 64 */		IW(0x40002138);
		/* ld r31, -24(r1) */		IW(0xe8ffe1eb);
		/* ld r15, -16(r1) */		IW(0xf0ffe1e9);
		/* ld r14, -8(r1) */		IW(0xf8ffc1e9);
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
		/* ld r31, -24(r1) */		IW(0xe8ffe1eb);
		/* ld r15, -16(r1) */		IW(0xf0ffe1e9);
		/* ld r14, -8(r1) */		IW(0xf8ffc1e9);
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
			//IW(0x00000048 | (((uint32_t)offset & 0xff) << 24) | ((((uint32_t)offset >> 8) & 0xff) << 16) | ((((uint32_t)offset >> 16) & 0xff) << 8));
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

#endif /* defined(ARCH_ARM64) && defined(USE_JIT) */
