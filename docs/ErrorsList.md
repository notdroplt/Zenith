# Error list

this file lists simple compiler errors (those that do not need any given context, at most the lines where they happen)

## summary:

- errors Z0000 - Z00FF: internal errors
    - [Error Z0000: Memory Allocation Error](#Z0000-memory-allocation-error)
    - [Error Z0001: File not found](#Z0001-File-Not-Found)
    - [Error Z0002: ]
- errors Z0100 - Z02FF: tokenizing / parsing errors
- errors Z0300 - Z06FF: syntax errors
- errors Z0700 - Z08FF: compil
----

## Z000 Memory Allocation Error

this is an internal compiler error, thrown when `malloc` fails, there is not much there to do about it

### solution

remove some load from the computer and start the compiler again

----

## Z0001 File not found

this error will always return as `Z0001:filename`, giving the file the compiler wasn't able to find

### solution

check for any typos on the file name or sync the hard drive and run the compiler again

----