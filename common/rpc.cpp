#include "rpc_client.h"
#include "rpc_server.h"
#include "rpc.pb.h"

uint64 ProtoRPCHolderBase::total_id_ = 0;


void ProtoRPCHolderBase::setTimeout(long ms)
{
    timeout_ms_ = ms;

    if (emitter_ && timeout_ms_ != -1)
    {
        emitter_->rpc_timeout_holders_.insert(std::make_pair(id_, shared_from_this()));
    }
}