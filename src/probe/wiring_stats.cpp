// blobprobe wiring: what the sheet's wiring looks like at birth, and whether it stays alive.
//
// One line of wiring numbers, then one line of liveness after --ticks ticks with no input:
//   fan       mean local connections per neuron (and far)
//   own       share of local connections from the neuron's own tile
//   slots     mean local tiles a neuron hears from
//   dist      mean cell distance of a local connection
//   recip     share of local connections A<-B where B<-A exists too
//   hub       mean connections in, and out, of the top 2% by connections out, over the median
//   flow      share of local connections on the favoured layer flows (wire_layers)
//   upx upy   mean source minus receiver, in cells (streams make this nonzero)
//   ownI ownE share from own tile for inhibitory and for excitatory receivers
//   fartiles  distinct far tiles per tile with far links (topography makes this small)
//   cross     local connections across the middle row, and far links each way (two sheets)
//   alive: spikes per 1,024 per tick against the target, and the shares silent and stuck
// --age N first lives N ticks with cue A or B (20-tick blocks) on tile (1, 1), as the
// feature and carry probes do, so learning on shows what a life does to the wiring.

#include "blob/birth_shape.hpp"
#include "blob/blob.hpp"
#include "blob/weight_levels.hpp"
#include "probe/carry_measure.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);

