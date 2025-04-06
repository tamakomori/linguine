Compiler Overview
=================

```
 +--------+  parser  +-----+  HIR-pass  +-----+  LIR-pass  +-----+
 | Source |  ----->  | AST |  ------->  | HIR |  ------->  | LIR |
 +--------+          +-----+            +-----+            +-----+
```

## AST

The Linguine compiler first transforms source code into an AST
(abstract syntax tree)—a data structure optimized for program
parsing.

## HIR

The AST is then converted to HIR (high-level intermediate
representation), which is better suited for optimization. Future
versions will implement additional optimizers.

## LIR

Next, the HIR is transformed into LIR (low-level intermediate
representation), also known as "bytecode"—a byte sequence designed
for interpretation.

## JIT

Finally, the JIT compiler translates this LIR into native code.

```
+-----+  JIT-pass  +-------------+
| LIR |  ------->  | Native Code |
+-----+            +-------------+
```

## C Backend

Plus, LIR can be translated to C source code.

```
+-----+  C-backend  +----------+
| LIR |  -------->  | C Source |
+-----+             +----------+
```
