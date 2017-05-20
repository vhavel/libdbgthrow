#include <dlfcn.h>
#include <execinfo.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
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

struct OutputFileHolder
{
    OutputFileHolder()
    {
        out = stderr;
        char *out_filename_env = getenv("BACKTRACE_FILENAME");
        if (out_filename_env)
        {
            out = fopen(out_filename_env, "a");
            if (!out)
                out = stderr;
        }
    }

    ~OutputFileHolder()
    {
        if (out && out != stderr)
            fclose(out);
    }

    FILE *out;
};
OutputFileHolder g_output;

}

extern "C" void __cxa_throw (
        void *thrown_exception,
        std::type_info *tinfo,
        void (*dest)(void *))
{
    int dm_status;
    char* exp_demangled = abi::__cxa_demangle(tinfo->name(), 0, 0, &dm_status);
    if (dm_status == 0) {
        fprintf(g_output.out, "%s thrown at:\n", exp_demangled);
        free(exp_demangled);
    } else {
        fprintf(g_output.out, "%s thrown at:\n", tinfo->name());
    }

    size_t depth = 10;
    char *depth_env = getenv("BACKTRACE_DEPTH");
    if (depth_env)
        depth = std::max(1, atoi(depth_env));

    // make space for one more function (this one), which we ommit afterwards
    ++depth;

    void** bt_stack = (void**)malloc(depth * sizeof(void*));
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

            module_offset = symbol_str + last_closing_br + 1;

            fprintf(g_output.out, "  Module %s%s\n", module, module_offset);

            int status;
            char* demangled = abi::__cxa_demangle(function_mangled, 0, 0, &status);
            if (status == 0) {
                fprintf(g_output.out, "    %s\n", demangled);
                free(demangled);
            } else {
                fprintf(g_output.out, "    %s\n", function_mangled);
            }

        } else {
            fprintf(g_output.out, "%s\n", symbol_str);
        }
    }
    free(bt_stack);
    free(symbols);

    g_orig_cxa_throw.m_fptr(thrown_exception, tinfo, dest);
}

