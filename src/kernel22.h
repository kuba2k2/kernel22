// Copyright (c) Kuba Szczodrzyński 2024-2-12.

#pragma once

#define _CRT_SECURE_NO_WARNINGS 1

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <Windows.h>
#include <processthreadsapi.h>
#include <psapi.h>
#include <strsafe.h>
#include <winternl.h>

#include "k22_common.h"
#include "k22_config.h"
#include "k22_extern.h"
#include "k22_logger.h"

#if K22_CORE
#define K22_CORE_PROC __declspec(dllexport)
#elif K22_LOADER
#define K22_CORE_PROC __declspec(dllimport)
#endif

#include "k22_core.h"

#if K22_LOADER
#include "k22_debugger.h"
#include "k22_loader.h"
#endif
