// blobprobe: small measurements of a running blob. One word picks the measurement.

#include <cstdio>
#include <cstdlib>
#include <cstring>

int probe_rhythm(int argc, char** argv);
int probe_carry(int argc, char** argv);

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

int main(int argc, char** argv) {
    if (argc > 1 && std::strcmp(argv[1], "rhythm") == 0) {
        return probe_rhythm(argc, argv);
    }
    if (argc > 1 && std::strcmp(argv[1], "carry") == 0) {
        return probe_carry(argc, argv);
    }
    std::fputs("blobprobe rhythm|carry [--label text] [--seed n] [--g.name value ...]\n", stderr);
    return 1;
}
