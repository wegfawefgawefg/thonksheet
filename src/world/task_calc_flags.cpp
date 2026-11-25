#include "world/task.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// blobrun flags for the calculation battery (task_calc.hpp), kept apart from run_flags.cpp
// so the battery adds one line there

static const char* const op_names[(int)CalcOp::count] = {
    "copy",   "add",    "sub",   "mul",     "inc",    "compare", "and",    "or",
    "xor",    "not",    "shl",   "rotl",    "ones",   "parity",  "alu4",   "alu8",
    "program", "max",   "sort3", "kth",     "turn",   "mirror",  "move",   "scale",
    "turn3d", "move3d", "angle", "turn3",
};

const char* calc_op_name(CalcOp op) {
    return op_names[(int)op];
}

const char* const calc_flags_usage =
    "calculation battery (--task calc; docs/curriculum.md):\n"
    "  --calc OP          copy add sub mul inc compare and or xor not shl rotl ones parity\n"
    "                     alu4 alu8 program max sort3 kth turn mirror move scale turn3d\n"
    "                     move3d angle turn3 (default add)\n"
    "  --calc-bits N      operand width, register width for program (default 2)\n"
    "  --calc-len N       program: instructions (default 3)\n"
    "  --calc-size N      geometry: grid side; angle: radius (default 4)\n"
    "  --calc-list N      kth: numbers in the list (default 4)\n"
    "  --calc-hold N      cases held out, never rewarded, in sixteenths (default 4)\n"
    "  --calc-grade G     whole (one good or bad for the answer, default) or bit (a verdict\n"
    "                     per output bit, delivered to that bit's motor cells)\n"
    "  --calc-form F      bits (two motor lines per output bit, default) or buttons (one\n"
    "                     button per possible answer, at most 16; for one-button engines)\n"
    "  --calc-ball R      held-out cases bring every case within R line flips along\n"
    "                     (default 0), so no seen case is that near one\n"
    "  --calc-lay L       two operands as all of a then all of b (0, default) or in pairs\n"
    "                     a0 b0 a1 b1 ... (1)\n";

static bool whole_number(const char* text, int& out) {
    char* end = nullptr;
    long v = std::strtol(text, &end, 10);
    if (end == text || *end != '\0') {
        std::fprintf(stderr, "blobrun: '%s' is not a whole number\n", text);
        return false;
    }
    out = (int)v;
    return true;
}

int calc_parse_flag(const char* flag, const char* value, TaskSettings& settings) {
    CalcSettings& c = settings.calc;
    if (std::strncmp(flag, "--calc", 6) != 0) {
        return 0;
    }
    if (std::strcmp(flag, "--calc") == 0) {
        for (int i = 0; i < (int)CalcOp::count; i++) {
            if (std::strcmp(value, op_names[i]) == 0) {
                c.op = (CalcOp)i;
                return 1;
            }
        }
        std::fprintf(stderr, "blobrun: no calc op '%s'\n%s", value, calc_flags_usage);
        return -1;
    }
    if (std::strcmp(flag, "--calc-form") == 0) {
        if (std::strcmp(value, "bits") == 0) {
            c.form = 0;
        } else if (std::strcmp(value, "buttons") == 0) {
            c.form = 1;
        } else {
            std::fprintf(stderr, "blobrun: --calc-form is bits or buttons\n");
            return -1;
        }
        return 1;
    }
    if (std::strcmp(flag, "--calc-grade") == 0) {
        if (std::strcmp(value, "whole") == 0) {
            c.grade = 0;
        } else if (std::strcmp(value, "bit") == 0) {
            c.grade = 1;
        } else {
            std::fprintf(stderr, "blobrun: --calc-grade is whole or bit\n");
            return -1;
        }
        return 1;
    }
    int* target = std::strcmp(flag, "--calc-bits") == 0   ? &c.bits
                  : std::strcmp(flag, "--calc-len") == 0  ? &c.length
                  : std::strcmp(flag, "--calc-size") == 0 ? &c.size
                  : std::strcmp(flag, "--calc-list") == 0 ? &c.list
                  : std::strcmp(flag, "--calc-hold") == 0 ? &c.hold
                  : std::strcmp(flag, "--calc-ball") == 0 ? &c.ball
                  : std::strcmp(flag, "--calc-lay") == 0  ? &c.lay
                                                          : nullptr;
    if (!target) {
        return 0;
    }
    return whole_number(value, *target) ? 1 : -1;
}
