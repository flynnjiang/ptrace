#include <string.h>

extern int ptrace_combine_main(int argc, char *argv[]);
extern int ptrace_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    if (strstr(argv[0], "combine"))
        return ptrace_combine_main(argc, argv);

    return ptrace_main(argc, argv);
}
