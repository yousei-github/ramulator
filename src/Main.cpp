#include "Processor.h"
#include "Config.h"
#include "Controller.h"
#include "SpeedyController.h"
#include "Memory.h"
#include "DRAM.h"
#include "Statistics.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>
#include <functional>
#include <map>
#include <ProjectConfiguration.h>  // User file

/* Standards */
#include "Gem5Wrapper.h"
#include "DDR3.h"
#include "DDR4.h"
#include "DSARP.h"
#include "GDDR5.h"
#include "LPDDR3.h"
#include "LPDDR4.h"
#include "WideIO.h"
#include "WideIO2.h"
#include "HBM.h"
#include "SALP.h"
#include "ALDRAM.h"
#include "TLDRAM.h"
#include "STTMRAM.h"
#include "PCM.h"

using namespace std;
using namespace ramulator;

bool ramulator::warmup_complete = false;

#if USER_CODES == ENABLE
/* Define */

/* Declaration */
#if MEMORY_USE_HYBRID == ENABLE
void configure_fast_memory_to_run_simulation(const std::string& standard, Config& configs,
                                             const std::string& standard2, Config& configs2,
                                             const vector<const char*>& files);
template <typename T>
void next_configure_slow_memory_to_run_simulation(const std::string& standard2, Config& configs2,
                                                  Config& configs, T* spec,
                                                  const vector<const char*>& files);

template <typename T, typename T2>
void start_run_simulation(const Config& configs, T* spec,
                          const Config& configs2, T2* spec2,
                          const vector<const char*>& files);
template <typename T, typename T2>
void simulation_run_dramtrace(const Config& configs, Memory<T, Controller>& memory,
                              const Config& configs2, Memory<T2, Controller>& memory2,
                              const char* tracename);
#endif

template <typename T>
void run_dramtrace(const Config& configs, Memory<T, Controller>& memory, const char* tracename);

template <typename T>
void run_cputrace(const Config& configs, Memory<T, Controller>& memory, const std::vector<const char*>& files);

template <typename T>
void start_run(const Config& configs, T* spec, const vector<const char*>& files);