int probe_wiring(int argc, char** argv) {
    Genome genome;
    apply_genome_args(argc, argv, genome);
    uint64_t seed = (uint64_t)int_arg(argc, argv, "--seed", 1);
    int ticks = int_arg(argc, argv, "--ticks", 20000);
    int settle = int_arg(argc, argv, "--settle", 5000);
    const char* label = text_arg(argc, argv, "--label", "run");
    int age = int_arg(argc, argv, "--age", 0);
    Blob* blob = blob_birth(genome, seed);
    Blob& b = *blob;
    uint64_t state = 0x2545F4914F6CDD1Dull ^ seed;
    for (int i = 0; i < age; i++) {
        if (i % 20 == 0) {
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            carry_set_cue(b, b.tiles_w + 1, (int)(state >> 40) & 1);
        }
        blob_tick(b);
    }
    if (age > 0) {
        blob_detach_all(b);
    }
    int neurons = b.neuron_count, half = b.tiles_h / 2;

    std::vector<int> in(neurons, 0), out(neurons, 0);
    int64_t local = 0, far = 0, own = 0, recip = 0, favoured = 0, cross = 0;
    int64_t slots_used = 0, dist = 0, upx = 0, upy = 0;
    int64_t own_i = 0, all_i = 0, own_e = 0, all_e = 0;
    int64_t far_fwd = 0, far_back = 0;
    for (int n = 0; n < neurons; n++) {
        int t = n / TILE_NEURONS;
        bool inh = (b.inhibitory[t] >> (n % TILE_NEURONS)) & 1;
        for (int k = 0; k < SLOTS; k++) {
            uint32_t st = k < LOCAL_SLOTS ? b.local_tile[t * LOCAL_SLOTS + k]
                                          : b.far_tile[n * FAR_SLOTS + k - LOCAL_SLOTS];
            int64_t w = blob_weight_index(n, k);
            uint64_t conn = connected_bits(b.w0[w], b.w1[w]);
            if (st == (uint32_t)b.tile_count || !conn) {
                continue;
            }
            int c = __builtin_popcountll(conn);
            in[n] += c;
            if (k >= LOCAL_SLOTS) {
                far += c;
                bool from_far_half = (int)st / b.tiles_w >= half, me_far = t / b.tiles_w >= half;
                far_fwd += (!from_far_half && me_far) ? c : 0;
                far_back += (from_far_half && !me_far) ? c : 0;
            } else {
                local += c;
                slots_used++;
                own += k == 4 ? c : 0;
                (inh ? own_i : own_e) += k == 4 ? c : 0;
                (inh ? all_i : all_e) += c;
                cross += (((int)st / b.tiles_w >= half) != (t / b.tiles_w >= half)) ? c : 0;
            }
            for (uint64_t m = conn; m; m &= m - 1) {
                int src = (int)st * TILE_NEURONS + __builtin_ctzll(m);
                out[src]++;
                if (k >= LOCAL_SLOTS) {
                    continue;
                }
                int dx = blob_neuron_x(b, src) - blob_neuron_x(b, n);
                int dy = blob_neuron_y(b, src) - blob_neuron_y(b, n);
                dist += std::max(std::abs(dx), std::abs(dy));
                upx += dx;
                upy += dy;
                favoured += wire_layer_favoured(wire_group(b, src), wire_group(b, n), k == 4);
                int64_t back = blob_weight_index(src, LOCAL_SLOTS - 1 - k);
                recip += (connected_bits(b.w0[back], b.w1[back]) >> (n % TILE_NEURONS)) & 1;
            }
        }
    }
    // hubs: the top 2% by connections out
    std::vector<int> order(neurons);
    for (int n = 0; n < neurons; n++) {
        order[n] = n;
    }
    std::sort(order.begin(), order.end(), [&](int a, int c) { return out[a] > out[c]; });
    int top = neurons / 50;
    double top_in = 0, top_out = 0;
    for (int i = 0; i < top; i++) {
        top_in += in[order[i]];
        top_out += out[order[i]];
    }
    std::vector<int> sorted_in = in, sorted_out = out;
    std::sort(sorted_in.begin(), sorted_in.end());
    std::sort(sorted_out.begin(), sorted_out.end());
    double med_in = sorted_in[neurons / 2], med_out = sorted_out[neurons / 2];
    // distinct far tiles per tile
    double far_tiles = 0;
    int far_from = 0;
    for (int t = 0; t < b.tile_count; t++) {
        std::vector<uint32_t> seen;
        for (int k = 0; k < TILE_NEURONS * FAR_SLOTS; k++) {
            uint32_t ft = b.far_tile[t * TILE_NEURONS * FAR_SLOTS + k];
            if (ft != (uint32_t)b.tile_count &&
                std::find(seen.begin(), seen.end(), ft) == seen.end()) {
                seen.push_back(ft);
            }
        }
        if (!seen.empty()) {
            far_tiles += (double)seen.size();
            far_from++;
        }
    }
    double l = local > 0 ? (double)local : 1.0;
    std::printf(
        "%-24s fan %.1f+%.1f own %.3f slots %.2f dist %.2f recip %.3f hub %.1f/%.0f %.1f/%.0f "
        "flow %.3f upx %+.2f upy %+.2f ownI %.3f ownE %.3f fartiles %.1f cross %lld fwd "
        "%lld back %lld\n",
        label, (double)local / neurons, (double)far / neurons, own / l,
        (double)slots_used / neurons, dist / l, recip / l, top ? top_in / top : 0.0, med_in,
        top ? top_out / top : 0.0, med_out, favoured / l, upx / l, upy / l,
        all_i ? (double)own_i / all_i : 0.0, all_e ? (double)own_e / all_e : 0.0,
        far_from ? far_tiles / far_from : 0.0, (long long)cross, (long long)far_fwd,
        (long long)far_back);

    // alive: no input; count after settling
    std::vector<uint32_t> spikes(neurons, 0);
    int64_t fired = 0;
    for (int i = 0; i < settle + ticks; i++) {
        blob_tick(b);
        if (i < settle) {
            continue;
        }
        for (int t = 0; t < b.tile_count; t++) {
            for (uint64_t s = b.spikes[b.now][t]; s; s &= s - 1) {
                spikes[t * TILE_NEURONS + __builtin_ctzll(s)]++;
                fired++;
            }
        }
    }
    int counted = 0, silent = 0, stuck = 0;
    for (int n = 0; n < neurons; n++) {
        if ((b.sensor[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1) {
            continue;
        }
        counted++;
        silent += spikes[n] == 0;
        stuck += (int64_t)spikes[n] * 4 > ticks;
    }
    std::printf("%-24s alive %.1f per 1024 per tick (target %d), silent %.3f, stuck %.3f\n", label,
                ticks ? (double)fired * 1024.0 / ((double)ticks * neurons) : 0.0,
                b.genome.sector_target_per_1024, counted ? (double)silent / counted : 0.0,
                counted ? (double)stuck / counted : 0.0);
    blob_free(blob);
    return 0;
}
