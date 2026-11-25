// the memory engine's tick, frame and reward (src/memory/memory_tick.cpp, memory_code.cpp,
// memory_learn.cpp). needs from the including file: g_life, g_world, g_tid, g_wg, the shared
// words and world_row(), cell_attached(), button_cell(), hash_now(), level_of(), rng_load().

// what crosses threads goes through the buffers: make it seen before the barrier
void sync() {
    memoryBarrierBuffer();
    barrier();
}

uint row_base() {
    return g_life * uint(IN_WORDS * 64 * FIELD_WORDS);
}
uint counter_at(int t, uint slot, int bits) {
    return g_life * uint(COUNTERS) + ((uint(t) << bits) | slot) * uint(UNITS);
}
uint slot_at(int ring, int t) {
    return g_life * uint(FRAMES * TABLES) + uint(ring * TABLES + t);
}
uint frame_at(int ring) {
    return g_life * uint(FRAMES) + uint(ring);
}
uint cell_at(int c) {
    return g_life * uint(MOTOR_CELLS) + uint(c);
}

// what the world wrote on sensor tile t this tick, as plug_write leaves sense_in (the task's
// lines, then good and bad)
uint64_t sensed(int t) {
    uint64_t s = s_resting == 0 ? s_task[t] : 0UL;
    s |= (s_lines & 1u) != 0u ? world_row(1, t) : 0UL;
    s |= (s_lines & 2u) != 0u ? world_row(2, t) : 0UL;
    return s;
}

// a random attached cell with no button, kept until it is pressed or a reward redraws it
void check_new_cell(int motor_lines) {
    int nc = lives[g_life].new_cell;
    if (nc >= 0 && unit_of[cell_at(nc)] < 0 && cell_attached(nc, motor_lines)) {
        return;
    }
    int count = 0;
    for (int c = 0; c < MOTOR_CELLS; c++) {
        count += unit_of[cell_at(c)] < 0 && cell_attached(c, motor_lines) ? 1 : 0;
    }
    lives[g_life].new_cell = -1;
    if (count == 0) {
        return;
    }
    Rng r = rng_load();
    int pick = int(rng_below(r, uint(count)));
    rng_store(r);
    for (int c = 0; c < MOTOR_CELLS; c++) {
        if (unit_of[cell_at(c)] < 0 && cell_attached(c, motor_lines) && pick-- == 0) {
            lives[g_life].new_cell = c;
            return;
        }
    }
}

// the largest sum among attached buttons, ties by the draw made at the last reward; a new
// button counts as a sum of 0
int choose(uint attached, bool unassigned) {
    int eu = lives[g_life].explore_unit;
    if (eu >= 0 && ((attached >> eu) & 1u) != 0u) {
        return eu;
    }
    if (eu == -1 && unassigned) {
        return -1;
    }
    int best = -2;
    int64_t best_score = 0L;
    for (int u = 0; u < lives[g_life].units; u++) {
        if (((attached >> u) & 1u) == 0u) {
            continue;
        }
        int64_t score = int64_t(s_sum[u]) * 256L + int64_t(lives[g_life].tie_key[u]);
        if (best == -2 || score > best_score) {
            best = u;
            best_score = score;
        }
    }
    if (unassigned) {
        int64_t score = int64_t(lives[g_life].tie_key[UNITS]);
        if (best == -2 || score > best_score) {
            best = -1;
        }
    }
    return best;
}

