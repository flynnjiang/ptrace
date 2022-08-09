#ifndef __PTRACE_H__
#define __PTRACE_H__

#ifdef ENABLE_PTRACE

#include <stdint.h>

typedef uint32_t    u32;
typedef int32_t     i32;
typedef uint64_t    u64;
typedef int64_t     i64;
typedef const char *str;

/* categorys */
#define PTRACE_CAT0 ptrace0
#define PTRACE_CAT1 ptrace1
#define PTRACE_CAT2 ptrace2

#define _PTRACE_BEGIN_FUNC_NAME(cat) ptrace_begin_##cat
#define _PTRACE_BEGIN_FUNC(cat) \
        const char *_PTRACE_BEGIN_FUNC_NAME(cat)(const char *name)

#define _PTRACE_END_FUNC_NAME(cat) ptrace_end_##cat
#define _PTRACE_END_FUNC(cat) \
        void _PTRACE_END_FUNC_NAME(cat)(const char **dummy)

#define _PTRACE_BEGIN_FUNC_1_NAME(cat, type) ptrace_begin_##cat##_##type
#define _PTRACE_BEGIN_FUNC_1(cat, type) \
        const char *_PTRACE_BEGIN_FUNC_1_NAME(cat, type)(\
        const char *name, const char *arg, type val)

#define _PTRACE_BEGIN_FUNC_2_NAME(cat, type1, type2) \
        ptrace_begin_##cat##_##type1##_##type2
#define _PTRACE_BEGIN_FUNC_2(cat, type1, type2) \
        const char *_PTRACE_BEGIN_FUNC_2_NAME(cat, type1, type2)(\
        const char *name, \
        const char *arg1, type1 val1, \
        const char *arg2, type2 val2)

#define _PTRACE_COUNTER_FUNC_NAME(cat, type) ptrace_counter_##cat##_##type
#define _PTRACE_COUNTER_FUNC(cat, type) \
    void _PTRACE_COUNTER_FUNC_NAME(cat, type)(const char *track, type val)

/* api for category x */
#define PTRACE_INIT ptrace_init

#define PTRACE_END_CAT(cat)                       _PTRACE_END_FUNC_NAME(cat)((const char **)0)
#define PTRACE_BEGIN_CAT(cat, name)               _PTRACE_BEGIN_FUNC_NAME(cat)(name)
#define PTRACE_BEGIN_CAT_I32(cat, name, arg, val) _PTRACE_BEGIN_FUNC_1_NAME(cat, i32)(name, arg, val)
#define PTRACE_BEGIN_CAT_U32(cat, name, arg, val) _PTRACE_BEGIN_FUNC_1_NAME(cat, u32)(name, arg, val)
#define PTRACE_BEGIN_CAT_I64(cat, name, arg, val) _PTRACE_BEGIN_FUNC_1_NAME(cat, i64)(name, arg, val)
#define PTRACE_BEGIN_CAT_U64(cat, name, arg, val) _PTRACE_BEGIN_FUNC_1_NAME(cat, u64)(name, arg, val)
#define PTRACE_BEGIN_CAT_STR(cat, name, arg, val) _PTRACE_BEGIN_FUNC_1_NAME(cat, str)(name, arg, val)
#define PTRACE_BEGIN_CAT_I32_I32(cat, name, arg1, val1, arg2, val2) \
        _PTRACE_BEGIN_FUNC_2_NAME(cat, i32, i32)(name, arg1, val1, arg2, val2)
#define PTRACE_BEGIN_CAT_I32_U32(cat, name, arg1, val1, arg2, val2) \
        _PTRACE_BEGIN_FUNC_2_NAME(cat, i32, u32)(name, arg1, val1, arg2, val2)
#define PTRACE_BEGIN_CAT_U32_U32(cat, name, arg1, val1, arg2, val2) \
        _PTRACE_BEGIN_FUNC_2_NAME(cat, u32, u32)(name, arg1, val1, arg2, val2)

#define __PTRACE_SCOPE_CAT(cat, name, line) \
        const char *ptrace_dummy_##line \
        __attribute__((cleanup (_PTRACE_END_FUNC_NAME(cat)), unused)) = \
        _PTRACE_BEGIN_FUNC_NAME(cat)(name)
#define _PTRACE_SCOPE_CAT(cat, name, line) __PTRACE_SCOPE_CAT(cat, name, line)
#define PTRACE_SCOPE_CAT(cat, name) _PTRACE_SCOPE_CAT(cat, name, __LINE__)

