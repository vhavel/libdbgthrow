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
  f() at /home/vojta/libdbgthrow/test/test1.cpp:6 (./test1)
  main at /home/vojta/libdbgthrow/test/test1.cpp:15 (./test1)
  __libc_start_main (/lib/x86_64-linux-gnu/libc.so.6 [0x7fcd4061f740])
  ?? (./test1 [(nil)])
```


Configuration
-------------

| Environment Variable        | Default  |
| ----------------------------|----------:
| `DBGTHROW_BACKTRACE_DEPTH`  | 10       |
| `DBGTHROW_OUTPUT_FILENAME`  | `stderr` |
| `DBGTHROW_EXCEPT_PATTERN`   | <all>    |

