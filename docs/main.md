# Zenith documentation

Just to note, this documentation is about the *language and its*
*environment*, not about the *compiler*.

Another note, because the language is still very new, these docs will
lack some features, and also have ones that will be or are being worked 
on, or even outdated features.

One point of thought that goes throughout the language development, was 
the use of as little of reserved tokens as possible, so higher level
constructs are always defined (and hopefully documented too) on code
and not somewhere else.

## Syntax
### Comments

Comments are the same as a lot of languages, either starting with `//`
and going until the end of line, or anything between `/*` and `*/`

### Literals
Literals are constant values written directly on the code, they can be:

- Numbers 
  - A normal integer: `12345`
  - A normal decimal: `2.71`
  - A prefixed (`0b`, `0o`, `0d`, `0x`, `0h`) integer: `0xBABA`
  - A decimal number multiplied by a power of a base `0d1e2` (`100`)
  - Also using underscores for separation `12_345`
- Strings
  - A text surrounded by quotes: `"right text"`
- Arrays
  - Values between braces separated by commas: `{ 0, 2, 0, 4 }`

#### Type Literals

Zenith does not define any type in the sense of a keyword like C does 
for `int` or `char`, some type aliases are actually defined on the
`Core/Types` module.

Types will be notated always in PascalCase, and if the value is known, 
it will be inside braces as in `TypeName{value}`.

Type aliasing happens when a type is simply assigned to other one, in 
the form of `Alias : Type`. 

Numeric types are ranges inclusive on both ends, with an optional step 
value between any element inside the range type, taking the form of
`Numeric : [ start ; end ; epsilon ]`. If omitted, epsilon is assumed to
be `1`, and if `0`, we assume the target hardware's decimal precision.

Aggregate types are ways to use more than one type together. There are 
two ways to do so, the first one, is making a list of possible types to
choose from, called "Sum Types", using the syntax `SType : T1 | T2 | Tn`.
And also other way of making a list, but instead using all the types 
provided in order, called "Product types"

Product types, aggregate data by using all the given types at once, 
allowing more complex structures to be tied up together. Every product
type needs a name assigned to it, making the compiler generate a
function to construct the type that way. To create one, the syntax is
`PType : ConstructorName { T1 T2 Tn }`

### Operations
#### Precedence

Operator precedence defines which operations to do first and which to do
last, so for example $1+2*3$ is $7$, and not $9$, because we do $1+(2*3)$
and not $(1+2)*3$. Operators with high precedence are grouped together
first than the lower ones, but ones with the same precedence levels are
grouped left-to-right, except `->`, which groups right-to-left.

- parenthesis
- - `(a)`, group an expression and allow lower precedence operators inside
- unary operations
- - `+a`, unary plus: does nothing, is there just for symmetry.
- - `-a`, unary minus: changes the arithmetic sign of the expression.
- - `!a`, unary bang: inverts the boolean result of the atomic expression.
- - `~a`, unary not: invert all bits on the number.
- - `*a`, unary star: dereference a value at `a`.
- - `&a`, unary and: get a reference of an expression
- call operation
- - `a₁ u₂`, apply `a₂` into `a₁`
- aggregate precedence:
- - `a₁.u₂`a binary dot: get member of aggregate value
- - `a₁ # a₂`, binary hash: index member of an array
- bitwise precedence:
- - `a₁ & a₂`, binary and: perform a bitwise and between two values
- - `a₁ | a₂`, binary or: perform a bitwise or between two values
- - `a₁ ^ a₂`, binary xor: perform a bitwise xor between two values
- shift precedence:
- - `a₁ >> a₂`, binary left shift: shifts a number to the left
- - `a₁ << a₂`, binary right shift: shifts a number to the right
- arithmetic precedence 1:
- - `a₁ * a₂`, binary star: multiplies numbers OR composes two functions
- - `a₁ / a₂`, binary slash: divides numbers
- - `a₁ % a₂`, binary modulo: remainder of a division
- arithmetic precedence 2:
- - `a₁ + a₂`, plus: sums numbers
- - `a₁ - a₂`, minus: subtracts numbers
- comparisons:
- - `a₁ == a₂`, compares if `a₁` and `a₂` are the same
- - `a₁ != a₂`, compares if `a₁` and `a₂` are different
- - `a₁ > a₂`,  compares if `a₁` is greater than `a₂`
- - `a₁ < a₂`,  compares if `a₁` is less than `a₂`
- - `a₁ >= a₂`, compares if `a₁` is not less than `a₂`
- - `a₁ <= a₂`, compares if `a₁` is not greater than `a₂`
- Logic:
- - `a && b`, gets the boolean and of two bools
- - `a || b`, gets the boolean or of two bools
- ternary:
- - `a₁ ? a₂ : a₃`, choose `a₂` if `a₁` results in `Boolean{true}`, else `a₃`

