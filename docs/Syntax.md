# Syntax

This document will at least try to summarize most of the references
so no one (even myself) will get lost while working.

We will try to summarize from the bottom up, so early on 

## Comments

Comments are tokenized-out so they aren't needed for the compiler to worry

they should work as:  

```ebnf

anything_not_newline = (* all possible characters except newline ones *)

anything = anything_not_newline | (* newlines *)

single_line_comment = "/" + "/" + { anything_not_newline } + '\n' ;

multiline_comment = "/" + "*" + { anything } + "*" + "/" ;
```

so basically

```zenith
// anything until the end of the line
```

or

```zenith
/*
 * literally anything between these braces 
 */
```

## Literals

literals come in various forms, for numbers we have:

```ebnf
binary_prefix = "0" + ( "b" | "B" )

octal_prefix = "0" + ( "o" | "O" )

decimal_prefix = ["0" + ("d" | "D") ]

hex_prefix = "0" +  ( "h" | "H" | "x" | "X" )

(* depending on the prefix the real number will change *)

binary_mantissa = { "0" | "1" }

octal_mantissa = { "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" }

decimal_mantissa = { "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" } 

hex_mantissa = { (* [0-9A-Fa-f] *) "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" | "a" | "b" | "c" | "d" | "e" | "f" | "A" | "B" | "C" | "D" | "E" | "F" }

prefix = binary_prefix | octal_prefix | decimal_prefix | hex_prefix

mantissa = binary_mantissa | octal_mantissa | decimal_mantissa | hex_mantissa

number = [ prefix ] + mantissa + [ "." + mantissa ] + 
         [ ( "e" | "E" ) + [ "-" ] + decimal_mantissa ]
``` 

