#include "tinyrpc_client.h"
#include "tinyrpc_server.h"
#include "tinyrpc.pb.h"

uint64_t RPCHolderBase::total_id_ = 0;


void RPCHolderBase::setTimeout(long ms)
{
    timeout_ms_ = ms;

    if (emitter_ && timeout_ms_ != -1)
    {
        emitter_->rpc_timeout_holders_.insert(std::make_pair(id_, shared_from_this()));
    }
}