### Intermediate Expressions

Sometimes, it is possible to need to use the same value more than once
during a declaration, but still use only inside it. For that, the
language provides intermediate expressions, those are blocks which allow
incomplete expressions to be separated and written in a tidier form.
As an example: 
`tanh x = ((pow e x) - (pow e (-x)))/((pow e x) + (pow e (-x)));` could
be rewritten as:

```
tanh x = {
  ex = pow e x;
  enx = pow e (-x);
} (ex - enx)/(ex + enx);
```

Although this process is done already by the compiler somewhere down the
line, its a good visualization tool for who else might look at the code
later.

### Match Expressions

The first expression to use one (of two) existent keywords. Using a 
`match` is a way to branch code base on the pattern defined by specific
middle expression. Assuming we have the following type definition:
`Operation res err : Result { res } | Error { err };` and we want a
`stringOp : Operation -> String`. Because `res` and `err` are not 
required the same, we need to actually branch according, matching their
constructor:

```
stringOp op = match {
    Result res = join "Result: " (stringRes res)
    Error err = join "Error: " (stringErr err)
} op
```

## Modules

Zenith is based entirely on the idea of modules, most of what is used 
is imported from modules, and a function is only compiled if it is 
exported directly. Special modules are the way for the program to
communicate directly to the compiler, instrumenting as would with 
runtime flags.

### Importing and exporting

Normally, the first lines in a file will be from importing code to work
with, and the last ones will be exporting whatever was done.

To **import** a function that was exported with the name `func`, in the
namespace `nmsp`, with the name `fn`, the syntax is:

`module << "nmsp" { "fn" : func };`

To **export**, a function `f` into the namespace `nmsp` as `g`, the 
syntax slightly changes to:

`module >> "nmsp" { "g" : f };`

Different from some languages like C++ and C#, you can only work with
values that are in the current working namespace (the `module` keyword 
itself), so its useful to look to `>>` and `<<` as to where the value
comes from and goes to.

Modules can, and should, be nested, to organize and dedicate space to
correlate functionality. To nest modules, a filepath-like syntax is
used, so `Core/Types` defines basic types that most operations use, 
while `Core/Functions` defines some useful functions for applications.

### Special modules

Special modules are namespaces that the compiler expects some values to
be exported or some values outside the closed scope of the code

The main function, should be sent to the `Core/Init` namespace.
the name exported should not matter, as only one function can exist in 
there, else the compiler will throw an error.

## Types 
### Constructors 

For sum and product types, a constructor is used to generate the 
abstract type from simpler ones. This functionality is one of the few
that the compiler is going to generate for you and not a thing you will
do yourself. Lets take for example the `Rational` number: 

```
Rational : Frac { Int Int };
```

After the type declaration, the function `Frac : Int -> Int -> Rational`
is going to become available inside `module`. The given constructor for
every type is trivial for its functionality, it just returns the needed
value given its members.

### Cast Types

Cast Types allow greater generalization of the code as it (somewhat) 
removes the type constraint for every function.

For example, the `id` function is defined as `id x = x;`. The value of
`x` here could be absolutely anything, so its not up to `id` to 
determinate what constraint to put. So we end up with `id` being typed
as `id x : a' -> a' = x`. Constraint types should use at least one tick
`'` to avoid name collisions. 