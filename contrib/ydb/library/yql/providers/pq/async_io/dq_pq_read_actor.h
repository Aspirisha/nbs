#pragma once

#include <contrib/ydb/library/yql/dq/actors/compute/dq_compute_actor_async_io_factory.h>
#include <contrib/ydb/library/yql/dq/actors/compute/dq_compute_actor_async_io.h>

#include <contrib/ydb/library/yql/providers/common/token_accessor/client/factory.h>
#include <contrib/ydb/library/yql/minikql/computation/mkql_computation_node_holders.h>

#include <contrib/ydb/library/yql/providers/pq/proto/dq_io.pb.h>
#include <contrib/ydb/library/yql/providers/pq/proto/dq_task_params.pb.h>

#include <contrib/ydb/public/sdk/cpp/client/ydb_driver/driver.h>

#include <contrib/ydb/library/actors/core/actor.h>

#include <util/generic/size_literals.h>
#include <util/system/types.h>


namespace NYql::NDq {
class TDqAsyncIoFactory;

const i64 PQReadDefaultFreeSpace = 16_MB;

std::pair<IDqComputeActorAsyncInput*, NActors::IActor*> CreateDqPqReadActor(
    NPq::NProto::TDqPqTopicSource&& settings,
    ui64 inputIndex,
    TCollectStatsLevel statsLevel,
    TTxId txId,
    ui64 taskId,
    const THashMap<TString, TString>& secureParams,
    const THashMap<TString, TString>& taskParams,
    NYdb::TDriver driver,
    ISecuredServiceAccountCredentialsFactory::TPtr credentialsFactory,
    const NActors::TActorId& computeActorId,
    const NKikimr::NMiniKQL::THolderFactory& holderFactory,
    i64 bufferSize = PQReadDefaultFreeSpace,
    bool rangesMode = true
    );

void RegisterDqPqReadActorFactory(TDqAsyncIoFactory& factory, NYdb::TDriver driver, ISecuredServiceAccountCredentialsFactory::TPtr credentialsFactory, bool rangesMode = true);

} // namespace NYql::NDq
