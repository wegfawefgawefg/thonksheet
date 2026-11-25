// blobmnist ceiling: what any reader could get from a never-taught sheet. The sheet runs
// frozen (learning off) over --fit training digits and the test digits; each digit's spike
// count for every neuron over its ticks is kept; a linear classifier (the same one the
// pixel baseline uses) is fitted on the training counts and scored on the test counts.
// Several views: the whole sheet without its sensor cells; the sensor cells alone (the
// pixels as the sheet sees them); what the motor cells can hear (their tiles and the tiles
// around, sensor cells left out); everything. Beside them, the same reader on the same
// digits' bits.
//   --fit N        training digits read (default 10000)
//   --settle N     digits shown before any are kept (default 200)
//   --test-count N test digits (default 10000)
//   --passes N     reader passes (default 3); --lr-1000 N step size in thousandths (10)
//   --pretrain N   N digits of reward-free wiring by co-activity before freezing
//   --shift 1      the reader also scored on test digits moved 1 and 2 pixels
//   --val 1        validation slice instead of the test set
//   --scale M      how the reader scales features (mnist_learn.hpp; default 0)
// the rest are blobrun's flags.

#include "mnist/mnist_run.hpp"
#include "world/mnist_data.hpp"

#include <cstdio>

static const char* const ceiling_own[] = {"--fit",     "--settle",   "--test-count", "--passes",
                                          "--lr-1000", "--pretrain", "--shift",      "--val",
                                          "--gap",     "--views",    "--scale"};

// a copy of rows with only the given columns
static void pick_columns(const Rows& in, const std::vector<int>& cols, Rows& out) {
    out = Rows{};
    out.d = (int)cols.size();
    std::vector<uint8_t> x(cols.size());
    for (int r = 0; r < in.n; r++) {
        const uint8_t* row = &in.v[(size_t)r * in.d];
        for (size_t j = 0; j < cols.size(); j++) {
            x[j] = row[cols[j]];
        }
        rows_add(out, x.data(), in.label[(size_t)r]);
    }
}

struct View {
    const char* name;
    std::vector<int> cols;
};

static bool bit_of(const uint64_t* mask, int n) {
    return (mask[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1;
}

static void make_views(const Sheet& s, std::vector<View>& views) {
    const Blob& b = *s.blob;
    View inner{"sheet, sensor cells left out", {}}, sense{"sensor cells alone", {}};
    View heard{"what the motor cells hear", {}}, all{"every neuron", {}};
    std::vector<uint8_t> near((size_t)b.tile_count, 0);
    const Plug& plug = s.tt->plug;
    for (int line = 0; line < plug.motor_lines; line++) {
        const int* cells = nullptr;
        int count = plug_motor_cells(plug, line, &cells);
        for (int k = 0; k < count; k++) {
            int t = cells[k] / TILE_NEURONS;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int x = t % b.tiles_w + dx, y = t / b.tiles_w + dy;
                    if (x >= 0 && x < b.tiles_w && y >= 0 && y < b.tiles_h) {
                        near[(size_t)(y * b.tiles_w + x)] = 1;
                    }
                }
            }
        }
    }
    for (int n = 0; n < b.neuron_count; n++) {
        bool sensor = bit_of(b.sensor, n);
        (sensor ? sense : inner).cols.push_back(n);
        if (!sensor && near[(size_t)(n / TILE_NEURONS)]) {
            heard.cols.push_back(n);
        }
        all.cols.push_back(n);
    }
    views = {inner, sense, heard, all};
}

