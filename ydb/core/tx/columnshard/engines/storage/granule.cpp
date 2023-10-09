#include "granule.h"
#include "storage.h"
#include <library/cpp/actors/core/log.h>
#include "optimizer/intervals/optimizer.h"
#include "optimizer/levels/optimizer.h"

namespace NKikimr::NOlap {

TGranuleAdditiveSummary::ECompactionClass TGranuleMeta::GetCompactionType(const TCompactionLimits& limits) const {
    return GetAdditiveSummary().GetCompactionClass(
        limits, ModificationLastTime, TMonotonic::Now());
}

ui64 TGranuleMeta::Size() const {
    return GetAdditiveSummary().GetGranuleSize();
}

void TGranuleMeta::UpsertPortion(const TPortionInfo& info) {
    AFL_TRACE(NKikimrServices::TX_COLUMNSHARD)("event", "upsert_portion")("portion", info.DebugString())("granule", GetGranuleId());
    auto it = Portions.find(info.GetPortion());
    AFL_VERIFY(info.GetGranule() == GetGranuleId())("event", "incompatible_granule")("portion", info.DebugString())("granule", GetGranuleId());

    AFL_VERIFY(info.Valid())("event", "invalid_portion")("portion", info.DebugString());
    AFL_VERIFY(info.ValidSnapshotInfo())("event", "incorrect_portion_snapshots")("portion", info.DebugString());
    for (auto& record : info.Records) {
        AFL_VERIFY(record.Valid())("event", "incorrect_record")("record", record.DebugString())("portion", info.DebugString());
    }

    if (it == Portions.end()) {
        OnBeforeChangePortion(nullptr);
        auto portionNew = std::make_shared<TPortionInfo>(info);
        it = Portions.emplace(portionNew->GetPortion(), portionNew).first;
    } else {
        OnBeforeChangePortion(it->second);
        it->second = std::make_shared<TPortionInfo>(info);
    }
    OnAfterChangePortion(it->second, nullptr);
}

bool TGranuleMeta::ErasePortion(const ui64 portion) {
    auto it = Portions.find(portion);
    if (it == Portions.end()) {
        AFL_WARN(NKikimrServices::TX_COLUMNSHARD)("event", "portion_erased_already")("portion_id", portion)("pathId", Record.PathId);
        return false;
    } else {
        AFL_TRACE(NKikimrServices::TX_COLUMNSHARD)("event", "portion_erased")("portion_info", it->second->DebugString())("pathId", Record.PathId);
    }
    OnBeforeChangePortion(it->second);
    Portions.erase(it);
    OnAfterChangePortion(nullptr, nullptr);
    return true;
}

void TGranuleMeta::AddColumnRecord(const TIndexInfo& indexInfo, const TPortionInfo& portion, const TColumnRecord& rec, const NKikimrTxColumnShard::TIndexPortionMeta* portionMeta) {
    auto it = Portions.find(portion.GetPortion());
    AFL_TRACE(NKikimrServices::TX_COLUMNSHARD)("event", "add_column_record")("portion_info", portion.DebugString())("record", rec.DebugString());
    if (it == Portions.end()) {
        Y_VERIFY(portion.Records.empty());
        auto portionNew = std::make_shared<TPortionInfo>(portion);
        portionNew->AddRecord(indexInfo, rec, portionMeta);
        it = Portions.emplace(portion.GetPortion(), portionNew).first;
    } else {
        AFL_VERIFY(it->second->IsEqualWithSnapshots(portion))("self", it->second->DebugString())("item", portion.DebugString());
        it->second->AddRecord(indexInfo, rec, portionMeta);
    }
    if (portionMeta) {
        it->second->InitOperator(Owner->GetStoragesManager()->InitializePortionOperator(*it->second), false);
    }
}

void TGranuleMeta::OnAfterChangePortion(const std::shared_ptr<TPortionInfo> portionAfter, NStorageOptimizer::IOptimizerPlanner::TModificationGuard* modificationGuard) {
    if (portionAfter) {
        AFL_VERIFY(PortionsByPK[portionAfter->IndexKeyStart()].emplace(portionAfter->GetPortion(), portionAfter).second);

        THashMap<TUnifiedBlobId, ui64> blobIdSize;
        for (auto&& i : portionAfter->Records) {
            blobIdSize[i.BlobRange.BlobId] += i.BlobRange.Size;
        }
        for (auto&& i : blobIdSize) {
            PortionInfoGuard.OnNewBlob(portionAfter->HasRemoveSnapshot() ? NPortion::EProduced::INACTIVE : portionAfter->GetMeta().Produced, i.second);
        }
        if (!portionAfter->HasRemoveSnapshot()) {
            if (modificationGuard) {
                modificationGuard->AddPortion(portionAfter);
            } else {
                OptimizerPlanner->StartModificationGuard().AddPortion(portionAfter);
            }
        }
    }
    if (!!AdditiveSummaryCache) {
        auto g = AdditiveSummaryCache->StartEdit(Counters);
        if (portionAfter && !portionAfter->HasRemoveSnapshot()) {
            g.AddPortion(*portionAfter);
        }
    }

    ModificationLastTime = TMonotonic::Now();
    Owner->UpdateGranuleInfo(*this);
}

void TGranuleMeta::OnBeforeChangePortion(const std::shared_ptr<TPortionInfo> portionBefore) {
    if (portionBefore) {
        {
            auto itByKey = PortionsByPK.find(portionBefore->IndexKeyStart());
            Y_VERIFY(itByKey != PortionsByPK.end());
            auto itPortion = itByKey->second.find(portionBefore->GetPortion());
            Y_VERIFY(itPortion != itByKey->second.end());
            itByKey->second.erase(itPortion);
            if (itByKey->second.empty()) {
                PortionsByPK.erase(itByKey);
            }
        }

        THashMap<TUnifiedBlobId, ui64> blobIdSize;
        for (auto&& i : portionBefore->Records) {
            blobIdSize[i.BlobRange.BlobId] += i.BlobRange.Size;
        }
        for (auto&& i : blobIdSize) {
            PortionInfoGuard.OnDropBlob(portionBefore->HasRemoveSnapshot() ? NPortion::EProduced::INACTIVE : portionBefore->GetMeta().Produced, i.second);
        }
        if (!portionBefore->HasRemoveSnapshot()) {
            OptimizerPlanner->StartModificationGuard().RemovePortion(portionBefore);
        }
    }
    if (!!AdditiveSummaryCache) {
        auto g = AdditiveSummaryCache->StartEdit(Counters);
        if (portionBefore && !portionBefore->HasRemoveSnapshot()) {
            g.RemovePortion(*portionBefore);
        }
    }
}

void TGranuleMeta::OnCompactionFinished() {
    AllowInsertionFlag = false;
    Y_VERIFY(Activity.erase(EActivity::GeneralCompaction));
    AFL_DEBUG(NKikimrServices::TX_COLUMNSHARD)("event", "OnCompactionFinished")("info", DebugString());
    Owner->UpdateGranuleInfo(*this);
}

void TGranuleMeta::OnCompactionFailed(const TString& reason) {
    AllowInsertionFlag = false;
    Y_VERIFY(Activity.erase(EActivity::GeneralCompaction));
    AFL_WARN(NKikimrServices::TX_COLUMNSHARD)("event", "OnCompactionFailed")("reason", reason)("info", DebugString());
    Owner->UpdateGranuleInfo(*this);
}

void TGranuleMeta::OnCompactionStarted() {
    AllowInsertionFlag = false;
    Y_VERIFY(Activity.empty());
    Activity.emplace(EActivity::GeneralCompaction);
}

void TGranuleMeta::RebuildAdditiveMetrics() const {
    TGranuleAdditiveSummary result;
    {
        auto g = result.StartEdit(Counters);
        for (auto&& i : Portions) {
            if (i.second->HasRemoveSnapshot()) {
                continue;
            }
            g.AddPortion(*i.second);
        }
    }
    AdditiveSummaryCache = result;
}

const NKikimr::NOlap::TGranuleAdditiveSummary& TGranuleMeta::GetAdditiveSummary() const {
    if (!AdditiveSummaryCache) {
        RebuildAdditiveMetrics();
    }
    return *AdditiveSummaryCache;
}

TGranuleMeta::TGranuleMeta(const TGranuleRecord& rec, std::shared_ptr<TGranulesStorage> owner, const NColumnShard::TGranuleDataCounters& counters, const TVersionedIndex& versionedIndex)
    : Owner(owner)
    , Counters(counters)
    , PortionInfoGuard(Owner->GetCounters().BuildPortionBlobsGuard())
    , Record(rec)
{
    Y_VERIFY(Owner);
    OptimizerPlanner = std::make_shared<NStorageOptimizer::NLevels::TLevelsOptimizerPlanner>(rec.Granule, owner->GetStoragesManager(), versionedIndex.GetLastSchema()->GetIndexInfo().GetReplaceKey());

}

bool TGranuleMeta::InCompaction() const {
    return Activity.contains(EActivity::GeneralCompaction);
}

} // namespace NKikimr::NOlap
