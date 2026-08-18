#pragma once

#define _SC_PAGESIZE 30

inline long sysconf(int) { return 4096; }

