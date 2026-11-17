<div align="center">

# Medical Device Software Platform
### RISC-V + QP/C + QM

*A platform to simplify medical device software development, built from an event-driven framework, model-based code generation and an open RISC-V microcontroller.*

![Platform](https://img.shields.io/badge/Platform-RISC--V-orange)
![MCU](https://img.shields.io/badge/MCU-GD32VF103-orange)
![Framework](https://img.shields.io/badge/Framework-QP%2FC%208.x-blue)
![Modeler](https://img.shields.io/badge/Modeler-QM%207.0.3-blue)
![Kernel](https://img.shields.io/badge/Kernel-QV-lightblue)
![Language](https://img.shields.io/badge/Code-MISRA--C-lightgrey)
<br>
![IEC 62304](https://img.shields.io/badge/Standard-IEC%2062304-green)
![ISO 13485](https://img.shields.io/badge/Standard-ISO%2013485-blue)
![ISO 14971](https://img.shields.io/badge/Standard-ISO%2014971-red)
![IEC 61508](https://img.shields.io/badge/Standard-IEC%2061508-grey)

**Bachelor's thesis in Medical Engineering (15 credits) · KTH Royal Institute of Technology · 2026**
Thrainn Leo Thorisson & Hugo Persson

📄 [Read the thesis (Swedish, PDF)](Preliminary%20report/Examensarbete.pdf)

</div>

---

## 📖 Table of Contents
- [About the project](#-about-the-project)
- [Platform architecture](#-platform-architecture)
- [Why these tools?](#-why-these-tools)
- [Repository structure](#-repository-structure)
- [Demo application: Infusion pump](#-demo-application-infusion-pump)
- [Porting QP/C to RISC-V](#-porting-qpc-to-risc-v)
- [Working towards standards](#-working-towards-standards)
- [Getting started](#-getting-started)
- [Future work](#-future-work)
- [Authors & acknowledgements](#-authors--acknowledgements)
- [License](#-license)

---

## 🩺 About the project

Medical device software has to meet strict regulatory requirements and international standards before it can reach the market. Prototypes are tested, documented and validated over and over, which makes development slow and expensive.

This thesis asks one question:

> **Can a platform be created that makes medical device development easier?**

Our answer is a platform that combines:

| Component | Role |
|---|---|
| **QP/C** | Real-time event framework (RTEF) built on *active objects* |
| **QV** | QP's built-in cooperative, run-to-completion kernel |
| **QM** | Graphical modeler for hierarchical state machines, with automatic C code generation |
| **RISC-V (GD32VF103)** | Open, license-free ISA running bare-metal (no OS) |

As a proof of concept we built a **simulated infusion pump** on the platform, modeled entirely in QM and running on real RISC-V hardware.

---

## 🏗 Platform architecture

```
┌──────────────────────────────────────────────────────────┐
│                QM  (graphical state machines)            │
│        InfusionPump.qm  ──► auto-generated MISRA-C       │
└───────────────────────────┬──────────────────────────────┘
                            │ code generation
┌───────────────────────────▼──────────────────────────────┐
│   Application: Active objects                            │
│   ┌────────────┐  START/STOP_MED  ┌──────────────────┐   │
│   │  PumpMgr   │ ───────────────► │ Medicine[0..2]   │   │
│   └────────────┘   ALARM (pub/sub)└──────────────────┘   │
├──────────────────────────────────────────────────────────┤
│   QP/C framework  +  QV kernel (run-to-completion)       │
├──────────────────────────────────────────────────────────┤
│   BSP (bsp.c): GPIO, machine timer, ECLIC, Q_onError     │
├──────────────────────────────────────────────────────────┤
│   RISC-V GD32VF103 (Nuclei N200 core), bare-metal        │
└──────────────────────────────────────────────────────────┘
```

**Hardware independence:** All hardware-specific code lives in the Board Support Package. Moving to another microcontroller only requires rewriting the BSP. The models and the generated application code stay unchanged.

---

## 🤔 Why these tools?

<details>
<summary><b>QP/C instead of a traditional RTOS</b></summary>

In a traditional RTOS, threads share global memory and rely on mutexes, blocking and delays for synchronization. That opens the door to **race conditions** and **deadlocks**.

In QP/C each *active object* owns its data and talks to others only through asynchronous events. Combined with the QV kernel's **run-to-completion** scheduling, an event is never interrupted by another event in the same context, so:

- no shared resources between objects
- no mutexes, no blocking
- no race conditions or deadlocks by design

We chose the license-free QP/C over the commercial SafeQP/C.
</details>

<details>
<summary><b>QM for modeling and code generation</b></summary>

- Hierarchical (nested) states mean less repeated logic and more readable models
- Generates **MISRA-C compliant** code in a fraction of a second
- To change behavior, edit the model and regenerate the code, with no hand-editing of complex code
- Less manual programming, so fewer human errors
</details>

<details>
<summary><b>The QV kernel</b></summary>

QV is simple to run on an OS-less system. In Quantum Leaps' own benchmarks against µC/OS-II, it also showed short **interrupt latency, response time and recovery time**. That matters in safety-critical systems, which must be able to reach a safe state quickly.
</details>

<details>
<summary><b>RISC-V</b></summary>

Unlike closed architectures such as ARM Cortex-M, RISC-V is an **open, license-free ISA**. The GD32VF103 was chosen for its open ISA and low complexity rather than raw performance. That gave a controlled environment for porting QP/C.
</details>

---

## 📁 Repository structure

```
qp_RISCV_implementation/
├── InfusionsPump/          # Demo application: simulated infusion pump
│   ├── qm/                 # QM model files (InfusionPump.qm)
│   ├── qpc/                # QP/C framework sources + RISC-V port
│   ├── app.h               # Signals, events and active object declarations
│   ├── bsp.c / bsp.h       # Board Support Package for GD32VF103
│   ├── main.c              # Entry point: init and start active objects
│   ├── pumpMgr.c           # QM-generated: PumpMgr active object
│   ├── medicine.c          # QM-generated: Medicine active objects
│   └── Makefile
│
├── QProjects/              # First port: the "Blinky" example on RISC-V
│   ├── bsp/                # BSP for Blinky
│   ├── qpc/                # QP/C framework
│   ├── Blinky.c            # Two-state LED blinker (on/off)
│   ├── app.h
│   ├── main.c
│   └── Makefile
│
├── Preliminary report/
│   └── Examensarbete.pdf   # The thesis (Swedish, English abstract)
│
└── README.md
```

The platform was built in two steps:
1. **Blinky** (`QProjects/`): Quantum Leaps' simple two-state example, ported to RISC-V to bring up the framework, timer and BSP.
2. **Infusion pump** (`InfusionsPump/`): a full application that shows the whole platform workflow.

---

## 💉 Demo application: Infusion pump

The demo simulates an infusion pump with **three medicine dosing channels** and a **safety alarm**.

### Hardware mapping

| I/O | Pin | Function |
|---|---|---|
| Switch 1 | `PA8` | Start/stop medicine 0 |
| Switch 2 | `PA7` | Start/stop medicine 1 |
| Switch 3 | `PA6` | Start/stop medicine 2 |
| Switch 4 | `PA5` | Trigger/clear alarm |
| LED 0 | `PB0` | Medicine 0 dosing |
| LED 1 | `PB1` | Medicine 1 dosing |
| LED 2 | `PB2` | Medicine 2 dosing |

Switches are configured as inputs with internal pull-ups. LEDs are push-pull outputs. If `Q_onError` fires, it lights LEDs B0 and B1 to indicate a fatal error.

### Active objects

**`PumpMgr`** is the pump's control system. It receives switch events from the BSP and forwards them to the right medicine.

```
active
├── operational   ── START_MED → post to Medicine[id]
│                 ── ALARM     → publish ALARM, go to alarm_state
└── alarm_state   ── entry: periodic alarm tick to running medicines
                  ── RESUME    → publish RESUME, restart medicines that were running
(active handles STOP_MED and DOSE_FINISHED in both substates)
```

**`Medicine[0..2]`** are three identical active objects, each driving one LED.

```
operational
├── off      ── START_MED → dosing
└── dosing   ── STOP_MED  → off
alarm        (entered on ALARM, which every Medicine subscribes to)
```

### Event flow

1. Flipping switch 1–3 makes the BSP post `START_MED_SIG` / `STOP_MED_SIG` with a `medId` to `PumpMgr`.
2. `PumpMgr` forwards the event to `Medicine[medId]`, which moves from *off* to *dosing*.
3. Flipping switch 4 makes `PumpMgr` **publish** `ALARM_SIG`. Every subscribed medicine stops dosing at once. This simulates a detected system fault and a transition to a safe state.
4. Switching the alarm off publishes `RESUME_SIG`, and the medicines that were running go back to dosing.

---

## ⚙️ Porting QP/C to RISC-V

The QP/C port configuration (`qp_port.h`) handles the framework internals. The BSP connects it to the GD32VF103 hardware:

- **`BSP_init()`** enables GPIO clocks and configures the LEDs and switches
- **`QF_onStartup()`** enables the machine timer interrupt in the **ECLIC** and schedules the first tick
- **`eclic_mtip_handler()`** is the 100 Hz system tick. It reschedules itself, drives QP time events and reads the switches with edge detection
- **`QV_onIdle()`** re-enables interrupts while idle
- **`Q_onError()`** is the global fault handler: disable interrupts, signal on LEDs, freeze in a safe state

**Tick calculation** using the core's 64-bit `mtime` / `mtimecmp` registers:

```
TIMER_FREQ = 108 MHz / 4         = 27 000 000 Hz
tick_step  = TIMER_FREQ / 100 Hz = 270 000 cycles per tick
mtimecmp   = mtime + tick_step     (each interrupt)
```

```c
void eclic_mtip_handler(void) {
    uint32_t tick_step = TIMER_FREQ / BSP_TICKS_PER_SEC;
    uint64_t next_tick = *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIME);
    *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP) = next_tick + tick_step;

    QTIMEEVT_TICK_X(0U, (void *)0);   // drive QP time events
    /* ... switch edge detection and event posting ... */
}
```

---

## 🛡 Working towards standards

> ⚠️ **Note:** QP/C is a *Software Element out of Context* (SEooC). The framework supports standards work, but it **cannot by itself guarantee** that an end product complies. This repository is an academic proof of concept and **not a certified medical device**.

| Platform feature | Supports |
|---|---|
| Active objects: strict software partitioning and encapsulation | **IEC 62304**: modular software architecture · **ISO 26262**: Freedom From Interference |
| Publish/subscribe alarm: every unit reacts at once and predictably | Deterministic, safe-state behavior |
| `Q_onError` global fault handler: interrupts off, system frozen in a safe state | **IEC 61508**: functional safety |
| Run-to-completion scheduling with no shared resources | Removes race conditions and deadlocks |
| QM-generated MISRA-C code | Less manual coding, fewer systematic errors |

---

## 🚀 Getting started

### Prerequisites
- A **GD32VF103**-based development board, 4 switches and 3 LEDs (or use the onboard ones)
- RISC-V GCC toolchain with the GD32VF103 / Nuclei SDK hardware libraries
- [QM](https://www.state-machine.com/products/qm) 7.x (to open and edit the `.qm` models)
- [QP/C](https://www.state-machine.com/products/qp) 8.x (included in the `qpc/` folders)

### Build and flash

```bash
git clone https://github.com/HugoPersson01/qp_RISCV_implementation.git
cd qp_RISCV_implementation/InfusionsPump
make
```

<!-- TODO: add the exact toolchain prefix, SDK path and flashing command (e.g. OpenOCD / dfu-util) used in the project -->

### Edit the model
1. Open `InfusionsPump/qm/InfusionPump.qm` in QM.
2. Change the state machines graphically.
3. Click **Generate Code**. `pumpMgr.c` and `medicine.c` are regenerated.
4. Rebuild and flash.

> Do not edit the generated `.c` files by hand. QM overwrites them on the next code generation.

---

## 🔭 Future work

- **SafeQP/C**: move to Quantum Leaps' safety-certified framework, which comes with ready-made certification artifacts. It shares the QP/C API, so existing code needs no changes.
- **QP/Spy**: add real-time software tracing to improve verification, debugging and documentation.

---

## 👥 Authors & acknowledgements

- **Thrainn Leo Thorisson**
- **Hugo Persson** ([@HugoPersson01](https://github.com/HugoPersson01))

Supervisor: **Olof Forsberg** (KTH) · Examiner: **Dmitry Grishenkov**
KTH Royal Institute of Technology, School of Engineering Sciences in Chemistry, Biotechnology and Health · TRITA-CBH-GRU-2026:308

Framework and tools by [Quantum Leaps](https://www.state-machine.com/).

---

## 📜 License

QP/C and the code generated by QM are licensed by Quantum Leaps under **GPL-3.0-or-later**. Closed-source or commercial use requires a commercial license from [Quantum Leaps](https://www.state-machine.com/licensing).
