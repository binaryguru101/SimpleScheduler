# SimpleScheduler Project

This project involves creating a **SimpleScheduler**, a basic implementation of a process scheduler in plain C. The scheduler simulates the behavior of an operating system in managing limited CPU resources across multiple processes. While simplified compared to real-world schedulers, this assignment demonstrates core concepts of process scheduling and CPU management.

## Functionality

The **SimpleScheduler** performs the following tasks:

1. **Ready Queue Management**: Processes in the ready state are queued for CPU execution.
2. **CPU Scheduling Policy**: Implements a scheduling policy to determine which process gets CPU time next.
3. **Quantum-Based Execution**: Each process is allotted a specific quantum of CPU time. Once the quantum expires:
   - The process is moved from the running queue back to the ready queue.
   - Another process is selected from the ready queue to run.
4. **Limited Capabilities**: The implementation is designed to provide a simplified version of a process scheduler to demonstrate the core principles.

## Key Learning Objectives

- Understand how operating systems manage processes in the ready queue.
- Learn the basics of CPU scheduling policies and quantum-based execution.
- Practice implementing system-level functionality in C.
