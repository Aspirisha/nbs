LIBRARY()

SRCS(
    grpc_service.cpp
)

PEERDIR(
    library/cpp/grpc/server
    library/cpp/lwtrace
    contrib/ydb/core/grpc_services
    contrib/ydb/core/protos
    contrib/ydb/library/login
    contrib/ydb/public/api/grpc
    contrib/ydb/public/lib/operation_id
)

END()