/* Definition */
int main(int argc, const char* argv[])
{
  if (argc < 2)
  {
    printf("Usage: %s <configs-file> --mode=cpu,dram [--stats <filename>] <trace-filename1> <trace-filename2>\n"
           "Example: %s ramulator-configs.cfg --mode=cpu cpu.trace cpu.trace\n",
           argv[0], argv[0]);
    return 0;
  }

#if MEMORY_USE_HYBRID == ENABLE
  // Read memory configuration file.
  Config configs(argv[1]);  // fast memory
  Config configs2(argv[2]); // slow memory

  const std::string& standard = configs["standard"];
  assert(standard != "" || "DRAM standard should be specified.");
  const std::string& standard2 = configs2["standard"];
  assert(standard2 != "" || "DRAM standard should be specified.");

  // Read trace type.
  const char* trace_type = strstr(argv[3], "=");
  trace_type++;
  if (strcmp(trace_type, "cpu") == 0)
  {
    configs.add("trace_type", "CPU");
    configs2.add("trace_type", "CPU");
  }
  else if (strcmp(trace_type, "dram") == 0)
  {
    configs.add("trace_type", "DRAM");
    configs2.add("trace_type", "DRAM");
  }
  else
  {
    printf("invalid trace type: %s\n", trace_type);
    assert(false);
  }

  int trace_start = 4;
  string stats_out;
  if (strcmp(argv[trace_start], "--stats") == 0)
  {
    Stats::statlist.output(argv[trace_start + 1]);
    stats_out = argv[trace_start + 1];
    trace_start += 2;
  }
  else
  {
    // Open .stats file.
    string stats_file_name = standard + "_" + standard2;
    Stats::statlist.output(stats_file_name + ".stats");
    stats_out = stats_file_name + string(".stats");
  }

  // A separate file defines mapping for easy config.
  if (strcmp(argv[trace_start], "--mapping") == 0)
  {
    configs.add("mapping", argv[trace_start + 1]);
    configs2.add("mapping", argv[trace_start + 2]);
    trace_start += 2 + 1;
  }
  else
  {
    configs.add("mapping", "defaultmapping");
    configs2.add("mapping", "defaultmapping");
  }
#else
  // Read memory configuration file.
  Config configs(argv[1]);

  const std::string& standard = configs["standard"];
  assert(standard != "" || "DRAM standard should be specified.");

  // Read trace type.
  const char* trace_type = strstr(argv[2], "=");
  trace_type++;
  if (strcmp(trace_type, "cpu") == 0)
  {
    configs.add("trace_type", "CPU");
  }
  else if (strcmp(trace_type, "dram") == 0)
  {
    configs.add("trace_type", "DRAM");
  }
  else
  {
    printf("invalid trace type: %s\n", trace_type);
    assert(false);
  }

  int trace_start = 3;
  string stats_out;
  if (strcmp(argv[trace_start], "--stats") == 0)
  {
    Stats::statlist.output(argv[trace_start + 1]);
    stats_out = argv[trace_start + 1];
    trace_start += 2;
  }
  else
  {
    // Open .stats file.
    Stats::statlist.output(standard + ".stats");
    stats_out = standard + string(".stats");
  }

  // A separate file defines mapping for easy config.
  if (strcmp(argv[trace_start], "--mapping") == 0)
  {
    configs.add("mapping", argv[trace_start + 1]);
    trace_start += 2;
  }
  else
  {
    configs.add("mapping", "defaultmapping");
  }
#endif


  std::vector<const char*> files(&argv[trace_start], &argv[argc]);
#if MEMORY_USE_HYBRID == ENABLE
  configs.set_core_num(argc - trace_start);
  configs2.set_core_num(argc - trace_start);

  configure_fast_memory_to_run_simulation(standard, configs, standard2, configs2, files);
#else
  configs.set_core_num(argc - trace_start);

  if (standard == "DDR3")
  {
    DDR3* ddr3 = new DDR3(configs["org"], configs["speed"]);
    start_run(configs, ddr3, files);
  }
  else if (standard == "DDR4")
  {
    DDR4* ddr4 = new DDR4(configs["org"], configs["speed"]);
    start_run(configs, ddr4, files);
  }
  else if (standard == "SALP-MASA")
  {
    SALP* salp8 = new SALP(configs["org"], configs["speed"], "SALP-MASA", configs.get_subarrays());
    start_run(configs, salp8, files);
  }
  else if (standard == "LPDDR3")
  {
    LPDDR3* lpddr3 = new LPDDR3(configs["org"], configs["speed"]);
    start_run(configs, lpddr3, files);
  }
  else if (standard == "LPDDR4")
  {
    // total cap: 2GB, 1/2 of others
    LPDDR4* lpddr4 = new LPDDR4(configs["org"], configs["speed"]);
    start_run(configs, lpddr4, files);
  }
  else if (standard == "GDDR5")
  {
    GDDR5* gddr5 = new GDDR5(configs["org"], configs["speed"]);
    start_run(configs, gddr5, files);
  }
  else if (standard == "HBM")
  {
    HBM* hbm = new HBM(configs["org"], configs["speed"]);
    start_run(configs, hbm, files);
  }
  else if (standard == "WideIO")
  {
    // total cap: 1GB, 1/4 of others
    WideIO* wio = new WideIO(configs["org"], configs["speed"]);
    start_run(configs, wio, files);
  }
  else if (standard == "WideIO2")
  {
    // total cap: 2GB, 1/2 of others
    WideIO2* wio2 = new WideIO2(configs["org"], configs["speed"], configs.get_channels());
    wio2->channel_width *= 2;
    start_run(configs, wio2, files);
  }
  else if (standard == "STTMRAM")
  {
    STTMRAM* sttmram = new STTMRAM(configs["org"], configs["speed"]);
    start_run(configs, sttmram, files);
  }
  else if (standard == "PCM")
  {
    PCM* pcm = new PCM(configs["org"], configs["speed"]);
    start_run(configs, pcm, files);
  }
  // Various refresh mechanisms
  else if (standard == "DSARP")
  {
    DSARP* dsddr3_dsarp = new DSARP(configs["org"], configs["speed"], DSARP::Type::DSARP, configs.get_subarrays());
    start_run(configs, dsddr3_dsarp, files);
  }
  else if (standard == "ALDRAM")
  {
    ALDRAM* aldram = new ALDRAM(configs["org"], configs["speed"]);
    start_run(configs, aldram, files);
  }
  else if (standard == "TLDRAM")
  {
    TLDRAM* tldram = new TLDRAM(configs["org"], configs["speed"], configs.get_subarrays());
    start_run(configs, tldram, files);
  }
#endif

  printf("Simulation done. Statistics written to %s\n", stats_out.c_str());

  return 0;
}

