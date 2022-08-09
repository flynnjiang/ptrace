#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
//#include <uuid/uuid.h>

#include <fstream>

#include "perfetto_trace.pb.h"

struct modify_info {
    int32_t diff_uid;
    int32_t diff_seq_id;
    int32_t diff_pid;
    int32_t diff_tid;
    int32_t diff_cpu;
    int64_t diff_time;
};

static uint64_t new_uuid(uint64_t uuid)
{
#if 0
    uuid_t ut;
    uint64_t ul;
    static std::map<uint64_t, uint64_t> uuid_map;

    if (uuid_map.find(uuid) == uuid_map.end()) {
        uuid_generate_time(ut);
        memcpy(&ul, ut, sizeof(ul))
        uuid_map[uuid] = ul;

        return ul;
    }

    return uuid_map[uuid];
#endif
    return uuid + 100;
}

static int modify_process_tree(
        ::perfetto::protos::TracePacket *pkt,
        const struct modify_info *mod)
{
     ::perfetto::protos::ProcessTree *pt;

    if (! pkt->has_process_tree())
        return -1;

    pt = pkt->mutable_process_tree();

    for (int i = 0; i < pt->processes_size(); i++) {
        ::perfetto::protos::ProcessTree_Process *ptp = pt->mutable_processes(i);

        if (ptp->has_pid())
            ptp->set_pid(ptp->pid() + mod->diff_pid);

        if (ptp->has_ppid())
            ptp->set_ppid(ptp->ppid() + mod->diff_pid);

        if (ptp->has_uid())
            ptp->set_uid(ptp->uid() + mod->diff_uid);
    }

    for (int i = 0; i < pt->threads_size(); i++) {
        ::perfetto::protos::ProcessTree_Thread *ptt = pt->mutable_threads(i);

        if (ptt->has_tid())
            ptt->set_tid(ptt->tid() + mod->diff_tid);

        if (ptt->has_tgid())
            ptt->set_tgid(ptt->tgid() + mod->diff_tid);
    }

    if (pt->has_collection_end_timestamp()) {
        pt->set_collection_end_timestamp(
                pt->collection_end_timestamp() + mod->diff_time);
    }

    return 0;
}

static int modify_ftrace_events(
        ::perfetto::protos::TracePacket *pkt,
        const struct modify_info *mod)
{
    ::perfetto::protos::FtraceEventBundle *fes;
    ::perfetto::protos::FtraceEvent *fe;

    if (! pkt->has_ftrace_events())
        return -1;

    fes = pkt->mutable_ftrace_events();

    if (fes->has_cpu())
        fes->set_cpu(fes->cpu() + mod->diff_cpu);

    for (int i = 0; i < fes->event_size(); i++) {
        fe = fes->mutable_event(i);
        if (fe->has_timestamp())
            fe->set_timestamp(fe->timestamp() + mod->diff_time);

        if (fe->has_pid())
            fe->set_pid(fe->pid() + mod->diff_pid);

        //sched_switch

        if (fe->has_sched_wakeup()) {
            ::perfetto::protos::SchedWakeupFtraceEvent *swfe;
            swfe = fe->mutable_sched_wakeup();
            if (swfe->has_pid())
                swfe->set_pid(swfe->pid() + mod->diff_pid);

            if (swfe->has_target_cpu())
                swfe->set_target_cpu(swfe->target_cpu() + mod->diff_cpu);
        }

        //sched_blocked_reason
        //sched_wakeup_new
        //sched_cpu_hotplug
        //sched_waking

        //sched_process_exec
        //sched_process_exit
        //sched_process_fork
        //sched_process_free
        //sched_process_hang
        //sched_process_wait

    }

    if (fes->has_compact_sched()) {
        ::perfetto::protos::FtraceEventBundle_CompactSched *cs;
        cs = fes->mutable_compact_sched();

        if (cs->switch_timestamp_size() > 0) {
            cs->set_switch_timestamp(0,
                    cs->switch_timestamp(0) + mod->diff_time);
        }

        for (int i = 0; i < cs->switch_next_pid_size(); i++) {
            if (cs->switch_next_pid(i)) {
                cs->set_switch_next_pid(i,
                        cs->switch_next_pid(i) + mod->diff_pid);
            }
        }
    }

    return 0;
}

