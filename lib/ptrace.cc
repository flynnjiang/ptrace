#include <stdio.h>

#include "perfetto.h"

#define ENABLE_PTRACE
#include "ptrace.h"

#define _STR(s) #s
#define STR(s)  _STR(s)

#define PTRACE_DEFINE_BEGIN_FUNC(cat) \
_PTRACE_BEGIN_FUNC(cat) \
{\
    TRACE_EVENT_BEGIN(#cat, ::perfetto::StaticString{name});\
    return name;\
}

#define PTRACE_DEFINE_BEGIN_FUNC_1(cat, type) \
_PTRACE_BEGIN_FUNC_1(cat, type) \
{\
    TRACE_EVENT_BEGIN(#cat, ::perfetto::StaticString{name}, arg, val);\
    return name;\
}

#define PTRACE_DEFINE_BEGIN_FUNC_2(cat, type1, type2) \
_PTRACE_BEGIN_FUNC_2(cat, type1, type2) \
{\
    TRACE_EVENT_BEGIN(#cat, ::perfetto::StaticString{name}, \
                      arg1, val1, arg2, val2);\
    return name;\
}

#define PTRACE_DEFINE_END_FUNC(cat) \
_PTRACE_END_FUNC(cat) \
{\
    (void)dummy; \
    TRACE_EVENT_END(#cat);\
}

#define PTRACE_DEFINE_COUNTER_FUNC(cat, type) \
_PTRACE_COUNTER_FUNC(cat, type) \
{\
    TRACE_COUNTER(#cat, track, val);\
}

#define PTRACE_DEFINE_FUNCS(cat)                \
    PTRACE_DEFINE_BEGIN_FUNC(cat)               \
    PTRACE_DEFINE_BEGIN_FUNC_1(cat, u32)        \
    PTRACE_DEFINE_BEGIN_FUNC_1(cat, i32)        \
    PTRACE_DEFINE_BEGIN_FUNC_1(cat, u64)        \
    PTRACE_DEFINE_BEGIN_FUNC_1(cat, i64)        \
    PTRACE_DEFINE_BEGIN_FUNC_1(cat, str)        \
    PTRACE_DEFINE_BEGIN_FUNC_2(cat, i32, i32)   \
    PTRACE_DEFINE_BEGIN_FUNC_2(cat, i32, u32)   \
    PTRACE_DEFINE_BEGIN_FUNC_2(cat, u32, u32)   \
    PTRACE_DEFINE_END_FUNC(cat)                 \
    PTRACE_DEFINE_COUNTER_FUNC(cat, u32)        \
    PTRACE_DEFINE_COUNTER_FUNC(cat, i32)        \
    PTRACE_DEFINE_COUNTER_FUNC(cat, u64)        \
    PTRACE_DEFINE_COUNTER_FUNC(cat, i64)        \
    PTRACE_DEFINE_COUNTER_FUNC(cat, float)      \
    PTRACE_DEFINE_COUNTER_FUNC(cat, double)

PERFETTO_DEFINE_CATEGORIES(
    ::perfetto::Category(STR(PTRACE_CAT0)).SetDescription("PTrace category 0"),
    ::perfetto::Category(STR(PTRACE_CAT1)).SetDescription("PTrace category 1"),
    ::perfetto::Category(STR(PTRACE_CAT2)).SetDescription("PTrace category 2"));

PERFETTO_TRACK_EVENT_STATIC_STORAGE();

PTRACE_DEFINE_FUNCS(PTRACE_CAT0)
PTRACE_DEFINE_FUNCS(PTRACE_CAT1)
PTRACE_DEFINE_FUNCS(PTRACE_CAT2)

static bool ptrace_initialized = false;

int ptrace_init(void)
{
    ::perfetto::TracingInitArgs args;

    if (ptrace_initialized)
        return 0;

    fprintf(stderr, "\033[33mPTRACE %s\n\033[0m", PTRACE_VERSION);
    args.backends |= perfetto::kSystemBackend;
    //args.backends |= perfetto::kInProcessBackend;
    ::perfetto::Tracing::Initialize(args);

    if (!::perfetto::TrackEvent::Register()) {
        fprintf(stderr, "\033[33mPTRACE: init failed\n\033[0m");
        return -1;
    }

    ptrace_initialized = true;
    fprintf(stderr, "\033[33mPTRACE: init OK\n\033[0m");

    return 0;
}
