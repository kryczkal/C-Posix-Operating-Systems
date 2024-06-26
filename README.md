## Introduction
This repository is a collection of programs showcasing my ability to design and implement programs in C using POSIX standards.
It focuses on the knowledge of operating systems and concurrent programming, including the use of sockets, processes, signals, descriptors, synchronization techniques, thread management, and inter-process communication.

## Subjects

### Sockets
- [`Calc-Server`](Sockets/Calculator-Network-Client-Server) An online calculator using tcp sockets and local sockets with the corresponding clients

### Process, Signals, and Descriptors
- [`classroom-scenario.c`](Processes-signals-and-descriptors/classroom-scenario.c): Simulates classroom scheduling
- [`epidemic-simulation.c`](Processes-signals-and-descriptors/epidemic-simulation.c): Models an epidemic spread scenario

### Synchronization Techniques
- Includes files showcasing the use of mutexes, semaphores, and barriers for thread synchronization in concurrent programming.
- [`card-game-simulation.c`](Synchronization/card-game-simulation.c): Implements a multi-player card game simulation
- [`concurrent-array-usage.c`](Synchronization/concurrent-array-usage.c): Soncurrent array operations
- [`dice-game-simulation.c`](Synchronization/dice-game-simulation.c): a dice game simulation.
- [`toy-factory.c`](Synchronization/toy-factory.c) & [`toy-factory.h`](Synchronization/toy-factory.h): consumer-producent dynamics using semaphores in a toy-factory simulation
- [`video-player`](Synchronization/video-player.c) & [`video-player.h`](Synchronization/video-player.h): A consumer-producent scenario in a multi-threaded video-player with concurrent execution of decoding, encoding, and displaying video frames.

### Thread Management and Signals
- [`student-simulation.c`](Threads-and-signals/student-simulation.c): Student lifetime simulation

### Fifo and Pipes
- Contains programs that demonstrate inter-process communication using FIFOs and pipes.
- [`client.c`](Fifo-and-Pipes/Fifo-Client-Server/client.c) & [`server.c`](Fifo-and-Pipes/Fifo-Client-Server/server.c)
- [`pipe-prog1.c`](Fifo-and-Pipes/Pipe-Prog1/pipe-prog1.c)

### Posix Message Queues
- Contains programs that demonstrate inter-process communication using POSIX message queues.
- [`bingo-simulation.c`](Posix-message-queues/Bingo-simulation/bingo-simulation.c): A simulation of a bingo game
- [`server.c`](Posix-message-queues/Client-Server/server.c) & [`client.c`](Posix-message-queues/Client-Server/client.c)
- [`uber-driver-simulation.c`](Posix-message-queues/Uber-drivers-simulation/uber-driver-simulation.c): A simulation of a car transportation system

### Shared Memory
- [`robbery-simulation-server.c`](Shared-Memory/Client-Server-Shared-Memory/server.c) & [`robbery-simulation-client.c`](Shared-Memory/Client-Server-Shared-Memory/client.c) - A simulation of concurrent robbers robbing a dungeon using shared memory with mmap
- [`monte-carlo-pi.c`](Shared-Memory/Monte-Carlo-Pi/monte-carlo-pi.c) - Computing pi using many threads with shared memory using mmap

### File System Management
- Contains programs that demonstrate file management, directory operations, and file system interfaces.
- [`file-system-interface.c`](File-system/file-system-interface.c)

### POSIX Execution Environment
- Features programs which simulate scheduling and execution in a POSIX environment, among other utilities that exhibit process control and environment management.