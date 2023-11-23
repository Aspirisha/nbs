UNITTEST_FOR(contrib/ydb/core/tx/schemeshard)

FORK_SUBTESTS()

IF (WITH_VALGRIND)
    SPLIT_FACTOR(40)
ENDIF()

TIMEOUT(600)

SIZE(MEDIUM)

PEERDIR(
    library/cpp/getopt
    library/cpp/regex/pcre
    library/cpp/svnversion
    contrib/ydb/core/kqp/ut/common
    contrib/ydb/core/testlib/pg
    contrib/ydb/core/tx
    contrib/ydb/core/tx/schemeshard/ut_helpers
    contrib/ydb/library/yql/public/udf/service/exception_policy
)

YQL_LAST_ABI_VERSION()

SRCS(
    ut_base.cpp
    ut_info_types.cpp
    ut_allocate_pq.cpp
    ut_table_pg_types.cpp
)

END()
