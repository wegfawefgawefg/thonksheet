#include "blob/genome.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// "--g.name value" on the command line overrides one genome number, for tuning runs.

struct GenomeField {
    const char* name;
    int Genome::*field;
};

static const GenomeField fields[] = {
    {"tiles_w", &Genome::tiles_w},
    {"tiles_h", &Genome::tiles_h},
    {"inhibitory_percent", &Genome::inhibitory_percent},
    {"threshold_min", &Genome::threshold_min},
    {"threshold_max", &Genome::threshold_max},
    {"leak_shift_min", &Genome::leak_shift_min},
    {"leak_shift_max", &Genome::leak_shift_max},
    {"cooldown_min", &Genome::cooldown_min},
    {"cooldown_max", &Genome::cooldown_max},
    {"pacemakers_per_1024", &Genome::pacemakers_per_1024},
    {"pacemaker_period_min", &Genome::pacemaker_period_min},
    {"pacemaker_period_max", &Genome::pacemaker_period_max},
    {"local_connect_shift", &Genome::local_connect_shift},
    {"strong_shift", &Genome::strong_shift},
    {"far_percent", &Genome::far_percent},
    {"far_reach_tiles", &Genome::far_reach_tiles},
    {"far_delay_per_tile", &Genome::far_delay_per_tile},
    {"far_connect_shift", &Genome::far_connect_shift},
    {"local_delay_min", &Genome::local_delay_min},
    {"local_delay_max", &Genome::local_delay_max},
    {"sensor_x", &Genome::sensor_x},
    {"sensor_y", &Genome::sensor_y},
    {"sensor_w", &Genome::sensor_w},
    {"sensor_h", &Genome::sensor_h},
    {"sensor_stride", &Genome::sensor_stride},
    {"motor_x", &Genome::motor_x},
    {"motor_y", &Genome::motor_y},
    {"motor_w", &Genome::motor_w},
    {"motor_h", &Genome::motor_h},
    {"motor_stride", &Genome::motor_stride},
    {"fan_in_max", &Genome::fan_in_max},
    {"inhibit_gain", &Genome::inhibit_gain},
    {"branch_threshold", &Genome::branch_threshold},
    {"branch_bonus", &Genome::branch_bonus},
    {"noise_shift", &Genome::noise_shift},
    {"sector_tiles", &Genome::sector_tiles},
    {"sector_target_per_1024", &Genome::sector_target_per_1024},
    {"sector_bias_max", &Genome::sector_bias_max},
    {"threshold_up_shift", &Genome::threshold_up_shift},
    {"threshold_down_shift", &Genome::threshold_down_shift},
    {"threshold_cap", &Genome::threshold_cap},
    {"sector_smooth_shift", &Genome::sector_smooth_shift},
    {"learning_on", &Genome::learning_on},
    {"recent_ticks", &Genome::recent_ticks},
    {"predict_shift", &Genome::predict_shift},
    {"reward_shift", &Genome::reward_shift},
    {"reward_ticks", &Genome::reward_ticks},
    {"grow_shift", &Genome::grow_shift},
    {"prune_shift", &Genome::prune_shift},
    {"prune_bit_shift", &Genome::prune_bit_shift},
    {"far_redraw_shift", &Genome::far_redraw_shift},
    {"surprise_boost", &Genome::surprise_boost},
    {"surprise_high_per_1024", &Genome::surprise_high_per_1024},
    {"sector_reg_on", &Genome::sector_reg_on},
    {"threshold_reg_on", &Genome::threshold_reg_on},
    {"noise_on", &Genome::noise_on},
    {"predict_on", &Genome::predict_on},
    {"good_on", &Genome::good_on},
    {"bad_on", &Genome::bad_on},
    {"grow_on", &Genome::grow_on},
    {"prune_on", &Genome::prune_on},
    {"far_redraw_on", &Genome::far_redraw_on},
    {"strong_log", &Genome::strong_log},
    {"learn_mode", &Genome::learn_mode},
    {"flip_count", &Genome::flip_count},
    {"flip_scope", &Genome::flip_scope},
    {"flip_window_ticks", &Genome::flip_window_ticks},
    {"flip_average_shift", &Genome::flip_average_shift},
    {"noise_learn_shift", &Genome::noise_learn_shift},
    {"noise_trace_ticks", &Genome::noise_trace_ticks},
    {"reward_average_shift", &Genome::reward_average_shift},
    {"readout_shift", &Genome::readout_shift},
    {"readout_trace_ticks", &Genome::readout_trace_ticks},
    {"readout_counts", &Genome::readout_counts},
    {"readout_know_button", &Genome::readout_know_button},
    {"readout_good", &Genome::readout_good},
    {"readout_new", &Genome::readout_new},
    {"readout_budget", &Genome::readout_budget},
    {"readout_far", &Genome::readout_far},
    {"readout_center", &Genome::readout_center},
    {"readout_balance", &Genome::readout_balance},
    {"readout_evidence", &Genome::readout_evidence},
    {"readout_evidence_bits", &Genome::readout_evidence_bits},
    {"readout_evidence_strong", &Genome::readout_evidence_strong},
    {"lose_shift_steps", &Genome::lose_shift_steps},
    {"win_stay_steps", &Genome::win_stay_steps},
    {"lose_shift_fires", &Genome::lose_shift_fires},
    {"bad_cut_on", &Genome::bad_cut_on},
    {"noise_learn_sign", &Genome::noise_learn_sign},
    {"noise_learn_pair", &Genome::noise_learn_pair},
    {"hold_percent", &Genome::hold_percent},
    {"hold_cap", &Genome::hold_cap},
    {"readout_lock_levels", &Genome::readout_lock_levels},
    {"readout_lock_shift", &Genome::readout_lock_shift},
    {"readout_lock_hold", &Genome::readout_lock_hold},
    {"readout_decay", &Genome::readout_decay},
};

void apply_genome_args(int argc, char** argv, Genome& g) {
    for (int i = 0; i + 1 < argc; i++) {
        if (std::strncmp(argv[i], "--g.", 4) != 0) {
            continue;
        }
        const char* name = argv[i] + 4;
        bool known = false;
        for (const GenomeField& f : fields) {
            if (std::strcmp(f.name, name) == 0) {
                g.*f.field = (int)std::strtol(argv[i + 1], nullptr, 10);
                known = true;
            }
        }
        if (!known) {
            std::fprintf(stderr, "unknown genome field: %s\n", name);
            std::exit(1);
        }
    }
}
