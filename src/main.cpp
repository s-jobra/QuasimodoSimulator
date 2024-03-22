#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include <sys/resource.h>
#include "sim.h"
#include "error.h"

#include "quantum_circuit.h"
#include "quantum_circuit_factory.h"

#define HELP_MSG \
" Usage: sim [options] \n\
\n\
 Options with no argument:\n\
 --help,     -h          show this message\n\
 --info,     -i          measure the simulation runtime and peak memory usage\n\
 \n\
 Options with a required argument:\n\
 --type,     -t          specify the backend type: 'CFLOBDD','BDD','WBDD' (default 'CFLOBDD')\n\
 --file,     -f          specify the input QASM file (default STDIN)\n\
 --nsamples, -n          specify the number of samples used for measurement (default 1024)\n\
 \n\
 Options with an optional argument:\n\
 --measure,  -m          perform the measure operations encountered in the circuit, \n\
                         optional arg specifies the file for saving the measurement result (default STDOUT)"

/**
 * Returns the peak physical memory usage of the process in kilobytes. For an unsupported OS returns -1.
 */
static long get_peak_mem()
{
    long peak = 0;
    #if defined(__unix__) || defined(__APPLE__)
        struct rusage rs_usage;
        if (getrusage(RUSAGE_SELF, &rs_usage) == 0) {
            peak = rs_usage.ru_maxrss;
        }
    #else
        // Unknown OS
        peak = -1;
    #endif
    return peak;
}

int main(int argc, char *argv[])
{
    FILE *input = stdin;
    FILE *measure_output = stdout;
    bool opt_infile = false;
    bool opt_info = false;
    bool opt_measure = false;
    unsigned long samples = 1024;
    std::string sim_type = "CFLOBDD";
    
    int opt;
    static struct option long_options[] = {
        {"help",     no_argument,        0, 'h'},
        {"info",     no_argument,        0, 'i'},
        {"type",     required_argument,  0, 't'},
        {"file",     required_argument,  0, 'f'},
        {"measure",  optional_argument,  0, 'm'},
        {"nsamples", required_argument,  0, 'n'},
        {0, 0, 0, 0}
    };
    char *endptr;
    while((opt = getopt_long(argc, argv, "hit:f:m::n:", long_options, 0)) != -1) {
        switch(opt) {
            case 'h':
                printf("%s\n", HELP_MSG);
                exit(0);
            case 'i':
                opt_info = true;
                break;
            case 't':
                sim_type = optarg;
                if (sim_type != "CFLOBDD" && sim_type != "BDD" && sim_type != "WBDD") {
                    error_exit("Invalid simulation backend option '%s'.\n", optarg);
                }
                break;
            case 'f':
                opt_infile = true;
                input = fopen(optarg, "r");
                if (input == NULL) {
                    error_exit("Invalid input file '%s'.\n", optarg);
                }
                break;
            case 'm':
                opt_measure = true;
                if (!optarg && optind < argc && argv[optind][0] != '-') {
                    optarg = argv[optind++];
                    measure_output = fopen(optarg, "w");
                    if (measure_output == NULL) {
                        error_exit("Invalid output file '%s'.\n", optarg);
                    }
                }
                break;
            case 'n':
                samples = strtoul(optarg, &endptr, 10);
                if (*endptr != '\0') {
                    error_exit("Invalid number of samples.\n");
                }
                break;
            case '?':
                exit(1); // error msg already printed by getopt_long
        }
    }

    // Init:
    QuantumCircuit* qc = QuantumCircuitFactory::create(sim_type);
    assert(qc != NULL);
    int *bits_to_measure;
    bool is_measure = false;
    int n_qubits;

    // Sim:
    struct timespec t_start, t_finish;
    double t_el;
    clock_gettime(CLOCK_MONOTONIC, &t_start); // Start the timer

    sim_file(input, qc, &n_qubits, &bits_to_measure, &is_measure);

    if (opt_measure && is_measure) {
        // Quasimodo only supports measurement of all qubits and in the same order
        bool valid_measure_all = true;
        for (int i = 0; i < n_qubits; i++) {
            if (bits_to_measure[i] != i) {
                valid_measure_all = false;
                break;
            }
        }
        if (valid_measure_all) {
            measure_all(samples, measure_output, qc, n_qubits);
        }
        else {
            error_exit("Unsupported measurement operation - must measure all qubits and their order must remain the same.\n");
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t_finish); // End the timer
    
    // Output:
    t_el = t_finish.tv_sec - t_start.tv_sec + (t_finish.tv_nsec - t_start.tv_nsec) * 1.0e-9;
    if (opt_info) {
        printf("Time=%.3gs\n", t_el);
        #if defined(__unix__) || defined(__APPLE__)
            printf("Peak Memory Usage=%ldkB\n", get_peak_mem());
        #else
            printf("Peak Memory Usage not supported for this OS.\n");
        #endif
    }

    // Finish:
    if (opt_infile) {
        fclose(input);
    }

    return 0;
}