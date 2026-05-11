# Implementation & Experimental Focus

This directory contains the source code for a Medical Infusion Pump platform. The core objective of this experiment is to demonstrate a **"Safe by Design"** methodology using the Active Object (Actor) design pattern on open-source RISC-V hardware.

## 📁 Project Roadmap

- `app.h` & `main.c` – System entry point and Active Object initialization.
- `pumpMgr.c` & `medicine.c` – Core application logic generated from QM™ models.
- `bsp/` – **Board Support Package**. Manages RISC-V hardware timers and ISRs.
- `qpc/` – **The QP/C Framework**. Localized execution engine for event-driven logic.
- `Makefile` – Optimized build script for the RISC-V GNU toolchain.

## 🔬 Why this Experiment? (The Philosophy)

Traditional embedded software often relies on shared global variables and complex RTOS threading, which are prone to race conditions and non-deterministic behavior. This experiment replaces that model with **Active Objects** to achieve:

### 1. Total Encapsulation (Software Partitioning)
Each module (e.g., `Medicine` and `PumpMgr`) is an isolated "Active Object." They own their own data and do not share memory.
- **Goal:** To prove that an error in one module cannot corrupt the state of another, fulfilling the **IEC 62304** requirement for software isolation.

### 2. Asynchronous Event-Driven Communication
Instead of direct function calls, objects communicate via **Asynchronous Events**.
- **Goal:** To eliminate blocking. In a medical pump, the alarm system must never "wait" for a UI update or a sensor read. Events ensure the system remains responsive to critical signals at all times.

### 3. Run-to-Completion (RTC) Determinism
Each event is processed fully before the next one starts.
- **Goal:** To ensure **Predictability (IEC 61508)**. By using RTC, we eliminate the need for complex mutexes and semaphores, effectively preventing deadlocks and race conditions by design.



## ⚙️ Workflow: From Design to Execution

1. **Modeling (QM™):** The logic is designed as Hierarchical State Machines (HSMs). This allows for visual verification of all possible system states, ensuring no "dead states" exist.
2. **Generation:** MISRA-C compliant code is generated, linking the visual model directly to the implementation for 100% traceability.
3. **Execution (RISC-V):** The code runs on a transparent, open-source hardware layer, ensuring that the timing and execution are fully auditable.

## 🛠 Toolchain & Build

1. **Compiler:** `riscv-none-embed-gcc`
2. **Build:** Run `make` to compile the local `qpc/` framework and application files.
3. **Trace:** Use **QSPY** to monitor the event-flow and state transitions in real-time. (Our testing does not implement **QSPY**)

---
*By utilizing Active Objects, this experiment demonstrates a platform where safety is not an added feature, but a fundamental property of the software architecture.*
