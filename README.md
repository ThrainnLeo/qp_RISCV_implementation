# Medical Device Software Platform (RISC-V + QP/C + QM)

[![Platform: RISC-V](https://img.shields.io/badge/Platform-RISC--V-orange)](https://riscv.org/)
[![Framework: QP/C](https://img.shields.io/badge/Framework-QP%2FC-blue)](https://www.state-machine.com/qpc)
[![Standard: IEC 62304](https://img.shields.io/badge/Standard-IEC%2062304-green)](https://www.iso.org/standard/38421.html)
[![Standard: ISO 13485](https://img.shields.io/badge/Standard-ISO%2013485-blue)](https://www.iso.org/iso-13485-medical-devices.html)
[![Standard: ISO 14971](https://img.shields.io/badge/Standard-ISO%2014971-red)](https://www.iso.org/standard/72704.html)
[![Standard: IEC 61508](https://img.shields.io/badge/Standard-IEC%2061508-lightgrey)](https://www.iec.ch/functional-safety)

## 📌 Overview
This project demonstrates a modern, safety-critical software platform designed for medical devices. By combining a model-based design workflow with an open-source hardware foundation, the platform lowers the threshold for regulatory compliance while ensuring high software quality and patient safety.

## 📂 Repository Structure
The project is divided into two main stages to ensure a verified and stable implementation:

### 1. [QProjects/](./QProjects) (Porting & Validation)
This directory contains the initial phase of the experiment.
- **Objective:** To port the QP/C Real-Time Embedded Framework (RTEF) to the RISC-V architecture.
- **Content:** Basic implementations and "Blinky" tests used to verify that the Board Support Package (BSP), machine timer interrupts, and the event-driven engine are working correctly on the target hardware.
- **Outcome:** A confirmed and stable porting of QP/C to RISC-V, serving as the foundation for the final experiment.

### 2. [InfusionPump/](./InfusionPump) (Final Experiment)
This is the core of the project and represents the final medical device platform.
- **Objective:** To implement a fully functional, safety-critical medical infusion pump logic.
- **Content:** Advanced Active Objects (e.g., `PumpMgr` and `Medicine`) designed as Hierarchical State Machines. It features asynchronous alarm handling, deterministic dosage control, and software partitioning.
- **Outcome:** A robust demonstration of how international standards like **IEC 62304** and **IEC 61508** can be addressed through modern software architecture.

## 🚀 Key Features
- **Model-Based Development:** Visual state machine design using the QM™ modeling tool.
- **Automatic Code Generation:** MISRA-C compliant code generation to minimize human implementation errors.
- **Deterministic Scheduling:** Non-blocking execution context using Active Objects for strict "Freedom from Interference."
- **Full Transparency:** No "Black Boxes" – full visibility from the RISC-V instruction set to the application logic.

## 🛠 Tech Stack
- **Hardware:** RISC-V (GD32VF103 / Longan Nano)
- **Framework:** [Quantum Leaps QP/C](https://www.state-machine.com/qpc)
- **Modeling Tool:** QM™ (Quantum Model-based design)
- **Language:** C (MISRA-compliant)

## 📂 Architecture
The platform is organized into a layered architecture to support modularity and easier certification:
1. **Application Layer:** Encapsulated Active Objects (e.g., Pump Manager, Medicines).
2. **Framework Layer:** QP/C RTEF handling event delivery and state machine execution.
3. **BSP Layer:** Deterministic hardware drivers for RISC-V.

## ⚠️ Regulatory Compliance & Standards
This platform is architected to support the following international standards:

### 🛡️ Software Lifecycle (IEC 62304)
- Implements **Software Partitioning** to ensure that critical functions (Class C) are isolated from non-critical tasks.
- Provides a modular structure that simplifies unit testing and verification.

### ⚙️ Quality Management (ISO 13485)
- Supports **Design Controls** through a traceable workflow from QM models to generated source code.
- Ensures a reproducible build process, essential for the Technical File and Clinical Evaluation Reports (CER).

### 📈 Risk Management (ISO 14971)
- Integrates **Risk Mitigation** at the architectural level by using an event-driven model that eliminates common hazards like race conditions and deadlocks.
- Implements a "Safe State" mechanism in the BSP to handle initialization failures.

### 🔒 Functional Safety (IEC 61508)
- Achieves **Determinism** through a Run-to-Completion (RTC) execution model.
- Uses hardware-based timers on RISC-V to ensure high temporal precision for safety-critical events.

## 📊 Evaluation & Traceability
The system supports real-time software instrumentation using **QSPY (QS)**. This allows for deep introspection and verification of the system's behavior against requirements without violating real-time constraints.

---
*Developed as part of a Thesis Project focused on modernizing medical device development through open-source hardware and event-driven frameworks.*
