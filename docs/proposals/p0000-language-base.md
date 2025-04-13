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
- `digit` be characters `0` through `9`.
- `letter` be characters `A` through `Z`, and `a` through `z`.
- `character` be any printable ascii character.
- `newline` be characters `\r`, `\n` or a combination of both

```ebnf
Int       = digit { digit } ;
Float     = [ digit { digit } ] [ "."  digit { digit } ] [ ("e" | "E") ["+" | "-"] digit { digit } ] ;
String    = '"' { character - '"' } '"' ;
ID        = ( letter | "_" ) { letter | digit | "_" | "'" } ;

Comment   = "//" { (character) - newline } newline ;
```

### 1.2. Grammar

```ebnf
Module   = "module" ("<<" | ">>") String Intr ;
Expr     = Ternary | Binary | Unary | Primary;
Formula  = ID { Primary } [":" Type] ["=" Expr] ;
Ternary  = Binary "?" Binary ":" Ternary ;
Binary   = [ Binary BinaryOp ] Unary ;
Unary    = UnaryOp ( Unary | Call ) ;
Call     = ( Call | Primary ) [ Primary ] ;
Primary  = Int | Float | String | ID | "(" Expr ")" | Match | Intr Primary;

Intr     = "{" { Formula } "}" ;
IntrApp  = Intr Primary ;
Match    = "match" IntrApp ;

Range    = "[" expr ";" expr [ ";" expr ] "]" ;
Product  = ID [ Intr ] ;
TPrimary = Range | Product | "(" Type ")" | "?" Primary;
TUnary   = "*" ( TUnary | TPrimary ) ;
TArray   = ( TArray | TUnary ) [ "#" Primary ] ;
TBinary  = TArray [ "->" TBinary ] ;
Type     = TBinary [ "|" Type ] ;

BinaryOp = "#" | "." | "|" | "&" | "^" | "<<" | ">>" 
         | "*" | "/" | "+" | "-" | "%" | "==" | "!=" 
         | "<" | ">" | "<=" | ">=" | "&&" | "||" ;
UnaryOp  = "+" | "-" | "~" | "!" | "*" | "&" ;

Program = { ( Module | Formula | Comment ) ";" };
```

## 2. Typing

There are only the following primitives: $$C = \{ Bool^0, Integer^0, 
Rational^0, Pointer^1, Array^1, \to^2, Aggregate^n \}$$

> Integers and Rationals are taken as nullary types as they dont take
> another types as parameters, but can take castings and compile time 
> numeric values.

### 2.1. Available Types
#### 2.1.1. Booleans

Booleans are already bounded by definition, any specialization is 
isomorphic to a unit type.

#### 2.1.2. Integers

All integers are in the form `[start, end]{value}`, with 
$a,b \in \mathbb{Z}, a \le value \le b $, and specialized values may 
occur. Coercion is only implicit from `IntA` to `IntB` if the size of
`IntA` is smaller or equal to `IntB`, and both integers have the same
sign.

#### 2.1.3 Rationals

All rational types are written in the same form as integers,
but instead of being a subset of $\mathbb{Z}$, be now a subset of
$\mathbb{Q}$.

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

Types sets are given from now on as follows:
- Booleans: $B$
- Integers: $I$
- Rationals: $R$
- Products: $P$
- Sums: $S$
- Castings: any letter followed by `'`
- Pointers: $^*T'$
- Arrays: $T'[I]$
 
Some operators offer like behavior into the Numeric set 
$N = I \cup R \cup N[I]$, which offer less redundancy to write in the
documentation

All operations are well defined for types given below:

$$
\begin{align*}
    +^1 &= N \to N \\
    -^1 &= N \to N \\
    *^1 &= *a' \to a' \\
    \&^1 &= a' \to *a' \\
    \sim^1 &= N \to N \\
    !^1 &= B \to B \\
    \#^2 &= a'[I] \to I \to a' \\
    .^2 &= (a' \to b') \to (b' \to c') \to (a' \to c') \\
    |^2 &= I \to I \to I \\
    \&^2 &= I \to I \to I \\
    \wedge^2 &= I \to I \to I \\
    |^2 &= I \to I \to I \\
    \ll^2 &= I \to I \to I \\
    \gg^2 &= I \to I \to I \\
    +^2 &= N \to N \to N \\
    -^2 &= N \to N \to N \\
    *^2 &= N \to N \to N \\
    /^2 &= N \to N \to N \\
    \%^2 &= I \to I \to I \\
    ==^2 &= a' \to a' \to B \\
    !=^2 &= a' \to a' \to B \\
    >^2 &= a' \to a' \to B \\
    >=^2 &= a' \to a' \to B \\
    <^2 &= a' \to a' \to B \\
    <=^2 &= a' \to a' \to B \\
    ||^2 &= B \to B \to B \\
    \&\&^2 &= B \to B \to B
\end{align*}
$$

### 2.3. Induction and deduction rules

To allow more compiler freedom, the typing in every program should only
hint the analyzer about what the type might be, but any restrictions or
relaxations that may seem necessary can be done as long as they do not
affect the expected behavior.


