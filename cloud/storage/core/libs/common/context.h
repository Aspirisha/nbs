#pragma once

#include "public.h"

#include <library/cpp/deprecated/atomic/atomic.h>
#include <library/cpp/lwtrace/shuttle.h>

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

enum class EProcessingStage
{
    Postponed,
    Backoff,
    Last
};

struct TRequestTime
{
    TDuration TotalTime;
    TDuration ExecutionTime;

    explicit operator bool() const
    {
        return TotalTime || ExecutionTime;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TCallContextBase
    : public TThrRefBase
{
private:
    TAtomic Stage2Time[static_cast<int>(EProcessingStage::Last)] = {};
    TAtomic RequestStartedCycles = 0;
    TAtomic ResponseSentCycles = 0;
    TAtomic PossiblePostponeMicroSeconds = 0;

    // Used only in tablet throttler.
    TAtomic PostponeTs = 0;

protected:
    TAtomic PostponeTsCycles = 0;

public:
    ui64 RequestId;
    NLWTrace::TOrbit LWOrbit;

    TCallContextBase(ui64 requestId);

    TDuration GetPossiblePostponeDuration() const;
    void SetPossiblePostponeDuration(TDuration d);

    ui64 GetRequestStartedCycles() const;
    void SetRequestStartedCycles(ui64 cycles);

    ui64 GetPostponeCycles() const;
    void SetPostponeCycles(ui64 cycles);

    ui64 GetResponseSentCycles() const;
    void SetResponseSentCycles(ui64 cycles);

    void Postpone(ui64 nowCycles);
    TDuration Advance(ui64 nowCycles);

    TDuration Time(EProcessingStage stage) const;
    void AddTime(EProcessingStage stage, TDuration d);

    TRequestTime CalcRequestTime(ui64 nowCycles) const;
};

}   // namespace NCloud