#if MEMORY_USE_HYBRID == ENABLE
void configure_fast_memory_to_run_simulation
(const std::string& standard, Config& configs,
 const std::string& standard2, Config& configs2,
 const vector<const char*>& files)
{
  if (standard == "DDR3")
  {
    DDR3* ddr3 = new DDR3(configs["org"], configs["speed"]);
    next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, ddr3, files);
  }
  else if (standard == "DDR4")
  {
    DDR4* ddr4 = new DDR4(configs["org"], configs["speed"]);
    next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, ddr4, files);
  }
  else if (standard == "SALP-MASA")
  {
    SALP* salp8 = new SALP(configs["org"], configs["speed"], "SALP-MASA", configs.get_subarrays());
    next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, salp8, files);
  }
  else if (standard == "LPDDR3")
  {
    LPDDR3* lpddr3 = new LPDDR3(configs["org"], configs["speed"]);
    next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, lpddr3, files);
  }
  else if (standard == "LPDDR4")
  {
    // total cap: 2GB, 1/2 of others
    LPDDR4* lpddr4 = new LPDDR4(configs["org"], configs["speed"]);
    next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, lpddr4, files);
  }
  else if (standard == "GDDR5")
  {
    GDDR5* gddr5 = new GDDR5(configs["org"], configs["speed"]);
    next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, gddr5, files);
  }
  else if (standard == "HBM")
  {
    HBM* hbm = new HBM(configs["org"], configs["speed"]);
    next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, hbm, files);
  }
  else if (standard == "WideIO")
  {
    // total cap: 1GB, 1/4 of others
    WideIO* wio = new WideIO(configs["org"], configs["speed"]);
    next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, wio, files);
  }
  else if (standard == "WideIO2")
  {
    // total cap: 2GB, 1/2 of others
    WideIO2* wio2 = new WideIO2(configs["org"], configs["speed"], configs.get_channels());
    wio2->channel_width *= 2;
    next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, wio2, files);
  }
  else if (standard == "STTMRAM")
  {
    STTMRAM* sttmram = new STTMRAM(configs["org"], configs["speed"]);
    next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, sttmram, files);
  }
  else if (standard == "PCM")
  {
    PCM* pcm = new PCM(configs["org"], configs["speed"]);
    next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, pcm, files);
  }
  // Various refresh mechanisms
  else if (standard == "DSARP")
  {
    DSARP* dsddr3_dsarp = new DSARP(configs["org"], configs["speed"], DSARP::Type::DSARP, configs.get_subarrays());
    next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, dsddr3_dsarp, files);
  }
  else if (standard == "ALDRAM")
  {
    ALDRAM* aldram = new ALDRAM(configs["org"], configs["speed"]);
    next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, aldram, files);
  }
  else if (standard == "TLDRAM")
  {
    TLDRAM* tldram = new TLDRAM(configs["org"], configs["speed"], configs.get_subarrays());
    next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, tldram, files);
  }
}

