#include "blob/blob.hpp"

#include <cstdlib>

// the motor cells back to their born wiring, and the readout's counters gone: for a frozen
// read of the inside after a life in which the output learned (docs/results-2026-11-25-hop.md;
// the same as blobprobe transfer's fresh readout). a blob is born again from the same genome
// and seed for the wiring, then freed.
void blob_motor_reborn(Blob& blob, uint64_t seed) {
    Blob* born = blob_birth(blob.genome, seed);
    for (int t = 0; t < blob.tile_count; t++) {
        for (uint64_t cells = blob.motor[t]; cells; cells &= cells - 1) {
            int n = t * TILE_NEURONS + __builtin_ctzll(cells);
            for (int k = 0; k < SLOTS; k++) {
                int64_t w = blob_weight_index(n, k);
                blob.w0[w] = born->w0[w];
                blob.w1[w] = born->w1[w];
                blob.slot_delay[w] = born->slot_delay[w];
            }
            for (int k = 0; k < FAR_SLOTS; k++) {
                blob.far_tile[n * FAR_SLOTS + k] = born->far_tile[n * FAR_SLOTS + k];
            }
            blob.fan_in[n] = born->fan_in[n];
        }
    }
    blob_free(born);
    std::free(blob.readout_row);
    std::free(blob.readout_evidence);
    std::free(blob.readout_keep);
    blob.readout_row = nullptr;
    blob.readout_evidence = nullptr;
    blob.readout_keep = nullptr;
}
