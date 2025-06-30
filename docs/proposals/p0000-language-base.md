# Proposal #0000a, base language syntax and constructs

Author: Arthur  
Date: 2025-04-13

### Abstract

Formalize general compiler and language constructs

> This document is not finished, future commits might append and change
> content 

## 1. Syntax
### 1.1. language tokens and keywords

This only takes care of the "complex" tokens, ones like `+` and `==` are
defined inside the parser

The implicit definitions are as follows: 
- `digit`: characters `0` through `9`.
- `letter`: characters `A` through `Z`, and `a` through `z`.
- `character`: anything
- `newline`: characters `\r`, `\n` or a combination of both

```ebnf
Int       = digit { digit } ;
Float     = [ digit { digit } ] [ "."  digit { digit } ] [ ("e" | "E") ["+" | "-"] digit { digit } ] ;
String    = '"' { character - '"' } '"' ;
ID        = ( letter | "_" ) { letter | digit | "_" | "'" } ;

Comment   = "//" { (character) - newline } newline ;
```

### 1.2. Grammar

```ebnf
Expr     = Ternary ;
Ternary  = Pipe [ "?" Ternary ":" Ternary ] ;
Pipe     = LogicalOr { "|>" LogicalOr } ;
Logical  = Relative { ( "||" | "&&" ) Relative } ;
Relative = Additive { ( "==" | "!=" | "<" | ">" | "<=" | ">=" ) Additive } ;
Additive = Multi { ( "+" | "-" ) Multi } ;
Multi    = Shift { ( "*" | "/" | "%" ) Shift } ;
Shift    = Bitwise { ( "<<" | ">>" ) Bitwise } ;
Bitwise  = Member { "&" | "|" | "^" Member } ;
Member   = Unary { ( "." | "#" ) Unary } ;
Unary    = UnaryOp ( Unary | Postfix ) ;
Postfix  = Primary { [ Primary ] | ( "." ID ) | ( "#" Unary ) } ;
Primary  = Int | Float | String | ID | "[" { Expr ";" } "]" 
          | "(" Expr ")" | Match | Intr Primary;

Module   = "module" ("<<" | ">>") String Intr ;
Formula  = ID { Primary } (":" Type ["=" Expr] | "=" Expr) ";" ;
Intr     = "{" { Formula } "}" ;
IntrApp  = Intr Primary ;
Match    = "match" Expr Intr ;

Range    = "[" expr ";" expr [ ";" expr ] "]" ;
Product  = ID [ Intr ] ;
TPrimary = Range | Product | "(" Type ")" | "?" Primary;
TUnary   = "*" ( TUnary | TPrimary ) ;
TArray   = ( TArray | TUnary ) [ "#" Primary ] ;
TBinary  = TArray [ "->" TBinary ] ;
Type     = TBinary [ "|" Type ] ;

BinaryOp = "#" | "." | "|" | "&" | "^" | "<<" | ">>" 
         | "*" | "/" | "+" | "-" | "%" | "==" | "!=" 
         | "<" | ">" | "<=" | ">=" | "&&" | "||" | "|>";
UnaryOp  = "+" | "-" | "~" | "!" | "*" | "&" ;

Program = { ( Module | Formula | Comment ) };
```

## 2. Typing

There are only the following primitives: $$C = \{ Bool^0, Integer^0, 
Rational^0, Pointer^1, Array^1, \to^2, Aggregate^n \}$$

> Integers and Rationals are taken as nullary types as they take no type
> parameters, instead, castings and compile time numeric values as 
> limiters.

### 2.1. Available Types
#### 2.1.1. Booleans

Booleans are already bounded by definition, any specialization is 
isomorphic to a unit type.

#### 2.1.2. Integers

All integers are in the form `[start, end, padding]{value}`, with 
$a,b \in \mathbb{Z}, a \le value \le b $, and specialized values may 
occur. Coercion is only implicit from `IntA` to `IntB` if the size of
`IntA` is smaller or equal to `IntB`, and both integers have the same
sign. 

#### 2.1.3 Rationals

All rational types are written in the same form as integers,
but instead of being a subset of $\mathbb{Z}$, be now a subset of
$\mathbb{Q}$. Normally, languages call rationals "floats" or "decimals",
but computers work with just 

#### 2.1.4 Pointers

Much like references in other languages, pointers allow for easier 
manipulation of data without the need of a pass-by-copy. Most of the 
time, pointers are not used directly unless manual memory management is
used. They can't, unlike C-like languages, be used as arrays.

#### 2.1.4 Arrays

Arrays are collections of type `T` elements that are of size `n`, array
operations are bound checked on compile time, allowing minimal runtime
overhead, while still achieving out-of-bounds security.

#### 2.1.5 Functions

