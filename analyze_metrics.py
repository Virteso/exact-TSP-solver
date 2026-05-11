#!/usr/bin/env python3
"""Analyze code complexity metrics using lizard and export to JSON/CSV."""

import subprocess
import json
import csv
import sys
from collections import defaultdict

names = [
    "brute_force",
    "held_karp",
    "branch_and_bound",
    "cplex_tsp_solver",
    "one_tree"
]

all_results = []

for name in names:
    cpp_file = f"cpp/{name}.cpp"
    py_file = f"python/{name}.py"
    
    for file_path, language in [(cpp_file, "C++"), (py_file, "Python")]:
        try:
            result = subprocess.run(['lizard', file_path, '--csv'], 
                                  capture_output=True, text=True, timeout=30)
            if result.returncode == 0:
                # Parse CSV output - aggregate function-level metrics
                # CSV format: NLOC,CCN,Token,Param,Length,FunctionName,FileName,...
                lines = result.stdout.strip().split('\n')
                
                total_nloc = 0
                total_ccn = 0
                total_token = 0
                total_param = 0
                total_length = 0
                func_count = 0
                
                reader = csv.reader(iter(lines))
                for row in reader:
                    if len(row) >= 6:
                        try:
                            nloc = int(row[0]) if row[0] else 0
                            ccn = int(row[1]) if row[1] else 0
                            token = int(row[2]) if row[2] else 0
                            param = int(row[3]) if row[3] else 0
                            length = int(row[4]) if row[4] else 0
                            
                            total_nloc += nloc
                            total_ccn += ccn
                            total_token += token
                            total_param += param
                            total_length += length
                            func_count += 1
                        except (ValueError, IndexError):
                            continue
                
                if func_count > 0:
                    all_results.append({
                        'name': name,
                        'language': language,
                        'file': file_path,
                        'total_nloc': total_nloc,
                        'avg_ccn': total_ccn / func_count,
                        'avg_token': total_token / func_count,
                        'functions': func_count
                    })
        except subprocess.TimeoutExpired:
            print(f"Timeout analyzing {file_path}", file=sys.stderr)
        except Exception as e:
            print(f"Error analyzing {file_path}: {e}", file=sys.stderr)

# Write to JSON
if all_results:
    with open('metrics.json', 'w') as f:
        json.dump(all_results, f, indent=2)
    print(f"✓ Metrics exported to metrics.json")
    
    # Write to CSV
    with open('metrics.csv', 'w', newline='') as f:
        fieldnames = ['name', 'language', 'file', 'total_nloc', 'avg_ccn', 'avg_token', 'functions']
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(all_results)
    print(f"✓ Metrics exported to metrics.csv")
    
    # Print summary
    print("\nMetrics Summary:")
    print(f"{'Name':<20} {'Language':<10} {'NLOC':<8} {'Avg CCN':<10} {'Avg Tokens':<12} {'Functions':<10}")
    print("-" * 72)
    for row in all_results:
        print(f"{row['name']:<20} {row['language']:<10} {row['total_nloc']:<8} {row['avg_ccn']:<10.1f} {row['avg_token']:<12.1f} {row['functions']:<10}")
else:
    print("No metrics data extracted", file=sys.stderr)