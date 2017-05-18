#include <dlfcn.h>
#include <execinfo.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <iostream>
#include <cxxabi.h>
#include <cstring>

namespace {

struct OrigCxaThrowRoutine
{
    OrigCxaThrowRoutine()
    {
        m_fptr = (cxa_throw_type) dlsym(RTLD_NEXT, "__cxa_throw");
    }

    typedef void (*cxa_throw_type)(void *, void *, void (*) (void *));
    cxa_throw_type m_fptr;
};
OrigCxaThrowRoutine g_orig_cxa_throw;

}

extern "C" void __cxa_throw (
        void *thrown_exception,
        std::type_info *tinfo,
        void (*dest)(void *))
{

    int dm_status;
    char* exp_demangled = abi::__cxa_demangle(tinfo->name(), 0, 0, &dm_status);
    if (dm_status == 0) {
        std::cerr << exp_demangled << " thrown at:\n";
        free(exp_demangled);
    } else
        std::cerr << tinfo->name() << " thrown at:\n";
    
    size_t depth = 10;
    char *depth_env = getenv("BACKTRACE_DEPTH");
    if (depth_env)
        depth = std::max(1, atoi(depth_env));

    // make space for one more function (this one), which we ommit afterwards
    ++depth;

    std::vector<void*> bt_stack(depth);
    int size = backtrace(bt_stack.data(), depth);
    char **symbols = backtrace_symbols(bt_stack.data()+1, std::min((size_t)size-1, depth-1));
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
            std::string module;
            std::string function_mangled;
            std::string function_offset;
            std::string module_offset;

            module.assign(symbol_str, symbol_str + opening_br );
            function_mangled.assign(symbol_str + opening_br + 1, symbol_str + last_plus);
            function_offset.assign(symbol_str + last_plus + 1, symbol_str + last_closing_br);
            module_offset.assign(symbol_str + last_closing_br + 1);

            std::cerr << "  Module " << module << module_offset << '\n';
            std::cerr << "    ";

            int status;
            char* demangled = abi::__cxa_demangle(function_mangled.c_str(), 0, 0, &status);
            if (status == 0)
            {
                std::cerr << demangled;
                free(demangled);
            }
            else
            {
                std::cerr << function_mangled;
            }

            std::cerr << std::endl;

        } else {
            std::cerr << symbol_str << std::endl;
        }



    }
    free(symbols);

    g_orig_cxa_throw.m_fptr(thrown_exception, tinfo, dest);
}

