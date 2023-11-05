UNITTEST_FOR(contrib/ydb/core/blobstorage/vdisk/huge)

FORK_SUBTESTS()

TIMEOUT(600)

SIZE(MEDIUM)

PEERDIR(
    library/cpp/getopt
    library/cpp/svnversion
    contrib/ydb/core/base
    contrib/ydb/core/blobstorage
    contrib/ydb/core/blobstorage/groupinfo
    contrib/ydb/core/erasure
)

SRCS(
    blobstorage_hullhugedefs_ut.cpp
    blobstorage_hullhugeheap_ctx_ut.cpp
    blobstorage_hullhugeheap_ut.cpp
    blobstorage_hullhugerecovery_ut.cpp
    blobstorage_hullhuge_ut.cpp
    top_ut.cpp
)

END()