#define ___PTRACE_SCOPE_CAT_1(cat, name, line, type, arg, val) \
        const char *ptrace_dummy_##line \
        __attribute__((cleanup (_PTRACE_END_FUNC_NAME(cat)), unused)) = \
        _PTRACE_BEGIN_FUNC_1_NAME(cat, type)(name, arg, val)
#define __PTRACE_SCOPE_CAT_1(cat, name, line, type, arg, val) \
        ___PTRACE_SCOPE_CAT_1(cat, name, line, type, arg, val)
#define _PTRACE_SCOPE_CAT_1(cat, name, type, arg, val) \
        __PTRACE_SCOPE_CAT_1(cat, name, __LINE__, type, arg, val)
#define PTRACE_SCOPE_CAT_I32(cat, name, arg, val) _PTRACE_SCOPE_CAT_1(cat, name, i32, arg, val)
#define PTRACE_SCOPE_CAT_U32(cat, name, arg, val) _PTRACE_SCOPE_CAT_1(cat, name, u32, arg, val)
#define PTRACE_SCOPE_CAT_I64(cat, name, arg, val) _PTRACE_SCOPE_CAT_1(cat, name, i64, arg, val)
#define PTRACE_SCOPE_CAT_U64(cat, name, arg, val) _PTRACE_SCOPE_CAT_1(cat, name, u64, arg, val)
#define PTRACE_SCOPE_CAT_STR(cat, name, arg, val) _PTRACE_SCOPE_CAT_1(cat, name, str, arg, val)

#define ___PTRACE_SCOPE_CAT_2(cat, name, line, type1, arg1, val1, type2, arg2, val2) \
        const char *ptrace_dummy_##line \
        __attribute__((cleanup (_PTRACE_END_FUNC_NAME(cat)), unused)) = \
        _PTRACE_BEGIN_FUNC_2_NAME(cat, type1, type2)(name, arg1, val1, arg2, val2)
#define __PTRACE_SCOPE_CAT_2(cat, name, line, type1, arg1, val1, type2, arg2, val2) \
        ___PTRACE_SCOPE_CAT_2(cat, name, line, type1, arg1, val1, type2, arg2, val2)
#define _PTRACE_SCOPE_CAT_2(cat, name, type1, arg1, val1, type2, arg2, val2) \
        __PTRACE_SCOPE_CAT_2(cat, name, __LINE__, type1, arg1, val1, type2, arg2, val2)
#define PTRACE_SCOPE_CAT_I32_I32(cat, name, arg1, val1, arg2, val2) \
        _PTRACE_SCOPE_CAT_2(cat, name, i32, arg1, val1, i32, arg2, val2)
#define PTRACE_SCOPE_CAT_I32_U32(cat, name, arg1, val1, arg2, val2) \
        _PTRACE_SCOPE_CAT_2(cat, name, i32, arg1, val1, u32, arg2, val2)
#define PTRACE_SCOPE_CAT_U32_U32(cat, name, arg1, val1, arg2, val2) \
        _PTRACE_SCOPE_CAT_2(cat, name, u32, arg1, val1, u32, arg2, val2)

#define PTRACE_FUNC_CAT(cat)               PTRACE_SCOPE_CAT(cat, __func__)
#define PTRACE_FUNC_CAT_I32(cat, arg, val) PTRACE_SCOPE_CAT_I32(cat, __func__, arg, val)
#define PTRACE_FUNC_CAT_U32(cat, arg, val) PTRACE_SCOPE_CAT_U32(cat, __func__, arg, val)
#define PTRACE_FUNC_CAT_I64(cat, arg, val) PTRACE_SCOPE_CAT_I64(cat, __func__, arg, val)
#define PTRACE_FUNC_CAT_U64(cat, arg, val) PTRACE_SCOPE_CAT_U64(cat, __func__, arg, val)
#define PTRACE_FUNC_CAT_STR(cat, arg, val) PTRACE_SCOPE_CAT_STR(cat, __func__, arg, val)
#define PTRACE_FUNC_CAT_I32_I32(cat, arg1, val1, arg2, val2) \
        PTRACE_SCOPE_CAT_I32_I32(cat, __func__, arg1, val1, arg2, val2)
#define PTRACE_FUNC_CAT_I32_U32(cat, arg1, val1, arg2, val2) \
        PTRACE_SCOPE_CAT_I32_U32(cat, __func__, arg1, val1, arg2, val2)
