// minimalprobe MODE [flags]
//   alive  a fresh blob with random input (or none): rate, pulse, silent and stuck neurons
//   life   one long Echo (or any task) life, then the same plus the wiring at the end
//   idle   teach a task, test frozen, unplug for a long stretch, test frozen again
//   speed  ticks per second on one core at 4,096, 65,536 and 1,048,576 neurons
//   trace  delayed echo: teach, freeze, then how readable the cue is tick by tick after it
//          goes off (the whole sheet and the motor neurons)
//   linear a ridge reader of every neuron's spike counts on a real task, frozen sheet (as
//          blobprobe linear): --trials N, --warmup N ticks, --teach-steps N learned first
// --engine full|minimal picks the engine (default minimal). blobrun's task, plug, seed and
// --g. flags all work; --m.name value sets a minimal genome number.

#include "minimal/probe/probe.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static const char* const probe_only[] = {"--idle-ticks", "--test-steps",  "--tail-ticks",
                                         "--input",      "--warmup",      "--tiles",
                                         "--seconds-ms", "--teach-steps", "--trials"};

bool probe_args(int argc, char** argv, ProbeArgs& args) {
    // probe flags are read here; the rest goes to blobrun's parser
    std::vector<char*> rest = {argv[0]};
    for (int i = 1; i < argc; i++) {
        bool mine = false;
        for (const char* name : probe_only) {
            if (std::strcmp(argv[i], name) == 0 && i + 1 < argc) {
                int64_t v = std::strtoll(argv[i + 1], nullptr, 10);
                if (std::strcmp(name, "--idle-ticks") == 0) {
                    args.idle_ticks = v;
                } else if (std::strcmp(name, "--test-steps") == 0) {
                    args.test_steps = v;
                } else if (std::strcmp(name, "--tail-ticks") == 0) {
                    args.tail_ticks = v;
                } else if (std::strcmp(name, "--input") == 0) {
                    args.input = (int)v;
                } else if (std::strcmp(name, "--warmup") == 0) {
                    args.warmup = (int)v;
                } else if (std::strcmp(name, "--tiles") == 0) {
                    args.tiles = (int)v;
                } else if (std::strcmp(name, "--teach-steps") == 0) {
                    args.teach_steps = v;
                } else if (std::strcmp(name, "--trials") == 0) {
                    args.trials = (int)v;
                } else {
                    args.seconds_ms = (int)v;
                }
                mine = true;
                i++;
                break;
            }
        }
        if (!mine) {
            rest.push_back(argv[i]);
        }
    }
    apply_genome_args((int)rest.size(), rest.data(), args.flags.genome);
    apply_minimal_args((int)rest.size(), rest.data(), args.mg);
    if (!run_flags_parse((int)rest.size(), rest.data(), args.flags)) {
        return false;
    }
    if (args.flags.engine == "full" || args.flags.engine == "blob") {
        args.engine = ProbeEngine::full;
    } else if (args.flags.engine == "minimal") {
        args.engine = ProbeEngine::minimal;
    } else {
        std::fprintf(stderr, "minimalprobe: engine full or minimal\n");
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fputs(
            "minimalprobe alive|life|idle|speed|trace|linear [--engine full|minimal] [flags]\n",
            stderr);
        return 1;
    }
    ProbeArgs args;
    // blobrun defaults to the full engine; the probe defaults to the minimal one
    args.flags.engine = "minimal";
    if (!probe_args(argc - 1, argv + 1, args)) {
        return 1;
    }
    const char* mode = argv[1];
    if (std::strcmp(mode, "alive") == 0) {
        return probe_alive(args);
    }
    if (std::strcmp(mode, "life") == 0) {
        return probe_life(args);
    }
    if (std::strcmp(mode, "idle") == 0) {
        return probe_idle(args);
    }
    if (std::strcmp(mode, "speed") == 0) {
        return probe_speed(args);
    }
    if (std::strcmp(mode, "trace") == 0) {
        return probe_trace(args);
    }
    if (std::strcmp(mode, "linear") == 0) {
        return probe_linear(args);
    }
    std::fprintf(stderr, "minimalprobe: no mode '%s'\n", mode);
    return 1;
}