Function types allow for higher order functions and a ton more of 
necessary functional language constructs. All function types consist of
one argument and a single return value, for multiple arguments, a 
function type that returns other functions should be used.

#### 2.1.6 Aggregates (Products)

Products are types where there can be any amount of subtypes, and all 
members of a product type can coexist at the same time.

#### 2.1.7 Aggregates (Sums)

Sum types also allow any amount of subtypes to be grouped together. But,
unlike products, only one of them can exist at a point in time.

### 2.2. Operation Types

Types sets are notated in the compiler as follows:
- Booleans: $B$
- Integers: $I$
- Rationals: $R$
- Products: $P$
- Sums: $S$
- Castings: any name followed by `'` or numbers surrounded by `<>`
- Pointers: $^*T'$
- Arrays: $T'\#I$, $I$ might be omitted to represent any size
 
Some operators offer similar behavior into the Numeric set 
$N = I \cup R \cup N\#I$.

Default operations are only defined for types given below:

| function | notation |
| :-: | :-: |
| `+`$^1$ | $N \to N$ |
| `-`$^1$ | $N \to N$ |
| `*`$^1$ | $*a' \to a'$ |
| `&`$^1$ | $a' \to *a'$ |
| `~`$^1$ | $N \to N$ |
| `!`$^1$ | $B \to B$ |
|||
| `->`$^2$| $a' \to b' \to (a' \to b')$ |
| `#`$^2$ | $a'[I] \to I \to a'$ |
| `.`$^2$ | $(a' \to b') \to (b' \to c') \to (a' \to c')$ |
| `\|`$^2$ | $I \to I \to I$ |
| `&`$^2$ | $I \to I \to I$ |
| `^`$^2$ | $I \to I \to I$ |
| `<<`$^2$ | $I \to I \to I$ |
| `>>`$^2$ | $I \to I \to I$ |
| `+`$^2$ | $N \to N \to N$ |
| `-`$^2$ | $N \to N \to N$ |
| `*`$^2$ | $N \to N \to N$ |
| `/`$^2$ | $N \to N \to N$ |
| `%`$^2$ | $I \to I \to I$ |
| `==`$^2$ | $a' \to a' \to B$ |
| `!=`$^2$ | $a' \to a' \to B$ |
| `>`$^2$ | $a' \to a' \to B$ |
| `>=`$^2$ | $a' \to a' \to B$ |
| `<`$^2$ | $a' \to a' \to B$ |
| `<=`$^2$ | $a' \to a' \to B$ |
| `\|\|`$^2$ | $B \to B \to B$ |
| `&&`$^2$ | $B \to B \to B$ |
| `\|>`$^2$ | $M a \to (a \to M b) \to M b $ |

> The nature of `|>` does not make default implementations for monadic
> structures

#### 2.2.1 Type Conversions

Using $\rhd$ (`|>`) where the binder is a type allows for type 
conversions that normally would error on the compiler. In case the type
of `x` is the same as used in the binder, the whole operation is a no-op.

Being $x_t$ the type of `x`, and $b$ the current binding type:

- $x \rhd Boolean$: defined for numeric and boolean types, 
equivalent to `x != 0`

- $x \rhd Integer$: defined for numeric and boolean types:
    - $x_t : Boolean$: works as `x ? 1 : 0`
    - $x_t : Rational$: truncates `x`
    - $x_t : Integer, x_t \supset b$: truncate `x`

- $x \rhd Rational$: defined for numeric and boolean types:
    - $x_t : Boolean$: `x ? 1.0 : 0.0`
    - $x_t : Integer$: `x * 1.0` without erroring out
    - $x_t : Rational, x_t \supset b$: truncate `x`

- $x \rhd *t'$: defined for when $x_t : *U$ or $U\#$, allows for pointer
aliasing

### 2.3. Induction and deduction rules

To allow more compiler freedom, the typing in every program should only
hint the analyzer about what the type might be, but any modifications
that may seem necessary can be done as long as they do not affect 
expected behavior. 

### 2.4 Overloading

Function overloading happens for all functions that have the same name 
but different signatures. Overloading is only invalid if an application 
with values of the same type has more than one possible outcome that
cannot be disambiguated from the type system. Example:

```
A : ...;
B : ...;
C : ...;
// Assume C and D are different, non-coercible types
D : ...;
E : C;


// "default implementation" 
f : A -> B -> C; 

// function signature changes, overload valid
f : A -> D; // overload 1

// C and D are different, validity is checked on the call site
f : A -> B -> D; // overload 2

// E coerces to C, as this is another declaration, the compiler fails as
// the function is not transparent anymore
f : A -> B -> E;
```

#### 2.4.1 Operator overloading

the special module name subspace "/@operators" allows the exporting of
operator functions into the parent namespace. To overload an operator,
