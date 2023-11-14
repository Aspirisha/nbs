PROGRAM()

SRCS(
    udf_resolver.cpp
    discover.cpp
)

IF (OS_LINUX)
    # prevent external python extensions to lookup protobuf symbols (and maybe
    # other common stuff) in main binary
    EXPORTS_SCRIPT(${ARCADIA_ROOT}/contrib/ydb/library/yql/tools/exports.symlist)

    PEERDIR(
        contrib/libs/libc_compat
    )
ENDIF()

PEERDIR(
    library/cpp/getopt
    library/cpp/protobuf/util
    contrib/ydb/library/yql/minikql
    contrib/ydb/library/yql/public/udf/service/terminate_policy
    contrib/ydb/library/yql/core
    contrib/ydb/library/yql/providers/common/proto
    contrib/ydb/library/yql/providers/common/schema/mkql
    contrib/ydb/library/yql/utils/backtrace
    contrib/ydb/library/yql/utils/sys
    contrib/ydb/library/yql/sql/pg_dummy
)

YQL_LAST_ABI_VERSION()

END()
