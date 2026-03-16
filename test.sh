#!/bin/bash

# System Metadata Logging
LOG_FILE="algo_bench_$(date +%Y%m%d_%H%M).log"
{
    echo "--- Environment Metadata ---"
    echo "Date: $(date)"
    echo "CPU: $(lscpu | grep 'Model name' | sed 's/Model name:[[:space:]]*//')"
    echo "Scaling Governor: $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo 'N/A')"
    echo "Max Frequency: $(lscpu | grep 'CPU max MHz' | awk '{print $4}') MHz"
    echo "----------------------------"
} > "$LOG_FILE"

# Wait for Load to Settle (Standard for repeatability)
echo "Waiting for background noise to drop..."
while [ $(awk '{print ($1 > 0.5)}' /proc/loadavg) -eq 1 ]; do sleep 2; done

# Background System Monitor (Low Overhead)
# Captures: User%, Sys%, Idle%, Context Switches
echo "type,timestamp,cpu_user,cpu_sys,cpu_idle,context_switches" > "$LOG_FILE"
(
  while true; do
    vmstat 1 2 | tail -1 | awk '{print "SYS_STAT",strftime("%H:%M:%S"),$13,$14,$15,$12}' >> "$LOG_FILE"
  done
) &
LOGGER_PID=$!

# Execute the Program
echo "Running Benchmark (Warm-up + Execution)..."
taskset -c 1 pyvenv/bin/python python/test.py tsplib/gr17.tsp 0 | tee -a "$LOG_FILE"
# Test on another core
taskset -c 2 pyvenv/bin/python python/test.py tsplib/gr17.tsp 0 | tee -a "$LOG_FILE"

# Cleanup
kill $LOGGER_PID
echo "Results and system logs saved to $LOG_FILE"