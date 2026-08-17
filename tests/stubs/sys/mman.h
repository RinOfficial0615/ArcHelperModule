#pragma once

#include <cstddef>

#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4

inline int mprotect(void *, std::size_t, int) { return 0; }

