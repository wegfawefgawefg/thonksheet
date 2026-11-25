#include "engine.hpp"

#include <cstdio>

const char* const engine_names[ENGINE_COUNT] = {"fake blob", "full engine", "minimal engine",
                                                "table neurons"};

bool engine_built(Engine engine) {
    bool built = engine == ENGINE_FAKE;
#if BLOB_HAS_CORE
    built = built || engine == ENGINE_BLOB;
#endif
#if BLOB_HAS_MINIMAL
    built = built || engine == ENGINE_MINIMAL;
#endif
#if BLOB_HAS_TABLE
    built = built || engine == ENGINE_TABLE;
#endif
    if (!built) {
        std::fprintf(stderr, "blobview: this build has no %s\n",
                     engine_names[engine < ENGINE_COUNT ? engine : 0]);
    }
    return built;
}

Blob* engine_birth(const EngineSettings& e, uint64_t seed, bool learning_on) {
    Genome g = e.genome;
    g.learning_on = learning_on ? g.learning_on : 0;
    switch (e.engine) {
#if BLOB_HAS_CORE
    case ENGINE_BLOB:
        return blob_birth(g, seed);
#endif
#if BLOB_HAS_MINIMAL
    case ENGINE_MINIMAL:
        return minimal_birth(g, e.minimal, seed);
#endif
#if BLOB_HAS_TABLE
    case ENGINE_TABLE: {
        TableGenome tg = e.table;
        tg.learning_on = learning_on ? tg.learning_on : 0;
        return table_birth(g, tg, seed);
    }
#endif
    default:
        return nullptr;
    }
}

void engine_free(Engine engine, Blob* blob) {
    if (blob == nullptr) {
        return;
    }
    switch (engine) {
#if BLOB_HAS_CORE
    case ENGINE_BLOB:
        blob_free(blob);
        break;
#endif
#if BLOB_HAS_MINIMAL
    case ENGINE_MINIMAL:
        minimal_free(blob);
        break;
#endif
#if BLOB_HAS_TABLE
    case ENGINE_TABLE:
        table_free(blob);
        break;
#endif
    default:
        break;
    }
}

void engine_tick(Engine engine, Blob& blob) {
    switch (engine) {
#if BLOB_HAS_CORE
    case ENGINE_BLOB:
        blob_tick(blob);
        break;
#endif
#if BLOB_HAS_MINIMAL
    case ENGINE_MINIMAL:
        minimal_tick(blob);
        break;
#endif
#if BLOB_HAS_TABLE
    case ENGINE_TABLE:
        table_tick(blob);
        break;
#endif
    default:
        break;
    }
}

void engine_reward(Engine engine, Blob& blob, int valence) {
    switch (engine) {
#if BLOB_HAS_CORE
    case ENGINE_BLOB:
        blob_reward(blob, valence);
        break;
#endif
#if BLOB_HAS_MINIMAL
    case ENGINE_MINIMAL:
        minimal_reward(blob, valence);
        break;
#endif
#if BLOB_HAS_TABLE
    case ENGINE_TABLE:
        table_reward(blob, valence);
        break;
#endif
    default:
        break;
    }
}

bool engine_set_learning(Engine engine, Blob& blob, bool on) {
    int* flag = nullptr;
    if (engine == ENGINE_BLOB) {
        flag = &blob.genome.learning_on;
    } else if (engine == ENGINE_MINIMAL) {
        flag = &minimal_of(blob).mg.learning_on;
    } else if (engine == ENGINE_TABLE) {
        flag = &table_of(blob).tg.learning_on;
    }
    if (flag == nullptr) {
        return false;
    }
    const bool was = *flag != 0;
    *flag = on ? 1 : 0;
    return was;
}

// a counter that only grows, as a count since the last call
static float since(uint64_t now, uint64_t& last) {
    const float d = now >= last ? static_cast<float>(now - last) : 0.0f;
    last = now;
    return d;
}

void engine_line_point(Engine engine, const Blob& blob, EngineCounts& last, LinePoint& p) {
    const BlobStats& s = blob.stats;
    p.v[LINE_FIRED] = static_cast<float>(s.fired);
    if (engine == ENGINE_MINIMAL) {
        const MinimalStats& m = minimal_of(blob).stats;
        p.v[LINE_GROWN] = since(m.gained, last.a);
        p.v[LINE_PRUNED] = since(m.lost, last.b);
        p.v[LINE_STRENGTHENED] = since(m.ev_moved, last.c);
        return;
    }
    if (engine == ENGINE_TABLE) {
        const TableStats& t = table_of(const_cast<Blob&>(blob)).stats;
        p.v[LINE_SURPRISE] = static_cast<float>(s.surprise);
        p.v[LINE_GROWN] = since(t.grown, last.a);
        p.v[LINE_STRENGTHENED] = since(t.learned, last.b);
        p.v[LINE_WEAKENED] = since(t.regulated, last.c);
        return;
    }
    p.v[LINE_SURPRISE] = static_cast<float>(s.surprise);
    p.v[LINE_GROWN] = static_cast<float>(s.grown);
    p.v[LINE_PRUNED] = static_cast<float>(s.pruned);
    p.v[LINE_STRENGTHENED] = static_cast<float>(s.strengthened);
    p.v[LINE_WEAKENED] = static_cast<float>(s.weakened);
}

uint32_t engine_lines(Engine engine) {
    const uint32_t reward = (1u << LINE_GOOD) | (1u << LINE_BAD) | (1u << LINE_FIRED);
    if (engine == ENGINE_MINIMAL) {
        return reward | (1u << LINE_GROWN) | (1u << LINE_PRUNED) | (1u << LINE_STRENGTHENED);
    }
    if (engine == ENGINE_TABLE) {
        return reward | (1u << LINE_SURPRISE) | (1u << LINE_GROWN) | (1u << LINE_STRENGTHENED) |
               (1u << LINE_WEAKENED);
    }
    return ~0u;
}

const char* engine_line_name(Engine engine, int line) {
    if (engine == ENGINE_MINIMAL) {
        if (line == LINE_GROWN) {
            return "gained";
        }
        if (line == LINE_PRUNED) {
            return "lost";
        }
        if (line == LINE_STRENGTHENED) {
            return "counters moved";
        }
    }
    if (engine == ENGINE_TABLE) {
        if (line == LINE_GROWN) {
            return "inputs swapped";
        }
        if (line == LINE_STRENGTHENED) {
            return "counters learned";
        }
        if (line == LINE_WEAKENED) {
            return "counters regulated";
        }
    }
    return line_names[line];
}
