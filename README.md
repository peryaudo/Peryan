![Peryan Logo](./peryan.png)

## About Peryan

Sieve of Erathosthenes written in Peryan (enumerates prime numbers less than 1000)

    var length = 1000
    
    var table = [Bool](length, true)
    
    table[0] = false
    table[1] = false
    
    repeat length
    	if table[cnt] == false : continue
    
    	var cnt_ = cnt
    	var cur = cnt_ + cnt_
    
    	repeat
    		cur += cnt_
    		if !(cur < length) : break
    
    		table[cur] = false
    	loop
    loop
    
    repeat length
    	if table[cnt] : mes String(cnt)
    loop

Peryan is a programming language which has following goals:

* Static typing and static compiling using LLVM
* Modern language features including native OOP support
* Compatibility with HSP

Peryan aims to be a highly useful language thourgh accomplishing these goals.

## Compile the Language Processor

Peryan currently supports POSIX and Windows and the development is done on OSX and Windows 10.

The language processor is written in C++ only using STL, and requires LLVM 3.8 to compile.
You have to install LLVM through your disribution's package manager (Linux) or brew install llvm (OSX).

    make
    make test

On Windows, it only support Visual C++ 2012. See build/win32 for detail.

## Compile Peryan Program (POSIX)

    cd build/unix
    make
    ./bin/peryan --runtime-path . ../../test/integration/cases/Sieve.pr -o sieve
    ./sieve

## Syntax Overview

### Function Definition

    func thisIsFunction (arg1 :: Int, arg2 :: Double, arg3 :: String) :: Int {
    	thisIsStatement
    	return 0
    }

You can omit type declarations using type inference:

    func myAbs (x) {
    	if x > 0 {
    		return x
    	} else {
    		return -x
    	}
    }
    mes String(myAbs(-1234))

### Variable Definition

    var thisIsVariable :: Int = 123

You can also omit type declarations as far as the type inference is possible.

    var thisIsVariable = 123

## Future Work

Peryan is on the early stage of the development and still has many important features unimplemented.

Following is features are planned to implement. The detailed document is also planned.

* OOP
* variant type
* Pattern matching
* Closure
* Partial application
* Guard
* HSP compatible features
* Runtime library
* Coroutine

## About the code of the language processor

The language processor's code is of bad quality and has many defects including never freed memory and segfaults.
I would appreciate your advice to @peryaudo.

## License

Peryan's code is under MIT License. See LICENSE.
Google Test under test/unit/gtest/ is under new BSD liense. See test/unit/gtest/COPYING.

## References

* 柏木 餅子, 風薬 『きつねさんでもわかるLLVM　〜コンパイラを自作するためのガイドブック〜』 2013年
* Terence Parr "Language Implementation Patterns" 2011
* Benjamin C. Pierce "Types and Programming Languages" 2013
* Benjamin C. Pierce, David N. Turner "Local Type Inference" ACM TOPLAS Vol. 22 No.1 Jan. 2000


