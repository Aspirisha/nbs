UNITTEST_FOR(cloud/blockstore/libs/ydbstats)

INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/tests/recipes/small.inc)

SRCS(
    ydbstats_ut.cpp
    ydbwriters_ut.cpp
)

PEERDIR(
    contrib/ydb/core/testlib/default
)

END()
