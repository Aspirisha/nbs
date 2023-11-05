#pragma once

#include <contrib/ydb/core/mind/bscontroller/defs.h>

#include <contrib/ydb/core/blobstorage/dsproxy/mock/dsproxy_mock.h>
#include <contrib/ydb/core/blobstorage/pdisk/mock/pdisk_mock.h>

#include <contrib/ydb/core/mind/bscontroller/bsc.h>
#include <contrib/ydb/core/mind/bscontroller/types.h>

#include <contrib/ydb/core/protos/blobstorage_distributed_config.pb.h>

#include <contrib/ydb/core/util/testactorsys.h>

using namespace NActors;
using namespace NKikimr;
using namespace NKikimr::NBsController;
