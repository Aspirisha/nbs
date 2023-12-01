#pragma once

#include <contrib/ydb/library/actors/core/actor.h>
#include <contrib/ydb/core/raw_socket/sock_config.h>
#include <contrib/ydb/core/raw_socket/sock_impl.h>

namespace NPG {

using namespace NKikimr::NRawSocket;

NActors::IActor* CreatePGConnection(const TActorId& listenerActorId, 
                                    TIntrusivePtr<TSocketDescriptor> socket,
                                    TNetworkConfig::TSocketAddressType address,
                                    const NActors::TActorId& databaseProxy);

} // namespace NPG
