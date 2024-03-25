#include "sim.h"

#define NO_ALT_END -2

/**
 * Function for number parsing from the input file (reads the number from the input until the end character is encountered)
 * Checks for two possible end characters. If only one character should be checked agains, set alt_end to NO_ALT_END.
 */
static long long parse_num(FILE *in, char end, char alt_end)
{
    int c = fgetc(in);
    char num[NUM_MAX_LEN] = {0};
    long long n;

    // Skip leading whitespace
    while (isspace(c)) {
        c = fgetc(in);
    }

    // Load number to string
    while ( c != end && c != alt_end) {
        if (c == EOF) {
            error_exit("Invalid format - reached an unexpected end of file when converting a number.\n");
        }
        else if (!isdigit(c) && c != '-') {
            // Check if isn't just trailing whitespace
            while (isspace(c)) {
                c = fgetc(in);
            }
            if (c == end || c == alt_end) {
                break;
            }
            else {
                error_exit("Invalid format - not a valid number (a non-digit character '%c' encountered while parsing a number).\n", c);
            }
        }
        else if (strlen(num) + 1 < NUM_MAX_LEN) {
            int *temp = &c;
            strncat(num, (char*)temp, 1);
        }
        else {
            error_exit("Invalid format - not a valid number (too many digits).\n");
        }
        c = fgetc(in);
    }

    // Convert to integer value
    char *ptr;
    errno = 0;
    n = strtoll(num, &ptr, 10);
    if (num == ptr || errno != 0) {
        error_exit("Invalid format - not a valid number.\n");
    }
    return n;
}

/** 
 * Function for getting the next qubit index for the command on the given line.
 */
static uint32_t get_q_num(FILE *in)
{
    int c;
    long long n;

    while ((c = fgetc(in)) != '[') {
        if (c == EOF) {
            error_exit("Invalid format - reached an unexpected end of file (expected a qubit index).\n");
        }
    }

    n = parse_num(in, ']', NO_ALT_END);
    if (n > UINT32_MAX || n < 0) {
        error_exit("Invalid format - not a valid qubit identifier.\n");
    }

    return ((uint32_t)n);
}

/**
 * Returns the number of iterations, should be called when a for loop is encountered
 */
static uint64_t get_iters(FILE *in)
{
    int c;
    long long start, end;
    long long step = 1;
    fpos_t second_num_pos;
    uint64_t iters;

    while ((c = fgetc(in)) != '[') {
        if (c == EOF) {
            error_exit("Invalid format - reached an unexpected end of file (expected a number of loop iterations).\n");
        }
    }

    start = parse_num(in, ':', NO_ALT_END);
    end = parse_num(in, ']', ':');
    if (fseek(in, -1, SEEK_CUR) != 0) {
        error_exit("Error has occured when parsing loop parameters (fseek error).\n");
    }
    if ((c = fgetc(in)) == ':') {
        step = end;
        end = parse_num(in, ']', NO_ALT_END);
    }
    
    // Note: expects 64bit long long
    //TODO: better error detection?
    if (step == 0) {
        error_exit("Invalid number of loop iterations - step must be non-zero.\n");
    }
    else if ((end == LLONG_MAX && start == LLONG_MIN) || (end == LLONG_MIN && start == LLONG_MAX)) {
        error_exit("Invalid number of loop iterations - overflow detected.\n");
    }
    else if ((end + 1 - start) % step != 0) {
        error_exit("Invalid number of loop iterations - not an integer.\n");
    }
    else if ((end < start && step > 0) || (end > start && step < 0)) {
        error_exit("Invalid number of loop iterations.\n");
    }

    iters = (end + 1 - start) / step;
    
    return ((uint64_t) iters);
}

