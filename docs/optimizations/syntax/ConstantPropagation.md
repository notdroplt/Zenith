# Zenith docs: `optimizations/syntax/Constants`

Because most of the time the elements will be constants, constant propagation will be
an important part of the compiler.

So for example:

```zenith
constant_i = 10

propagated = i - 4
```

the code will inline `constant_i` and return

```zenith
constant_i = 10

propagated = 10 - 4
```

and later other optimizations will kick in, such as [peephole optimizations](./PeepHole.md)
