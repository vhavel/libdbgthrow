#pragma once

namespace libdbgthrow
{
void pretty_print_bt(FILE* out, void** bt, size_t depth);
void pretty_print_sym(FILE* out, const char* name);
}
