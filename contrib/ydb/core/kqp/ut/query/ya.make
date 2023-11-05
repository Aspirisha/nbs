UNITTEST_FOR(contrib/ydb/core/kqp)

FORK_SUBTESTS()
SPLIT_FACTOR(50)

REQUIREMENTS(
    ram:32
)

IF (WITH_VALGRIND)
    TIMEOUT(3600)
    SIZE(LARGE)
    TAG(ya:fat)
ELSE()
    TIMEOUT(600)
    SIZE(MEDIUM)
ENDIF()

SRCS(
    kqp_explain_ut.cpp
    kqp_limits_ut.cpp
    kqp_params_ut.cpp
    kqp_query_ut.cpp
    kqp_stats_ut.cpp
)

PEERDIR(
    contrib/ydb/public/sdk/cpp/client/ydb_proto
    contrib/ydb/core/kqp
    contrib/ydb/core/kqp/ut/common
    contrib/ydb/library/yql/sql/pg_dummy
)

YQL_LAST_ABI_VERSION()

END()
