Workflow
========

## Source Execution

Use the `linguine` command to run a script source code.

```
 +--------+       +=======+
 | Source |  ==>  || Run ||
 +--------+       +=======+
```

Note that JIT-compilation is enabled by default. If you want to turn
off JIT, add the `--disable-jit` option.

## Bytecode Execution

Use the `linguine --bytecode` command to convert a `.ls` source code to a `.lsc` bytecode file.
Then use the `linguine` command to run the generated `.lsc` file.

```
 +-------------------+        +----------------------+       +=======+
 | Source File (.ls) |  --->  | Bytecode File (.lsc) |  ==>  || Run ||
 +-------------------+        +----------------------+       +=======+
```

## Standalone Execution

Use the `linguine --app` command to convert `.ls` files to a single `.c` file.
Then, use the `cc` command to compile the generated `.c` file with `liblinguine.a`.
Finally, run the generated app.

```
 +-------------------+        +--------------------+       +-------------------+      +=======+
 | Source File (.ls) |  --->  | C Source File (.c) |  -->  | Executable (.exe) |  ==> || Run ||
 +-------------------+        +--------------------+       +-------------------+      +=======+
```
