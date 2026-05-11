# Medical Device Software Platform (RISC-V + QP/C + QM)

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
![Platform: RISC-V](https://img.shields.io/badge/Platform-RISC--V-orange)
![Framework: QP/C](https://img.shields.io/badge/Framework-QP%2FC-blue)
![Standard: IEC 62304](https://img.shields.io/badge/Standard-IEC%2062304-green)

## 📌 Overview
This project demonstrates a modern, safety-critical software platform designed for medical devices (e.g., infusion pumps). It leverages a model-based design workflow to ensure strict adherence to international safety standards such as **IEC 62304** and **IEC 61508**.

The platform is built on an open-source hardware foundation (**RISC-V**) and utilizes an event-driven, active object framework (**QP/C**) to achieve deterministic behavior and software partitioning.

## 🚀 Key Features
- **Model-Based Development:** Visual state machine design using the QM™ modeling tool.
- **Automatic Code Generation:** Generates MISRA-C compliant code directly from state charts, reducing human implementation errors.
- **Hierarchical State Machines (HSM):** Robust logic for handling complex medical procedures and alarm states.
- **Deterministic Scheduling:** Non-blocking execution context with strict "Freedom from Interference" between software components.
- **Hardware Abstraction:** A well-structured BSP (Board Support Package) for RISC-V architectures.

## 🛠 Tech Stack
- **Hardware:** RISC-V (GD32VF103 / Longan Nano)
- **Framework:** [Quantum Leaps QP/C](https://www.state-machine.com/qpc)
- **Modeling Tool:** QM™ (Quantum Model-based design)
- **Language:** C (MISRA-compliant)
- **Toolchain:** GCC for RISC-V

## 📂 Architecture
The platform is organized into a layered architecture to ensure modularity and ease of certification:
1. **Application Layer:** Medical logic implemented as Active Objects (e.g., Pump Manager, Medication Logic).
2. **Framework Layer:** QP/C Real-Time Embedded Framework (RTEF) handling event queuing and dispatching.
3. **BSP Layer:** Hardware-specific drivers and system clock configuration for RISC-V.

## ⚠️ Safety & Standards
This platform is designed with the following standards in mind:
- **IEC 62304:** Software life cycle processes for medical device software.
- **IEC 61508:** Functional safety of electrical/electronic/programmable electronic safety-related systems.
- **ISO 13485:** Quality management for medical devices (Risk Management & Design Controls).

## 📊 Evaluation & Debugging
Real-time tracing and verification can be performed using **QSPY**, allowing for deep introspection of the event-driven system without affecting the deterministic real-time behavior.

---
*Developed as part of a Thesis Project focused on lowering the threshold for regulatory compliance in medical device development.*
