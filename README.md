# ps4-hdd [![ps4-hdd cross-platform CI](https://github.com/John-K/ps4-hdd/actions/workflows/cross_platform_ci.yml/badge.svg)](https://github.com/John-K/ps4-hdd/actions/workflows/cross_platform_ci.yml)
A lightning quick tool to decrypt Playstation 4 hard disk partitions with EAP keys.

Makes use of hardware accelerated crypto (AES-NI for x86_64, NEON for ARM64), multi-threading, and memory mapped files to make full use of your CPU and SSD.

Windows, Linux, and MacOS (Intel and Apple Silicon) are supported.

> [!WARNING]
> Using this tool with files on a Plan9 (ie Windows drive from WSL2) or FUSE filesystem will signifiantly limit the performance.
>
> For best perfomance, process files that are on a local and native filesystem volume.

## Usage

`ps4-hdd <XTS_KEY Hex> <XTS_TWEAK Hex> <IV_OFFSET> <in_path> <out_path>`
> [!TIP]
> XTS_KEY and XTS_TWEAK are hex strings with no spaces
> 
> IV_OFFSET is a decimal integer
>
> The websites linked below can be helpful in determining the correct IV_OFFSET for your partition

For example:
`ps4-hdd 00112233445566778899AABBCCDDEEFF FFEEDDCCBBAA99887766554433221100 1234567890 user.bin user-dec.bin`

## Resources
For more information see:
 * https://www.psdevwiki.com/ps4/Partitions
 * https://www.psdevwiki.com/ps4/Mounting_HDD_in_Linux

## Building
`ps4-hdd` requires a compiler with support for `c++23` / `c++2b`

### Linux
`make`

### Windows
`msbuild /m -p:Configuration=Release .`

or open the Visual Studio project - tested with Visual Studio Community Edition 2022

### MacOS
`make -f Makefile.MacOS`

## Future work
 * Add SIGINT handler to cleanly cancel decryption
 * Add support for ARMv8 AES instructions