template <typename T>
void next_configure_slow_memory_to_run_simulation
(const std::string& standard2, Config& configs2,
 Config& configs, T* spec,
 const vector<const char*>& files)
{
  if (standard2 == "DDR3")
  {
    DDR3* ddr3 = new DDR3(configs2["org"], configs2["speed"]);
    start_run_simulation(configs, spec, configs2, ddr3, files);
  }
  else if (standard2 == "DDR4")
  {
    DDR4* ddr4 = new DDR4(configs2["org"], configs2["speed"]);
    start_run_simulation(configs, spec, configs2, ddr4, files);
  }
  else if (standard2 == "SALP-MASA")
  {
    SALP* salp8 = new SALP(configs2["org"], configs2["speed"], "SALP-MASA", configs2.get_subarrays());
    start_run_simulation(configs, spec, configs2, salp8, files);
  }
  else if (standard2 == "LPDDR3")
  {
    LPDDR3* lpddr3 = new LPDDR3(configs2["org"], configs2["speed"]);
    start_run_simulation(configs, spec, configs2, lpddr3, files);
  }
  else if (standard2 == "LPDDR4")
  {
    // total cap: 2GB, 1/2 of others
    LPDDR4* lpddr4 = new LPDDR4(configs2["org"], configs2["speed"]);
    start_run_simulation(configs, spec, configs2, lpddr4, files);
  }
  else if (standard2 == "GDDR5")
  {
    GDDR5* gddr5 = new GDDR5(configs2["org"], configs2["speed"]);
    start_run_simulation(configs, spec, configs2, gddr5, files);
  }
  else if (standard2 == "HBM")
  {
    HBM* hbm = new HBM(configs2["org"], configs2["speed"]);
    start_run_simulation(configs, spec, configs2, hbm, files);
  }
  else if (standard2 == "WideIO")
  {
    // total cap: 1GB, 1/4 of others
    WideIO* wio = new WideIO(configs2["org"], configs2["speed"]);
    start_run_simulation(configs, spec, configs2, wio, files);
  }
  else if (standard2 == "WideIO2")
  {
    // total cap: 2GB, 1/2 of others
    WideIO2* wio2 = new WideIO2(configs2["org"], configs2["speed"], configs2.get_channels());
    wio2->channel_width *= 2;
    start_run_simulation(configs, spec, configs2, wio2, files);
  }
  else if (standard2 == "STTMRAM")
  {
    STTMRAM* sttmram = new STTMRAM(configs2["org"], configs2["speed"]);
    start_run_simulation(configs, spec, configs2, sttmram, files);
  }
  else if (standard2 == "PCM")
  {
    PCM* pcm = new PCM(configs2["org"], configs2["speed"]);
    start_run_simulation(configs, spec, configs2, pcm, files);
  }
  // Various refresh mechanisms
  else if (standard2 == "DSARP")
  {
    DSARP* dsddr3_dsarp = new DSARP(configs2["org"], configs2["speed"], DSARP::Type::DSARP, configs2.get_subarrays());
    start_run_simulation(configs, spec, configs2, dsddr3_dsarp, files);
  }
  else if (standard2 == "ALDRAM")
  {
    ALDRAM* aldram = new ALDRAM(configs2["org"], configs2["speed"]);
    start_run_simulation(configs, spec, configs2, aldram, files);
  }
  else if (standard2 == "TLDRAM")
  {
    TLDRAM* tldram = new TLDRAM(configs2["org"], configs2["speed"], configs2.get_subarrays());
    start_run_simulation(configs, spec, configs2, tldram, files);
  }
}

template <typename T, typename T2>
void start_run_simulation(const Config& configs, T* spec, const Config& configs2, T2* spec2, const vector<const char*>& files)
{
  // initiate controller and memory for fast memory
  int C = configs.get_channels(), R = configs.get_ranks();
  // Check and Set channel, rank number for fast memory
  spec->set_channel_number(C);
  spec->set_rank_number(R);
  std::vector<Controller<T>*> ctrls;
  for (int c = 0; c < C; c++)
  {
    DRAM<T>* channel = new DRAM<T>(spec, T::Level::Channel);
    channel->id = c;
    channel->regStats("");
    Controller<T>* ctrl = new Controller<T>(configs, channel);
    ctrls.push_back(ctrl);
  }
  Memory<T, Controller> memory(configs, ctrls);

  // initiate controller and memory for slow memory
  C = configs2.get_channels(), R = configs2.get_ranks();
  // Check and Set channel, rank number for slow memory
  spec2->set_channel_number(C);
  spec2->set_rank_number(R);
  std::vector<Controller<T2>*> ctrls2;
  for (int c = 0; c < C; c++)
  {
    DRAM<T2>* channel = new DRAM<T2>(spec2, T2::Level::Channel);
    channel->id = c;
    channel->regStats("");
    Controller<T2>* ctrl = new Controller<T2>(configs2, channel);
    ctrls2.push_back(ctrl);
  }
  Memory<T2, Controller> memory2(configs2, ctrls2);

  assert(files.size() != 0);
  if ((configs["trace_type"] == "CPU") && (configs2["trace_type"] == "CPU"))
  {
    run_cputrace(configs, memory, files);
  }
  else if ((configs["trace_type"] == "DRAM") && (configs2["trace_type"] == "DRAM"))
  {
    simulation_run_dramtrace(configs, memory, configs2, memory2, files[0]);
  }
  else
  {
    printf("%s: Error!\n", __FUNCTION__);
  }
}

