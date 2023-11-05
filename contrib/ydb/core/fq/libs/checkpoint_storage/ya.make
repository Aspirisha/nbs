LIBRARY()

SRCS(
    gc.cpp
    storage_proxy.cpp
    storage_service.cpp
    ydb_checkpoint_storage.cpp
    ydb_state_storage.cpp
)

PEERDIR(
    contrib/libs/fmt
    library/cpp/actors/core
    contrib/ydb/core/fq/libs/actors/logging
    contrib/ydb/core/fq/libs/control_plane_storage
    contrib/ydb/core/fq/libs/ydb
    contrib/ydb/core/fq/libs/checkpoint_storage/events
    contrib/ydb/core/fq/libs/checkpoint_storage/proto
    contrib/ydb/core/fq/libs/checkpointing_common
    contrib/ydb/core/fq/libs/shared_resources
    contrib/ydb/library/security
    contrib/ydb/library/yql/dq/actors/compute
    contrib/ydb/library/yql/dq/proto
    contrib/ydb/public/sdk/cpp/client/ydb_scheme
    contrib/ydb/public/sdk/cpp/client/ydb_table
)

YQL_LAST_ABI_VERSION()

END()

RECURSE(
    events
    proto
)
