#include "run/life.hpp"
#include "run/life_protocols.hpp"

#include <cmath>
#include <cstdio>
#include <string>

// extra scores per window (the calculation battery: whole answers, held-out cases, each
// output bit). early and late are the means of the first and last `edge` windows.

// windows with nothing to score (not a number) are left out; none at all is not a number
static double mean_of(const std::vector<double>& v, size_t from, size_t to) {
    double sum = 0.0;
    int count = 0;
    for (size_t i = from; i < to && i < v.size(); i++) {
        if (v[i] == v[i]) {
            sum += v[i];
            count++;
        }
    }
    return count > 0 ? sum / count : std::nan("");
}

void life_extra_report(const std::string& csv_path, const bool* ran, const LifeResult* results,
                       FILE* sum, size_t edge) {
    const LifeResult* any = nullptr;
    for (int d = 0; d < (int)Driver::count; d++) {
        if (ran[d] && results[d].extras > 0 && results[d].extra_task) {
            any = &results[d];
        }
    }
    if (!any) {
        return;
    }
    int extras = any->extras;
    char name[32];
    // late per driver; the first three to the screen
    std::printf("%-12s", "late");
    for (int d = 0; d < (int)Driver::count; d++) {
        if (ran[d]) {
            std::printf(" %10s", driver_names[d]);
        }
    }
    std::printf("\n");
    for (int i = 0; i < extras; i++) {
        task_extra_name(*any->extra_task, i, name, sizeof(name));
        if (i < 3) {
            std::printf("%-12s", name);
        }
        for (int d = 0; d < (int)Driver::count; d++) {
            if (!ran[d] || results[d].extras <= i) {
                continue;
            }
            const std::vector<double>& s = results[d].extra[i];
            size_t windows = s.size();
            size_t e = edge > windows ? windows : edge;
            double early = mean_of(s, 0, e), late = mean_of(s, windows - e, windows);
            if (i < 3) {
                std::printf(" %10.4f", late);
            }
            std::fprintf(sum, "%s,%s_early,%.6g\n", driver_names[d], name, early);
            std::fprintf(sum, "%s,%s_late,%.6g\n", driver_names[d], name, late);
        }
        if (i < 3) {
            std::printf("\n");
        }
    }
    // per window, one column per driver and score
    std::string path = csv_path;
    size_t dot = path.rfind(".csv");
    path = (dot != std::string::npos ? path.substr(0, dot) : path) + "-extra.csv";
    FILE* f = std::fopen(path.c_str(), "w");
    if (!f) {
        return;
    }
    std::fprintf(f, "window");
    for (int d = 0; d < (int)Driver::count; d++) {
        for (int i = 0; ran[d] && i < results[d].extras; i++) {
            task_extra_name(*any->extra_task, i, name, sizeof(name));
            std::fprintf(f, ",%s.%s", driver_names[d], name);
        }
    }
    std::fprintf(f, "\n");
    for (size_t w = 0; w < any->extra[0].size(); w++) {
        std::fprintf(f, "%zu", w);
        for (int d = 0; d < (int)Driver::count; d++) {
            for (int i = 0; ran[d] && i < results[d].extras; i++) {
                const std::vector<double>& s = results[d].extra[i];
                std::fprintf(f, ",%.4f", w < s.size() ? s[w] : 0.0);
            }
        }
        std::fprintf(f, "\n");
    }
    std::fclose(f);
}
