// blobmnist: MNIST through the sheet, and the plain learners it is measured against.
//   blobmnist baselines [--mnist-code t14] [--val 1]       linear, two-layer, random projection
//   blobmnist ceiling [blobrun flags]                     a linear reader of the frozen sheet
//   blobmnist online [blobrun flags]                      the sheet's readout from good and bad
//   blobmnist ladder [blobrun flags]                      readers one handicap at a time
// Each command's own flags are listed at the top of its file. scripts/mnist.sh runs it all.

#include "mnist/mnist_ladder.hpp"
#include "mnist/mnist_run.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

int int_flag(int argc, char** argv, const char* name, int fallback) {
    for (int i = 1; i + 1 < argc; i++) {
        if (std::strcmp(argv[i], name) == 0) {
            return std::atoi(argv[i + 1]);
        }
    }
    return fallback;
}

const char* text_flag(int argc, char** argv, const char* name, const char* fallback) {
    for (int i = 1; i + 1 < argc; i++) {
        if (std::strcmp(argv[i], name) == 0) {
            return argv[i + 1];
        }
    }
    return fallback;
}

int main(int argc, char** argv) {
    const char* what = argc > 1 ? argv[1] : "";
    if (std::strcmp(what, "baselines") == 0) {
        return mnist_baselines(argc, argv);
    }
    if (std::strcmp(what, "ceiling") == 0) {
        return mnist_ceiling(argc, argv);
    }
    if (std::strcmp(what, "online") == 0) {
        return mnist_online(argc, argv);
    }
    if (std::strcmp(what, "ladder") == 0) {
        return mnist_ladder(argc, argv);
    }
    std::fputs("blobmnist baselines|ceiling|online|ladder [flags]\n", stderr);
    return 1;
}
