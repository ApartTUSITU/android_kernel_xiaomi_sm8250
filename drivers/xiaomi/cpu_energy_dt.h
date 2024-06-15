#ifndef CPU_ENERGY_DT_H
#define CPU_ENERGY_DT_H

unsigned long get_nr_cap_stats(int cpu);
unsigned long get_nr_id_stats(int cpu);
void cpu_cap_dt(int cpu, unsigned int freq_index, unsigned int *power,
		unsigned int *freq, unsigned int *cap);
void cpu_ids_dt(int cpu, int idle_stat, unsigned int *power);

#endif
