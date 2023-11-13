#include "checkpoint_light.h"

#include <cloud/storage/core/libs/common/error.h>

namespace {

////////////////////////////////////////////////////////////////////////////////

const TString EmptyCheckpointId = "";

}   // namespace

////////////////////////////////////////////////////////////////////////////////

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

TCheckpointLight::TCheckpointLight(ui64 blocksCount)
    : BlocksCount{blocksCount}
    , CurrentDirtyBlocks{TCompressedBitmap{blocksCount}}
    , FutureDirtyBlocks{TCompressedBitmap{blocksCount}}
{
    CurrentDirtyBlocks.Set(0, BlocksCount);
    FutureDirtyBlocks.Set(0, BlocksCount);
}

const TString& TCheckpointLight::GetCheckpointId() const
{
    return CheckpointId;
}

const TString& TCheckpointLight::GetPreviousCheckpointId() const
{
    return PreviousCheckpointId;
}

void TCheckpointLight::CreateCheckpoint(const TString& checkpointId)
{
    if (checkpointId == CheckpointId) {
        return;
    }
    PreviousCheckpointId = std::move(CheckpointId);
    CheckpointId = std::move(checkpointId);

    CurrentDirtyBlocks = std::move(FutureDirtyBlocks);
    FutureDirtyBlocks.Clear();
}

void TCheckpointLight::DeleteCheckpoint(const TString& checkpointId)
{
    if (checkpointId == CheckpointId) {
        CheckpointId = EmptyCheckpointId;
    }
    if (checkpointId == PreviousCheckpointId) {
        PreviousCheckpointId = EmptyCheckpointId;
    }
}

bool TCheckpointLight::CheckpointExists(const TString& checkpointId) const {
    if (checkpointId == EmptyCheckpointId ||
        (checkpointId == PreviousCheckpointId && PreviousCheckpointId != EmptyCheckpointId) ||
        (checkpointId == CheckpointId && CheckpointId != EmptyCheckpointId)) {
        return true;
    }

    return false;
}

NProto::TError TCheckpointLight::FindDirtyBlocksBetweenCheckpoints(
    const TString& lowCheckpointId,
    const TString& highCheckpointId,
    const TBlockRange64& blockRange,
    TString& mask) const
{
    if (blockRange.End >= BlocksCount) {
        return MakeError(E_ARGUMENT, "Block range is out of bounds");
    }

    bool allOnes = !CheckpointExists(lowCheckpointId) ||
        !CheckpointExists(highCheckpointId) ||
        lowCheckpointId == EmptyCheckpointId;

    bool useCurrentDirtyBlocks = false;
    bool useFutureDirtyBlocks = false;
    if (lowCheckpointId == CheckpointId && highCheckpointId == EmptyCheckpointId) {
        useFutureDirtyBlocks = true;
    } else if (lowCheckpointId == PreviousCheckpointId &&
        highCheckpointId == CheckpointId) {
        useCurrentDirtyBlocks = true;
    } else if (lowCheckpointId == PreviousCheckpointId &&
        highCheckpointId == EmptyCheckpointId) {
        useCurrentDirtyBlocks = true;
        useFutureDirtyBlocks = true;
    } else {
        allOnes = true;
    }

    auto test = [&](ui64 i) {
        if (allOnes) {
            return true;
        }

        if (useCurrentDirtyBlocks && useFutureDirtyBlocks) {
            return CurrentDirtyBlocks.Test(i) || FutureDirtyBlocks.Test(i);
        }

        if (useFutureDirtyBlocks) {
            return FutureDirtyBlocks.Test(i);
        }

        if (useCurrentDirtyBlocks) {
            return CurrentDirtyBlocks.Test(i);
        }

        return true;
    };

    mask.clear();
    mask.assign((blockRange.Size() + 8 - 1) / 8, 0);

    auto blockIndex = blockRange.Start;
    for (size_t i = 0; i < mask.size(); ++i) {
        ui8 bitData = 0;
        for (auto j = 0; j < 8 && blockIndex <= blockRange.End; ++j, ++blockIndex) {
            bitData |= test(blockIndex) << j;
        }

        mask[i] = bitData;
    }

    return {};
}

void TCheckpointLight::Set(const TBlockRange64& blockRange)
{
    FutureDirtyBlocks.Set(blockRange.Start, blockRange.End + 1);
}

const TCompressedBitmap& TCheckpointLight::GetCurrentDirtyBlocks() const
{
    return CurrentDirtyBlocks;
}

}   // namespace NCloud::NBlockStore::NStorage
