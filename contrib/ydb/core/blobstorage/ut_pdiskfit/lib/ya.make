LIBRARY()

SRCS(
    basic_test.cpp
    objectwithstate.cpp
)

PEERDIR(
    library/cpp/actors/protos
    contrib/ydb/core/base
    contrib/ydb/core/blobstorage/pdisk
    contrib/ydb/library/pdisk_io
    library/cpp/deprecated/atomic
)

END()
