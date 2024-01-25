# Work Samples
Within this repo is a collection of various pieces of programming I've done on different projects. The first folder ("CSharp") contains samples from C\# projects I've done for game development, hobby work, and college curriculum. The second folder ("C") contains C projects done for university coursework. An explanation/brief of each sample follows:

# C\# Projects

## SchemaGenHandler.cs
A .NET CLI tool which produces .XSD schema from .XML files. It works by recursively iterating through each supplied directory, reading all discovered .XML files, parsing them into a graph structure, then constructing an .XSD document out of the graph. It leverages StringBuilder to greatly reduce the overhead of string operations, helper methods (see to: SchemaGenHandler_HelperFunctions.cs) to convert the graph data into XML/XSD syntax tidily, and documentation with examples to improve usability. This project is used to assist in the development of RimWorld mods by supplying an XSD file, which enables tools like IntelliSense to validate XML files being written by modders.

## Synchronizer.cs
The synchronization component of a multiplayer lockstep RTS. It's responsible for handling the game lock, propagating player inputs, and advancing the simulation. It maintains synchronization by implementing deterministic lockstep with a server-client model, only advancing the simulation when the inputs of all players has been received on the server at each input snapshot, paired with deterministic floating point math to minimize simulation drift.

## VertexPipeline.cs
A CPU implementation of a pipeline which converts world-coordinate defined vertices into NDC-space equivalents, applying many linear algebra concepts. It works by: building a perspective matrix for a camera, modifying the perspective matrix to factor in the position/rotation of the camera, homogenizing the vertex positions, then converting the homogenized vectors into NDC-space. This pipeline acts as a middleman between a simulation's objects and its renderer, and was made as a part of a final project for a linear algebra course.

# C Projects

##  BankersAlgorithm.c
An implementation of the Banker's resource allocation algorithm. The program asks the user for parameters of the processes (quantity, current resource allocation, maximum resources required), the parameters of the resources (quantity, amount available each), then attempts to sequence each process, checking if a deadlock has occurred. This was made to demonstrate understanding of allocation methods and deadlock detection.

## MemoryHoleFillingAlgorithms.c
An implementation of the first-fit and best-fit algorithms of memory block allocation. It takes the user's input for memory size and desired algorithm, then allows them to allocate, defragment, and deallocate memory as desired. It was made to help learn the process of memory management, and gain some familiarity with the C programming language.
