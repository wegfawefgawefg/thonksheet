#include "world/calc_ops.hpp"

// what each member of the calculation battery shows and wants. a case id packs the inputs
// (low bits first); calc_case unpacks one into the trial's lines and its answer. line 0 is
// the ask line; numbers are binary, low bit first; geometry is place-coded (one line of
// `size` on per coordinate), answers too.

static int bits_for(int count) {
    int b = 0;
    while ((1 << b) < count) {
        b++;
    }
    return b;
}

// lines `at` .. `at + n - 1` show `value` in binary
static void put_bits(uint32_t& lines, int& at, uint64_t value, int n) {
    for (int i = 0; i < n; i++) {
        lines |= (uint32_t)((value >> i) & 1) << (at + i);
    }
    at += n;
}

// line `at + value` of `n` is on
static void put_place(uint32_t& lines, int& at, int value, int n) {
    lines |= 1u << (at + value);
    at += n;
}

// answer bits `at` .. as one of `n` (place code)
static void out_place(uint64_t& answer, int& at, int value, int n) {
    answer |= 1ull << (at + value);
    at += n;
}

static uint64_t field(uint64_t id, int& shift, int bits) {
    uint64_t v = (id >> shift) & ((1ull << bits) - 1);
    shift += bits;
    return v;
}

// base-`base` digit of id, consumed in order
static int digit(uint64_t& id, int base) {
    int v = (int)(id % (uint64_t)base);
    id /= (uint64_t)base;
    return v;
}

static uint64_t power(uint64_t base, int times) {
    uint64_t v = 1;
    for (int i = 0; i < times; i++) {
        v *= base;
    }
    return v;
}

static int popcount(uint64_t v) {
    return __builtin_popcountll(v);
}

CalcLayout calc_layout(const CalcSettings& s) {
    CalcLayout l;
    int n = s.bits, z = s.size;
    uint64_t one = 1;
    switch (s.op) {
    case CalcOp::copy:
    case CalcOp::not_bits:
        l = {1 + n, n, one << n};
        break;
    case CalcOp::inc:
        l = {1 + n, n + 1, one << n};
        break;
    case CalcOp::add:
        l = {1 + 2 * n, n + 1, one << (2 * n)};
        break;
    case CalcOp::sub:
    case CalcOp::and_bits:
    case CalcOp::or_bits:
    case CalcOp::xor_bits:
    case CalcOp::max2:
        l = {1 + 2 * n, n, one << (2 * n)};
        break;
    case CalcOp::mul:
        l = {1 + 2 * n, 2 * n, one << (2 * n)};
        break;
    case CalcOp::compare:
        l = {1 + 2 * n, 1, one << (2 * n)};
        break;
    case CalcOp::shl:
    case CalcOp::rotl:
        l = {1 + n + bits_for(n), n, one << (n + bits_for(n))};
        break;
    case CalcOp::count_ones:
        l = {1 + n, bits_for(n + 1), one << n};
        break;
    case CalcOp::parity:
        l = {1 + n, 1, one << n};
        break;
    case CalcOp::alu4:
        l = {1 + 2 + 2 * n, n, one << (2 + 2 * n)};
        break;
    case CalcOp::alu8:
        l = {1 + 3 + 2 * n, n, one << (3 + 2 * n)};
        break;
    case CalcOp::program:
        // first instruction is always a load (n bits), the rest an op (2 bits) and n bits
        l = {1 + 2 + n, n, one << (n + (s.length - 1) * (2 + n))};
        break;
    case CalcOp::sort3:
        l = {1 + 3 * n, 3 * n, one << (3 * n)};
        break;
    case CalcOp::kth:
        l = {1 + s.list * n + bits_for(s.list), n, one << (s.list * n + bits_for(s.list))};
        break;
    case CalcOp::turn:
    case CalcOp::mirror:
        l = {1 + 2 * z, 2 * z, power(z, 2)};
        break;
    case CalcOp::move:
        l = {1 + 4 * z, 2 * z, power(z, 4)};
        break;
    case CalcOp::scale:
        l = {1 + 2 * z + (z - 1), 2 * z, power(z, 2) * (uint64_t)(z - 1)};
        break;
    case CalcOp::turn3d:
        l = {1 + 3 * z, 3 * z, power(z, 3)};
        break;
    case CalcOp::move3d:
        l = {1 + 6 * z, 3 * z, power(z, 6)};
        break;
    case CalcOp::angle:
        l = {1 + 2 * (2 * z + 1) + 8, 2 * (2 * z + 1), power(2 * z + 1, 2) * 8};
        break;
    case CalcOp::turn3:
        l = {1 + 6 * z, 6 * z, power(z, 6)};
        break;
    case CalcOp::count:
        break;
    }
    // steps that show a trial, the blank one first
    l.steps = 2;
    if (s.op == CalcOp::program) {
        l.steps = 1 + s.length;
    }
    return l;
}

