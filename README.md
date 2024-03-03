## Introduction
This repository is a curated collection of my work showcasing a range of skills from file system interfaces to multi-threaded simulations. Each program is a testament to my ability to design, implement, and manage complex systems using C and POSIX standards.

### File Structure
Each folder consists of bigger, named programs, and smaller programs with "prog[number]" naming convention, that implement smaller, highly specific functionality 

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
- [`toy-factory.c`](Synchronization/toy-factory.c) & [`toy-factory.h`](Synchronization/toy-factory.h): Exhibits a full lifecycle of toy manufacturing using semaphores for synchronization between various production stages.
- [`video-player`](Synchronization/video-player.c) & [`video-player.h`](Synchronization/video-player.h): Demonstrates a multi-threaded video-player using pthreads for concurrent execution of decoding, encoding, and displaying video frames.

### Thread Management and Signals
- Encompasses threading models, highlighting advanced thread management, signal handling, and inter-thread communication.
- [`student-simulation.c`](Threads-and-signals/student-simulation.c): A comprehensive simulation of student activities using advanced thread management and signal handling techniques for a robust, error-free operation.

## Conclusion
This portfolio is designed to demonstrate my backend development expertise and problem-solving capabilities. Each mini-project contained herein is an example of my commitment to high-quality, maintainable, and efficient software design. I am eager to bring these skills to a dynamic team and contribute to impactful projects.