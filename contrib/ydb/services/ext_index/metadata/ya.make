LIBRARY()

SRCS(
    object.cpp
    GLOBAL behaviour.cpp
    manager.cpp
    initializer.cpp
    snapshot.cpp
    fetcher.cpp
)

PEERDIR(
    library/cpp/actors/core
    contrib/ydb/core/base
    contrib/ydb/core/grpc_services/local_rpc
    contrib/ydb/core/grpc_services/base
    contrib/ydb/core/grpc_services
    contrib/ydb/services/metadata/request
    contrib/ydb/services/ext_index/metadata/extractor
)

END()
