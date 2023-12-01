UNITTEST_FOR(contrib/ydb/core/base)

FORK_SUBTESTS()

SIZE(MEDIUM)

TIMEOUT(600)

PEERDIR(
    contrib/ydb/library/actors/interconnect
    contrib/ydb/library/actors/core
    library/cpp/testing/unittest
    contrib/ydb/core/testlib/basics
    contrib/ydb/core/base
    contrib/ydb/core/testlib/basics/default
)

YQL_LAST_ABI_VERSION()

SRCS(
    board_subscriber_ut.cpp
)

END()
