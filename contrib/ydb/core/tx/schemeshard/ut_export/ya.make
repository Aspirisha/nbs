UNITTEST_FOR(contrib/ydb/core/tx/schemeshard)

FORK_SUBTESTS()

SPLIT_FACTOR(11)

IF (SANITIZER_TYPE == "thread" OR WITH_VALGRIND)
    TIMEOUT(3600)
    SIZE(LARGE)
    TAG(ya:fat)
ELSE()
    TIMEOUT(600)
    SIZE(MEDIUM)
ENDIF()

INCLUDE(${ARCADIA_ROOT}/contrib/ydb/tests/supp/ubsan_supp.inc)

IF (NOT OS_WINDOWS)
    PEERDIR(
        library/cpp/getopt
        library/cpp/regex/pcre
        library/cpp/svnversion
        contrib/ydb/core/testlib/default
        contrib/ydb/core/tx
        contrib/ydb/core/tx/schemeshard/ut_helpers
        contrib/ydb/core/wrappers/ut_helpers
        contrib/ydb/library/yql/public/udf/service/exception_policy
    )
    SRCS(
        ut_export.cpp
    )
ENDIF()

YQL_LAST_ABI_VERSION()

END()
