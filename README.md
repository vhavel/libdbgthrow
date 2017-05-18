libdbgthrow
==============

[![Build Status](https://travis-ci.org/vjhl/libdbgthrow.svg?branch=master)](https://travis-ci.org/vjhl/libdbgthrow)

An exception debugging library for C++ programs. Prints backtrace of each
exception thrown.

Example
-------

```
$ make
$ LD_PRELOAD=./libdbgthrow.so ./your_binary
```

```
std::runtime_error thrown at:
./your_binary	f()+0x30	 [0x400bd6]
./your_binary	main+0x9	 [0x400bf4]
/lib/x86_64-linux-gnu/libc.so.6	__libc_start_main+0xf0	 [0x7fe883ca4830]
./your_binary	_start+0x29	 [0x400ad9]
```

You may need to instruct your linker to add all symbols to the dynamic symbols table
otherwise you won't see any useful symbol names in the output (gcc hint: `-rdynamic`).
Alternatively, use the `add2name` utility for address-to-name translation

```
$ addr2line -e ./your_binary 0x400bd6
/home/user/your_source.cpp:7 (discriminator 2)
```

The number of lines is controlled by the  `BACKTRACE_DEPTH` environment variable, the library
prints 10 lines of stack by default.


