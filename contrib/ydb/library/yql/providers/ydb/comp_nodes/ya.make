LIBRARY()

PEERDIR(
    contrib/ydb/core/scheme
    contrib/ydb/library/mkql_proto/protos
    contrib/ydb/library/yql/dq/actors/protos
    contrib/ydb/library/yql/minikql/computation
    contrib/ydb/library/yql/providers/common/structured_token
    contrib/ydb/library/yql/providers/ydb/proto
    contrib/ydb/public/lib/experimental
    contrib/ydb/public/sdk/cpp/client/ydb_driver
)

SRCS(
    yql_kik_scan.cpp
    yql_ydb_factory.cpp
    yql_ydb_dq_transform.cpp
)

YQL_LAST_ABI_VERSION()

END()