// a frame: the slot of every table, the sums per button, the button to drive
void new_frame(MemGenome G, int motor_lines) {
    if (g_tid == 0) {
        int64_t index = lives[g_life].frame_count;
        lives[g_life].frame_count = index + 1L;
        s_e = uint(index & int64_t(FRAMES - 1));
        s_attached_units = 0u;
        s_unassigned = 0u;
    }
    for (int u = g_tid; u < UNITS; u += g_wg) {
        s_sum[u] = 0;
    }
    // the address: now's code and length, then each past event's code and length
    const uint level_base = g_life * uint(LEVELS * FIELD_WORDS);
    for (int j = g_tid; j < 10 * FIELD_WORDS; j += g_wg) {
        int f = j / FIELD_WORDS, w = j % FIELD_WORDS, k = (f - 2) / 2;
        uint64_t v;
        if (f == 0) {
            v = lives[g_life].code_now[w];
        } else if (f == 1) {
            v = levels[level_base + uint(lives[g_life].level_now * FIELD_WORDS + w)];
        } else if (f % 2 == 0) {
            v = lives[g_life].past_code[k * FIELD_WORDS + w];
        } else {
            v = levels[level_base + uint(lives[g_life].past_level[k] * FIELD_WORDS + w)];
        }
        s_field[f][w] = v;
    }
    sync();
    const int ring = int(s_e), bits = G.table_bits, units = lives[g_life].units;
    for (int t = g_tid; t < G.tables; t += g_wg) {
        uint slot = 0u;
        for (int i = 0; i < bits; i++) {
            uint p = pos[g_life * uint(POS) + uint(t * bits + i)];
            uint bit = p & 1023u;
            slot |= uint((s_field[p >> 10][bit >> 6] >> (bit & 63u)) & 1UL) << i;
        }
        frame_slot[slot_at(ring, t)] = slot;
        uint base = counter_at(t, slot, bits);
        for (int u = 0; u < units; u++) {
            int v = int(counters[base + uint(u)]);
            if (v != 0) {
                atomicAdd(s_sum[u], v);
            }
        }
    }
    // buttons with an attached cell, and whether any attached cell has no button yet
    for (int c = g_tid; c < MOTOR_CELLS; c += g_wg) {
        if (!cell_attached(c, motor_lines)) {
            continue;
        }
        int u = unit_of[cell_at(c)];
        if (u >= 0) {
            atomicOr(s_attached_units, 1u << u);
        } else {
            atomicOr(s_unassigned, 1u);
        }
    }
    sync();
    if (g_tid == 0) {
        bool unassigned = s_unassigned != 0u;
        if (unassigned) {
            check_new_cell(motor_lines);
            unassigned = lives[g_life].new_cell >= 0;
        }
        int drive = choose(s_attached_units, unassigned);
        lives[g_life].drive = drive;
        int64_t tick = lives[g_life].tick;
        frames[frame_at(ring)].first_tick = tick;
        frames[frame_at(ring)].last_tick = tick;
        frames[frame_at(ring)].unit = drive;
    }
    sync();
}

