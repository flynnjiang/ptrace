# Introduction

**PTrace** is a tracking tool based on **Perfetto**.

Its implementation references **vperfetto-min**, which provides the following
components and functions:

* **libptrace** - a C wrapper library for perfetto SDK
* **ptrace** - a wrapper of the `perfetto` command, which provides two tracking
  modes:
  - Single host tracing
  - Dual host tracing
* **ptrace-combine** - For combining two trace files

# Source

This repository lives at https://github.com/flynnjiang/ptrace.
Other repositories are likely forks, and code found there is not supported.

# Build

1. Install dependencies

On Fedora/CentOS:

```shell
sudo yum install protobuf-c protobuf-devel
```
On Debian/Ubuntu:

```shell
sudo apt-get install protobuf-compiler libprotobuf-dev
```

2. Build

```shell
meson setup build
cd build
ninja
ninja install
```

# Usage

## Add trace points

```c
#include <ptrace.h>

void test(void)
{
    PTRACE_FUNC(); /* Add a trace point */

    /* ... */
}

int main(void)
{
    PTRACE_INIT(); /* Initial */

    test();

    return 0;
}
```

Note: compile with `-DENABLE_PTRACE`  and link with `-lptrace` .

## Single Host Tracking

1. Start `traced` ( and `trace_probes`)
2. Run command: `ptrace`
3. Start the application to be tracked
4. Press `CTRL+C` to stop the tracing

## Dual Host Tracking

1. Start `traced` ( and `trace_probes`) on all hosts
2. On host #1 (as server), run command `ptrace -m server`
3. On host #2 (as client), run command `ptrace -m client -s <server IP addr>` or
   `ptrace -s <server IP addr>`
4. Start the application to be tracked on all hosts
5. Press `CTRL+C` on host #2 (as client) to stop the tracing

# Other

Base on Perfetto SDK V22.0

