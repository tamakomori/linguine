Linguine
========

Linguine is a lightweight scripting language with a familiar C-like
syntax, designed for seamless integration into your applications.

## Highlights

* Fast — Powered by a Just-in-Time (JIT) compiler
* Compact — ~200 KB including the JIT compiler
* Expressive — Built-in support for arrays, dictionaries, and iterators

## Build and Run

```
make
./linguine hello.ls
```

## Syntax

Please see [Syntax of Linguine](doc/syntax.md) for the details.

## JIT Compilation

Linguine features a lightweight Just-in-Time (JIT) compiler that
generates native machine code across multiple platforms. This
typically delivers a 2x to 4x performance boost over interpreted
execution. The JIT engine has been ported to a wide range of processor
architectures, ensuring broad compatibility with all major computing
environments.

## Small Footprint

Despite its capabilities, Linguine remains extremely compact. The full
compiler and runtime—JIT engine included—fit within approximately
200 KB of binary size.

## Modern Language Features

Linguine offers a rich set of features, including:

* Flexible arrays and dictionaries
* Clean iterator syntax
* A minimalist design that supports both beginner-friendliness and expressive power

## Easy Application Integration

Linguine is built with embedding in mind, making it an ideal choice for:

* Plugin systems
* Game scripting

In addition to the JIT engine, Linguine provides a C code generation
backend. This allows you to deploy scripts as native code on
platforms where JIT compilation is restricted or unavailable.

## Workflow

Please see [Workflow](doc/workflow.md) for the details.

## Compiler Overview

Please see [Compiler Overview](doc/compiler-overview.md) for the details.

## Development Status

### JIT Compiler

|Architecture   |32-bit |64-bit |Description                                                  |
|---------------|-------|-------|-------------------------------------------------------------|
|x86            |OK     |OK     |Tested on PC                                                 |
|Arm            |OK     |OK     |Tested on Apple Silicon (64-bit) and Raspberry Pi 4 (32-bit) |
|RISC-V         |       |       |                                                             |
|PowerPC        |OK     |OK     |Tested on Power8 (64-bit) and qemu-user (32-bit)             |
|S/390          |       |       |                                                             |
|SPARC          |       |       |                                                             |
|MIPS           |OK     |OK     |Tested on qemu-user (64-bit and 32-bit)                      |
|Itanium        |       |       |                                                             |
|PA-RISC        |       |       |                                                             |
|SH4            |       |       |                                                             |
|Alpha          |       |       |                                                             |
|68000          |       |       |                                                             |

### Operating Systems

|Operating System  |32-bit |64-bit |Description                                               |
|------------------|-------|-------|----------------------------------------------------------|
|Linux             |OK     |OK     |                                                          |
|macOS             |       |OK     |                                                          |
|Windows           |OK     |OK     |                                                          |
|FreeBSD           |       |       |                                                          |
|NetBSD            |       |       |                                                          |
|OpenBSD           |       |       |                                                          |
|AIX               |       |       |                                                          |
|Solaris           |       |       |                                                          |
|HP-UX             |       |       |                                                          |
|PS4               |       |       |                                                          |
|PS5               |       |       |                                                          |
|Switch            |       |       |                                                          |
|Switch 2          |       |       |                                                          |

### Compilers

|Compiler          |Status      |
|------------------|------------|
|gcc 13            |OK          |
|clang 16          |OK          |
|MSVC              |            |
