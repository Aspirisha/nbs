#include "general_compaction.h"
#include "compaction/column_portion_chunk.h"
#include "compaction/column_cursor.h"
#include "compaction/merge_context.h"
#include "compaction/merged_column.h"

#include <ydb/core/tx/columnshard/columnshard_impl.h>
#include <ydb/core/tx/columnshard/engines/portions/with_blobs.h>
#include <ydb/core/tx/columnshard/splitter/batch_slice.h>
#include <ydb/core/tx/columnshard/splitter/rb_splitter.h>
#include <ydb/core/formats/arrow/simple_builder/array.h>
#include <ydb/core/formats/arrow/simple_builder/filler.h>
#include "../reader/read_filter_merger.h"

namespace NKikimr::NOlap::NCompaction {

TConclusionStatus TGeneralCompactColumnEngineChanges::DoConstructBlobs(TConstructionContext& context) noexcept {
    std::vector<TPortionInfoWithBlobs> portions = TPortionInfoWithBlobs::RestorePortions(SwitchedPortions, Blobs);
    Blobs.clear();
    std::optional<TSnapshot> maxSnapshot;
    for (auto&& i : SwitchedPortions) {
        if (!maxSnapshot || *maxSnapshot < i.GetMinSnapshot()) {
            maxSnapshot = i.GetMinSnapshot();
        }
    }
    Y_VERIFY(maxSnapshot);

    static const TString portionIdFieldName = "$$__portion_id";
    static const TString portionRecordIndexFieldName = "$$__portion_record_idx";
    static const std::shared_ptr<arrow::Field> portionIdField = std::make_shared<arrow::Field>(portionIdFieldName, std::make_shared<arrow::UInt16Type>());
    static const std::shared_ptr<arrow::Field> portionRecordIndexField = std::make_shared<arrow::Field>(portionRecordIndexFieldName, std::make_shared<arrow::UInt32Type>());

    auto resultSchema = context.SchemaVersions.GetLastSchema();
    std::vector<std::string> pkFieldNames = resultSchema->GetIndexInfo().GetReplaceKey()->field_names();
    std::set<std::string> pkFieldNamesSet(pkFieldNames.begin(), pkFieldNames.end());
    for (auto&& i : TIndexInfo::GetSpecialColumnNames()) {
        pkFieldNamesSet.emplace(i);
    }

    std::shared_ptr<arrow::RecordBatch> batchResult;
    {
        arrow::FieldVector indexFields;
        indexFields.emplace_back(portionIdField);
        indexFields.emplace_back(portionRecordIndexField);
        auto dataSchema = std::make_shared<arrow::Schema>(indexFields);
        NIndexedReader::TMergePartialStream mergeStream(resultSchema->GetIndexInfo().GetReplaceKey(), dataSchema, false);
        ui32 idx = 0;
        for (auto&& i : portions) {
            auto dataSchema = context.SchemaVersions.GetSchema(i.GetPortionInfo().GetMinSnapshot());
            auto batch = i.GetBatch(dataSchema, *resultSchema, pkFieldNamesSet);
            {
                NArrow::NConstruction::IArrayBuilder::TPtr column = std::make_shared<NArrow::NConstruction::TSimpleArrayConstructor<NArrow::NConstruction::TIntConstFiller<arrow::UInt16Type>>>(portionIdFieldName, idx++);
                batch = NArrow::TStatusValidator::GetValid(batch->AddColumn(batch->num_columns(), portionIdField, column->BuildArray(batch->num_rows())));
            }
            {
                NArrow::NConstruction::IArrayBuilder::TPtr column = std::make_shared<NArrow::NConstruction::TSimpleArrayConstructor<NArrow::NConstruction::TIntSeqFiller<arrow::UInt32Type>>>(portionRecordIndexFieldName);
                batch = NArrow::TStatusValidator::GetValid(batch->AddColumn(batch->num_columns(), portionRecordIndexField, column->BuildArray(batch->num_rows())));
            }
            Y_VERIFY_DEBUG(NArrow::IsSortedAndUnique(batch, resultSchema->GetIndexInfo().GetReplaceKey()));
            mergeStream.AddPoolSource({}, batch, nullptr);
        }

        NIndexedReader::TRecordBatchBuilder indexesBuilder(indexFields);
        mergeStream.DrainAll(indexesBuilder);
        batchResult = indexesBuilder.Finalize();
    }
    
    auto columnPortionIdx = batchResult->GetColumnByName(portionIdFieldName);
    auto columnPortionRecordIdx = batchResult->GetColumnByName(portionRecordIndexFieldName);
    Y_VERIFY(columnPortionIdx && columnPortionRecordIdx);
    Y_VERIFY(columnPortionIdx->type_id() == arrow::UInt16Type::type_id);
    Y_VERIFY(columnPortionRecordIdx->type_id() == arrow::UInt32Type::type_id);
    const arrow::UInt16Array& pIdxArray = static_cast<const arrow::UInt16Array&>(*columnPortionIdx);
    const arrow::UInt32Array& pRecordIdxArray = static_cast<const arrow::UInt32Array&>(*columnPortionRecordIdx);

    const ui32 portionRecordsCountLimit = batchResult->num_rows() / (batchResult->num_rows() / 10000 + 1) + 1;

    TSerializationStats stats;
    for (auto&& i : SwitchedPortions) {
        stats.Merge(i.GetSerializationStat(*resultSchema));
    }

    std::map<std::string, std::vector<TColumnPortionResult>> columnChunks;

    for (auto&& f : resultSchema->GetSchema()->fields()) {
        const ui32 columnId = resultSchema->GetColumnId(f->name());
        auto columnInfo = stats.GetColumnInfo(columnId);
        Y_VERIFY(columnInfo);
        TColumnMergeContext context(resultSchema, portionRecordsCountLimit, 50 * 1024 * 1024, f, *columnInfo, SaverContext);
        TMergedColumn mColumn(context);
        {
            auto c = batchResult->GetColumnByName(f->name());
            AFL_VERIFY(!c);
            AFL_VERIFY(batchResult->num_rows() == pIdxArray.length());
            std::vector<TPortionColumnCursor> cursors;
            auto loader = resultSchema->GetColumnLoader(f->name());
            for (auto&& p : portions) {
                std::vector<const TColumnRecord*> records;
                std::vector<IPortionColumnChunk::TPtr> chunks;
                p.ExtractColumnChunks(columnId, records, chunks);
                cursors.emplace_back(TPortionColumnCursor(chunks, records, loader));
            }
            std::optional<ui16> predPortionIdx;
            for (ui32 idx = 0; idx < pIdxArray.length(); ++idx) {
                const ui16 portionIdx = pIdxArray.Value(idx);
                const ui32 portionRecordIdx = pRecordIdxArray.Value(idx);
                auto& cursor = cursors[portionIdx];
                cursor.Next(portionRecordIdx, mColumn);
                if (predPortionIdx && portionIdx != *predPortionIdx) {
                    cursors[*predPortionIdx].Fetch(mColumn);
                }
                if (idx + 1 == pIdxArray.length()) {
                    cursor.Fetch(mColumn);
                }
                predPortionIdx = portionIdx;
            }
        }
        AFL_VERIFY(mColumn.GetRecordsCount() == batchResult->num_rows())("f_name", f->name())("mCount", mColumn.GetRecordsCount())("bCount", batchResult->num_rows());
        columnChunks[f->name()] = mColumn.BuildResult();
    }

    Y_VERIFY(columnChunks.size());

    for (auto&& i : columnChunks) {
        if (i.second.size() != columnChunks.begin()->second.size()) {
            for (ui32 p = 0; p < std::min<ui32>(columnChunks.begin()->second.size(), i.second.size()); ++p) {
                AFL_DEBUG(NKikimrServices::TX_COLUMNSHARD)("p_first", columnChunks.begin()->second[p].DebugString())("p", i.second[p].DebugString());
            }
        }
        AFL_VERIFY(i.second.size() == columnChunks.begin()->second.size())("first", columnChunks.begin()->second.size())("current", i.second.size())("first_name", columnChunks.begin()->first)("current_name", i.first);
    }

    std::vector<TGeneralSerializedSlice> batchSlices;
    std::shared_ptr<TDefaultSchemaDetails> schemaDetails(new TDefaultSchemaDetails(resultSchema, SaverContext, std::move(stats)));

    for (ui32 i = 0; i < columnChunks.begin()->second.size(); ++i) {
        std::map<ui32, std::vector<IPortionColumnChunk::TPtr>> portionColumns;
        for (auto&& p : columnChunks) {
            portionColumns.emplace(resultSchema->GetColumnId(p.first), p.second[i].GetChunks());
        }
        batchSlices.emplace_back(portionColumns, schemaDetails, context.Counters.SplitterCounters, GetSplitSettings());
    }

    TSimilarSlicer slicer(4 * 1024 * 1024);
    auto packs = slicer.Split(batchSlices);

    for (auto&& i : packs) {
        TGeneralSerializedSlice slice(std::move(i));
        std::vector<std::vector<IPortionColumnChunk::TPtr>> chunksByBlobs = slice.GroupChunksByBlobs();
        AppendedPortions.emplace_back(TPortionInfoWithBlobs::BuildByBlobs(chunksByBlobs, nullptr, GranuleMeta->GetGranuleId(), *maxSnapshot, SaverContext.GetStorageOperator()));
        NArrow::TFirstLastSpecialKeys specialKeys(slice.GetFirstLastPKBatch(resultSchema->GetIndexInfo().GetReplaceKey()));
        AppendedPortions.back().GetPortionInfo().AddMetadata(*resultSchema, specialKeys, SaverContext.GetTierName());
    }
    if (IS_DEBUG_LOG_ENABLED(NKikimrServices::TX_COLUMNSHARD)) {
        TStringBuilder sbSwitched;
        sbSwitched << "";
        for (auto&& p : SwitchedPortions) {
            sbSwitched << p.DebugString() << ";";
        }
        sbSwitched << "";

        TStringBuilder sbAppended;
        for (auto&& p : AppendedPortions) {
            sbAppended << p.GetPortionInfo().DebugString() << ";";
        }
        AFL_DEBUG(NKikimrServices::TX_COLUMNSHARD)("event", "blobs_created_diff")("appended", sbAppended)("switched", sbSwitched);
    }
    AFL_INFO(NKikimrServices::TX_COLUMNSHARD)("event", "blobs_created")("appended", AppendedPortions.size())("switched", SwitchedPortions.size());

    return TConclusionStatus::Success();
}

void TGeneralCompactColumnEngineChanges::DoWriteIndexComplete(NColumnShard::TColumnShard& self, TWriteIndexCompleteContext& context) {
    TBase::DoWriteIndexComplete(self, context);
    self.IncCounter(context.FinishedSuccessfully ? NColumnShard::COUNTER_SPLIT_COMPACTION_SUCCESS : NColumnShard::COUNTER_SPLIT_COMPACTION_FAIL);
    self.IncCounter(NColumnShard::COUNTER_SPLIT_COMPACTION_BLOBS_WRITTEN, context.BlobsWritten);
    self.IncCounter(NColumnShard::COUNTER_SPLIT_COMPACTION_BYTES_WRITTEN, context.BytesWritten);
}

void TGeneralCompactColumnEngineChanges::DoStart(NColumnShard::TColumnShard& self) {
    TBase::DoStart(self);
    auto& g = *GranuleMeta;
    self.CSCounters.OnSplitCompactionInfo(g.GetAdditiveSummary().GetCompacted().GetPortionsSize(), g.GetAdditiveSummary().GetCompacted().GetPortionsCount());
}

NColumnShard::ECumulativeCounters TGeneralCompactColumnEngineChanges::GetCounterIndex(const bool isSuccess) const {
    return isSuccess ? NColumnShard::COUNTER_COMPACTION_SUCCESS : NColumnShard::COUNTER_COMPACTION_FAIL;
}

}
