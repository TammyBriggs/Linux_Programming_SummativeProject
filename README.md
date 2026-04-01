# Linux Programming Summative Assessment

This repository contains a collection of systems programming projects demonstrating proficiency in C, x86-64 Assembly, POSIX multithreading, Linux socket programming, and Python C-API extensions.

> **Note on Analysis Reports & Demo video:** The comprehensive reverse engineering reports, static/dynamic ELF analysis, `gdb` traces, `strace` logs, and system architecture documentation as well as demo video for the questions are located in the accompanying **Analysis Report Document**, not in this README.

## Prerequisites
To compile and execute these programs, ensure your Linux environment has the following installed:
* `gcc` (GNU Compiler Collection)
* `nasm` (Netwide Assembler)
* `python3` and `python3-dev` (for C extension headers)
* `make` (optional, for automation)

---

## Question 1: Reverse Engineering Your Own ELF Binary

A C program specifically structured to demonstrate control flow, dynamic memory allocation, and system-level execution, compiled without optimization and stripped of symbols for reverse engineering analysis.

* **Files:** `program.c`
* **Inputs:** None (parameters are hardcoded internally).

**Compilation:**
```bash
gcc -Wall -O0 -fno-inline -o program program.c
strip program
```
(Optimization is disabled to preserve control flow for the static analysis detailed in the main report).

**Execution:**
```bash
./program
```

**Expected Output:**
```
Success: Processed 17 characters.
```

---

## Question 2: x86 Assembly Data Processing

A 64-bit Linux NASM Assembly program that reads a local text file directly into memory, parses varying line endings (Unix `\n` and Windows `\r\n`), and calculates the total versus valid data entries using direct system calls.

* **Files:** `temperature.asm`, `temperature_data.txt`
* **Inputs:** A text file named `temperature_data.txt` in the same directory.

**Sample input:**
```
22.5

24.1
-5.0

18.4
```

**Compilation:**
```bash
nasm -f elf64 temperature.asm -o temperature.o
ld temperature.o -o temperature
```

**Execution:**
```bash
./temperature
```

**Expected Output:**
```
Total readings: 6
Valid readings: 4
```

---

## Question 3: Industrial Vibration Monitoring (Python C Extension)

A highly optimized, zero-allocation C extension for Python. It processes arrays of floating-point vibration data to compute statistical metrics (including Welford's Algorithm for variance) in a single pass.

* **Files:** `vibration.c`, `setup.py`, `test_vibration.py`
* **Inputs:** Handled programmatically via the `test_vibration.py` script (arrays/tuples of floats).

**Compilation:**
```bash
python3 setup.py build_ext --inplace
```

**Execution:**
```bash
python3 test_vibration.py
```

**Expected Output:**
```
--- Testing Industrial Vibration C Extension ---
Data: [2.5, -1.2, 3.8, 0.4, -2.1, 4.5, -0.8]
Threshold: 2.0

--- Computations ---
Peak-to-Peak:    6.6000
RMS:             2.5023
Standard Dev:    2.4965
Above Threshold: 3

--- Summary Dictionary ---
  Count: 7
  Mean: 1.0143
  Min: -2.1000
  Max: 4.5000

--- Testing Edge Cases ---
Testing empty list... Caught safely! -> Input data cannot be empty.
Testing string input... Caught safely! -> Input must be a list or tuple of floats.
```

---

## Question 4: Airport Baggage Handling Simulation

A multithreaded producer-consumer simulation utilizing a circular buffer. It demonstrates thread synchronization, race condition prevention, and shared memory protection using POSIX mutexes and condition variables.

* **Files:** `baggage_system.c`
* **Inputs:** None. The simulation runs autonomously for a target of 20 luggage items.

**Compilation:**
```bash
gcc -Wall -o baggage_system baggage_system.c -lpthread
```

**Execution:**
```bash
./baggage_system
```

**Expected Output (Live Simulation):**

Logs will print progressively over approximately 60 seconds as threads sleep, wake, and synchronize.
```
Starting Airport Baggage Handling Simulation (Target: 20 items)

[Producer] Loaded Luggage ID: 1 | Belt Size: 1/5
[Consumer] Dispatched Luggage ID: 1 to Aircraft | Belt Size: 0/5
...
=== [MONITOR REPORT] ===
Total Loaded onto Belt: 2
Total Dispatched to Aircraft: 1
Current Belt Size: 1/5
========================
...
[Producer] Belt is FULL. Waiting for space...
...
Simulation Complete. All 20 luggage items successfully dispatched.
```

---

## Question 5: Real-Time Digital Library Platform (TCP Sockets)

A robust, concurrent client-server architecture utilizing TCP sockets. Features include a strict C-struct message protocol, duplicate login prevention, graceful client inactivity timeouts, and an I/O multiplexed client (`select()`) capable of receiving asynchronous server shutdown hooks.

* **Files:** `protocol.h`, `server.c`, `client.c`
* **Inputs:**
  * Server Admin Console: Type `0` and press Enter to safely evict all clients and terminate the server.
  * Client Auth: `LIB001`, `LIB002`, `LIB003`, `LIB004`, `LIB005`
  * Client Action: Enter an integer Book ID (e.g., `1`) to reserve, or `0` to exit.

**Compilation:**
```bash
gcc -Wall -o server server.c -lpthread
gcc -Wall -o client client.c
```

**Execution:**

This system requires multiple terminal windows to demonstrate concurrency.

**Terminal 1 (Server):**
```bash
./server
```

**Terminal 2 (Client 1):**
```bash
./client
```
*(Authenticate with `LIB001`)*

**Terminal 3 (Client 2):**
```bash
./client
```
*(Authenticate with `LIB002` and attempt to reserve the same book as Client 1 to test mutex locks)*

**Expected Outputs:**
* **Server Logs:** Real-time status updates showing connected sockets, authentication warnings, and book states.
* **Client UI:** Dynamic menus updating based on server responses (e.g., `>>> SERVER: Already reserved by another user. <<<`).
* **Timeout:** If a client remains idle for 20 seconds, the server will sever the connection, and the client terminal will drop safely back to bash.
* **Shutdown Concurrency:** Utilizing asynchronous I/O multiplexing, the server's admin shutdown command concurrently broadcasts a termination message to all active clients (!!! Server is shutting down for maintenance. !!!), ensuring their sockets are safely closed and the client programs exit gracefully without hanging.

---
