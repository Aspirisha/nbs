#include "constructor.h"
#include "filter_assembler.h"
#include "column_assembler.h"
#include "committed_assembler.h"
#include <ydb/core/tx/columnshard/engines/reader/read_context.h>
#include <ydb/core/tx/columnshard/engines/scheme/filtered_scheme.h>
#include <ydb/core/tx/conveyor/usage/events.h>
#include <ydb/core/tx/conveyor/usage/service.h>

namespace NKikimr::NOlap::NPlainReader {

TPortionInfo::TPreparedBatchData TAssembleColumnsTaskConstructor::BuildBatchAssembler() {
    auto blobs = ExtractBlobsData();
    THashMap<TBlobRange, TPortionInfo::TAssembleBlobInfo> blobsDataAssemble;
    for (auto&& i : blobs) {
        blobsDataAssemble.emplace(i.first, i.second);
    }
    for (auto&& i : NullBlocks) {
        AFL_VERIFY(blobsDataAssemble.emplace(i.first, i.second).second);
    }
    auto blobSchema = ReadMetadata->GetLoadSchema(PortionInfo->GetMinSnapshot());
    auto readSchema = ReadMetadata->GetLoadSchema(ReadMetadata->GetSnapshot());
    ISnapshotSchema::TPtr resultSchema;
    if (ColumnIds.size()) {
        resultSchema = std::make_shared<TFilteredSnapshotSchema>(readSchema, ColumnIds);
    } else {
        resultSchema = readSchema;
    }

    return PortionInfo->PrepareForAssemble(*blobSchema, *resultSchema, blobsDataAssemble);
}

void TEFTaskConstructor::DoOnDataReady(const std::shared_ptr<NResourceBroker::NSubscribe::TResourcesGuard>& /*resourcesGuard*/) {
    NConveyor::TScanServiceOperator::SendTaskToExecute(std::make_shared<TAssembleFilter>(ScanActorId, BuildBatchAssembler(),
        ReadMetadata, SourceIdx, ColumnIds, UseEarlyFilter));
}

void TFFColumnsTaskConstructor::DoOnDataReady(const std::shared_ptr<NResourceBroker::NSubscribe::TResourcesGuard>& /*resourcesGuard*/) {
    NConveyor::TScanServiceOperator::SendTaskToExecute(std::make_shared<TAssembleFFBatch>(ScanActorId, BuildBatchAssembler(),
        SourceIdx, AppliedFilter));
}

void TCommittedColumnsTaskConstructor::DoOnDataReady(const std::shared_ptr<NResourceBroker::NSubscribe::TResourcesGuard>& /*resourcesGuard*/) {
    auto blobs = ExtractBlobsData();
    Y_ABORT_UNLESS(NullBlocks.size() == 0);
    Y_ABORT_UNLESS(blobs.size() == 1);
    NConveyor::TScanServiceOperator::SendTaskToExecute(std::make_shared<TCommittedAssembler>(ScanActorId, blobs.begin()->second,
        ReadMetadata, SourceIdx, CommittedBlob));
}

bool IFetchTaskConstructor::DoOnError(const TBlobRange& range, const IBlobsReadingAction::TErrorStatus& status) {
    AFL_ERROR(NKikimrServices::TX_COLUMNSHARD)("error_on_blob_reading", range.ToString())("scan_actor_id", ScanActorId)("status", status.GetErrorMessage())("status_code", status.GetStatus());
    NActors::TActorContext::AsActorContext().Send(ScanActorId, std::make_unique<NConveyor::TEvExecution::TEvTaskProcessedResult>(TConclusionStatus::Fail("cannot read blob range " + range.ToString())));
    return false;
}

}