template <typename T, typename T2>
void simulation_run_dramtrace(const Config& configs, Memory<T, Controller>& memory, const Config& configs2, Memory<T2, Controller>& memory2, const char* tracename)
{

  /* initialize DRAM trace */
  Trace trace(tracename);

  /* run simulation */
  bool stall = false, end = false;
  int reads = 0, writes = 0, clks = 0;
  long addr = 0;
  Request::Type type = Request::Type::READ;
  map<int, int> latencies;
  // below is a function.
  auto read_complete = [&latencies](Request& r)
  { latencies[r.depart - r.arrive]++; };

  Request req(addr, type, read_complete);

  while (!end || memory.pending_requests() || memory2.pending_requests())
  {
    if (!end && !stall)
    {
      end = !trace.get_dramtrace_request(addr, type);
    }

    if (!end)
    {
      req.addr = addr;
      req.type = type;

      // assign the request to the right memory.
      if (req.addr < memory.max_address)
      {
        stall = !memory.send(req);
      }
      else if (req.addr < memory.max_address + memory2.max_address)
      {
        stall = !memory2.send(req);
      }
      else
      {
        printf("%s: Error!\n", __FUNCTION__);
      }

      if (!stall)
      {
        if (type == Request::Type::READ)
          reads++;
        else if (type == Request::Type::WRITE)
          writes++;
      }
    }
    else
    {
      // make sure that all write requests in the write queue are drained
      memory.set_high_writeq_watermark(0.0f);
      memory2.set_high_writeq_watermark(0.0f);
    }

    memory.tick();
    memory2.tick();
    clks++;
    Stats::curTick++; // memory clock, global, for Statistics
  }
  // This a workaround for statistics set only initially lost in the end
  memory.finish();
  memory2.finish();
  Stats::statlist.printall();
}
#endif

template <typename T>
void run_dramtrace(const Config& configs, Memory<T, Controller>& memory, const char* tracename)
{

  /* initialize DRAM trace */
  Trace trace(tracename);

  /* run simulation */
  bool stall = false, end = false;
  int reads = 0, writes = 0, clks = 0;
  long addr = 0;
  Request::Type type = Request::Type::READ;
  map<int, int> latencies;
  auto read_complete = [&latencies](Request& r)
  { latencies[r.depart - r.arrive]++; };

  Request req(addr, type, read_complete);

  while (!end || memory.pending_requests())
  {
    if (!end && !stall)
    {
      end = !trace.get_dramtrace_request(addr, type);
    }

    if (!end)
    {
      req.addr = addr;
      req.type = type;
      stall = !memory.send(req);
      if (!stall)
      {
        if (type == Request::Type::READ)
          reads++;
        else if (type == Request::Type::WRITE)
          writes++;
      }
    }
    else
    {
      memory.set_high_writeq_watermark(0.0f); // make sure that all write requests in the
      // write queue are drained
    }

    memory.tick();
    clks++;
    Stats::curTick++; // memory clock, global, for Statistics
  }
  // This a workaround for statistics set only initially lost in the end
  memory.finish();
  Stats::statlist.printall();
}