#define PTRACE_FUNC_CAT_U32_U32(cat, arg1, val1, arg2, val2) \
        PTRACE_SCOPE_CAT_U32_U32(cat, __func__, arg1, val1, arg2, val2)

#define PTRACE_COUNTER_CAT_I32(cat, track, val) _PTRACE_COUNTER_FUNC_NAME(cat, i32)(track, val)
#define PTRACE_COUNTER_CAT_U32(cat, track, val) _PTRACE_COUNTER_FUNC_NAME(cat, u32)(track, val)
#define PTRACE_COUNTER_CAT_I64(cat, track, val) _PTRACE_COUNTER_FUNC_NAME(cat, i64)(track, val)
#define PTRACE_COUNTER_CAT_U64(cat, track, val) _PTRACE_COUNTER_FUNC_NAME(cat, u64)(track, val)
#define PTRACE_COUNTER_CAT_FLT(cat, track, val) _PTRACE_COUNTER_FUNC_NAME(cat, float)(track, val)
#define PTRACE_COUNTER_CAT_DBL(cat, track, val) _PTRACE_COUNTER_FUNC_NAME(cat, double)(track, val)



/* category 0 */
#define PTRACE_BEGIN(name)               PTRACE_BEGIN_CAT(PTRACE_CAT0, name)
#define PTRACE_BEGIN_I32(name, arg, val) PTRACE_BEGIN_CAT_I32(PTRACE_CAT0, name, arg, val)
#define PTRACE_BEGIN_U32(name, arg, val) PTRACE_BEGIN_CAT_U32(PTRACE_CAT0, name, arg, val)
#define PTRACE_BEGIN_I64(name, arg, val) PTRACE_BEGIN_CAT_I64(PTRACE_CAT0, name, arg, val)
#define PTRACE_BEGIN_U64(name, arg, val) PTRACE_BEGIN_CAT_U64(PTRACE_CAT0, name, arg, val)
#define PTRACE_BEGIN_STR(name, arg, val) PTRACE_BEGIN_CAT_STR(PTRACE_CAT0, name, arg, val)
#define PTRACE_BEGIN_I32_I32(name, arg1, val1, arg2, val2) \
        PTRACE_BEGIN_CAT_I32_I32(PTRACE_CAT0, name, arg1, val1, arg2, val2)
#define PTRACE_BEGIN_I32_U32(name, arg1, val1, arg2, val2) \
        PTRACE_BEGIN_CAT_I32_U32(PTRACE_CAT0, name, arg1, val1, arg2, val2)
#define PTRACE_BEGIN_U32_U32(name, arg1, val1, arg2, val2) \
        PTRACE_BEGIN_CAT_U32_U32(PTRACE_CAT0, name, arg1, val1, arg2, val2)
#define PTRACE_END()                     PTRACE_END_CAT(PTRACE_CAT0)

#define PTRACE_SCOPE(name)               PTRACE_SCOPE_CAT(PTRACE_CAT0, name)
#define PTRACE_SCOPE_I32(name, arg, val) PTRACE_SCOPE_CAT_I32(PTRACE_CAT0, name, arg, val)
#define PTRACE_SCOPE_U32(name, arg, val) PTRACE_SCOPE_CAT_U32(PTRACE_CAT0, name, arg, val)
#define PTRACE_SCOPE_I64(name, arg, val) PTRACE_SCOPE_CAT_I64(PTRACE_CAT0, name, arg, val)
#define PTRACE_SCOPE_U64(name, arg, val) PTRACE_SCOPE_CAT_U64(PTRACE_CAT0, name, arg, val)
#define PTRACE_SCOPE_STR(name, arg, val) PTRACE_SCOPE_CAT_STR(PTRACE_CAT0, name, arg, val)
#define PTRACE_SCOPE_I32_I32(name, arg1, val1, arg2, val2) \
        PTRACE_SCOPE_CAT_I32_I32(PTRACE_CAT0, name, arg1, val1, arg2, val2)
#define PTRACE_SCOPE_I32_U32(name, arg1, val1, arg2, val2) \
        PTRACE_SCOPE_CAT_I32_U32(PTRACE_CAT0, name, arg1, val1, arg2, val2)
#define PTRACE_SCOPE_U32_U32(name, arg1, val1, arg2, val2) \
        PTRACE_SCOPE_CAT_U32_U32(PTRACE_CAT0, name, arg1, val1, arg2, val2)

