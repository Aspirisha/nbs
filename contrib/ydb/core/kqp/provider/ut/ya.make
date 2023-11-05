UNITTEST_FOR(contrib/ydb/core/kqp/provider)

SRCS(
    yql_kikimr_gateway_ut.cpp
)

PEERDIR(
    contrib/ydb/core/client/minikql_result_lib
    contrib/ydb/core/kqp/ut/common
    contrib/ydb/library/yql/sql/pg_dummy
)

YQL_LAST_ABI_VERSION()

FORK_SUBTESTS()

IF (SANITIZER_TYPE == "thread" OR WITH_VALGRIND)
    TIMEOUT(1800)
    SIZE(LARGE)
    TAG(ya:fat)
ELSE()
    TIMEOUT(600)
    SIZE(MEDIUM)
ENDIF()

END()
