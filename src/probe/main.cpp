// blobprobe: small measurements of a running blob. One word picks the measurement.

#include <cstdio>
#include <cstdlib>
#include <cstring>

int probe_rhythm(int argc, char** argv);
int probe_carry(int argc, char** argv);
int probe_feature(int argc, char** argv);
int probe_linear(int argc, char** argv);
int probe_reader(int argc, char** argv);

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
    if (argc > 1 && std::strcmp(argv[1], "feature") == 0) {
        return probe_feature(argc, argv);
    }
    if (argc > 1 && std::strcmp(argv[1], "linear") == 0) {
        return probe_linear(argc, argv);
    }
    if (argc > 1 && std::strcmp(argv[1], "reader") == 0) {
        return probe_reader(argc, argv);
    }
    std::fputs(
        "blobprobe rhythm|carry|feature|linear|reader [--label text] [--seed n] [--g.name value ...]\n",
        stderr);
    return 1;
}
