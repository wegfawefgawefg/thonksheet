// The reference lines of the score plot: the same task run by the learning-off twin (same
// engine, genome and seed, learning off) and by coin flips (the world's stand-in), each a
// whole step at a time, never ahead of the main blob. They run on one thread of their own,
// so the main blob keeps its speed; they share nothing with it but two atomic step counts.

#include "sim.hpp"
#include "world/standin.hpp"

#include <chrono>
#include <cstdio>

bool twin_birth(Twin& twin, const SimSettings& settings) {
    twin.run = settings.run;
    twin.engine = settings.engine.engine;
    twin.learn_off = settings.twin;
    twin.steps = 0;
    twin.target = 0;
    twin.done = 0;
    {
        std::lock_guard<std::mutex> hold(twin.lock);
        twin.scores.clear();
        twin.coin_scores.clear();
    }
    if (twin.learn_off) {
        twin.blob = engine_birth(settings.engine, settings.seed, false);
        if (twin.blob == nullptr || !ticker_start(twin.ticker, *twin.blob, twin.run)) {
            return false;
        }
    }
    twin.coin = standin_birth(settings.engine.genome, settings.seed);
    return ticker_start(twin.coin_ticker, *twin.coin, twin.run);
}

void twin_free(Twin& twin) {
    engine_free(twin.engine, twin.blob);
    twin.blob = nullptr;
    if (twin.coin != nullptr) {
        standin_free(twin.coin);
        twin.coin = nullptr;
    }
}

// one whole task step for the twin and for the coin flips
static void twin_step(Twin& twin) {
    if (twin.blob != nullptr) {
        do {
            ticker_before_tick(twin.ticker, *twin.blob);
            engine_tick(twin.engine, *twin.blob);
        } while (!ticker_after_tick(twin.ticker, *twin.blob));
        if (twin.ticker.valence != 0) {
            plug_mark_acted(twin.ticker.plug, *twin.blob, twin.ticker.move);
            engine_reward(twin.engine, *twin.blob, twin.ticker.valence);
        }
    }
    TaskTicker& ct = twin.coin_ticker;
    bool answered = false;
    while (!answered) {
        const int perfect = ticker_before_tick(ct, *twin.coin);
        standin_tick(*twin.coin, ct.plug, StandinPlay::coin, perfect);
        answered = ticker_after_tick(ct, *twin.coin);
    }
    ++twin.steps;
    if (twin.steps % twin.run.window == 0) {
        std::lock_guard<std::mutex> hold(twin.lock);
        if (twin.blob != nullptr) {
            twin.scores.push_back(static_cast<float>(task_window_score(twin.ticker.task)));
        }
        twin.coin_scores.push_back(static_cast<float>(task_window_score(ct.task)));
    }
    twin.done.store(twin.steps, std::memory_order_relaxed);
}

static void twin_loop(Twin& twin) {
    while (!twin.quit.load()) {
        if (twin.steps < twin.target.load(std::memory_order_relaxed)) {
            twin_step(twin);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }
}

void twin_start(Twin& twin) {
    twin.quit = false;
    twin.thread = std::thread(twin_loop, std::ref(twin));
}

void twin_stop(Twin& twin) {
    twin.quit = true;
    if (twin.thread.joinable()) {
        twin.thread.join();
    }
}
