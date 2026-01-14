#include "cpuinfo.h"

#include "logcategory.h"

// Workaround for compilers already defining min/max
#define  __GLOBAL__
#include "common/global.h"

#ifdef __linux__
#include "common/args.h"
#include "common/cpu.h"
#endif

#ifdef ARCH_X86
#include "x86/freq/freq_avx.h"
#include "x86/freq/freq_avx512.h"
#endif

#ifdef ARCH_X86
#include "x86/cpuid.h"
#elif ARCH_ARM
#include "arm/midr.h"
#include "common/soc.h"
#endif

const char *cpu_name()
{
	static constexpr size_t cpu_name_len = 64;
	static char cpu_name[cpu_name_len];

	struct cpuInfo *cpu_info = get_cpu_info();

#ifdef ARCH_X86
	SDL_strlcpy(cpu_name, get_str_cpu_name(cpu_info, false), cpu_name_len);
	free_cpuinfo_struct(cpu_info);
	return cpu_name;
#elif ARCH_ARM
	SDL_strlcpy(cpu_name, get_soc_name(cpu_info->soc), cpu_name_len);
	free_cpuinfo_struct(cpu_info);
	return cpu_name;
#endif

	return "Unknown";
}

#ifdef __linux__
bool accurate_pp()
{
	return false; // Default value
}

bool measure_max_frequency_flag()
{
	return false; // Default value
}
#endif

#ifdef ARCH_X86
void *compute_avx512([[maybe_unused]] void *pthread_arg)
{
	return nullptr; // Unused, do nothing
}

void *compute_avx([[maybe_unused]] void *pthread_arg)
{
	return nullptr; // Unused, do nothing
}
#endif
