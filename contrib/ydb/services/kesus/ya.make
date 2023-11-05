LIBRARY()

SRCS(
    grpc_service.cpp
)

PEERDIR(
    library/cpp/grpc/server
    contrib/ydb/core/base
    contrib/ydb/core/grpc_services
    contrib/ydb/core/grpc_streaming
    contrib/ydb/core/kesus/proxy
    contrib/ydb/core/kesus/tablet
    contrib/ydb/public/api/grpc
    contrib/ydb/public/api/grpc/draft
    contrib/ydb/public/lib/operation_id
)

END()
