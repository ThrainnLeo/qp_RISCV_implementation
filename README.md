# Medical Device Software Platform (RISC-V + QP/C + QM)

![Platform: RISC-V](https://img.shields.io/badge/Platform-RISC--V-orange)
![Framework: QP/C](https://img.shields.io/badge/Framework-QP%2FC-blue)
![Standard: IEC 62304](https://img.shields.io/badge/Standard-IEC%2062304-green)
![Standard: ISO 13485](https://img.shields.io/badge/Standard-ISO%2013485-blue)
![Standard: ISO 14971](https://img.shields.io/badge/Standard-ISO%2014971-red)
![Standard: IEC 61508](https://img.shields.io/badge/Standard-IEC%2061508-lightgrey)

## 📌 Overview
This project demonstrates a modern, safety-critical software platform designed for medical devices. By combining a model-based design workflow with an open-source hardware foundation, the platform lowers the threshold for regulatory compliance while ensuring high software quality and patient safety.

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
1. **Application Layer:** Encapsulated Active Objects (e.g., Pump Manager, Alarm System).
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
