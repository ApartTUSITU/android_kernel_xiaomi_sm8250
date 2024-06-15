#ifndef MIGT_ENERGY_H
#define MIGT_ENERGY_H

int gpu_ea_start(struct devfreq *devf);
int gpu_ea_stop(struct devfreq *devf);
int gpu_ea_update_stats(struct devfreq *devf, u64 busy, u64 wall);

#endif
