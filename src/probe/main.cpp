// blobprobe: small measurements of a running blob. One word picks the measurement.

#include <cstdio>
#include <cstdlib>
#include <cstring>

int probe_rhythm(int argc, char** argv);
int probe_carry(int argc, char** argv);
int probe_feature(int argc, char** argv);
int probe_linear(int argc, char** argv);
int probe_transfer(int argc, char** argv);
int probe_invariance(int argc, char** argv);
int probe_wiring(int argc, char** argv);
int probe_replay(int argc, char** argv);
int probe_reader(int argc, char** argv);
int probe_critic(int argc, char** argv);
int probe_calc(int argc, char** argv);
int probe_predict(int argc, char** argv);

int int_arg(int argc, char** argv, const char* name, int fallback) {
    for (int i = 1; i + 1 < argc; i++) {
        if (std::strcmp(argv[i], name) == 0) {
            return std::atoi(argv[i + 1]);
        }
    }
    return fallback;
}

const char* text_arg(int argc, char** argv, const char* name, const char* fallback) {
    for (int i = 1; i + 1 < argc; i++) {
        if (std::strcmp(argv[i], name) == 0) {
            return argv[i + 1];
        }
    }
    return fallback;
}

// one line per measurement, so two branches adding one each merge cleanly
struct Probe {
    const char* name;
    int (*run)(int argc, char** argv);
};

static const Probe probes[] = {
    {"rhythm", probe_rhythm},
    {"carry", probe_carry},
    {"feature", probe_feature},
    {"linear", probe_linear},
    {"wiring", probe_wiring},
    {"replay", probe_replay},
    {"reader", probe_reader},
    {"invariance", probe_invariance},
    {"critic", probe_critic},
    {"transfer", probe_transfer},
    {"calc", probe_calc},
    {"predict", probe_predict},
};

int main(int argc, char** argv) {
    for (const Probe& probe : probes) {
        if (argc > 1 && std::strcmp(argv[1], probe.name) == 0) {
            return probe.run(argc, argv);
        }
    }
    std::fputs("blobprobe", stderr);
    for (const Probe& probe : probes) {
        std::fprintf(stderr, "%s%s", &probe == probes ? " " : "|", probe.name);
    }
    std::fputs(" [--label text] [--seed n] [--g.name value ...]\n", stderr);
    return 1;
}