// the 8 angles in 1/65536: cos and sin of k * 45 degrees
static const int64_t cos8[8] = {65536, 46341, 0, -46341, -65536, -46341, 0, 46341};
static const int64_t sin8[8] = {0, 46341, 65536, 46341, 0, -46341, -65536, -46341};

static int64_t round_fixed(int64_t v) {
    return v >= 0 ? (v + 32768) >> 16 : -((-v + 32768) >> 16);
}

// an alu result on n bits
static uint64_t alu(int op, uint64_t a, uint64_t b, uint64_t mask) {
    switch (op) {
    case 0:
        return (a + b) & mask;
    case 1:
        return (a - b) & mask;
    case 2:
        return a & b;
    case 3:
        return a | b;
    case 4:
        return a ^ b;
    case 5:
        return ~a & mask;
    case 6:
        return (a << 1) & mask;
    default:
        return a >> 1;
    }
}

bool calc_case(const CalcSettings& s, uint64_t id, CalcTrial& t) {
    CalcLayout l = calc_layout(s);
    t = CalcTrial{};
    t.steps = l.steps;
    int n = s.bits, z = s.size;
    uint64_t mask = (1ull << n) - 1;
    uint32_t& last = t.lines[l.steps - 1];
    last = 1; // the ask line
    int at = 1, shift = 0, out = 0;
    switch (s.op) {
    case CalcOp::copy:
    case CalcOp::not_bits:
    case CalcOp::inc:
    case CalcOp::count_ones:
    case CalcOp::parity: {
        uint64_t a = field(id, shift, n);
        put_bits(last, at, a, n);
        t.answer = s.op == CalcOp::copy         ? a
                   : s.op == CalcOp::not_bits   ? ~a & mask
                   : s.op == CalcOp::inc        ? a + 1
                   : s.op == CalcOp::count_ones ? (uint64_t)popcount(a)
                                                : (uint64_t)(popcount(a) & 1);
        break;
    }
    case CalcOp::add:
    case CalcOp::sub:
    case CalcOp::mul:
    case CalcOp::compare:
    case CalcOp::and_bits:
    case CalcOp::or_bits:
    case CalcOp::xor_bits:
    case CalcOp::max2: {
        uint64_t a = field(id, shift, n), b = field(id, shift, n);
        put_bits(last, at, a, n);
        put_bits(last, at, b, n);
        t.answer = s.op == CalcOp::add        ? a + b
                   : s.op == CalcOp::sub      ? (a - b) & mask
                   : s.op == CalcOp::mul      ? a * b
                   : s.op == CalcOp::compare  ? (uint64_t)(a > b)
                   : s.op == CalcOp::and_bits ? a & b
                   : s.op == CalcOp::or_bits  ? a | b
                   : s.op == CalcOp::xor_bits ? a ^ b
                                              : (a > b ? a : b);
        break;
    }
    case CalcOp::shl:
    case CalcOp::rotl: {
        int sb = bits_for(n);
        uint64_t a = field(id, shift, n), k = field(id, shift, sb);
        if ((int)k >= n) {
            return false;
        }
        put_bits(last, at, a, n);
        put_bits(last, at, k, sb);
        t.answer = s.op == CalcOp::shl ? (a << k) & mask
                                       : ((a << k) | (a >> ((n - k) % n))) & mask;
        break;
    }
    case CalcOp::alu4:
    case CalcOp::alu8: {
        int ob = s.op == CalcOp::alu4 ? 2 : 3;
        uint64_t op = field(id, shift, ob);
        uint64_t a = field(id, shift, n), b = field(id, shift, n);
        put_bits(last, at, op, ob);
        put_bits(last, at, a, n);
        put_bits(last, at, b, n);
        // the 4-op alu is add, sub, and, xor
        static const int four[4] = {0, 1, 2, 4};
        t.answer = alu(ob == 2 ? four[op] : (int)op, a, b, mask);
        break;
    }
    case CalcOp::program: {
        // ops: 0 load v, 1 add v, 2 xor v, 3 shift left by 1 (its value lines stay off)
        uint64_t reg = 0;
        for (int i = 0; i < s.length; i++) {
            uint64_t op = i == 0 ? 0 : field(id, shift, 2);
            uint64_t v = field(id, shift, n);
            if (op == 3 && v != 0) {
                return false;
            }
            int a2 = 1;
            put_bits(t.lines[1 + i], a2, op, 2);
            put_bits(t.lines[1 + i], a2, v, n);
            reg = op == 0 ? v : op == 1 ? reg + v : op == 2 ? reg ^ v : reg << 1;
            reg &= mask;
        }
        t.answer = reg;
        break;
    }
    case CalcOp::sort3: {
        uint64_t v[3] = {field(id, shift, n), field(id, shift, n), field(id, shift, n)};
        for (int i = 0; i < 3; i++) {
            put_bits(last, at, v[i], n);
        }
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2 - i; j++) {
                if (v[j] > v[j + 1]) {
                    uint64_t x = v[j];
                    v[j] = v[j + 1];
                    v[j + 1] = x;
                }
            }
        }
        t.answer = v[0] | v[1] << n | v[2] << (2 * n);
        break;
    }
    case CalcOp::kth: {
        int kb = bits_for(s.list);
        uint64_t v[16] = {};
        for (int i = 0; i < s.list; i++) {
            v[i] = field(id, shift, n);
            put_bits(last, at, v[i], n);
        }
        uint64_t k = field(id, shift, kb);
        if ((int)k >= s.list) {
            return false;
        }
        put_bits(last, at, k, kb);
        t.answer = v[k];
        break;
    }
    case CalcOp::turn:
    case CalcOp::mirror:
    case CalcOp::move:
    case CalcOp::scale: {
        uint64_t rest = id;
        int x = digit(rest, z), y = digit(rest, z);
        put_place(last, at, x, z);
        put_place(last, at, y, z);
        int nx = x, ny = y;
        if (s.op == CalcOp::turn) {
            nx = z - 1 - y;
            ny = x;
        } else if (s.op == CalcOp::mirror) {
            nx = z - 1 - x;
        } else if (s.op == CalcOp::move) {
            int dx = digit(rest, z), dy = digit(rest, z);
            put_place(last, at, dx, z);
            put_place(last, at, dy, z);
            nx = (x + dx) % z;
            ny = (y + dy) % z;
        } else {
            int k = 1 + digit(rest, z - 1);
            if (x * k >= z || y * k >= z) {
                return false;
            }
            put_place(last, at, k - 1, z - 1);
            nx = x * k;
            ny = y * k;
        }
        out_place(t.answer, out, nx, z);
        out_place(t.answer, out, ny, z);
        break;
    }
    case CalcOp::turn3d:
    case CalcOp::move3d: {
        uint64_t rest = id;
        int p[3], d[3] = {0, 0, 0};
        for (int i = 0; i < 3; i++) {
            p[i] = digit(rest, z);
            put_place(last, at, p[i], z);
        }
        int q[3] = {z - 1 - p[1], p[0], p[2]};
        if (s.op == CalcOp::move3d) {
            for (int i = 0; i < 3; i++) {
                d[i] = digit(rest, z);
                put_place(last, at, d[i], z);
                q[i] = (p[i] + d[i]) % z;
            }
        }
        for (int i = 0; i < 3; i++) {
            out_place(t.answer, out, q[i], z);
        }
        break;
    }
    case CalcOp::angle: {
        int g = 2 * z + 1;
        uint64_t rest = id;
        int x = digit(rest, g) - z, y = digit(rest, g) - z, a = digit(rest, 8);
        if (x * x + y * y > z * z) {
            return false;
        }
        put_place(last, at, x + z, g);
        put_place(last, at, y + z, g);
        put_place(last, at, a, 8);
        int64_t nx = round_fixed(x * cos8[a] - y * sin8[a]);
        int64_t ny = round_fixed(x * sin8[a] + y * cos8[a]);
        out_place(t.answer, out, (int)nx + z, g);
        out_place(t.answer, out, (int)ny + z, g);
        break;
    }
    case CalcOp::turn3: {
        uint64_t rest = id;
        for (int i = 0; i < 3; i++) {
            int x = digit(rest, z), y = digit(rest, z);
            put_place(last, at, x, z);
            put_place(last, at, y, z);
            out_place(t.answer, out, z - 1 - y, z);
            out_place(t.answer, out, x, z);
        }
        break;
    }
    case CalcOp::count:
        return false;
    }
    return true;
}
