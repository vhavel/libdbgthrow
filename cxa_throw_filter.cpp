#include <dlfcn.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cxxabi.h>

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
    pretty_print_sym(g_output.out, tinfo->name());
    fputs(" thrown at:\n", g_output.out);

    size_t depth = 10;
    char *depth_env = getenv("BACKTRACE_DEPTH");
    if (depth_env)
        depth = std::max(1, atoi(depth_env));
    // make space for one more function (this one), which we ommit afterwards
    ++depth;

    pretty_print_bt(g_output.out, depth);

    g_orig_cxa_throw.m_fptr(thrown_exception, tinfo, dest);
}

