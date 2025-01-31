#!/usr/bin/env bash

# Helper script that processes .output files generated by pyco_bench
#
# By default, the output are stored in ./results directory
# This is called by default in `run_pyco.sh`, other usage is strongy disadvised,
# unless you are familiar with outputs of pyco_bench.
#
# The result of the script is in form of `;` delimited .csv file for set of outputs of benchmarks.

# WARNING: The results expects that the header will be the same for all processed benchmarks !!!

usage() { {
        [ $# -gt 0 ] && echo "error: $1"
        echo "usage: ./process_pyco.sh [opt1, ..., optn] [bench1, ..., benchm]"
        echo "options:"
        echo "  -o|--output-file<result.csv>>     target output file[default=result.csv]"
        echo "  -p|--number-of-params<int>        number of params in target"
    } >&2
}

output_file=result.csv
benchmarks=()
basedir=$(realpath $(dirname "$0"))
rootdir=$(realpath "$basedir/..")
number_of_params=1

# Processes the arguments
while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help)
            usage
            exit 0;;
        -o|--output-file)
            output_file=$2
            shift 2;;
        -p|--number-of-params)
            number_of_params=$2
            shift 2;;
        *)
            benchmarks+=( $1 )
            shift 1;;
    esac
done

# $1: filename, $2: extension
# Adds extension $2 to the $filename, unless it already contains it
escape_extension () {
  if [[ "$1" == *.$2 ]];
  then
    echo "$1"
  else
    echo "$1.$2"
  fi
}

# $1: filename, $2: directory
# Adds $2 directory before the filename, unless it already contains it
prepend_directory() {
  if [[ "$1" == $2/* ]];
  then
    echo "$1"
  else
    echo "$2/$1"
  fi
}

processed_header=false


# For each benchmark
for benchmark in ${benchmarks[@]}
do
    # We ensure that the results are in results/data directory
    result_file=$(basename "$benchmark")
    benchmark_file=$benchmark

    # Each partial result is transformed using `pyco_proc` to csv representation
    echo "Processing $benchmark_file"
    $rootdir/pyco_proc --csv --param-no "$number_of_params" > "$result_file.csv" < "$benchmark_file"
    if [ $processed_header = false ];
    then
      # For first result we ensure that the header will be there
      processed_header=true
      cat "$result_file.csv" > "$output_file"
    else
      # We cat rest of the results
      awk 'FNR > 1' "$result_file.csv" >> "$output_file"
    fi
    rm "$result_file.csv"
done

echo "[!] Saved results to $output_file"