template <typename T>
void run_cputrace(const Config& configs, Memory<T, Controller>& memory, const std::vector<const char*>& files)
{
  int cpu_tick = configs.get_cpu_tick();
  int mem_tick = configs.get_mem_tick();
  auto send = bind(&Memory<T, Controller>::send, &memory, placeholders::_1);
  Processor proc(configs, files, send, memory);

  long warmup_insts = configs.get_warmup_insts();
  bool is_warming_up = (warmup_insts != 0);

  for (long i = 0; is_warming_up; i++)
  {
    proc.tick();
    Stats::curTick++;
    if (i % cpu_tick == (cpu_tick - 1))
      for (int j = 0; j < mem_tick; j++)
        memory.tick();

    is_warming_up = false;
    for (unsigned int c = 0; c < proc.cores.size(); c++)
    {
      if (proc.cores[c]->get_insts() < warmup_insts)
        is_warming_up = true;
    }

    if (is_warming_up && proc.has_reached_limit())
    {
      printf("WARNING: The end of the input trace file was reached during warmup. "
             "Consider changing warmup_insts in the config file. \n");
      break;
    }
  }

  warmup_complete = true;
  printf("Warmup complete! Resetting stats...\n");
  Stats::reset_stats();
  proc.reset_stats();
  assert(proc.get_insts() == 0);

  printf("Starting the simulation...\n");

  int tick_mult = cpu_tick * mem_tick;
  for (long i = 0;; i++)
  {
    if (((i % tick_mult) % mem_tick) == 0)
    { // When the CPU is ticked cpu_tick times,
      // the memory controller should be ticked mem_tick times
      proc.tick();
      Stats::curTick++; // processor clock, global, for Statistics

      if (configs.calc_weighted_speedup())
      {
        if (proc.has_reached_limit())
        {
          break;
        }
      }
      else
      {
        if (configs.is_early_exit())
        {
          if (proc.finished())
            break;
        }
        else
        {
          if (proc.finished() && (memory.pending_requests() == 0))
            break;
        }
      }
    }

    if (((i % tick_mult) % cpu_tick) == 0) // TODO_hasan: Better if the processor ticks the memory controller
      memory.tick();
  }
  // This a workaround for statistics set only initially lost in the end
  memory.finish();
  Stats::statlist.printall();
}

template <typename T>
void start_run(const Config& configs, T* spec, const vector<const char*>& files)
{
  // initiate controller and memory
  int C = configs.get_channels(), R = configs.get_ranks();
  // Check and Set channel, rank number
  spec->set_channel_number(C);
  spec->set_rank_number(R);
  std::vector<Controller<T>*> ctrls;
  for (int c = 0; c < C; c++)
  {
    DRAM<T>* channel = new DRAM<T>(spec, T::Level::Channel);
    channel->id = c;
    channel->regStats("");
    Controller<T>* ctrl = new Controller<T>(configs, channel);
    ctrls.push_back(ctrl);
  }
  Memory<T, Controller> memory(configs, ctrls);

  assert(files.size() != 0);
  if (configs["trace_type"] == "CPU")
  {
    run_cputrace(configs, memory, files);
  }
  else if (configs["trace_type"] == "DRAM")
  {
    run_dramtrace(configs, memory, files[0]);
  }
}

#else

template <typename T>
void run_dramtrace(const Config& configs, Memory<T, Controller>& memory, const char* tracename)
{

  /* initialize DRAM trace */
  Trace trace(tracename);

  /* run simulation */
  bool stall = false, end = false;
  int reads = 0, writes = 0, clks = 0;
  long addr = 0;
  Request::Type type = Request::Type::READ;
  map<int, int> latencies;
  auto read_complete = [&latencies](Request& r)
  { latencies[r.depart - r.arrive]++; };

  Request req(addr, type, read_complete);

  while (!end || memory.pending_requests())
  {
    if (!end && !stall)
    {
      end = !trace.get_dramtrace_request(addr, type);
    }

    if (!end)
    {
      req.addr = addr;
      req.type = type;
      stall = !memory.send(req);
      if (!stall)
      {
        if (type == Request::Type::READ)
          reads++;
        else if (type == Request::Type::WRITE)
          writes++;
      }
    }
    else
    {
      memory.set_high_writeq_watermark(0.0f); // make sure that all write requests in the
      // write queue are drained
    }

    memory.tick();
    clks++;
    Stats::curTick++; // memory clock, global, for Statistics
  }
  // This a workaround for statistics set only initially lost in the end
  memory.finish();
  Stats::statlist.printall();
}

