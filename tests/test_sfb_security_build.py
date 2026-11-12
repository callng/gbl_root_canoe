"""Compile the security library without host or cross-system C headers.

Run with Python 3 and Clang; no firmware, sysroot or generated files required.
"""
from pathlib import Path
import shlex
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]
EDK = ROOT / "submodules/uefi/edk2"
LIBRARY = EDK / "QcomModulePkg/Library/SfbSecurityLib"


def main():
    clang = shutil.which("clang")
    if not clang:
        raise RuntimeError("clang is required")

    # Use the real library's build options, so removing its private CRT
    # include path fails even on machines with an AArch64 libc installed.
    build_flags = []
    sources = []
    section = ""
    for line in (LIBRARY / "SfbSecurityLib.inf").read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("["):
            section = line
        elif section == "[Sources]" and line.endswith(".c"):
            sources.append(line)
        elif section == "[BuildOptions]" and line.startswith("GCC:*_*_*_CC_FLAGS"):
            build_flags.extend(
                flag.replace("$(WORKSPACE)", EDK.as_posix())
                for flag in shlex.split(line.split("=", 1)[1]))
    if not sources:
        raise RuntimeError("No security-library sources found")

    include_paths = [
        "MdePkg/Include", "MdePkg/Include/AArch64", "CryptoPkg/Include",
        "CryptoPkg/Library/MbedTlsLib/Include",
        "CryptoPkg/Library/MbedTlsLib/mbedtls/include", "QcomModulePkg/Include",
    ]
    flags = ["--target=aarch64-linux-gnu", "-ffreestanding", "-nostdinc",
             "-fsyntax-only", "-fshort-wchar", "-fno-short-enums",
             "-Wall", "-Werror", "-Wno-address", "-DMDEPKG_NDEBUG"]
    flags += build_flags + [f"-I{EDK / path}" for path in include_paths]
    for source in sources:
        subprocess.run([clang, *flags, str(LIBRARY / source)], check=True)
    print(f"SFB security: {len(sources)} AArch64 sources compiled without system C headers")


if __name__ == "__main__":
    main()
