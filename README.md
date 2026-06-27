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

## ⚙️ COMPILATION & HARDWARE TARGETING

Since this engine bypasses virtual abstraction layers, it must be compiled with strict alignment to your physical GPU architecture. Compiling with an incorrect Compute Capability (`sm_XX`) will break the inline PTX assembly pipeline or trigger fallback JIT compilation.

### 1. Identify Your Hardware Target

| NVIDIA GPU Generation | Architecture Microname | Compute Capability Flag |
| :--- | :--- | :--- |
| **Blackwell (Consumer)** (RTX 50xx Series) | `Blackwell` | **`-arch=sm_120`** |
| **Blackwell (Data Center)** (GB200) | `Blackwell` | `-arch=sm_100` / `-arch=sm_101` |
| **Hopper** (H100, H200) | `Hopper` | `-arch=sm_90` |
| **Ada Lovelace** (RTX 40xx) | `Ada` | `-arch=sm_89` |
| **Ampere** (RTX 30xx, A100) | `Ampere` | `-arch=sm_80` / `-arch=sm_86` |

> ⚠️ **Note for cutting-edge deployments:** Modern consumer Blackwell chips (including mobile and desktop RTX 50-series) utilize **Compute Capability 12.0**. Compiling specifically with `-arch=sm_120` unlocks specialized hardware paths natively.

### 2. Production Build Pipeline

To compile the core with aggressive loop unrolling, maximum register allocation, and advanced C++20/C++26 metaprogramming under CUDA 12.8+, execute the following build command:

```bash
nvcc -O3 -std=c++20 \
  -arch=sm_120 \
  --use_fast_math \
  -Xptxas -v \
  -Xptxas --maxrregcount=64 \
  secp256k1_core.cu -o secp256k1_bench
  
```
  ### 3. Deconstructing the Compiler Flags:

* **`-O3`**: Enables maximum host and device code optimization layers.
* **`-std=c++20`**: Unlocks modern syntax and concepts required for clean CUDA/C++ integration.
* **`--use_fast_math`**: Forces the compiler to use high-throughput hardware intrinsics for algebraic approximations, reducing raw instruction cycles.
* **`-Xptxas -v`**: Forces the PTX assembler to print verbose resource usage per thread (registers count and memory memory spills).
* **`-Xptxas --maxrregcount=64`**: Sets a hard ceiling on register allocation per thread to maximize warp occupancy and prevent context-switching bottlenecks.

---

## ⚠️ NOTICE & LICENSE

This project is licensed under the **GNU Affero General Public License v3 (AGPL-3.0)**. 

* **Strict Copyleft:** Corporate usage, modification, or inclusion into closed enterprise networks/commercial products is explicitly forbidden without 100% public disclosure of your entire surrounding source code. 
* *The mathematical core remains open, free, and protected by the laws of digital human sovereignty.*

---
Developed by **math-core** | June 2026.