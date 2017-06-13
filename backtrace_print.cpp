#include <cstdio>
#include <cstdlib>
#include <cxxabi.h>
#include <execinfo.h>
#include <cstring>

#include <dlfcn.h>
#include <bfd.h>

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

namespace
{
    class SymbolDebugInfo
    {
    public:
        SymbolDebugInfo(const char* module)
            : m_bfd_file(NULL)
            , m_syms(NULL)
            , m_syms_size(0)
        {
            bfd_init();

            m_bfd_file = bfd_openr(module, NULL);
            if (!m_bfd_file)
                return;

            if (bfd_check_format(m_bfd_file, bfd_object))
                return;

            flagword file_flags = bfd_get_file_flags(m_bfd_file);
            if ((file_flags & HAS_SYMS) == 0)
                return;

            long symcount = bfd_read_minisymbols(m_bfd_file, false,
                    (void**)&m_syms, &m_syms_size);

            if (symcount == 0) {
                symcount = bfd_read_minisymbols(m_bfd_file, true,
                        (void**)&m_syms, &m_syms_size);
            }
            if (symcount < 0)
                return;
        }

        struct sym_info {
            bfd_vma pc;
            const char *filename;
            const char *functionname;
            unsigned int line;
            const SymbolDebugInfo* ctx;
        };

        static void find_address_in_section(bfd* abfd, asection* section, void* data)
        {
            sym_info* psi = (sym_info*)data;
            const SymbolDebugInfo* this_ = psi->ctx;

            if (psi->filename)
                return;

            bfd_vma vma;
            bfd_size_type size;

            if ((bfd_get_section_flags(abfd, section) & SEC_ALLOC) == 0)
                return;

            vma = bfd_get_section_vma(abfd, section);
            if (psi->pc < vma)
                return;

            size = bfd_get_section_size(section);
            if (psi->pc >= vma + size)
                return;

            bfd_find_nearest_line(abfd, section, this_->m_syms,
                psi->pc - vma, &psi->filename, &psi->functionname, &psi->line);
        }

        sym_info translate(void* addr) const
        {
            sym_info si = {};
            char addr_str[16];
            sprintf(addr_str, "%p", addr);
            si.pc = bfd_scan_vma(addr_str, NULL, 16);

            si.ctx = this;
            bfd_map_over_sections(m_bfd_file, find_address_in_section, &si);
            return si;
        }

        ~SymbolDebugInfo()
        {
            if (m_syms) {
                free(m_syms);
                m_syms = NULL;
            }

            if (m_bfd_file) {
                bfd_close(m_bfd_file);
                m_bfd_file = NULL;
            }
        }

    private:
        bfd* m_bfd_file;
        asymbol** m_syms;
        unsigned int m_syms_size;
    };
}

void pretty_print_bt(FILE* out, void** bt_stack, size_t depth)
{
    for (size_t symbol_no = 0; symbol_no < depth; ++symbol_no) {
        void *symbol_addr = bt_stack[symbol_no];
        if (!symbol_addr)
            break;

        Dl_info dl_info;
        int res = dladdr(symbol_addr, &dl_info);

        if (res == 0) {
            fputs("  ", out);
            fprintf(out, "[%p]\n", symbol_addr);
            continue;
        }

        SymbolDebugInfo dbg_info = SymbolDebugInfo(dl_info.dli_fname);
        SymbolDebugInfo::sym_info si = dbg_info.translate(symbol_addr);

        fputs("  ", out);
        if (!si.functionname) {
            // no debug info?
            if (dl_info.dli_sname)
                pretty_print_sym(out, dl_info.dli_sname);
            else
                fputs("??", out);
            fprintf(out, " (%s [%p])\n", dl_info.dli_fname, dl_info.dli_saddr);
        } else {
            pretty_print_sym(out, si.functionname);
            if (si.filename)
                fprintf(out, " at %s:%d", si.filename, si.line);
            fprintf(out, " (%s)\n", dl_info.dli_fname);
        }
    }
    fputc('\n', out);
}
}