void sim_file(FILE *in, QuantumCircuit *circ, int *n_qubits, int **bits_to_measure, bool *is_measure)
{
    //TODO: refactoring
    //TODO: add line counter and display in errors
    int c;
    char cmd[CMD_MAX_LEN];
    bool init = false;

    bool is_loop = false;
    bool loop_first = true;
    fpos_t loop_start;
    uint64_t iters;

    while ((c = fgetc(in)) != EOF) {
        for (int i=0; i < CMD_MAX_LEN; i++) {
            cmd[i] = '\0';
        }

        while (isspace(c)) {
            c = fgetc(in);
        }

        if (c == EOF) {
            return;
        }

        // Skip one-line comments
        if (c == '/') {
            if ((c = fgetc(in)) == '/') {
                while ((c = fgetc(in)) != '\n') {
                    if (c == EOF) {
                        return;
                    }
                }
                continue;
            }
            else {
                error_exit("Invalid command, expected a one-line comment.\n");
            }
        }

        // Load the command
        do {
            if (c == EOF) {
                error_exit("Invalid format - reached an unexpected end of file when loading a command.\n");
            }
            else if (strlen(cmd) + 1 < CMD_MAX_LEN) {
                int *temp = &c;
                strncat(cmd, (char *)temp, 1);
            }
            else {
                error_exit("Invalid command (command too long).\n");
            }
        } while (!isspace(c = fgetc(in)));

        if (c == EOF) {
            error_exit("Invalid format - reached an unexpected end of file immediately after a command.\n");
        }

        // Identify the command
        if (strcmp(cmd, "OPENQASM") == 0) {}
        else if (strcmp(cmd, "include") == 0) {}
        else if (strcmp(cmd, "creg") == 0) {} //TODO: check if is valid?
        else if (strcmp(cmd, "qreg") == 0) {
            uint32_t n = get_q_num(in);
            circ->setNumQubits(n);
            *n_qubits = (int)n;
            *bits_to_measure = (int*)my_malloc(n * sizeof(int));
            for (int i=0; i < n; i++) {
                (*bits_to_measure)[i] = -1;
            }
            init = true;
        }
        else if (init) {
            if (strcmp(cmd, "for") == 0) {
                iters = get_iters(in);
                if (iters == 0) {
                    // skip symbolic
                    while ((c = fgetc(in)) != '}') { //TODO: check for comments - shouldn't count commented }
                        if (c == EOF) {
                            error_exit("Invalid format - reached an unexpected end of file (there is an unfinished loop).\n");
                        }
                    }
                    continue;
                }
                while ((c = fgetc(in)) != '{') {
                    if (c == EOF) {
                        error_exit("Invalid format - reached an unexpected end of file at the start of a loop.\n");
                    }
                }
                is_loop = true; // TODO: allow nested loops?
                if (fgetpos(in, &loop_start) != 0) {
                    error_exit("Could not get the current position of the stream to mark the start of a loop.\n");
                }
                continue; // ';' not expected
            }
            else if (strcmp(cmd, "}") == 0) {
                if (!is_loop) {
                    error_exit("Invalid loop syntax - reached an unexpected end of a loop.\n");
                }
                iters--;
                if (!iters) {
                    is_loop = false;
                }
                else { // next iteration
                    if (fsetpos(in, &loop_start) != 0) {
                        error_exit("Could not set a new position of the stream.\n");
                    }
                }
                continue; // ';' not expected
            }
            else if (strcmp(cmd, "measure") == 0) {
                uint32_t qt = get_q_num(in);
                uint32_t ct = get_q_num(in);
                *is_measure = true;
                (*bits_to_measure)[qt] = ct;
            }
            else if (strcasecmp(cmd, "x") == 0) {
                uint32_t qt = get_q_num(in);
                circ->ApplyNOTGate(qt);
            }
            else if (strcasecmp(cmd, "y") == 0) {
                uint32_t qt = get_q_num(in);
                circ->ApplyPauliYGate(qt);
            }
            else if (strcasecmp(cmd, "z") == 0) {
                uint32_t qt = get_q_num(in);
                circ->ApplyPauliZGate(qt);
            }
            else if (strcasecmp(cmd, "h") == 0) {
                uint32_t qt = get_q_num(in);
                circ->ApplyHadamardGate(qt);
            }
            else if (strcasecmp(cmd, "s") == 0) {
                uint32_t qt = get_q_num(in);
                circ->ApplySGate(qt);
            }
            else if (strcasecmp(cmd, "t") == 0) {
                uint32_t qt = get_q_num(in);
                circ->ApplyTGate(qt);
            }
            else if (strcasecmp(cmd, "cx") == 0) {
                uint32_t qc = get_q_num(in);
                uint32_t qt = get_q_num(in);
                circ->ApplyCNOTGate(qc, qt);
            }
            else if (strcasecmp(cmd, "cz") == 0) {
                uint32_t qc = get_q_num(in);
                uint32_t qt = get_q_num(in);
                circ->ApplyCZGate(qc, qt);
            }
            else if (strcasecmp(cmd, "ccx") == 0) {
                uint32_t qc1 = get_q_num(in);
                uint32_t qc2 = get_q_num(in);
                uint32_t qt = get_q_num(in);
                circ->ApplyCCNOTGate(qc1, qc2, qt);
            }
            else if (strcasecmp(cmd, "cswap") == 0) {
                uint32_t qc = get_q_num(in);
                uint32_t qt1 = get_q_num(in);
                uint32_t qt2 = get_q_num(in);
                circ->ApplyCSwapGate(qc, qt1, qt2);
            }
            else {
                error_exit("Invalid command '%s'.\n", cmd);
            }
        }
        else {
            error_exit("Circuit not initialized.\n");
        }

        // Skip all remaining characters on the currently read line
        while ((c = fgetc(in)) != ';') {
            if (c == EOF) {
                error_exit("Invalid format - reached an unexpected end of file (expected ';' to end the current line).\n");
            }
        }
    } // while
}

void measure_all(unsigned long samples, FILE *output, QuantumCircuit *circ, int n)
{
    std::string curr_state;

    htab_t *state_table = htab_init(n*n); //TODO: is optimal?
    
    for (unsigned long i=0; i < samples; i++) {
        curr_state = circ->Measure();
        htab_m_lookup_add(state_table, (htab_m_key_t)(curr_state.c_str()));
    }
    htab_m_print_all(state_table, output);
    htab_m_free(state_table);
}

/* end of "sim.c" */