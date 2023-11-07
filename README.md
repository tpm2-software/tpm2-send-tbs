[![Build Status](https://github.com/tpm2-software/tpm2-send-tbs/workflows/CI/badge.svg)](https://github.com/tpm2-software/tpm2-send-tbs/actions)

> [!WARNING]
> This project is in an alpha state with [known limitations](#limitation-some-tpm-commands-fail). Use with caution!

# tpm2-send-tbs

tpm2-send-tbs is a zero-dependency utility for sending raw bytes to the TPM.

Want to access the TPM 2.0 from within WSL2? Just compile `tpm2-send-tbs.exe` and then call it from your WSL2 shell.


# Usage

tpm2-send-tbs takes an input stream (by default `stdin`) and an output stream (by default `stdout`).

```cmd
tpm2-send-tbs [--debug] [--bin] [-in <input file>] [-out <output file>]
```

## Examples

By default, tpm2-send-tbs reads a hex stream from `stdin` and writes to `stdout`:

```cmd
REM cmd.exe:
echo 80010000000c0000017b0004 | build/tpm2-send-tbs.exe
```

```bash
# bash:
printf "80010000000c0000017b0004" | build/tpm2-send-tbs.exe
```

You can use `--bin` to switch to binary format.

```bash
# bash:
printf "80010000000c0000017b0004" | xxd -r -p | build/tpm2-send-tbs.exe --bin | xxd -p
```

### Note:

`xxd` buffers until its input pipe is closed. With many processes, commands/responses are a back and forth. E.g. tcti-cmd waits for a TPM response before sending the next command.  Thus, `xxd` would block indefinitely, here. You can use `build/hex` and `build/unhex` instead.

```bash
# bash:
tpm2_getrandom -T "cmd: build/hex | build/tpm2-send-tbs.exe | build/unhex" --hex 4
```

### Note:

The WSL2 pipe is broken. It turns LF into CR+LF, even if opened in bytewise mode. As a result, `tpm2_getrandom -T "cmd: build/tpm2-send-tbs.exe --bin" --hex 4` will not work.

# Build

In your WSL2 (or Linux), run make. This will use mingw if installed. Otherwise, msvc will be used (requires [Build Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)).

```bash
make
```

## Compile Manually

Using mingw:
```bash
x86_64-w64-mingw32-gcc -Wall -Wextra -D_WIN32_WINNT=0x0600 src/tpm2-send-tbs.c -o tpm2-send-tbs.exe -L /mnt/c/Program\ Files\ \(x86\)/Windows\ Kits/10/Lib/*/um/x64 -l:tbs.lib
```

Alternatively, in a windows-only context:

```cmd
REM setup environment
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"

REM compile
cd build
cl /W4 src/tpm2-send-tbs.c /link tbs.lib
```

# Test

Install the [tpm2-tools](https://github.com/tpm2-software/tpm2-tools). In your WSL2, run the smoke tests:

```
make check
```

### Limitation: Some TPM Commands Fail

At the moment, some TPM commands will fail. The root cause of this is unclear. It could be a limitation of the TBS, insufficient priviledges or something entirely different. Hints and patches welcome!

Example:

```bash
tpm2_nvread 0x01C00002 -T "cmd: build/hex | build/tpm2-send-tbs.exe --debug | build/unhex"
WARN: Reading full size of the NV index
read cmd[59]: 80 01 00 00 00 3b 00 00 01 76 40 00 00 07 40 00 00 07 00 20 4f 83 b6 b9 fa 2d d2 e2 30 c8 a5 ce 6d 62 ee 9b 94 45 5e 69 47 a4 52 7f 79 39 15 2c 2f e7 b5 7b 00 00 00 00 10 00 0b
send rsp[48]: 80 01 00 00 00 30 00 00 00 00 02 00 00 00 00 20 c5 db 0b a8 b1 c7 03 45 76 d2 37 1b fd dd f1 ef f9 fc 1d b8 ea 5f 57 46 90 f1 6e e6 25 16 f7 38
read cmd[14]: 80 01 00 00 00 0e 00 00 01 69 01 c0 00 02
Failed when attempting to submit TBS context: 80284001
```
