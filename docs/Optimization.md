## Constant Propagation

Because most of the time the elements will be constants, constant propagation is
a very important part of the optimization process.

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

## Peephole optimization

Peephole optimization consists in reducting sections of code with faster or
smaller alternatives without changing results

a lot of peephole optimizations work alongside with constant propagation

### **Redundant code** 

with the following piece of code, any reference to `const2` will be translated as an invocation to `const1`:

```zenith
const1 = a + b

const2 = const1

const3 = const2 + b + c
```

which will become

```zenith
const1 = a + b 

const3 = const1 + b + c
```

and if `const1` is only referred once, it can still be inlined to

```zenith
const3 = a + b + b + c
```

### Constant folding  

After some optimizations, Both values on an operation *might* become known at compile-time, as a simple example:

```zenith
pi = 3.14

e = 2.71

someval = pi * e

# after propagation

someval = 3.14 * 2.71

# after folding

someval = 8.5094
``` 

### Instruction reduction
The compiler might reduce the complexity of some instructions as it seems fit, for example:

```zenith
n = a * 3

// can reduce to

n_1st_case = a << 1 | a 

n_2nd_case = a + a + a  
``` 

### precision preserving
The compiler is also free to sacrifice size a little if it means precision is preserving

```zenith
precision = a/b + c/d
```

can result in less precise results on integer division. Given that, the compiler is free 
to do mathematically equivalent substitutions if any portion of code allows it

turning the example into

```zenith
precision = (a*d + c*b)/(b*d)
```

the amount of instructions did increase (from 3 to 5) but so did precision most of the
time

### Type related inductions
To any function that has any stronger idea on the types passed to it, the compiler is
free to use any identities known to it, for example

```
//annotations do not exist but are still planned
@periodic(2*pi, -1, -pi)
sin : Real -> [-1, 1]

// the period is reduced to 2
sin_pi angle = sin (angle * pi)

func int -> int
func = sin_pi
```

$\forall x \in \mathbf{N} \rightarrow sin\ x * \pi = 0$ so the compiler is free to make 
`func x` into a definition of `func x = 0` or even make `

- **Tautologic Sequences**  
Tautologic sequences are inlined as they are proven to be either always true, always false, or always return the same value. Being:


$$x = x, \forall x \rightarrow true$$  
$$x > x, \forall x \rightarrow false$$
$$x \ne x, \forall x \rightarrow false$$
$$x \le k, \forall x \in Domain \space \vert \space Domain \subset \mathbf{U} \le k \rightarrow true$$ 
$$x > k, \forall x \in Domain \space \vert \space Domain \subset \mathbf{U} \le k \rightarrow false$$
$$x \pm 0 , \forall x \rightarrow x$$
$$x \times 1, \forall x \rightarrow x$$
$$x \div 1, \forall x \rightarrow x$$


Note, $\mathbf{U}$ represents the biggest set the language is able to work with, currently, $\mathbf{U} = \mathbf{N}$