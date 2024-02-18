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

#include "k22_config.h"
#include "k22_logger.h"
#include "k22_utils.h"

#if K22_LOADER
#include "k22_debugger.h"
#include "k22_patch.h"
#include "k22_process.h"
#endif

#if K22_MODULE

#endif
