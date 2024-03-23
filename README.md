# Quasimodo Simulator
Parser for OpenQASM circuits using [Quasimodo](https://github.com/trishullab/Quasimodo) for backend.

Originally used as a parser for the [MEDUSA](https://github.com/s-jobra/MEDUSA) simulator, therefore most of it is written in C.

Assumes all non-python prerequisites are installed as stated in Quasimodo's README.
Then you can build the simulator with:
```
./scripts/config-quasimodo
make
```

## Usage
The simulator accepts path to the circuit file with either as a program argument or from the standard input, such as:
```
./QuasimodoSim -f circuit.qasm 
```
```
./QuasimodoSim <circuit.qasm 
```
You can also run the simulator with the flag `-i` to print runtime (wall-clock time) and peak physical memory usage to the standard output.
To enable qubit measurement, use flag `-m` (you can specify the number of measurement samples with `-n`).
You can find more information about program options with `-h`.