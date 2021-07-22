# Overview of B-Minor 2020

This is an informal specification of B-Minor 2020, a C-like language for use in an undergraduate compilers class. B-minor includes expressions, basic control flow, recursive functions, and strict type checking. It is object-code compatible with ordinary C and thus can take advantage of the standard C library, within its defined types. It is similar enough to C to feel familiar, but different enough to give you some sense of alternatives.

This document is deliberately informal: the most precise specification of a language is the compiler itself, and it is your job to write the compiler! It is your job to read the document carefully and extract a formal specification. You will certainly find elements that are unclear or incompletely specified, and you are encouraged to raise questions in class.

Note that the definition of B-Minor changes slightly for each ND class. Changes from the [textbook](http://compilerbook.org) definition are **bolded**.

## Whitespace and Comments

In B-minor, whitespace is any combination of the following characters: tabs, spaces, linefeed (`\n`), and carriage return (`\r`). The placement of whitespace is not significant in B-minor. Both C-style and C++-style comments are valid in B-minor:

```
* A C-style comment */
a=5; // A C++ style comment
```

## Identifiers

Identifiers (i.e. variable and function names) may contain letters, numbers, and underscores and may be up to 256 characters long. Identifiers must begin with a letter or an underscore. These are examples of valid identifiers:

```
i x mystr fog123 BigLongName55
```

The following strings are B-minor keywords and may not be used as identifiers:

```
array boolean char else false for function if integer print return string true void while
```

## Types

B-minor has four atomic types: integers, booleans, characters, and strings. A variable is declared as a name followed by a colon, then a type and an optional initializer. For example:

```
x: integer;
y: integer = -123;
b: boolean = false;
c: char    = 'q';
s: string  = "hello bminor\n";
```

An `integer` is a signed 64 bit value **with an optional leading plus or minus.**

`boolean` can take the literal values `true` or `false`.

`char` is a single 8-bit ASCII character in single quotes.

`string` is a double-quoted constant string that is null-terminated and cannot be modified.

Both `char` and `string` may contain the following backslash codes. `\n` indicates a linefeed (ASCII value 10), `\0` indicates a null (ASCII value zero), and a backslash followed by anything else indicates exactly the following character. **Both strings and identifiers may be up to 256 characters long.**

B-minor also supports arrays of a fixed size. They may be declared with no value, which causes them to contain all zeros:

```
a: array [5] integer;
```

Or, the entire array may be given specific values:

```
a: array [5] integer = {1,2,3,4,5};
```

```
months: array [12] string = {"January","February","March",...};
```

## Expressions

B-minor has many of the arithmetic operators found in C, with the same meaning and level of precedence:

| operators       | description                              | precedence |
|-----------------|------------------------------------------|------------|
| () [] f()       | grouping, array subscript, function call | highest    |
| ++ --           | postfix increment, decrement             |            |
| - !             | unary negation, logical not              |            |
| ^               | exponentiation                           |            |
| * / %           | multiplication, division, modulus        |            |
| + -             | addition, subtraction                    |            |
| < <= > >= == != | comparison                               |            |
| &&              | logical and                              |            |
| \|\|            | logical or                               |            |
| =               | assignment                               | lowest     |


B-minor is *strictly typed*. This means that you may only assign a value to a variable (or function parameter) if the types match *exactly*. You cannot perform many of the fast-and-loose conversions found in C.

Following are examples of some (but not all) type errors:

```
x: integer = 65;
y: char = 'A';
if(x>y) ... // error: x and y are of different types!

f: integer = 0;
if(f) ...      // error: f is not a boolean!

writechar: function void ( char c );
a: integer = 65;
writechar(a);  // error: a is not a char!

b: array [2] boolean = {true,false};
x: integer = 0;
x = b[0];      // error: x is not a boolean!
```

Following are some (but not all) examples of correct type assignments:

```
b: boolean;
x: integer = 3;
y: integer = 5;
b = x<y;     // ok: the expression x<y is boolean

f: integer = 0;
if(f==0) ...    // ok: f==0 is a boolean expression

c: char = 'a';
if(c=='a') ...  // ok: c and 'a' are both chars
```

## Declarations and Statements

In B-minor, you may declare global variables with optional constant initializers, function prototypes, and function definitions. Within functions, you may declare local variables (including arrays) with optional initialization expressions. Scoping rules are identical to C. Function definitions may not be nested.

Within functions, basic statements may be arithmetic expressions, return statements, print statements, if and if-else statements, for loops, or code within inner { } groups. B-minor does not have switch statements, while-loops, or do-while loops.

The `print` statement is a little unusual because it is a statement and not a function call. `print` takes a list of expressions separated by commas, and prints each out to the console, like this:

```
print "The temperature is: ", temp, " degrees\n";
```

## Functions

Functions are declared in the same way as variables, except giving a type of `function` followed by the return type, arguments, and code:

```
square: function integer ( x: integer ) = {
	return x^2;
}
```

The return type must be one of the four atomic types, or `void` to indicate no type. Function arguments may be of any type. `integer`, `boolean`, and `char` arguments are passed by value, while `string` and `array` arguments are passed by reference. As in C, arrays passed by reference have an indeterminate size, and so the length is typically passed as an extra argument:

```
printarray: function void ( a: array [] integer, size: integer ) = {
	i: integer;
	for( i=0;i<size;i++) {
		print a[i], "\n";
	}
}

```

A function prototype may be given, which states the existence and type of the function, but includes no code. This must be done if the user wishes to call an external function linked by another library. For example, to invoke the C function `puts`:

```
puts: function void ( s: string );

main: function integer () = {
	puts("hello world");
}
```

A complete program must have a `main` function that returns an integer. the arguments to `main` may either be empty, or use `argc` and `argv` as in C. (The declaration of `argc` and `argv` is left as an exercise to the reader.)
