# pyrite
A crappy, garbage collected, statically typed, multi-paradigm programming language implementation.    
Pyrite source code is compiled to LLVM IR. At the moment, Pyrite is still a fairly incomplete language.

This is what a working Pyrite program looks like:
```swift
func chainCount(n:Int) -> Int
    if( n == 1 ) 0 
    else 1 + chainCount( if(n%2 == 0) (n/2) else (n*3 + 1) )

func collatz(min:Int, max:Int) -> [Int]
{
    let mNumCount = [0,0]
    for( curr_num in min..(max + 1) )
        mNumCount      = if( chainCount(curr_num) > mNumCount[1] ) [curr_num, chainCount(curr_num)] 
                         else mNumCount
   mNumCount
}

func main()
{
    let result = collatz(1, 1_000_000)
    println("Resulting number is " + result[0].str()
            + ", max count is " + result[1].str())
}
```

More examples are in the Parser/tests directory.

###Building/Running
This has just been tested on Ubuntu 14.04 and El Capitan.  
####Prerequisites
 0. llvm-config-3.9, lli-3.9, llvm-link-3.9, llvm-dis-3.9, opt-3.9
 0. OCaml
 0. clang/++ version 3.9.x
 
####To Build
 Run ```make```.
  
####To Run
 Create a Pyrite file, and go into the Parser directory and run:  
 ```sh run_test.sh path/to/your/file.it```  
 This will compile and execute the file.

##Loose Language Spec

####A Pyrite Program
All Pyrite programs require a main function with no arguments that returns void.
Pyrite programs cannot link to other Pyrite files at the moment. All scoping is lexical.
Statements are not terminated by semicolons.

####Types
There is currently no support for user-defined types.
 * Base - Base class
 * Bool : Base - Boolean (e.g. true)
 * Float : Base - Double precision floating point (e.g. 1.0)
 * Int : Base - 64 bit signed integer (e.g. 1)
 * List<T> : Base - List of objects (e.g. [1,2])
 * String : Base - ASCII string (e.g. "this is a string")
 * IntRange : Base - An iterable int generator (e.g. 0..100)
 * Void

####Functions
Functions must be declared at file scope.  
The prototype of a function looks like:    
```swift
func funcName(paramA:AType, paramB:BType) -> ReturnType
```
This is then followed by either a single statement or a block of statements.
If a function consists of only a single statement, then curly braces are optional.
If a function returns Void, then specifying that the return type is Void is optional.
There is no 'return' keyword and instead, the last statement in a function is used as the return value.

####Loops
#####A for loop
```swift
for(i in a)
```
Where 'a' is an IntRange type or a List type.
This is then followed by either a single statement or a block of statements.  
#####A while loop
```swift
while(cond)
```
Followed by a statement or a block.

####If-Else
#####The 'if' form
```swift
if(cond)
```
Followed by a statement or block.  
#####The 'if-else' form
```swift
if(cond)
  stmt
else
  stmt
```
If the value of a statement or a block in both the 'if' and 'else' bodies represents the same type,
then this form can be used as a value of that type. For example:
```swift
let boolToInt:Int = if(condition) 1 else 0
```
Is valid.

####List generators
```swift
let listy = [a for (a in 0..100)]
```
In the example above, listy would be a list of ints from 0 to 99.

####Variables
Variables are declared using the 'let' keyword.
The type annotation for a variable declaration is optional if the type can be inferred
from the right hand side of the declaration. If a variable is used before it is initialized,
then the result is currently undefined.
#####Declarations
```swift
let a = 1
let b:Int = 1
let c:Int
let d:[Int] = []
let e = [1]
```

####Operators
#####Binary
```swift
+, -, *, /, %, |, &, ^
```
#####Augmented assigns
```swift
+=, -=, *=, /=, %=, |=, &=, ^=
```
#####Unary
```swift
-
```