// one tick: read what the world wrote, cut events, make a frame when the address changes,
// fire the chosen button's motor cells, and predict the next input (memory_tick)
void memory_tick(MemGenome G) {
    const int motor_lines = lives[g_life].motor_lines;
    if (g_tid == 0) {
        lives[g_life].tick += 1L;
        s_changed = 0;
        s_surprise = 0;
    }
    sync();
    // what is sensed, and how much of it the last prediction got wrong
    for (int w = g_tid; w < IN_WORDS; w += g_wg) {
        int t = int(meta[w]);
        uint64_t attached = world_row(3, t) & world_row(0, t);
        uint64_t sense = sensed(t) & attached;
        s_in[w] = sense;
        int wrong = bc64((lives[g_life].predicted[w] ^ sense) & attached);
        if (wrong != 0) {
            atomicAdd(s_surprise, wrong);
        }
        if (sense != lives[g_life].in_now[w]) {
            atomicOr(s_changed, 1);
        }
    }
    sync();
    if (s_changed != 0) {
        // the input that ended becomes the newest past event (push_event)
        for (int w = g_tid; w < FIELD_WORDS; w += g_wg) {
            for (int j = DEPTH - 1; j > 0; j--) {
                lives[g_life].past_code[j * FIELD_WORDS + w] =
                    lives[g_life].past_code[(j - 1) * FIELD_WORDS + w];
            }
            lives[g_life].past_code[w] = lives[g_life].code_now[w];
        }
        if (g_tid == 0) {
            for (int j = DEPTH - 1; j > 0; j--) {
                lives[g_life].past_level[j] = lives[g_life].past_level[j - 1];
                lives[g_life].past_in_hash[j] = lives[g_life].past_in_hash[j - 1];
            }
            lives[g_life].past_level[0] =
                level_of(lives[g_life].tick - lives[g_life].now_start);
            lives[g_life].past_in_hash[0] = hash_now();
        }
        sync();
        for (int w = g_tid; w < IN_WORDS; w += g_wg) {
            lives[g_life].in_now[w] = s_in[w];
        }
        // the code of now: each code word the parity (or OR) of the on cells' rows
        for (int w = g_tid; w < FIELD_WORDS; w += g_wg) {
            uint64_t word = 0UL;
            for (int iw = 0; iw < IN_WORDS; iw++) {
                for (uint64_t on = s_in[iw]; on != 0UL; on &= on - 1UL) {
                    int c = iw * 64 + lsb64(on);
                    uint64_t row = rows[row_base() + uint(c * FIELD_WORDS + w)];
                    word = G.code == 0 ? word ^ row : word | row;
                }
            }
            lives[g_life].code_now[w] = word;
        }
        if (g_tid == 0) {
            lives[g_life].now_start = lives[g_life].tick;
        }
        sync();
    }

    // a new frame when the address changes (a new event, or now's length moved a bucket),
    // and after a reward, which redraws ties and exploring
    if (g_tid == 0) {
        int level = level_of(lives[g_life].tick - lives[g_life].now_start);
        bool due = s_changed != 0 || level != lives[g_life].level_now ||
                   lives[g_life].frame_count == 0L || lives[g_life].frame_due != 0;
        s_newframe = due ? 1 : 0;
        if (due) {
            lives[g_life].level_now = level;
            lives[g_life].frame_due = 0;
        } else {
            int last = int((lives[g_life].frame_count - 1L) & int64_t(FRAMES - 1));
            frames[frame_at(last)].last_tick = lives[g_life].tick;
        }
    }
    sync();
    if (s_newframe != 0) {
        new_frame(G, motor_lines);
    }

    // fire the chosen button's attached cells, or the one cell a new button tries
    const int drive = lives[g_life].drive, new_cell = lives[g_life].new_cell;
    for (int c = g_tid; c < MOTOR_CELLS; c += g_wg) {
        bool fire = drive == -1 && new_cell >= 0
                        ? c == new_cell
                        : drive >= 0 && unit_of[cell_at(c)] == drive && cell_attached(c, motor_lines);
        if (fire) {
            uint n = motor_neuron(c);
            atomicOr(s_map[n >> 6], 1UL << (n & 63u));
        }
    }

    // the next-input store: learn what followed the last tick's key, then look up this one's
    const uint tag_base = g_life * uint(ENTRIES), in_base = g_life * uint(ENTRIES * IN_WORDS);
    if (G.predict != 0) {
        const uint64_t mask = (1UL << G.predict_bits) - 1UL;
        if (g_tid == 0) {
            bool learn = lives[g_life].predict_have != 0 && G.learning_on != 0;
            uint64_t key = lives[g_life].predict_key;
            s_e = learn ? uint(key & mask) : 0xffffffffu;
            if (learn) {
                predict_tag[tag_base + uint(key & mask)] = key | 1UL;
            }
        }
        sync();
        for (int w = g_tid; w < IN_WORDS && s_e != 0xffffffffu; w += g_wg) {
            predict_in[in_base + s_e * uint(IN_WORDS) + uint(w)] = s_in[w];
        }
        sync();
        if (g_tid == 0) {
            int64_t in_event = lives[g_life].tick - lives[g_life].now_start;
            uint64_t key = mix(mix(hash_now(), lives[g_life].past_in_hash[0]),
                               uint64_t(in_event < 255L ? in_event : 255L));
            lives[g_life].predict_key = key;
            lives[g_life].predict_have = 1;
            s_e = uint(key & mask);
            s_hit = predict_tag[tag_base + s_e] == (key | 1UL) ? 1 : 0;
        }
        sync();
        for (int w = g_tid; w < IN_WORDS; w += g_wg) {
            lives[g_life].predicted[w] = s_hit != 0
                                             ? predict_in[in_base + s_e * uint(IN_WORDS) + uint(w)]
                                             : lives[g_life].in_now[w];
        }
    } else {
        for (int w = g_tid; w < IN_WORDS; w += g_wg) {
            lives[g_life].predicted[w] = s_in[w];
        }
    }
    sync();
}

void move_counter(uint at, int by, int top) {
    int v = int(counters[at]) + by;
    counters[at] = int8_t(v > top ? top : (v < -top ? -top : v));
}

