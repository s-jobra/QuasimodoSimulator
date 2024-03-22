#include <ctype.h>  // For isspace(), isdigit()
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>

#include "error.h"
#include "htab.h"
#include "quantum_circuit.h"

#ifndef SIMULATOR_H
#define SIMULATOR_H

#define CMD_MAX_LEN 10 // Max. supported length of qasm command
#define NUM_MAX_LEN 25 // Max. number of characters in a parsed number

/**
 * Parses a given QASM file and simulates this circuit
 * 
 * @param in input QASM file
 * 
 * @param circ the state vector of the circuit
 * 
 * @param n_qubits number of qubits in the circuit
 * 
 * @param bits_to_measure array for storing the qubits that are to be measured
 * 
 * @param is_measure true if some measure operation is present
 * 
 */
void sim_file(FILE *in, QuantumCircuit *circ, int *n_qubits, int **bits_to_measure, bool *is_measure);

/**
 * Measures all bits in the given array (compatible only with measurement at the end of the circuit)
 * 
 * @param samples the total number of samples
 * 
 * @param output stream for the output of results
 * 
 * @param circ the state vector of the circuit
 * 
 * @param n number of qubits in the circuit
 * 
 */
void measure_all(unsigned long samples, FILE *output, QuantumCircuit *circ, int n);

#endif
/* end of "sim.h" */
