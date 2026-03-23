#!/bin/bash

mkdir -p results

LOGGER_PID=""
WATCHDOG_PID=""

stop_logger() {
  if [ -n "$LOGGER_PID" ] && kill -0 "$LOGGER_PID" 2>/dev/null; then
    kill "$LOGGER_PID" 2>/dev/null
    wait "$LOGGER_PID" 2>/dev/null
  fi

  if [ -n "$WATCHDOG_PID" ] && kill -0 "$WATCHDOG_PID" 2>/dev/null; then
    kill "$WATCHDOG_PID" 2>/dev/null
    wait "$WATCHDOG_PID" 2>/dev/null
  fi
}

cleanup() {
  stop_logger
}

on_signal() {
  echo -e "\n[!] Interrupt received. Stopping background logger..."
  exit 1
}

# Ensure cleanup runs for all exits and signals
trap cleanup EXIT
trap on_signal SIGINT SIGTERM

# System Metadata Logging
LOG_FILE="results/algo_bench_$(date +%Y%m%d_%H%M).log"
{
    echo "--- Environment Metadata ---"
    echo "Date: $(date)"
    echo "CPU: $(lscpu | grep 'Model name' | sed 's/Model name:[[:space:]]*//')"
    echo "Scaling Governor: $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo 'N/A')"
    echo "Max Frequency: $(lscpu | grep 'CPU max MHz' | awk '{print $4}') MHz"
    echo "----------------------------"
} > "$LOG_FILE"

# Wait for Load to Settle
echo "Waiting for background noise to drop..."
# while [ $(awk '{print ($1 > 0.5)}' /proc/loadavg) -eq 1 ]; do sleep 2; done

# Background System Monitor (Overhead)
# Captures: User%, Sys%, Idle%, Context Switches
echo >> "$LOG_FILE"
echo "type,timestamp,cpu_user,cpu_sys,cpu_idle,context_switches" >> "$LOG_FILE"
(
  while true; do
    vmstat 1 2 | tail -1 | awk '{print "SYS_STAT",strftime("%H:%M:%S"),$13,$14,$15,$12}' >> "$LOG_FILE"
  done
) &
LOGGER_PID=$!

# Watchdog: if parent script dies unexpectedly, kill logger.
SCRIPT_PID=$$
(
  while kill -0 "$SCRIPT_PID" 2>/dev/null; do
    sleep 1
  done
  kill "$LOGGER_PID" 2>/dev/null
) &
WATCHDOG_PID=$!

# Execute the Program
echo "Running Benchmark (Warm-up + Execution)..."
taskset -c 1 pyvenv/bin/python python/test.py tsplib/gr17.tsp 0 | tee -a "$LOG_FILE"
# Test on another core
taskset -c 2 pyvenv/bin/python python/test.py tsplib/gr17.tsp 0 | tee -a "$LOG_FILE"

# Cleanup
echo "Results and system logs saved to $LOG_FILE"