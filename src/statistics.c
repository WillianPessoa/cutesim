#include "cutesim/statistics.h"

ProcStats stats_for_process(const Process *p) {
    ProcStats st = { 0 };
    st.pid       = p->pid;
    st.arrival   = p->arrival_tick;
    st.service   = p->cpu_ticks;
    st.io_ticks  = p->io_ticks;
    st.io_count  = p->io_count;
    st.completed = (p->completion_tick >= 0) ? 1 : 0;
    st.response  = (p->first_cpu_tick >= 0) ? p->first_cpu_tick - p->arrival_tick : -1;
    if (st.completed) {
        st.turnaround = p->completion_tick - p->arrival_tick;
        st.waiting    = st.turnaround - st.service - st.io_ticks;
    }
    return st;
}

SimStats stats_compute(const Process *const *procs, int count, int total_ticks) {
    SimStats st      = { 0 };
    st.process_count = count;
    st.total_ticks   = total_ticks;

    long sum_turnaround = 0;
    long sum_waiting    = 0;
    long sum_response   = 0;

    for (int i = 0; i < count; i++) {
        ProcStats ps = stats_for_process(procs[i]);
        st.busy_ticks += ps.service;
        if (ps.completed) {
            st.completed_count++;
            sum_turnaround += ps.turnaround;
            sum_waiting += ps.waiting;
            sum_response += ps.response;
        }
    }

    if (total_ticks > 0) {
        st.cpu_utilization = (double)st.busy_ticks / total_ticks;
        st.throughput      = (double)st.completed_count / total_ticks;
    }
    if (st.completed_count > 0) {
        st.avg_turnaround = (double)sum_turnaround / st.completed_count;
        st.avg_waiting    = (double)sum_waiting / st.completed_count;
        st.avg_response   = (double)sum_response / st.completed_count;
    }
    return st;
}
