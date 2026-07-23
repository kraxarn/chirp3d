# Minimum

Most of these aren't tested, but "should work" according to their respective specifications.

## CPU

Technically, SSE2/NEON support is only required by the pre-built binaries.
The engine can be compiled without it, and it *should* work.

### x86 (SSE2)

* [Intel Pentium 4](https://en.wikipedia.org/wiki/Pentium_4)
* [AMD Opteron](https://en.wikipedia.org/wiki/Opteron)

### x86-64 (SSE2)

* [Any :)](https://en.wikipedia.org/wiki/X86-64#Microarchitecture_levels)

### AArch64 (NEON)

* [ARM Cortex-R](https://en.wikipedia.org/wiki/ARM_architecture_family#Advanced_SIMD_(Neon))

## Linux (Vulkan)

* [Nvidia GeForce 600 series](https://en.wikipedia.org/wiki/GeForce_600_series)
* [AMD Radeon HD 7000 series](https://en.wikipedia.org/wiki/Radeon_HD_7000_series)
* [Intel Arc](https://en.wikipedia.org/wiki/Intel_Arc#Alchemist) / [Intel HD Graphics 530](https://en.wikipedia.org/wiki/Skylake_(microarchitecture)#GPU)
* [Adreno 500 series](https://en.wikipedia.org/wiki/Adreno#Adreno_500_series)

## macOS 10.14 (Metal)

* [Nvidia GeForce 600 series](https://en.wikipedia.org/wiki/Metal_(API)#Supported_GPUs)
* [AMD Radeon HD 7000 series](https://en.wikipedia.org/wiki/Metal_(API)#Supported_GPUs)
* [Intel HD/Ivy Graphics](https://en.wikipedia.org/wiki/Metal_(API)#Supported_GPUs)
* [Apple M1](https://en.wikipedia.org/wiki/Metal_(API)#Supported_GPUs)

## Windows 10 (Direct3D 12)

* [Nvidia GeForce 400 series](https://en.wikipedia.org/wiki/GeForce_400_series)
* [AMD Radeon HD 7000 series](https://en.wikipedia.org/wiki/Radeon_HD_7000_series)
* [Intel Arc](https://en.wikipedia.org/wiki/Intel_Arc#Alchemist) / [Intel HD 5300](https://en.wikipedia.org/wiki/Broadwell_(microarchitecture)#GPU)
* [Adreno 600 series](https://en.wikipedia.org/wiki/Adreno#Adreno_600_series)

# Recommended

Minimum requirements that are verified to work.

## CPU

* AMD Zen4
* Qualcomm Kryo
* Apple M2

## Linux (Vulkan)

* Radeon RX 6000 series
* Radeon Graphics (Raphael)

## macOS 15 (Metal)

* Apple M2

## Windows 11 (Direct3D 12)

* Adreno 600 series
