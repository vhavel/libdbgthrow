#include <cstdio>
#include <cstdlib>
#include <cxxabi.h>
#include <execinfo.h>
#include <cstring>

#include <dlfcn.h>

#include "backtrace_print.h"

namespace libdbgthrow
{

void pretty_print_sym(FILE* out, const char* name)
{
    int dm_status;
    char* exp_demangled = abi::__cxa_demangle(name, 0, 0, &dm_status);
    if (dm_status == 0) {
        fputs(exp_demangled, out);
        free(exp_demangled);
    } else {
        fputs(name, out);
    }
}

void pretty_print_bt(FILE* out, void** bt_stack, size_t depth)
{
    for (size_t symbol_no = 0; symbol_no < depth; ++symbol_no) {
        void *symbol_addr = bt_stack[symbol_no];
        if (!symbol_addr)
            break;

        Dl_info dl_info;
        int res = dladdr(symbol_addr, &dl_info);
        
        fputs("  ", out);
        if (res == 0) {
            fprintf(out, "[%p]\n", symbol_addr);
            continue;
        }
        if (dl_info.dli_sname)
            pretty_print_sym(out, dl_info.dli_sname);
        fprintf(out, " (%s) [%p]\n", dl_info.dli_fname, dl_info.dli_saddr);
    }
    fputc('\n', out);
}
}

