UNITTEST_FOR(contrib/ydb/public/lib/json_value)

TIMEOUT(600)

SIZE(MEDIUM)

FORK_SUBTESTS()

SRCS(
    ydb_json_value_ut.cpp
)

PEERDIR(
    library/cpp/json
    library/cpp/testing/unittest
    contrib/ydb/public/sdk/cpp/client/ydb_proto
    contrib/ydb/public/sdk/cpp/client/ydb_params
)

END()
