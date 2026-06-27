# ⚡ Massively Parallel secp256k1 Core Engine (CUDA++)

> **"In Math We Trust."**
> A high-performance, monolithic, branchless implementation of the secp256k1 elliptic curve optimized for massive SIMT execution on modern GPU architectures.

---

## 🏛️ THE HARDWARE PARADOX

Official asymmetric cryptography standards (like the `libsecp256k1` implemented in Bitcoin Core) were engineered in an era dominated by single-core CPUs. When forced into parallel execution across thousands of GPU streaming multiprocessors, these legacy standards collapse due to **Warp Divergence, uncoalesced memory access, and data-dependent pipeline stalls.**

This repository serves as an academic proof-of-concept and private R&D sandbox. It demonstrates how to bypass abstraction layers and force geometric curves directly into binary silicon at maximum hardware saturation.

---

## 🚀 ARCHITECTURAL BREAKTHROUGHS

Unlike legacy implementations (which rely on sprawling 74+ file codebases and deep CPU loops), this core delivers pure hardware dominance via:

1. **0% Warp Divergence (Branchless Execution):** Conditional branches (`if/else`) inside point addition and scalar multiplication are fully eliminated. All threads within a Warp execute the same instruction pipeline synchronously, preventing execution path collapse.
2. **On-Die PTX Assembly:** Big-integer addition and subtraction bypass compiler overhead, direct-mapping 256-bit structures (`u256_t`) into 32-bit registers using inline PTX assembly commands (`add.cc.u32` / `addc.cc.u32`) to exploit carry flags straight on silicon.
3. **Optimized Multi-Scalar Fixed-Window Pipeline:** The private key is atomized into 8-bit chunk windows, executing unrolled `point_add_mixed` cycles against a specialized GPU device table (`G_TABLE`), maximizing ALU saturation per clock cycle.
4. **Delayed Modular Inversion:** The expensive Fermat's Little Theorem inversion (`mod_inv`) is delayed and executed exactly **once** at the very end of the coordinate conversion chain, saving thousands of memory and instruction cycles.
5. **Silicon-Level Entropy:** Features a GPU-native on-die `xorshift32` engine, generating 256-bit keys instantly within individual GPU cores, obliterating the PCIe host-to-device memory bandwidth bottleneck.

---

## 📦 CORE STRUCTURE

The codebase is engineered as a clean, dependency-free monolithic framework:
* **`secp256k1_core.h`**: The architectural mathematical blueprint, containing PTX arithmetic, modular calculations, Jacobian field algebra, and the branchless fixed-window pipeline.
* **`secp256k1_core.cu`**: The main execution engine and entry point for mass verification routing.

---

## ⚙️ COMPILATION & TESTING

To build and compile this hardware-aligned engine, utilize the standard Nvidia CUDA Toolkit compiler (`nvcc`) under Linux:

```bash
# Compile with heavy optimizations targeting your specific GPU architecture
nvcc -O3 -std=c++17 -arch=sm_80 secp256k1_core.cu -o secp256k1_bench
```

---

## ⚠️ NOTICE & LICENSE

This project is licensed under the **GNU Affero General Public License v3 (AGPL-3.0)**. 

* **Strict Copyleft:** Corporate usage, modification, or inclusion into closed enterprise networks/commercial products is explicitly forbidden without 100% public disclosure of your entire surrounding source code. 
* *The mathematical core remains open, free, and protected by the laws of digital human sovereignty.*

---
Developed by **math-core** | June 2026.
