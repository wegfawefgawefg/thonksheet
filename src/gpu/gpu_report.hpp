#pragma once

#include "gpu/gpu_flags.hpp"

#include <vector>

// the scores out: one row per life (late and early as blobrun reports them, the twin's
// beside), and optionally every window
bool gpu_write_lives(const GpuFlags& f, const GpuSettings& settings,
                     const std::vector<GpuLifeSpec>& lives,
                     const std::vector<std::vector<double>>& scores);
bool gpu_write_windows(const GpuFlags& f, const std::vector<GpuLifeSpec>& lives,
                       const std::vector<std::vector<double>>& scores);
// mean of the first and last `edge` windows (--edge, or a tenth)
void gpu_early_late(const GpuFlags& f, const std::vector<double>& s, double& early, double& late);