#define PTRACE_FUNC()             PTRACE_FUNC_CAT(PTRACE_CAT0)
#define PTRACE_FUNC_I32(arg, val) PTRACE_FUNC_CAT_I32(PTRACE_CAT0, arg, val)
#define PTRACE_FUNC_U32(arg, val) PTRACE_FUNC_CAT_U32(PTRACE_CAT0, arg, val)
#define PTRACE_FUNC_I64(arg, val) PTRACE_FUNC_CAT_I64(PTRACE_CAT0, arg, val)
#define PTRACE_FUNC_U64(arg, val) PTRACE_FUNC_CAT_U64(PTRACE_CAT0, arg, val)
#define PTRACE_FUNC_STR(arg, val) PTRACE_FUNC_CAT_STR(PTRACE_CAT0, arg, val)
#define PTRACE_FUNC_I32_I32(arg1, val1, arg2, val2) \
        PTRACE_FUNC_CAT_I32_I32(PTRACE_CAT0, arg1, val1, arg2, val2)
#define PTRACE_FUNC_I32_U32(arg1, val1, arg2, val2) \
        PTRACE_FUNC_CAT_I32_U32(PTRACE_CAT0, arg1, val1, arg2, val2)
#define PTRACE_FUNC_U32_U32(arg1, val1, arg2, val2) \
        PTRACE_FUNC_CAT_U32_U32(PTRACE_CAT0, arg1, val1, arg2, val2)

#define PTRACE_COUNTER_I32(name, val) PTRACE_COUNTER_CAT_I32(PTRACE_CAT0, name, val)
#define PTRACE_COUNTER_U32(name, val) PTRACE_COUNTER_CAT_U32(PTRACE_CAT0, name, val)
#define PTRACE_COUNTER_I64(name, val) PTRACE_COUNTER_CAT_I64(PTRACE_CAT0, name, val)
#define PTRACE_COUNTER_U64(name, val) PTRACE_COUNTER_CAT_U64(PTRACE_CAT0, name, val)
#define PTRACE_COUNTER_FLT(name, val) PTRACE_COUNTER_CAT_FLT(PTRACE_CAT0, name, val)
#define PTRACE_COUNTER_DBL(name, val) PTRACE_COUNTER_CAT_DBL(PTRACE_CAT0, name, val)



#define _PTRACE_DECLARE_FUNCS(cat)              \
    extern _PTRACE_BEGIN_FUNC(cat);             \
    extern _PTRACE_BEGIN_FUNC_1(cat, i32);      \
    extern _PTRACE_BEGIN_FUNC_1(cat, u32);      \
    extern _PTRACE_BEGIN_FUNC_1(cat, i64);      \
    extern _PTRACE_BEGIN_FUNC_1(cat, u64);      \
    extern _PTRACE_BEGIN_FUNC_1(cat, str);      \
    extern _PTRACE_BEGIN_FUNC_2(cat, i32, i32); \
    extern _PTRACE_BEGIN_FUNC_2(cat, i32, u32); \
    extern _PTRACE_BEGIN_FUNC_2(cat, u32, u32); \
    extern _PTRACE_END_FUNC(cat);               \
    extern _PTRACE_COUNTER_FUNC(cat, u32);      \
    extern _PTRACE_COUNTER_FUNC(cat, i32);      \
    extern _PTRACE_COUNTER_FUNC(cat, u64);      \
    extern _PTRACE_COUNTER_FUNC(cat, i64);      \
    extern _PTRACE_COUNTER_FUNC(cat, float);    \
    extern _PTRACE_COUNTER_FUNC(cat, double);



#ifdef __cplusplus
extern "C" {
#endif

extern int ptrace_init(void);

_PTRACE_DECLARE_FUNCS(PTRACE_CAT0)
_PTRACE_DECLARE_FUNCS(PTRACE_CAT1)
_PTRACE_DECLARE_FUNCS(PTRACE_CAT2)

#ifdef __cplusplus
}
#endif



#else /* ENABLE_PTRACE */

#define PTRACE_INIT()

