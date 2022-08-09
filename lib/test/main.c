#include <stdio.h>
#include <unistd.h>

#define ENABLE_PTRACE
#include "ptrace.h"

void test(void)
{
    PTRACE_FUNC();

    printf("test1\n");
}

int main(int argc, char *argv[])
{
    int i;

    PTRACE_INIT();

    test();

    for (i = 0; i < 100; i++) {
        PTRACE_SCOPE_I32("loop", "i", i);
        PTRACE_COUNTER_I32("i", i);
        usleep(1000);
    }

    return 0;
}
