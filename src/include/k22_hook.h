// Copyright (c) Kuba Szczodrzyński 2024-8-4.

#include "kernel22.h"

#include "k22_macros.h"

#define K22_HOOK_PROC(ret, name, args)                                                                                 \
	ret(*CONCAT(Real, name)) args;                                                                                     \
	ret CONCAT(Hook, name) args

#define K22_HOOK_DEF(ret, name, args)                                                                                  \
	extern ret(*CONCAT(Real, name)) args;                                                                              \
	extern ret CONCAT(Hook, name) args

#define K22_HOOK_REAL_DEF(ret, name, args) K22_CORE_PROC ret CONCAT(K22Real, name) args

#define K22_HOOK_REAL_PROC(ret, name, args, callargs)                                                                  \
	ret CONCAT(K22Real, name) args {                                                                                   \
		if (CONCAT(Real, name))                                                                                        \
			return CONCAT(Real, name) callargs;                                                                        \
		return name callargs;                                                                                          \
	}

#define K22_HOOK_CREATE(name)                                                                                          \
	do {                                                                                                               \
		if (!K22HookCreate(name, CONCAT(Hook, name), (LPVOID)&CONCAT(Real, name)))                                     \
			return FALSE;                                                                                              \
	} while (0)

#define K22_HOOK_CREATE_EX(name, proc)                                                                                 \
	do {                                                                                                               \
		if (!K22HookCreate(proc, CONCAT(Hook, name), (LPVOID)&CONCAT(Real, name)))                                     \
			return FALSE;                                                                                              \
	} while (0)

#define K22_HOOK_REMOVE(name)                                                                                          \
	do {                                                                                                               \
		if (!K22HookRemove(name))                                                                                      \
			return FALSE;                                                                                              \
	} while (0)