#define PTRACE_BEGIN_CAT(cat, name)
#define PTRACE_BEGIN_CAT_I32(cat, name, arg, val)
#define PTRACE_BEGIN_CAT_U32(cat, name, arg, val)
#define PTRACE_BEGIN_CAT_I64(cat, name, arg, val)
#define PTRACE_BEGIN_CAT_U64(cat, name, arg, val)
#define PTRACE_BEGIN_CAT_STR(cat, name, arg, val)
#define PTRACE_BEGIN_CAT_I32_I32(cat, name, arg1, val1, arg2, val2)
#define PTRACE_BEGIN_CAT_I32_U32(cat, name, arg1, val1, arg2, val2)
#define PTRACE_BEGIN_CAT_U32_U32(cat, name, arg1, val1, arg2, val2)
#define PTRACE_END_CAT(cat)
#define PTRACE_SCOPE_CAT(cat, name)
#define PTRACE_SCOPE_CAT_I32(cat, name, arg, val)
#define PTRACE_SCOPE_CAT_U32(cat, name, arg, val)
#define PTRACE_SCOPE_CAT_I64(cat, name, arg, val)
#define PTRACE_SCOPE_CAT_U64(cat, name, arg, val)
#define PTRACE_SCOPE_CAT_STR(cat, name, arg, val)
#define PTRACE_SCOPE_CAT_I32_I32(cat, name, arg1, val1, arg2, val2)
#define PTRACE_SCOPE_CAT_U32_U32(cat, name, arg1, val1, arg2, val2)
#define PTRACE_SCOPE_CAT_U32_U32(cat, name, arg1, val1, arg2, val2)
#define PTRACE_FUNC_CAT(cat)
#define PTRACE_FUNC_CAT_I32(cat, arg, val)
#define PTRACE_FUNC_CAT_U32(cat, arg, val)
#define PTRACE_FUNC_CAT_I64(cat, arg, val)
#define PTRACE_FUNC_CAT_U64(cat, arg, val)
#define PTRACE_FUNC_CAT_STR(cat, arg, val)
#define PTRACE_FUNC_CAT_I32_I32(cat, arg1, val1, arg2, val2)
#define PTRACE_FUNC_CAT_U32_U32(cat, arg1, val1, arg2, val2)
#define PTRACE_FUNC_CAT_U32_U32(cat, arg1, val1, arg2, val2)
#define PTRACE_COUNTER_CAT_I32(cat, name, val)
#define PTRACE_COUNTER_CAT_U32(cat, name, val)
#define PTRACE_COUNTER_CAT_I64(cat, name, val)
#define PTRACE_COUNTER_CAT_U64(cat, name, val)
#define PTRACE_COUNTER_CAT_FLT(cat, name, val)
#define PTRACE_COUNTER_CAT_DBL(cat, name, val)

#define PTRACE_BEGIN(name)
#define PTRACE_BEGIN_I32(name, arg, val)
#define PTRACE_BEGIN_U32(name, arg, val)
#define PTRACE_BEGIN_I64(name, arg, val)
#define PTRACE_BEGIN_U64(name, arg, val)
#define PTRACE_BEGIN_STR(name, arg, val)
#define PTRACE_BEGIN_I32_I32(name, arg1, val1, arg2, val2)
#define PTRACE_BEGIN_I32_U32(name, arg1, val1, arg2, val2)
#define PTRACE_BEGIN_U32_U32(name, arg1, val1, arg2, val2)
#define PTRACE_END()
#define PTRACE_SCOPE(name)
#define PTRACE_SCOPE_I32(name, arg, val)
#define PTRACE_SCOPE_U32(name, arg, val)
#define PTRACE_SCOPE_I64(name, arg, val)
#define PTRACE_SCOPE_U64(name, arg, val)
#define PTRACE_SCOPE_STR(name, arg, val)
#define PTRACE_SCOPE_I32_I32(name, arg1, val1, arg2, val2)
#define PTRACE_SCOPE_U32_U32(name, arg1, val1, arg2, val2)
#define PTRACE_SCOPE_U32_U32(name, arg1, val1, arg2, val2)
#define PTRACE_FUNC()
#define PTRACE_FUNC_I32(arg, val)
#define PTRACE_FUNC_U32(arg, val)
#define PTRACE_FUNC_I64(arg, val)
#define PTRACE_FUNC_U64(arg, val)
#define PTRACE_FUNC_STR(arg, val)
#define PTRACE_FUNC_I32_I32(arg1, val1, arg2, val2)
#define PTRACE_FUNC_U32_U32(arg1, val1, arg2, val2)
#define PTRACE_FUNC_U32_U32(arg1, val1, arg2, val2)
#define PTRACE_COUNTER_I32(name, val)
#define PTRACE_COUNTER_U32(name, val)
#define PTRACE_COUNTER_I64(name, val)
#define PTRACE_COUNTER_U64(name, val)
#define PTRACE_COUNTER_FLT(name, val)
#define PTRACE_COUNTER_DBL(name, val)

#endif /* ENABLE_PTRACE */

#endif /* __PTRACE_H__ */
