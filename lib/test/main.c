#include <stdio.h>
#include <unistd.h>

#define ENABLE_PTRACE
#include "ptrace.h"

void func_b(void)
{
    PTRACE_FUNC();

    usleep(500);
}

void func_c(void)
{
    PTRACE_FUNC();

    usleep(1000);
}

void func_a(unsigned num)
{
    PTRACE_FUNC_U32("num", num);

    func_c();
    usleep(500);
}

int main(int argc, char *argv[])
{
    PTRACE_INIT();

    unsigned i;
    for (i = 0; i < 10; i++) {
        PTRACE_SCOPE_I32("main_loop", "i", i);
        PTRACE_COUNTER_I32("counter_i", i);

        func_a(i);
        func_b();
    }

    return 0;
}
