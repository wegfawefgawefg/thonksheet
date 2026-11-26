#include "blob/hop_state.hpp"
#include "blob/random_words.hpp"

#include <cstdlib>

// the one-hop rule's error lines from the buttons (learn_hop.cpp has the rule). what each
// motor line should have done at the verdict just given, as a sign per line.
//
// hop_teach 1, 2: from what was pressed. on bad, a pressed line that was not right should
// have been lower (-1); with 2 the right line that was not pressed higher (+1); with
// hop_good, on good, the pressed line higher.
// hop_teach 3: from the answer alone, every verdict: a line's error is its target (1 right,
// 0 not) less its running rate of being right, in 1/65536ths.
// hop_teach 4, 5 (idea 178): the output's own signed error per line with a "fine" zone,
// every verdict, from the lines' spikes in the step (blob_hop_votes).
//   4, by share: a line's share of all the lines' spikes less its target (1 right, 0 not);
//      under hop_fine 32nds either way is fine (0). the right line is told "higher" unless
//      it has nearly all the spikes, a wrong line "lower" when it has a real share.
//   5, by margin: a wrong line is told "lower" when its spikes are within hop_fine 32nds of
//      the right line's, or above them; the right line "higher" when any wrong line is. a
//      right line that wins by more than the margin teaches nothing.
// the rule's ladder (docs/results-2026-11-25-rule-ladder.md, rung T5) used the sign of a
// softmax output less the answer, fine under 0.1. spikes have no softmax: 4 is that formula
// on shares, 5 is what the formula does when the output is sharp.
//
// coin controls (hop_coin): 1 every error on a line drawn at random (hop_teach 3: the right
// line drawn at random); 2 on distinct random lines; 3 the errors of another verdict, drawn
// from the last 256, so the errors keep their own counts and pairings and only lose their
// tie to what was shown (the ladder's coin).

void blob_hop_votes(Blob& blob, const int* count, int lines) {
    if (!blob.genome.hop_on || !blob.hop) {
        return;
    }
    HopState& h = *blob.hop;
    h.votes = lines < 64 ? lines : 64;
    for (int l = 0; l < h.votes; l++) {
        h.vote[l] = count[l];
    }
}

// hop_teach 3: every line's error, target less its running rate (the coin control draws the
// target line at random); then the rates move toward the targets
static int target_errors(Blob& blob, HopState& h, int* line, int* sign) {
    const Genome& g = blob.genome;
    uint64_t right = h.right;
    if (g.hop_coin) {
        right = 1ull << rng_below(blob.rng, (uint32_t)h.lines);
    }
    int shift = g.hop_rate_shift < 1 ? 1 : (g.hop_rate_shift > 16 ? 16 : g.hop_rate_shift);
    int count = 0;
    for (int l = 0; l < h.lines; l++) {
        int32_t target = (int32_t)((right >> l) & 1) << 16;
        int32_t e = target - h.rate[l];
        h.rate[l] += e / (1 << shift);
        line[count] = l;
        sign[count++] = e;
    }
    return count;
}

// hop_teach 4, 5: the output's own miss per line, from the step's spikes
static int vote_errors(const Blob& blob, const HopState& h, int* line, int* sign) {
    const Genome& g = blob.genome;
    int lines = h.votes < h.lines ? h.votes : h.lines;
    int fine = g.hop_fine < 0 ? 0 : (g.hop_fine > 32 ? 32 : g.hop_fine);
    int64_t total = 0, best_right = 0;
    for (int l = 0; l < lines; l++) {
        int64_t v = h.vote[l] > 0 ? h.vote[l] : 0;
        total += v;
        best_right = ((h.right >> l) & 1) && v > best_right ? v : best_right;
    }
    int count = 0;
    bool close = best_right == 0; // a silent right line always has something to learn
    for (int l = 0; l < lines; l++) {
        if ((h.right >> l) & 1) {
            continue;
        }
        int64_t v = h.vote[l] > 0 ? h.vote[l] : 0;
        bool loud = g.hop_teach == 4 ? v > 0 && 32 * v >= fine * total
                                     : v > 0 && 32 * v > (32 - fine) * best_right;
        close |= loud;
        if (loud) {
            line[count] = l;
            sign[count++] = -1;
        }
    }
    for (int l = 0; l < lines; l++) {
        if (!((h.right >> l) & 1)) {
            continue;
        }
        int64_t v = h.vote[l] > 0 ? h.vote[l] : 0;
        bool low = g.hop_teach == 4 ? total == 0 || 32 * (total - v) >= fine * total : close;
        if (low) {
            line[count] = l;
            sign[count++] = 1;
        }
    }
    return count;
}

// hop_coin 3: this verdict's errors go into the ring, another verdict's come out
static int ring_swap(Blob& blob, HopState& h, int* line, int* sign, int count) {
    if (!h.ring_up) {
        h.ring_up = (uint64_t*)std::calloc(HOP_COIN_RING, sizeof(uint64_t));
        h.ring_down = (uint64_t*)std::calloc(HOP_COIN_RING, sizeof(uint64_t));
    }
    uint64_t up = 0, down = 0;
    for (int i = 0; i < count; i++) {
        (sign[i] > 0 ? up : down) |= 1ull << line[i];
    }
    uint32_t kept = h.ring_count < HOP_COIN_RING ? h.ring_count : HOP_COIN_RING;
    uint64_t got_up = up, got_down = down;
    if (kept > 0) {
        uint32_t r = rng_below(blob.rng, kept);
        got_up = h.ring_up[r];
        got_down = h.ring_down[r];
    }
    h.ring_up[h.ring_count % HOP_COIN_RING] = up;
    h.ring_down[h.ring_count % HOP_COIN_RING] = down;
    h.ring_count++;
    count = 0;
    for (int l = 0; l < 64; l++) {
        int e = (int)((got_up >> l) & 1) - (int)((got_down >> l) & 1);
        if (e != 0) {
            line[count] = l;
            sign[count++] = e;
        }
    }
    return count;
}

int hop_button_errors(Blob& blob, HopState& h, int valence, int* line, int* sign) {
    const Genome& g = blob.genome;
    int count = 0;
    if (g.hop_teach == 3) {
        return target_errors(blob, h, line, sign);
    }
    if (g.hop_teach == 4 || g.hop_teach == 5) {
        count = vote_errors(blob, h, line, sign);
    }
    for (int l = 0; (g.hop_teach == 1 || g.hop_teach == 2) && l < h.lines; l++) {
        bool pressed = (h.pressed >> l) & 1, right = (h.right >> l) & 1;
        int e = 0;
        if (valence < 0) {
            e = pressed && !right ? -1 : (g.hop_teach == 2 && right && !pressed ? 1 : 0);
        } else if (g.hop_good && pressed) {
            e = 1;
        }
        if (e != 0) {
            line[count] = l;
            sign[count++] = e;
        }
    }
    if (g.hop_coin == 3) {
        return ring_swap(blob, h, line, sign, count);
    }
    // the coin control: the same errors, each on a line of its kind drawn at random.
    // hop_coin 2: on distinct lines, so errors never cancel on one line and as many cells
    // are taught
    for (int i = 0; g.hop_coin && i < count; i++) {
        line[i] = (int)rng_below(blob.rng, (uint32_t)h.lines);
        for (int j = 0; g.hop_coin == 2 && j < i; j++) {
            if (line[j] == line[i]) {
                line[i] = (int)rng_below(blob.rng, (uint32_t)h.lines);
                j = -1;
            }
        }
    }
    return count;
}
