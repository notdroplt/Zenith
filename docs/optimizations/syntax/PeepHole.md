# Zenith docs: `optimizations/syntax/peephole`

Peephole optimization consists in reducting sections of code with faster or
smaller alternatives without changing results

a lot of peephole optimizations work alongside with [constant propagation](./ConstantPropagation.md)

- **Redundant code**  
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

- **Constant folding**  
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

- **Instruction reduction**  
    The compiler might reduce the complexity of some instructions as it seems fit, for example:

    ```zenith
    n = a * 3

    # can reduce to

    n_1st_case = a << 1 | a 

    n_2nd_case = a + a + a  
    ``` 

    First case will occur only when $a \in \mathbf{N}$, second case works as a fallback to the first. 

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