static int modify_ftrace_stats(
        ::perfetto::protos::TracePacket *pkt,
        const struct modify_info *mod)
{
    double diff_time;
    ::perfetto::protos::FtraceStats *fs;
    ::perfetto::protos::FtraceCpuStats *fcs;

    if (! pkt->has_ftrace_stats())
        return -1;

    diff_time = (double)(mod->diff_time) / 1000000000.0;

    fs = pkt->mutable_ftrace_stats();

    for (int i = 0; i < fs->cpu_stats_size(); i++) {
        fcs = fs->mutable_cpu_stats(i);

        if (fcs->has_cpu())
            fcs->set_cpu(fcs->cpu() + mod->diff_cpu);

        if (fcs->has_oldest_event_ts())
            fcs->set_oldest_event_ts(fcs->oldest_event_ts() + diff_time);

        if (fcs->has_now_ts())
            fcs->set_now_ts(fcs->now_ts() + diff_time);
    }

    return 0;
}



static int modify_system_info(
        ::perfetto::protos::TracePacket *pkt,
        const struct modify_info *mod)
{
    if (pkt->has_system_info())
        pkt->clear_system_info();

    return 0;
}


static int modify_trace_config(
        ::perfetto::protos::TracePacket *pkt,
        const struct modify_info *mod)
{
    if (pkt->has_trace_config())
        pkt->clear_trace_config();

    return 0;
}

static int modify_trace_packet_defaults(
        ::perfetto::protos::TracePacket *pkt,
        const struct modify_info *mod)
{
    ::perfetto::protos::TracePacketDefaults *tpd;

    if (! pkt->has_trace_packet_defaults())
        return -1;

    tpd = pkt->mutable_trace_packet_defaults();

    if (tpd->has_track_event_defaults()) {
        ::perfetto::protos::TrackEventDefaults *ted;
        ted = tpd->mutable_track_event_defaults();

        if (ted->has_track_uuid())
            ted->set_track_uuid(new_uuid(ted->track_uuid()));
    }

    return 0;
}

static int modify_track_descriptor(
        ::perfetto::protos::TracePacket *pkt,
        const struct modify_info *mod)
{
    ::perfetto::protos::TrackDescriptor *td;

    if (! pkt->has_track_descriptor())
        return -1;

    td = pkt->mutable_track_descriptor();

    if (td->has_uuid())
        td->set_uuid(new_uuid(td->uuid()));

    if (td->has_parent_uuid())
        td->set_parent_uuid(new_uuid(td->parent_uuid()));

    if (td->has_thread()) {
        ::perfetto::protos::ThreadDescriptor *thread = td->mutable_thread();

        if (thread->has_pid())
            thread->set_pid(thread->pid() + mod->diff_pid);

        if (thread->has_tid())
            thread->set_tid(thread->tid() + mod->diff_tid);
    }

    if (td->has_process()) {
        ::perfetto::protos::ProcessDescriptor *process = td->mutable_process();

        if (process->has_pid())
            process->set_pid(process->pid() + mod->diff_pid);
    }

    return 0;
}

static int modify_track_event(
        ::perfetto::protos::TracePacket *pkt,
        const struct modify_info *mod)
{
    // do nothing
    return 0;
}

static int modify_clock_snapshot(
        ::perfetto::protos::TracePacket *pkt,
        const struct modify_info *mod)
{
    if (pkt->has_clock_snapshot())
        pkt->clear_clock_snapshot();

    return 0;
}

static int modify_service_event(
        ::perfetto::protos::TracePacket *pkt,
        const struct modify_info *mod)
{
    if (pkt->has_service_event())
        pkt->clear_service_event();

    return 0;
}

static int modify_trace_stats(
        ::perfetto::protos::TracePacket *pkt,
        const struct modify_info *mod)
{
    if (pkt->has_trace_stats())
        pkt->clear_trace_stats();

    return 0;
}

static int modify_synchronization_marker(
        ::perfetto::protos::TracePacket *pkt,
        const struct modify_info *mod)
{
    if (pkt->has_synchronization_marker())
        pkt->clear_synchronization_marker();

    return 0;
}

