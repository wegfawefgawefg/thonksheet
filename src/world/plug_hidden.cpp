#include "world/plug.hpp"

// fill-in: a hidden line's cells are written off, but each sensor cell still keeps what it
// would have done from the sheet's own input (blob.predicted). count those guesses against
// the truth the task kept back.
void plug_count_hidden(const Plug& plug, const Blob& blob, uint32_t hidden, uint32_t hidden_on,
                       TaskStepIn& in) {
    if (!blob.predicted) {
        return;
    }
    int group = plug_sensor_group(plug);
    for (int i = 0; i < plug.shape.singles && i < 32; i++) {
        if (!((hidden >> i) & 1)) {
            continue;
        }
        bool truth = (hidden_on >> i) & 1;
        int line = plug.first_single + i;
        for (int k = 0; k < group; k++) {
            int n = plug.sensor_neuron[line * PLUG_MAX_GROUP + k];
            int guess = (int)((blob.predicted[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1);
            if (truth) {
                in.fill_on++;
                in.fill_hit += guess;
            } else {
                in.fill_off++;
                in.fill_false += guess;
            }
        }
    }
}
