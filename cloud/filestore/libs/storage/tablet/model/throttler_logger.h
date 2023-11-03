#pragma once

#include "public.h"

#include <cloud/storage/core/libs/throttling/tablet_throttler_logger.h>

namespace NCloud::NFileStore::NStorage {

class TIndexTabletActor;

////////////////////////////////////////////////////////////////////////////////

class TThrottlerLogger final
    : public ITabletThrottlerLogger
{
private:
    struct TImpl;
    std::unique_ptr<TImpl> Impl;

public:
    TThrottlerLogger(std::function<void(ui32, TDuration)> updateDelayCounter);
    ~TThrottlerLogger();

    void SetupLogTag(TString logTag);

    void LogRequestPostponedBeforeSchedule(
        const NActors::TActorContext& ctx,
        TCallContextBase& callContext,
        TDuration delay,
        const char* methodName) const override;

    void LogRequestPostponedAfterSchedule(
        const NActors::TActorContext& ctx,
        TCallContextBase& callContext,
        ui32 postponedCount,
        const char* methodName) const override;

    void LogRequestPostponed(TCallContextBase& callContext) const override;

    void LogPostponedRequestAdvanced(
        TCallContextBase& callContext,
        ui32 opType) const override;

    void LogRequestAdvanced(
        const NActors::TActorContext& ctx,
        TCallContextBase& callContext,
        const char* methodName) const override;

    void UpdateDelayCounter(ui32 opType, TDuration time) override;
};

}   // namespace NCloud::NFileStore::NStorage