static void modify_trace(::perfetto::protos::Trace *trace,
                  const struct modify_info *mod)
{
    ::perfetto::protos::TracePacket *pkt;

    for (int i = 0; i < trace->packet_size(); i++) {
        pkt = trace->mutable_packet(i);

        if (pkt->has_timestamp())
            pkt->set_timestamp(pkt->timestamp() + mod->diff_time);

        switch (pkt->data_case()) {
        case ::perfetto::protos::TracePacket::DataCase::kProcessTree:
            modify_process_tree(pkt, mod);
            break;
        case ::perfetto::protos::TracePacket::DataCase::kFtraceEvents:
            modify_ftrace_events(pkt, mod);
            break;
        case ::perfetto::protos::TracePacket::DataCase::kFtraceStats:
            modify_ftrace_stats(pkt, mod);
            break;
        case ::perfetto::protos::TracePacket::DataCase::kSystemInfo:
            modify_system_info(pkt, mod);
            break;
        case ::perfetto::protos::TracePacket::DataCase::kTraceConfig:
            modify_trace_config(pkt, mod);
            break;
        case ::perfetto::protos::TracePacket::DataCase::kTrackDescriptor:
            modify_track_descriptor(pkt, mod);
            break;
        case ::perfetto::protos::TracePacket::DataCase::kTrackEvent:
            modify_track_event(pkt, mod);
            break;
        case ::perfetto::protos::TracePacket::DataCase::kClockSnapshot:
            modify_clock_snapshot(pkt, mod);
            break;
        case ::perfetto::protos::TracePacket::DataCase::kServiceEvent:
            modify_service_event(pkt, mod);
            break;
        case ::perfetto::protos::TracePacket::DataCase::kTraceStats:
            modify_trace_stats(pkt, mod);
            break;
        case ::perfetto::protos::TracePacket::DataCase::kSynchronizationMarker:
            modify_synchronization_marker(pkt, mod);
            break;
        case ::perfetto::protos::TracePacket::DataCase::DATA_NOT_SET:
            // do nothing
            break;
        default:
            fprintf(stderr, "unsupported data type: %d\n", pkt->data_case());
            break;
        }

        if (pkt->has_trusted_uid())
            pkt->set_trusted_uid(pkt->trusted_uid() + mod->diff_uid);
        
        if (pkt->has_trusted_packet_sequence_id())
            pkt->set_trusted_packet_sequence_id(
                    pkt->trusted_packet_sequence_id() + mod->diff_seq_id);

        modify_trace_packet_defaults(pkt, mod);
    }
}

int combine_file(const char *file1, const char *file2,
                 const char *output, int64_t time_diff_ns)
{
    int fd, fdo;
    struct modify_info mod;
    ssize_t n;
    char buf[1024*1024];
    ::perfetto::protos::Trace trace;

    printf("loading %s ...\n", file2);
    fd = open(file2, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open %s failed, errno = %d\n", file2, errno);
        return -1;
    }
    trace.ParseFromFileDescriptor(fd);
    close(fd);
    printf("load completed\n");

    mod.diff_uid    = 10000;
    mod.diff_seq_id = 10000;
    mod.diff_pid    = 1000000000;
    mod.diff_tid    = 1000000000;
    mod.diff_cpu    = 200;
    mod.diff_time   = time_diff_ns;

    fprintf(stderr, "modify %s:\n\
  packages      : %d\n\
  diff uid      : %d\n\
  diff seq id   : %d\n\
  diff pid      : %d\n\
  diff tid      : %d\n\
  diff cpu id   : %d\n\
  diff time     : %ld\n",
        file2, trace.packet_size(),
        mod.diff_uid, mod.diff_seq_id,
        mod.diff_pid, mod.diff_tid,
        mod.diff_cpu, mod.diff_time);

    modify_trace(&trace, &mod);
    printf("modify completed\n");

    fdo = open(output, O_WRONLY | O_CREAT, 0664);
    if (fdo < 0) {
        fprintf(stderr, "open %s failed, errno = %d\n", output, errno);
        return -1;
    }

    printf("combining %s into %s ...\n", file2, output);
    trace.SerializeToFileDescriptor(fdo);

    printf("combining %s into %s ...\n", file1, output);
    fd = open(file1, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open %s failed, errno = %d\n", file1, errno);
        close(fdo);
        return -1;
    }

    while ((n = read(fd, buf, sizeof(buf))) > 0)
        write(fdo, buf, n);

    close(fd);
    close(fdo);

    printf("combine completed\n");

#if 0
    std::ofstream ofs2_txt("file2.txt");
    ofs2_txt << trace.DebugString();
    ofs2_txt.close();

    std::ofstream ofs2_bin("file2.ptrace");
    trace.SerializeToOstream(&ofs2_bin);
    ofs2_bin.close();
#endif

    return 0;
}


static const char *g_program_name;

static void usage(void)
{
    printf("\
ptrace-combine %s\n\
Copyright (c) 2021-2022 Flynn\n\
Usage: %s <file1> <file2> <output> <time-diff>\n\
  file1             the 1st input file\n\
  file2             the 2nd input file\n\
  output            the combined output file\n\
  time-diff         time-diff = t1 - t2, tN means fileN's timestamp,\n\
                    in nanoseconds\n",
        PTRACE_VERSION, g_program_name);
}

int ptrace_combine_main(int argc, char *argv[])
{
    int64_t time_diff;

    g_program_name = argv[0];

    if (argc < 5) {
        usage();
        return -1;
    }

    time_diff = atoll(argv[4]);
    if (!time_diff) {
        fprintf(stderr, "invalid time difference\n");
        usage();
        return -1;
    }

    return combine_file(argv[1], argv[2], argv[3], atoll(argv[4]));
}
