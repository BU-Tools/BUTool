#ifndef __PRINT_HH__
#define __PRINT_HH__
/*
 * Modified for use with BUTool from the work found here:
 * https://github.com/WingTillDie/CxxPrintf
*/

#include <cstdarg>
#include <iostream>
#include <cstdio>
#include <stdlib.h>
#include <stdio.h>

struct printer {
    char *s;
};

printer printerHelper(const char *fmt, va_list argp);

std::ostream& operator<<(std::ostream& os, printer p);

#endif
