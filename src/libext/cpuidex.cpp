/*
 * Copyright 2016 Gary R. Van Sickle (grvs@users.sourceforge.net).
 *
 * This file is part of UniversalCodeGrep.
 *
 * UniversalCodeGrep is free software: you can redistribute it and/or modify it under the
 * terms of version 3 of the GNU General Public License as published by the Free
 * Software Foundation.
 *
 * UniversalCodeGrep is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * UniversalCodeGrep.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file  Portable CPUID-related functionality. */

#include <config.h>

#include "cpuidex.hpp"

#include <cstdint>
#include <cpuid.h>  // Need this because clang doesn't support __builtin_cpu_supports().
#include "immintrin.h"

// Need this because clang and gcc don't agree on what to do with a ".".
#ifndef bit_SSE4_2
#define bit_SSE4_2 bit_SSE42
#endif


// Results of the CPUID instruction.
static uint32_t eax, ebx, ecx, edx;

static bool CPUID_info_valid = false;


static void GetCPUIDInfo() noexcept
{
#if defined(__x86_64__)
	// Skip the CPUID call if we've already done it once.
	if(!CPUID_info_valid)
	{
		__get_cpuid(1, &eax, &ebx, &ecx, &edx);
		CPUID_info_valid = true;
	}
#else
	// For non-x86 platforms, we won't have a CPUID, so for now we won't try to determine any extensions.
	/// @todo Expand ISA extension support to other architectures.
	CPUID_info_valid = false;
#endif
}

bool sys_has_sse2() noexcept
{
	GetCPUIDInfo();
	return edx & bit_SSE2;
}

bool sys_has_sse4_2() noexcept
{
	GetCPUIDInfo();
	return ecx & bit_SSE4_2;
}

bool sys_has_popcnt() noexcept
{
	GetCPUIDInfo();
	return ecx & bit_POPCNT;
}

#if defined(__i386__) || defined(__x86_64__)
#define HAS_XGETBV
static uint32_t get_xgetbv(uint32_t xcr) {
  uint32_t t_eax;
  asm volatile (
    "xgetbv\n\t"
    : "=a"(t_eax)
    : "c"(xcr)
    : "memory", "cc", "edx");  // edx unused.
  return t_eax;
}
#endif
#ifdef HAS_XGETBV
static const uint32_t XCR_XFEATURE_ENABLED_MASK = 0;
#endif

bool sys_has_avx() noexcept
{
	bool supported = false;

	GetCPUIDInfo();
	if(ecx & bit_AVX)
	{
		// CPU supports AVX.
		if(ecx & bit_OSXSAVE)
		{
			// OS has enabled XGETBV.
			// Check if OS has enabled XMM and YMM state saving support.
			auto feature_mask = get_xgetbv(XCR_XFEATURE_ENABLED_MASK);
			if((feature_mask & 0x6) == 0x6)
			{
				supported = true;
			}
		}
	}

	return supported;
}

