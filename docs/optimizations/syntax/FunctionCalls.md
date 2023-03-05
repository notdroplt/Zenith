# Zenith docs: `optimizations/syntax/funccalls`

The need to optimize calls on a language that aims to be function is something necessary, there are some ways to reduce the overhead of one of the most important pieces of the language:

- **Compile-time calls**  
    Simple / small functions can be called inside the own compiler as if the language was interpreted, use a modulo function for example:
    
    ```zenith
    abs(x) => x > 0 ? x : -x
    ```

    there will be two reasons the function call might be inlined:

    1. there is one and only one call to said function
    2. the compiler thinks the function is "small enough" to be inlined  

    ```zenith
    num = abs(-4)

    # becomes

    num = 4
    ```

    