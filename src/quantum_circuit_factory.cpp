#include "quantum_circuit_factory.h"

QuantumCircuit* QuantumCircuitFactory::create(const std::string& type) {
    if (type == "CFLOBDD")
        return new CFLOBDDQuantumCircuit();
    else if (type == "WCFLOBDD")
        return new WeightedCFLOBDDQuantumCircuit();
    else if (type == "BDD")
        return new BDDQuantumCircuit();
    else if (type == "WBDD")
        return new MQTDDCircuit();
    else
        return NULL;
}