int mnist_ceiling(int argc, char** argv) {
    Sheet s;
    if (!sheet_start(s, argc, argv, ceiling_own, (int)(sizeof ceiling_own / sizeof *ceiling_own))) {
        return 1;
    }
    int fit = int_flag(argc, argv, "--fit", 10000);
    int settle = int_flag(argc, argv, "--settle", 200);
    int test_count = int_flag(argc, argv, "--test-count", 10000);
    int passes = int_flag(argc, argv, "--passes", 3);
    float lr = (float)int_flag(argc, argv, "--lr-1000", 3) / 1000.0f;
    int64_t pretrain = int_flag(argc, argv, "--pretrain", 0);
    bool shift = int_flag(argc, argv, "--shift", 0) != 0;
    // which views to fit, as a bit mask over the list in make_views (default all four)
    int view_mask = int_flag(argc, argv, "--views", 15);
    int scale_mode = int_flag(argc, argv, "--scale", 0);
    const char* scored = s.eval_test ? "test" : "validation";
    const Mnist* data = mnist_data();

    double t0 = now_seconds();
    if (pretrain > 0) {
        sheet_pretrain(s, pretrain);
    }
    s.blob->genome.learning_on = 0; // frozen from here on
    sheet_train(s, settle, false, nullptr);
    std::vector<int> images;
    s.images = &images;
    Rows train, test;
    double a = now_seconds();
    int64_t ticks0 = s.ticks;
    sheet_train(s, fit, false, &train);
    double sheet_secs = now_seconds() - a;
    double rate = (double)(s.ticks - ticks0) / sheet_secs;
    std::vector<int> test_images;
    s.images = &test_images;
    sheet_eval(s, 0, 0, test_count, &test);
    s.images = nullptr;
    std::vector<Rows> moved;
    if (shift) {
        for (int by = 1; by <= 2; by++) {
            for (int dir = 0; dir < SHIFT_DIRECTIONS; dir++) {
                int dx = 0, dy = 0;
                shift_direction(dir, by, dx, dy);
                moved.emplace_back();
                sheet_eval(s, dx, dy, test_count, &moved.back());
            }
        }
    }
    std::printf("ceiling  seed %llu  %d neurons  ticks/digit %d  gap %d  pretrain %lld  "
                "fit on %d training digits (sheet %.1f s, %.0f ticks/s)  scored on %s (%d)\n",
                (unsigned long long)s.flags.seed, s.blob->neuron_count, s.flags.plug.ticks_per_step,
                s.gap, (long long)pretrain, fit, sheet_secs, rate, scored, test.n);
    std::printf("%-34s %7s %8s %8s %8s %9s\n", "reader of", "cells", scored, "moved 1", "moved 2",
                "reader s");
    std::vector<View> views;
    make_views(s, views);
    for (size_t v = 0; v < views.size(); v++) {
        if (!((view_mask >> v) & 1)) {
            continue;
        }
        Rows tr, te;
        pick_columns(train, views[v].cols, tr);
        pick_columns(test, views[v].cols, te);
        double r0 = now_seconds();
        Scale sc = scale_fit(tr, scale_mode);
        Linear lin;
        linear_train(lin, tr, sc, MNIST_CLASSES, passes, lr, 1);
        double reader_secs = now_seconds() - r0;
        double m[2] = {0, 0};
        for (size_t k = 0; k < moved.size(); k++) {
            Rows mv;
            pick_columns(moved[k], views[v].cols, mv);
            m[k / SHIFT_DIRECTIONS] += linear_accuracy(lin, mv, sc) / SHIFT_DIRECTIONS;
        }
        std::printf("%-34s %7zu %8.4f %8.4f %8.4f %9.1f\n", views[v].name, views[v].cols.size(),
                    linear_accuracy(lin, te, sc), m[0], m[1], reader_secs);
        std::fflush(stdout);
    }
    // the same reader on the same digits' bits, as shown to the sheet
    {
        int code = s.flags.task_settings.mnist_code;
        int w = 0, h = 0;
        mnist_code_size(code, w, h);
        Rows tr, te;
        tr.d = te.d = w * h;
        std::vector<uint8_t> bits((size_t)(w * h));
        for (int i : images) {
            mnist_encode(&data->train.pixels[(size_t)i * MNIST_PIXELS], code, 0, 0, bits.data());
            rows_add(tr, bits.data(), data->train.labels[(size_t)i]);
        }
        const MnistSet& set = s.eval_test ? data->test : data->train;
        for (int i : test_images) {
            mnist_encode(&set.pixels[(size_t)i * MNIST_PIXELS], code, 0, 0, bits.data());
            rows_add(te, bits.data(), set.labels[(size_t)i]);
        }
        Scale sc = scale_fit(tr, scale_mode);
        Linear lin;
        double r0 = now_seconds();
        linear_train(lin, tr, sc, MNIST_CLASSES, passes, lr, 1);
        std::printf("%-34s %7d %8.4f %8s %8s %9.1f\n", "the bits alone (same digits)", w * h,
                    linear_accuracy(lin, te, sc), "-", "-", now_seconds() - r0);
    }
    std::printf("total wall %.1f s\n", now_seconds() - t0);
    sheet_free(s);
    return 0;
}