template <typename T>
void run_cputrace(const Config& configs, Memory<T, Controller>& memory, const std::vector<const char*>& files)
{
  int cpu_tick = configs.get_cpu_tick();
  int mem_tick = configs.get_mem_tick();
  auto send = bind(&Memory<T, Controller>::send, &memory, placeholders::_1);
  Processor proc(configs, files, send, memory);

  long warmup_insts = configs.get_warmup_insts();
  bool is_warming_up = (warmup_insts != 0);

  for (long i = 0; is_warming_up; i++)
  {
    proc.tick();
    Stats::curTick++;
    if (i % cpu_tick == (cpu_tick - 1))
      for (int j = 0; j < mem_tick; j++)
        memory.tick();

    is_warming_up = false;
    for (int c = 0; c < proc.cores.size(); c++)
    {
      if (proc.cores[c]->get_insts() < warmup_insts)
        is_warming_up = true;
    }

    if (is_warming_up && proc.has_reached_limit())
    {
      printf("WARNING: The end of the input trace file was reached during warmup. "
             "Consider changing warmup_insts in the config file. \n");
      break;
    }
  }

  warmup_complete = true;
  printf("Warmup complete! Resetting stats...\n");
  Stats::reset_stats();
  proc.reset_stats();
  assert(proc.get_insts() == 0);

  printf("Starting the simulation...\n");

  int tick_mult = cpu_tick * mem_tick;
  for (long i = 0;; i++)
  {
    if (((i % tick_mult) % mem_tick) == 0)
    { // When the CPU is ticked cpu_tick times,
      // the memory controller should be ticked mem_tick times
      proc.tick();
      Stats::curTick++; // processor clock, global, for Statistics

      if (configs.calc_weighted_speedup())
      {
        if (proc.has_reached_limit())
        {
          break;
        }
      }
      else
      {
        if (configs.is_early_exit())
        {
          if (proc.finished())
            break;
        }
        else
        {
          if (proc.finished() && (memory.pending_requests() == 0))
            break;
        }
      }
    }

    if (((i % tick_mult) % cpu_tick) == 0) // TODO_hasan: Better if the processor ticks the memory controller
      memory.tick();
  }
  // This a workaround for statistics set only initially lost in the end
  memory.finish();
  Stats::statlist.printall();
}

template <typename T>
void start_run(const Config& configs, T* spec, const vector<const char*>& files)
{
  // initiate controller and memory
  int C = configs.get_channels(), R = configs.get_ranks();
  // Check and Set channel, rank number
  spec->set_channel_number(C);
  spec->set_rank_number(R);
  std::vector<Controller<T>*> ctrls;
  for (int c = 0; c < C; c++)
  {
    DRAM<T>* channel = new DRAM<T>(spec, T::Level::Channel);
    channel->id = c;
    channel->regStats("");
    Controller<T>* ctrl = new Controller<T>(configs, channel);
    ctrls.push_back(ctrl);
  }
  Memory<T, Controller> memory(configs, ctrls);

  assert(files.size() != 0);
  if (configs["trace_type"] == "CPU")
  {
    run_cputrace(configs, memory, files);
  }
  else if (configs["trace_type"] == "DRAM")
  {
    run_dramtrace(configs, memory, files[0]);
  }
}

