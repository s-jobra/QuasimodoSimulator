#!/bin/bash
export LC_ALL=C.UTF-8

# It is assumed that the script is run from the repository's home folder.

#####################################################################################
# Constants:

# Output files
FILE_OUT_MEASURE="measure-Q.csv"
FILE_OUT="no-measure-Q.csv"

# Exec settings
EXEC="./QuasimodoSim"

BASE_OPT="-i"
MEASURE_OPT="-m"
TYPE_OPT="-t"

# Benchmark directories
BENCH_NO_MEASURE="../MEDUSA/benchmarks/no-measure/"
BENCH_MEASURE="../MEDUSA/benchmarks/measure/"

# Measurement settings
REPS=1
TIMEOUT="timeout 1h"   # for no timeout: ""

# Output settings
SEP=","
NO_RUN="---"
ERROR="Error"
TO="TO"

#####################################################################################
# Functions:

# Runs a single circuit with the QuasimodoSimulator with the specified backend type. Saves the results in the given variables.
sim_file() {
    local type="$1"
    local var_time_name="$2"
    local var_mem_name="$3"
    local var_fail_name="$4"
    
    local fail="${!var_fail_name}"
    local time_sum=0.0
    local mem_sum=0.0
    local output
    local time_cur
    local mem_cur

    if [[ $fail = false ]]; then
        for i in $(eval echo "{1..$REPS}"); do
            output=$($TIMEOUT $EXEC $run_opt $TYPE_OPT $type <$file 2>&1 | grep -vE "^ *'")
            
            if [[ -z $output ]]; then
                time_avg=$TO
                mem_avg=$TO
                fail=true
                break;
            elif grep -q "Time" <<< "$output"; then
                time_cur=$(echo "$output" | grep -oP '(?<=Time=)[0-9.eE+-]+' | awk '{printf "%.4f", $1}')
                mem_cur=$(echo "$output" | grep -oP '(?<=Peak Memory Usage=)[0-9]+' | awk '{printf "%.2f", $1 / 1024}')

                time_sum=$(echo "scale=4; $time_sum + $time_cur" | bc)
                mem_sum=$(echo "scale=2; $mem_sum + $mem_cur" | bc)
            else
                time_avg=$ERROR
                mem_avg=$ERROR
                fail=true
                break;
            fi
        done

        if [[ $fail = false ]]; then
            time_avg=$(echo "scale=4; $time_sum / $REPS.0" | bc)
            mem_avg=$(echo "scale=2; $mem_sum / $REPS.0" | bc)
        fi
    else
        time_avg=$NO_RUN
        mem_avg=$NO_RUN
    fi

    eval "$var_time_name"=$time_avg
    eval "$var_mem_name"=$mem_avg
    eval "$var_fail_name"=$fail
}

# Expects the file path to the test as the first argument and if it is symbolic circuit as the second argument
run_benchmark_file() {
    local file=$1

    file_name=$(basename "$file")
    dir_name=$(dirname "$file")
    benchmark_name="${dir_name##*/}/$file_name"

    sim_file "CFLOBDD" "cflobdd_time_avg" "cflobdd_mem_avg" "cflobdd_fail"
    sim_file "WCFLOBDD" "wcflobdd_time_avg" "wcflobdd_mem_avg" "wcflobdd_fail"
    sim_file "BDD" "bdd_time_avg" "bdd_mem_avg" "bdd_fail"
    sim_file "WBDD" "wbdd_time_avg" "wbdd_mem_avg" "wbdd_fail"
    printf "%s$SEP%s$SEP%s$SEP%s$SEP%s$SEP%s$SEP%s$SEP%s$SEP%s\n" "${benchmark_name%.qasm}" "$cflobdd_time_avg" "$cflobdd_mem_avg" \
           "$wcflobdd_time_avg" "$wcflobdd_mem_avg" "$bdd_time_avg" "$bdd_mem_avg" "$wbdd_time_avg" "$wbdd_mem_avg">> "$file_out"
}

run_benchmarks() {
    # Init
    if [[ $is_measure = true ]]; then
        benchmarks_fd=$BENCH_MEASURE
        run_opt="$BASE_OPT $MEASURE_OPT"
        file_out=$FILE_OUT_MEASURE
    else
        benchmarks_fd=$BENCH_NO_MEASURE
        run_opt=$BASE_OPT
        file_out=$FILE_OUT
    fi

    printf "%s$SEP%s$SEP%s$SEP%s$SEP%s$SEP%s$SEP%s$SEP%s$SEP%s\n" "Circuit" "Quasimodo CFLOBDD t" "Quasimodo CFLOBDD mem" \
           "Quasimodo WCFLOBDD t" "Quasimodo WCFLOBDD mem" "Quasimodo BDD t" "Quasimodo BDD mem" "Quasimodo WBDD t" \
           "Quasimodo WBDD mem" >> "$file_out"

    for folder in "$benchmarks_fd"*/; do
        folder_name=$(basename "$folder")

        # Initialize the flags for crashes and exceeding the timeout
        cflobdd_fail=false
        wcflobdd_fail=false
        bdd_fail=false
        wbdd_fail=false

        files=$(find "$folder" -maxdepth 1 -type f -name '*.qasm' | grep -vE "/NL_" | sort)
        for file in $files; do
            # To avoid an invalid iteration when no file matches the criteria
            if [ -f "$file" ]; then
                run_benchmark_file $file
            fi

            # Random, RevLib, Feynman - nonlinear progression in simulation time -> wait for TO on every benchmark (restart flags)
            if [[ ( "$folder_name" = "Random" ) || ( "$folder_name" = "RevLib" ) || ( "$folder_name" = "Feynman" ) ]]; then
                cflobdd_fail=false
                wcflobdd_fail=false
                bdd_fail=false
                wbdd_fail=false
            fi

            if [[ ( $cflobdd_fail = true ) && ( $wcflobdd_fail = true ) && ( $bdd_fail = true ) && ( $wbdd_fail = true ) ]]; then
                break;
            fi
        done
    done
}

# Removes 'LP-' from benchmark names and correctly reorders the given csv file.
format_csv() {
    local file=$1
    local tmp_file="tmp.csv"

    sed -i '/^LP-/ s/^LP-//' "$file"
    { head -n1 "$file"; tail -n+2 "$file" | sort -t, -k1,1; } > "$tmp_file"
    mv "$tmp_file" "$file"
}

#####################################################################################
# Output:
is_measure=false # global so it can be read from all the functions
run_benchmarks
format_csv $FILE_OUT

is_measure=true
run_benchmarks
format_csv $FILE_OUT_MEASURE