# Overview

mvs is an implementation in the C++14 language of the algorithm
described in [1] for computing the maximum subgraphs of a directed
acyclic graph under convexity, input and output constraints.

# Installation

Install **CMake** and use the included CMake script to build mvs:
```
mkdir build
cd build
cmake <path/to/mvs>
cmake --build .
```
The resulting binary is named **mvs**. The compilation requires a
system (mostly) compliant with the POSIX.1-2001 standard and a **c++**
compiler compatible with the C++14 standard.

# Input format

mvs uses a superset of the DIMACS format as graph input format. An
input file is a sequence of lines. The first line must be in the
format

p convex NODES EDGES NAME FREQUENCY

where NODES and EDGES are the number of nodes and edges, respectively,
NAME is the graph name, and FREQUENCY is an integer equal to the graph
frequency, in the case of a data flow graph, and to 0 otherwise. The
following lines can specify either edges or node attributes. A line
with format

e U V

specifies an edge from node U to node V, where U and V are integers in
the range [1, NODES]. A line with format

n U WEIGHT FORBIDDEN

specifies the attributes of node U, where WEIGHT is a real number
defining the weight of U, and FORBIDDEN is 1 if U is a forbidden
node and 0 otherwise.

mvs assumes that all the sources and sinks of the graph are forbidden
nodes. The FORBIDDEN attribute can be used to mark additional
nodes as forbidden.

# Usage

mvs reads the input from **standard input**. The command

`mvs MAX-IN MAX-OUT < FILE`

enumerates the maximum convex subgraphs of the input graph with at
most MAX-IN inputs and MAX-OUT outputs.

The output of mvs is in JSON format. mvs also logs to **standard error**
various debug messages. They can be ignored by redirecting **standard
error** to **/dev/null**. By default, mvs enumerates the maximum
subgraphs with respect to the number of nodes. To enumerate the
weighted maximum subgraphs use the **-w** option.

# Additional files

The mvs repository also contains the following files and directories:

* **data**

the benchmark graphs used in [1], in DIMACS format, DOT format [2] and
DOT JSON format [3]. In the DOT formats, node objects have the
additional attributes **weight** and **forbidden**, while graph
objects have the additional attribute **frequency**.

* **scripts/convert**

script to convert a graph in DOT JSON format to DIMACS or DOT format. Usage:

`convert --fmt dimacs FILE.json > FILE.dimacs`

`convert --fmt dot FILE.json > FILE.dot`

# References

[1] [Emanuele Giaquinta, Anadi Mishra, Laura Pozzi: Maximum Convex Subgraphs Under I/O Constraint for Automatic Identification of Custom Instructions](https://doi.org/10.1109/TCAD.2014.2387375)

[2] [The DOT Language](https://www.graphviz.org/doc/info/lang.html)

[3] [The DOT JSON format](https://www.graphviz.org/doc/info/output.html#d:dot_json)