int main(int argc, const char* argv[])
{
  if (argc < 2)
  {
    printf("Usage: %s <configs-file> --mode=cpu,dram [--stats <filename>] <trace-filename1> <trace-filename2>\n"
           "Example: %s ramulator-configs.cfg --mode=cpu cpu.trace cpu.trace\n",
           argv[0], argv[0]);
    return 0;
  }

  Config configs(argv[1]);

  const std::string& standard = configs["standard"];
  assert(standard != "" || "DRAM standard should be specified.");

  const char* trace_type = strstr(argv[2], "=");
  trace_type++;
  if (strcmp(trace_type, "cpu") == 0)
  {
    configs.add("trace_type", "CPU");
  }
  else if (strcmp(trace_type, "dram") == 0)
  {
    configs.add("trace_type", "DRAM");
  }
  else
  {
    printf("invalid trace type: %s\n", trace_type);
    assert(false);
  }

  int trace_start = 3;
  string stats_out;
  if (strcmp(argv[trace_start], "--stats") == 0)
  {
    Stats::statlist.output(argv[trace_start + 1]);
    stats_out = argv[trace_start + 1];
    trace_start += 2;
  }
  else
  {
    Stats::statlist.output(standard + ".stats");
    stats_out = standard + string(".stats");
  }

  // A separate file defines mapping for easy config.
  if (strcmp(argv[trace_start], "--mapping") == 0)
  {
    configs.add("mapping", argv[trace_start + 1]);
    trace_start += 2;
  }
  else
  {
    configs.add("mapping", "defaultmapping");
  }

  std::vector<const char*> files(&argv[trace_start], &argv[argc]);
  configs.set_core_num(argc - trace_start);

  if (standard == "DDR3")
  {
    DDR3* ddr3 = new DDR3(configs["org"], configs["speed"]);
    start_run(configs, ddr3, files);
  }
  else if (standard == "DDR4")
  {
    DDR4* ddr4 = new DDR4(configs["org"], configs["speed"]);
    start_run(configs, ddr4, files);
  }
  else if (standard == "SALP-MASA")
  {
    SALP* salp8 = new SALP(configs["org"], configs["speed"], "SALP-MASA", configs.get_subarrays());
    start_run(configs, salp8, files);
  }
  else if (standard == "LPDDR3")
  {
    LPDDR3* lpddr3 = new LPDDR3(configs["org"], configs["speed"]);
    start_run(configs, lpddr3, files);
  }
  else if (standard == "LPDDR4")
  {
    // total cap: 2GB, 1/2 of others
    LPDDR4* lpddr4 = new LPDDR4(configs["org"], configs["speed"]);
    start_run(configs, lpddr4, files);
  }
  else if (standard == "GDDR5")
  {
    GDDR5* gddr5 = new GDDR5(configs["org"], configs["speed"]);
    start_run(configs, gddr5, files);
  }
  else if (standard == "HBM")
  {
    HBM* hbm = new HBM(configs["org"], configs["speed"]);
    start_run(configs, hbm, files);
  }
  else if (standard == "WideIO")
  {
    // total cap: 1GB, 1/4 of others
    WideIO* wio = new WideIO(configs["org"], configs["speed"]);
    start_run(configs, wio, files);
  }
  else if (standard == "WideIO2")
  {
    // total cap: 2GB, 1/2 of others
    WideIO2* wio2 = new WideIO2(configs["org"], configs["speed"], configs.get_channels());
    wio2->channel_width *= 2;
    start_run(configs, wio2, files);
  }
  else if (standard == "STTMRAM")
  {
    STTMRAM* sttmram = new STTMRAM(configs["org"], configs["speed"]);
    start_run(configs, sttmram, files);
  }
  else if (standard == "PCM")
  {
    PCM* pcm = new PCM(configs["org"], configs["speed"]);
    start_run(configs, pcm, files);
  }
  // Various refresh mechanisms
  else if (standard == "DSARP")
  {
    DSARP* dsddr3_dsarp = new DSARP(configs["org"], configs["speed"], DSARP::Type::DSARP, configs.get_subarrays());
    start_run(configs, dsddr3_dsarp, files);
  }
  else if (standard == "ALDRAM")
  {
    ALDRAM* aldram = new ALDRAM(configs["org"], configs["speed"]);
    start_run(configs, aldram, files);
  }
  else if (standard == "TLDRAM")
  {
    TLDRAM* tldram = new TLDRAM(configs["org"], configs["speed"], configs.get_subarrays());
    start_run(configs, tldram, files);
  }

  printf("Simulation done. Statistics written to %s\n", stats_out.c_str());

  return 0;
}
#endif
