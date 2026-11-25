// blobprobe critic: does the critic know how things are going? Runs one life of a task as
// blobrun does (the world calls blob_reward every step when critic_on), and for the last
// --log steps writes one csv row per step: the critic cells' spikes over the step's last
// 15 ticks, the surprise and what it counted as, the move, the valence, whether the press
// was a random one, and whether the step ended in a fall (cartpole) or a judged dot (catch).
// scripts/critic_track.py bins the level by steps until the next fall. With --learning 0 the
// same cells are read on the learning-off twin, so the born level is the null.
// blobrun's task, plug, seed and --g. flags all work. Own flags: --log N (default 20000),
// --csv PATH (default out/critic-track.csv), --learning 0|1 (default 1), --oracle 0|1|2.
//
// --oracle is a diagnostic, never a candidate: the world's valence is replaced by "was this
// press the scripted player's" (+1 yes, -1 no, from the world's true state). 1: and with a
// critic the surprise is that valence alone (no level), so the actor gets a clean surprise
// one step late through its usual trace; without a critic the readout gets it as a
// one-step reward. 2: the critic still makes the surprise, from the clean valence.

#include "blob/blob.hpp"
#include "blob/critic.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);

static const char* const track_only[] = {"--log", "--csv", "--learning", "--oracle"};

int probe_critic(int argc, char** argv) {
    // own flags out, the rest to blobrun's parser (argv[1] is the word "critic")
    std::vector<char*> rest = {argv[0]};
    for (int i = 2; i < argc; i++) {
        bool mine = false;
        for (const char* name : track_only) {
            mine = mine || (std::strcmp(argv[i], name) == 0 && i + 1 < argc);
        }
        if (mine) {
            i++;
            continue;
        }
        rest.push_back(argv[i]);
    }
    RunFlags flags;
    apply_genome_args((int)rest.size(), rest.data(), flags.genome);
    if (!run_flags_parse((int)rest.size(), rest.data(), flags)) {
        return 1;
    }
    int64_t log_steps = int_arg(argc, argv, "--log", 20000);
    const char* path = text_arg(argc, argv, "--csv", "out/critic-track.csv");
    if (!int_arg(argc, argv, "--learning", 1)) {
        flags.genome.learning_on = 0;
    }
    int oracle = int_arg(argc, argv, "--oracle", 0);
    FILE* out = std::fopen(path, "w");
    if (!out) {
        std::fprintf(stderr, "blobprobe critic: cannot write %s\n", path);
        return 1;
    }
    std::fprintf(out, "step,level,surprise,sign,move,valence,explored,ended\n");

    Blob* blob = blob_birth(flags.genome, flags.seed);
    static TaskTicker tt;
    tt = TaskTicker{};
    if (!ticker_start(tt, *blob, flags)) {
        blob_free(blob);
        return 1;
    }
    bool every_step = flags.genome.critic_on != 0;
    if (oracle == 1 && every_step) {
        critic_state(*blob).oracle = 1;
    }
    int64_t step = 0, ends = 0, ends_logged = 0;
    std::vector<double> scores;
    while (step < flags.steps) {
        ticker_before_tick(tt, *blob);
        blob_tick(*blob);
        int64_t ends_before = tt.task.tally[1];
        if (!ticker_after_tick(tt, *blob)) {
            continue;
        }
        bool logged = step >= flags.steps - log_steps;
        int level = logged ? critic_level(*blob) : 0;
        int valence = tt.valence;
        if (oracle) {
            valence = tt.move < 0 ? 0 : (tt.move == tt.right_move ? 1 : -1);
        }
        if (valence != 0 || every_step) {
            plug_mark_acted(tt.plug, *blob, tt.move);
            plug_mark_vote(tt.plug, *blob, tt.move);
            blob_reward(*blob, valence);
        }
        // tally[1] counts falls in cartpole and judged dots in catch
        bool ended = tt.task.tally[1] > ends_before;
        ends += ended;
        if (logged) {
            const CriticState* c = blob->critic;
            bool learned = c && flags.genome.learning_on;
            std::fprintf(out, "%lld,%d,%d,%d,%d,%d,%d,%d\n", (long long)step, level,
                         learned ? c->surprise : 0, learned ? c->sign : 0, tt.move, tt.valence,
                         learned ? c->explored : 0, ended ? 1 : 0);
            ends_logged += ended;
        }
        step++;
        if (step % flags.window == 0) {
            scores.push_back(task_window_score(tt.task));
        }
    }
    std::fclose(out);
    // score: the mean of the first and the last tenth of the windows
    int edge = (int)scores.size() / 10 > 0 ? (int)scores.size() / 10 : 1;
    double early = 0, late = 0;
    for (int i = 0; i < edge && i < (int)scores.size(); i++) {
        early += scores[(size_t)i] / edge;
        late += scores[scores.size() - 1 - (size_t)i] / edge;
    }
    std::printf("critic track: %lld steps, %lld ends (%lld logged), score early %.3f late %.3f, "
                "csv %s\n",
                (long long)step, (long long)ends, (long long)ends_logged, early, late, path);
    blob_free(blob);
    return 0;
}
