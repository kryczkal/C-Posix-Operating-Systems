## Introduction
This repository is a curated collection of my work showcasing a range of skills from file system interfaces to multi-threaded simulations. Each program is a testament to my ability to design, implement, and manage complex systems using C and POSIX standards.

### File Structure
The repository is organized into directories based on the skill or ability demonstrated by the contained programs. Each program contains has a big comment in its file that provides an overview of its functionalities.

## Skills and Abilities

### File System Management
- Contains programs that demonstrate file management, directory operations, and file system interfaces.
- [`file-system-interface.c`](File-system/file-system-interface.c): Demonstrates the creation and manipulation of file systems, including file operations and directory traversal.

### POSIX Execution Environment
- Features programs which simulate scheduling and execution in a POSIX environment, among other utilities that exhibit process control and environment management.

### Process, Signals, and Descriptors
- Includes files showcasing process synchronization, signal handling, and descriptor manipulation.
- [`classroom-scenario.c`](Processes-signals-and-descriptors/classroom-scenario.c): Simulates classroom scheduling using process management and inter-process communication.
- [`epidemic-simulation.c`](Processes-signals-and-descriptors/epidemic-simulation.c): Models an epidemic spread scenario employing process synchronization and signal handling.

### Synchronization Techniques
- Includes files showcasing the use of mutexes, semaphores, and barriers for thread synchronization in concurrent programming.
- [`card-game-simulation.c`](Synchronization/card-game-simulation.c): Implements a multi-player card game simulation using POSIX threads and mutexes for state management.
- [`concurrent-array-usage.c`](Synchronization/concurrent-array-usage.c): Showcases concurrent array operations safeguarded with mutexes to ensure data consistency.
- [`dice-game-simulation.c`](Synchronization/dice-game-simulation.c): Uses barriers to synchronize thread progression in a dice game simulation.
- [`toy-factory.c`](Synchronization/toy-factory.c) & [`toy-factory.h`](Synchronization/toy-factory.h): Simulates a full lifecycle of toy manufacturing using semaphores for synchronization between various production stages.
- [`video-player`](Synchronization/video-player.c) & [`video-player.h`](Synchronization/video-player.h): A multi-threaded video-player using pthreads for concurrent execution of decoding, encoding, and displaying video frames.

### Thread Management and Signals
- Encompasses threading models, highlighting advanced thread management, signal handling, and inter-thread communication.
- [`student-simulation.c`](Threads-and-signals/student-simulation.c): Simulation of student activities using advanced thread management and signal handling techniques for a robust, error-free operation.

### Fifo and Pipes
- Contains programs that demonstrate inter-process communication using FIFOs and pipes.
- [`client.c`](Fifo-and-Pipes/Fifo-Client-Server/client.c) & [`server.c`](Fifo-and-Pipes/Fifo-Client-Server/server.c): These programs demonstrate inter-process communication using FIFOs (named pipes). The `client.c` program reads data from a file and sends it to a FIFO, with the process PID added at the front of each message. The `server.c` program reads data from the FIFO and prints the sender's PID and the message received. Multiple clients can send messages to the server, and the server will print the sender's PID and the message received.
- [`pipe-prog1.c`](Fifo-and-Pipes/Pipe-Prog1/pipe-prog1.c): A multi-process application that creates a specified number of child processes and establishes a communication channel with each of them. The parent process waits for a SIGINT signal and upon receiving it, selects a random child process and sends a random character to it. The child process, upon receiving the character, decides whether to terminate itself or to send a message back to the parent.

### Posix Message Queues
- Contains programs that demonstrate inter-process communication using POSIX message queues.
- [`bingo-simulation.c`](Posix-message-queues/Bingo-simulation/bingo-simulation.c): A simulation of a bingo game using POSIX message queues. The game involves multiple child processes (players) communicating their moves to a parent process (game controller) through message queues.
- [`server.c`](Posix-message-queues/Client-Server/server.c) & [`client.c`](Posix-message-queues/Client-Server/client.c): The `server.c` program performs operations based on the requests received from the `client.c` program. The server and client communicate using message queues named after the server's operation and the client's PID.
- [`uber-driver-simulation.c`](Posix-message-queues/Uber-drivers-simulation/uber-driver-simulation.c): A simulation of a car transportation system. The simulation involves multiple driver processes receiving tasks from a main process and reporting back the results of the tasks.

## Conclusion
This portfolio is designed to demonstrate my backend development expertise and problem-solving capabilities. Each mini-project contained herein is an example of my commitment to high-quality, maintainable, and efficient software design. I am eager to bring these skills to a dynamic team and contribute to impactful projects.