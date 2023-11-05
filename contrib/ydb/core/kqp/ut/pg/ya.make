UNITTEST_FOR(contrib/ydb/core/kqp)

FORK_SUBTESTS()

SIZE(MEDIUM)

SRCS(
    kqp_pg_ut.cpp
)

PEERDIR(
    contrib/ydb/core/kqp/ut/common
    contrib/ydb/library/yql/public/udf/service/exception_policy
    contrib/ydb/library/yql/sql/pg
    contrib/ydb/library/yql/parser/pg_wrapper
    contrib/ydb/public/lib/ut_helpers
)

ADDINCL(
    contrib/ydb/library/yql/parser/pg_wrapper/postgresql/src/include
)

IF (OS_WINDOWS)
CFLAGS(
   "-D__thread=__declspec(thread)"
   -Dfstat=microsoft_native_fstat
   -Dstat=microsoft_native_stat
)
ENDIF()

NO_COMPILER_WARNINGS()

YQL_LAST_ABI_VERSION()

REQUIREMENTS(ram:32)

END()