// the frames since the last reward (and before it, by how much better or worse this one is
// than usual) are credited: each (table, slot, button) once per reward
void credit(MemGenome G, int pressed, int valence) {
    if (g_tid == 0) {
        int mean = 2 * lives[g_life].good_avg - 256;
        int surprise = valence * 256 - mean;
        int older_dir = surprise > 0 ? 1 : -1;
        int older_chance = (surprise < 0 ? -surprise : surprise) / 2;
        int64_t count = lives[g_life].frame_count, tick = lives[g_life].tick;
        int64_t oldest = count > int64_t(FRAMES) ? count - int64_t(FRAMES) : 0L;
        int n = 0;
        Rng r = rng_load();
        for (int64_t i = count - 1L; i >= oldest; i--) {
            int ring = int(i & int64_t(FRAMES - 1));
            int64_t last = frames[frame_at(ring)].last_tick;
            bool before = last <= lives[g_life].last_reward_tick;
            bool too_old = last < tick - int64_t(before ? G.across_ticks : G.trace_ticks);
            if ((before && G.trace_across == 0) || too_old) {
                break;
            }
            int u = last > tick - int64_t(G.credit_ticks) ? pressed : frames[frame_at(ring)].unit;
            int d = valence;
            if (before && G.trace_across == 2) {
                if (int(rng_next(r) & 255UL) >= older_chance) {
                    continue;
                }
                d = older_dir;
            }
            if (u < 0) {
                continue;
            }
            s_cr_ring[n] = ring;
            s_cr_unit[n] = u;
            s_cr_dir[n] = d;
            n++;
        }
        rng_store(r);
        s_cr_count = n;
    }
    sync();
    const int n = s_cr_count, bits = G.table_bits, units = lives[g_life].units;
    for (int t = g_tid; t < G.tables; t += g_wg) {
        for (int k = 0; k < n; k++) {
            uint slot = frame_slot[slot_at(s_cr_ring[k], t)];
            bool seen = false;
            for (int j = 0; j < k && !seen; j++) {
                seen = s_cr_unit[j] == s_cr_unit[k] && frame_slot[slot_at(s_cr_ring[j], t)] == slot;
            }
            if (seen) {
                continue;
            }
            uint base = counter_at(t, slot, bits);
            int unit = s_cr_unit[k];
            if (s_cr_dir[k] > 0) {
                move_counter(base + uint(unit), G.good_step, G.counter_max);
            } else {
                move_counter(base + uint(unit), -G.bad_step, G.counter_max);
                for (int u = 0; u < units && G.others_up != 0; u++) {
                    if (u != unit) {
                        move_counter(base + uint(u), 1, G.counter_max);
                    }
                }
            }
        }
    }
    sync();
}

// good (+1) or bad (-1): which button was pressed (the cells of the button the world says),
// the good rate, the credit, and the draws for the next decision (memory_reward)
void memory_reward(MemGenome G, int valence, int acted) {
    const int motor_lines = lives[g_life].motor_lines;
    const bool pressed_line = acted >= 0 && acted < motor_lines;
    if (g_tid == 0) {
        s_first = 1 << 30;
        s_marked = 0;
        s_mixed = 0;
        s_fresh = 0;
    }
    sync();
    for (int c = g_tid; c < MOTOR_CELLS && pressed_line; c += g_wg) {
        if (button_cell(acted, c)) {
            atomicMin(s_first, c);
            atomicAdd(s_marked, 1);
        }
    }
    sync();
    // the button the marked cells share, or mixed
    const int shared_unit = s_marked > 0 ? unit_of[cell_at(s_first)] : -2;
    for (int c = g_tid; c < MOTOR_CELLS && pressed_line; c += g_wg) {
        if (button_cell(acted, c) && unit_of[cell_at(c)] != shared_unit) {
            atomicOr(s_mixed, 1);
        }
    }
    sync();
    if (g_tid == 0) {
        int units = lives[g_life].units, pressed = -1;
        if (s_marked == 0) {
            pressed = -1;
        } else if (s_mixed == 0 && shared_unit >= 0) {
            pressed = shared_unit;
        } else if (units < UNITS) {
            pressed = units; // a new button
            lives[g_life].units = units + 1;
            s_fresh = 1;
        }
        s_unit = pressed;
        int good = valence > 0 ? 256 : 0;
        int avg = lives[g_life].good_avg;
        lives[g_life].good_avg = avg + ((good - avg) >> G.avg_shift);
    }
    sync();
    for (int c = g_tid; c < MOTOR_CELLS && s_fresh != 0; c += g_wg) {
        if (button_cell(acted, c)) {
            unit_of[cell_at(c)] = s_unit;
        }
    }
    sync();
    if (G.learning_on != 0) {
        credit(G, s_unit, valence);
    }
    if (g_tid == 0) {
        lives[g_life].last_reward_tick = lives[g_life].tick;
        // the next decision: tie order, and maybe a random button
        Rng r = rng_load();
        for (int u = 0; u <= UNITS; u++) {
            lives[g_life].tie_key[u] = int(rng_next(r) & 255UL);
        }
        int avg = lives[g_life].good_avg, units = lives[g_life].units;
        int explore = G.explore_min + ((G.explore_max - G.explore_min) * (256 - avg) >> 8);
        lives[g_life].explore_unit = -2;
        if (int(rng_next(r) & 255UL) < explore) {
            int pick = int(rng_below(r, uint(units + 1)));
            lives[g_life].explore_unit = pick == units ? -1 : pick;
        }
        rng_store(r);
        lives[g_life].new_cell = -1;
        lives[g_life].frame_due = 1;
    }
    sync();
}
