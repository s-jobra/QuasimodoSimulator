#pragma once

#include <string>
#include "quantum_circuit.h"

class QuantumCircuitFactory {
public:
    static QuantumCircuit* create(const std::string& type);
};