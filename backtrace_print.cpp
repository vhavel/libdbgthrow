#include <cstdio>
#include <cstdlib>
#include <cxxabi.h>
#include <execinfo.h>
#include <cstring>

#include <algorithm>

#include "backtrace_print.h"

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

void pretty_print_bt(FILE* out, size_t depth)
{
    if (depth > 64) {
        depth = 64;
    }
    void* bt_stack[64] = {};

    int size = backtrace(bt_stack, depth);
    char **symbols = backtrace_symbols(bt_stack+1, std::min((size_t)size-1, depth-1));
    for (int symbol_no = 0; symbol_no < size-1; ++symbol_no) {
        char *symbol_str = symbols[symbol_no];
        if (!symbol_str)
            break;

        size_t s_len = strlen(symbol_str);
        size_t last_closing_br = 0;
        for (size_t i = 0; i < s_len; ++i) {
            if (symbol_str[i] == ')')
                last_closing_br = i;
        }
        size_t last_plus = 0;
        for (size_t i = last_closing_br; i > 0; --i) {
            if (symbol_str[i] == '+') {
                last_plus = i;
                break;
            }
        }

        size_t opening_br = 0;
        for (size_t i = last_plus; i > 0; --i) {
            if (symbol_str[i] == '(') {
                opening_br = i;
                break;
            }
        }

        if (opening_br > 0) {
            char* module;
            char* function_mangled;
            char* function_offset;
            char* module_offset;

            module = symbol_str;
            symbol_str[opening_br] = '\0';
 
            function_mangled = symbol_str + opening_br + 1;
            symbol_str[last_plus] = '\0';

            function_offset = symbol_str + last_plus + 1;
            symbol_str[last_closing_br] = '\0';

            module_offset = symbol_str + last_closing_br + 2;

            fputs("  ", out);
            pretty_print_sym(out, function_mangled);
            fprintf(out, " (%s %s)\n", module, module_offset);

        } else {
            fprintf(out, "%s\n", symbol_str);
        }
    }
    fputc('\n', out);

    free(symbols);
}


