

## Rate-control candidates (idea 175)

`scripts/ratecontrol.sh NAMES --tests read,pred,latch,map,alive [--seeds]`, `table`,
`learned NAME`, `smoke --quick` (`ratecontrol.py`, `ratecontrol_tests.py`,
`ratecontrol_latch.py`). Candidates are genome numbers, all off by default
(`src/blob/regulate_rate.cpp`): `rate_band_shift`, `rate_band_low`, `rate_band_high`,
`rate_band_step` (a dead band in place of the per-spike walk), the critical period and
`threshold_late_shift`, `sector_gain` (per-tile inhibition in proportion),
`rate_band_motor`. `blobprobe ratehealth` (`src/probe/rate_health.cpp`): rate against
target, silent, stuck and capped cells by kind after a life. None is recommended; the one
change worth having is `--g.threshold_cap 1000`.

