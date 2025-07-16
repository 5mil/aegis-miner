#include "miner.h"
#include <rpc/client.h>
#include <aegis_crypto.h> // Shared crypto library

QuantumMiner::QuantumMiner(const std::string& rpc_url, 
                          const std::string& rpc_user,
                          const std::string& rpc_pass)
    : rpc_client_(rpc_url)
{
    rpc_client_.set_credentials(rpc_user, rpc_pass);
}

void QuantumMiner::Start(int threads) {
    running_ = true;
    for (int i = 0; i < threads; ++i) {
        workers_.emplace_back(&QuantumMiner::MiningThread, this);
    }
}

void QuantumMiner::Stop() {
    running_ = false;
    for (auto& thread : workers_) {
        if (thread.joinable()) thread.join();
    }
}

void QuantumMiner::MiningThread() {
    while (running_) {
        // Get work from node
        auto work = rpc_client_.call("getminingdata");
        
        // Unpack work data
        uint256 header_hash = work["header_hash"].as<uint256>();
        uint256 boundary = work["boundary"].as<uint256>();
        uint256 prev_hash = work["prev_hash"].as<uint256>();
        
        // Mining loop
        for (uint32_t nonce = 0; nonce < UINT32_MAX && running_; ++nonce) {
            uint256 solution = QuantumHash(header_hash, nonce, prev_hash);
            
            if (UintToArith256(solution) < UintToArith256(boundary)) {
                // Submit solution
                rpc_client_.call("submitminingsolution", header_hash, nonce, solution);
                break;
            }
        }
    }
}