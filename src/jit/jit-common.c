/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Linguine
 * Copyright (c) 2025, Tamako Mori. All rights reserved.
 */

/*
 * JIT (common): Just-In-Time native code generation
 */

#if !defined(USE_JIT)

/*
 * Generate a JIT-compiled code for a function.
 */
bool
jit_build(
	struct rt_env *rt,
	struct rt_func *func)
{
	UNUSED_PARAMETER(rt);
	UNUSED_PARAMETER(func);

	/* stub */
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

	/* stub */
}

#else

#include "linguine/runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(TARGET_WINDOWS)
#include <Windows.h>		/* VirtualAlloc(), VirtualProtect(), VirtualFree() */
#else
#include <sys/mman.h>		/* mmap(), mprotect(), munmap() */
#endif

/*
 * Map a memory region for the generated code.
 */
bool
jit_map_memory_region(
	void **region,
	size_t size)
{
#if defined(TARGET_WINDOWS)
	*region = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
#else
	*region = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
#endif
	if (*region == NULL)
		return false;

	memset(*region, 0, size);

	return true;
}

/*
 * Make a region writable and non-executable.
 */
void
jit_map_writable(
	void *region,
	size_t size)
{
#if defined(TARGET_WINDOWS)
	DWORD dwOldProt;
	VirtualProtect(region, size, PAGE_READWRITE, &dwOldProt);
#else
	mprotect(region, size, PROT_READ | PROT_WRITE);
#endif
}

/*
 * Make a region executable and non-writable.
 */
void
jit_map_executable(
	void *region,
	size_t size)
{
#if defined(TARGET_WINDOWS)
	DWORD dwOldProt;
	VirtualProtect(region, size, PAGE_EXECUTE_READ, &dwOldProt);
	FlushInstructionCache(GetCurrentProcess(), region, size);
#else
	mprotect(region, size, PROT_EXEC | PROT_READ);
	__builtin___clear_cache((char *)region, (char *)region + size);
#endif
}

#endif /* defined(USE_JIT) */
