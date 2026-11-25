#pragma once
// The engines behind the Blob shell, as the viewer drives them: the full engine (src/blob),
// the minimal one (src/minimal, --engine minimal) and table neurons (src/table, --table).
// The fake blob is the viewer's own and stays in fake_blob.hpp. Everything else in the
// viewer reads the shell; only this file and the fills per engine know which is which.

#include "blob/blob.hpp"
#include "minimal/minimal.hpp"
#include "table/table.hpp"
#include "view_frame.hpp"

enum Engine { ENGINE_FAKE, ENGINE_BLOB, ENGINE_MINIMAL, ENGINE_TABLE, ENGINE_COUNT };
extern const char* const engine_names[ENGINE_COUNT];

struct EngineSettings {
    Engine engine = ENGINE_BLOB;
    Genome genome;         // sheet, regions and the full engine's numbers
    MinimalGenome minimal; // --m.NAME
    TableGenome table;     // --t.NAME
};

// false (with a message) when this build has no such engine
bool engine_built(Engine engine);
// a newborn; learning_on off makes the learning-off twin
Blob* engine_birth(const EngineSettings& e, uint64_t seed, bool learning_on);
void engine_free(Engine engine, Blob* blob);
void engine_tick(Engine engine, Blob& blob);
void engine_reward(Engine engine, Blob& blob, int valence);
// learning on or off in a live blob (the readability probe measures without teaching);
// returns what it was
bool engine_set_learning(Engine engine, Blob& blob, bool on);

// the engine's own counts since the last call, as the lines over time; which lines it has
struct EngineCounts {
    uint64_t a = 0, b = 0, c = 0, d = 0;
};
void engine_line_point(Engine engine, const Blob& blob, EngineCounts& last, LinePoint& p);
uint32_t engine_lines(Engine engine); // a bit per Line the engine fills
const char* engine_line_name(Engine engine, int line);
