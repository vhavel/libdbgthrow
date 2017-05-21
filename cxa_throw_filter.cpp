#include <dlfcn.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cxxabi.h>
#include <execinfo.h>

#include <algorithm>

#include "backtrace_print.h"

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
        char *out_filename_env = getenv("DBGTHROW_BACKTRACE_FILENAME");
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

struct ExceptFilter
{
    ExceptFilter()
    {
        pattern = getenv("DBGTHROW_EXCEPT_PATTERN");
    }

    bool match(const char* s) const
    {
        return !pattern || strcmp(pattern, s) == 0;
    }

    char* pattern;
};
ExceptFilter g_filter;

}

extern "C" void __cxa_throw (
        void *thrown_exception,
        std::type_info *tinfo,
        void (*dest)(void *))
{
    int dm_status;
    char* exp_name_demangled = abi::__cxa_demangle(tinfo->name(), 0, 0, &dm_status);
    const char* exp_name = exp_name_demangled;
    if (dm_status != 0) {
        exp_name = tinfo->name();
    }

    if (g_filter.match(exp_name)) {
        pretty_print_sym(g_output.out, tinfo->name());
        fputs(" thrown at:\n", g_output.out);

        size_t depth = 10;
        char *depth_env = getenv("DBGTHROW_BACKTRACE_DEPTH");
        if (depth_env)
            depth = std::max(1, atoi(depth_env));
        // make space for one more function (this one), which we ommit afterwards
        ++depth;

        if (depth > 64) {
            depth = 64;
        }
        void* bt_stack[64] = {};

        int size = backtrace(bt_stack, depth);
        pretty_print_bt(g_output.out, bt_stack+1, std::min((size_t)size-1, depth-1));
    }

    free(exp_name_demangled);

    g_orig_cxa_throw.m_fptr(thrown_exception, tinfo, dest);
}

