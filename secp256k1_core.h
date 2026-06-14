#ifndef MATH_CORE_SECP256K1_H
#define MATH_CORE_SECP256K1_H

#include <stdint.h>

// =====================================================================
// --- ARCHITECTURAL TYPES & MEMORY ALIGNMENT ---
// =====================================================================
typedef struct __align__(32) { uint32_t v[8]; } u256_t;
typedef struct { u256_t x; u256_t y; } point_affine_t;
typedef struct { u256_t x; u256_t y; u256_t z; } point_jacobian_t;

// =====================================================================
// --- SECP256K1 DOMAIN PARAMETERS ---
// Allocated in __constant__ memory for optimal L1 cache broadcast
// =====================================================================
__constant__ uint32_t SECP256K1_P[8] = {
    0xFFFFFC2F, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
};

__constant__ uint32_t SECP256K1_P_MINUS_2[8] = {
    0xFFFFFC2D, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
};

// Generator Point G (Base Point - Configured for init tests)
__constant__ uint32_t SECP256K1_G_X[8] = {
    0x16F81798, 0x59F2815B, 0x2DCE28D9, 0x029BFCDB,
    0xCE870B07, 0x55A06295, 0xF9DCBBAC, 0x79BE667E
};

__constant__ uint32_t SECP256K1_G_Y[8] = {
    0xFB10D4B8, 0x9C47D08F, 0xA6855419, 0xFD17B448,
    0x0E1108A8, 0x5DA4FBFC, 0x26A3C465, 0x483ADA77
};

// Precomputed State Table: 9 windows x 256 sub-points
__device__ point_affine_t G_TABLE[9][256];

// =====================================================================
// --- LOW-LEVEL PTX ARITHMETIC ---
// =====================================================================
__device__ __forceinline__ int u256_cmp(const u256_t *a, const u256_t *b) {
    #pragma unroll
    for (int i = 7; i >= 0; i--) {
        if (a->v[i] > b->v[i]) return 1;
        if (a->v[i] < b->v[i]) return -1;
    }
    return 0;
}

__device__ __forceinline__ void u256_add(u256_t *r, const u256_t *a, const u256_t *b) {
    asm volatile(
        "add.cc.u32      %0, %8, %16;\n\t"
        "addc.cc.u32     %1, %9, %17;\n\t"
        "addc.cc.u32     %2, %10, %18;\n\t"
        "addc.cc.u32     %3, %11, %19;\n\t"
        "addc.cc.u32     %4, %12, %20;\n\t"
        "addc.cc.u32     %5, %13, %21;\n\t"
        "addc.cc.u32     %6, %14, %22;\n\t"
        "addc.u32        %7, %15, %23;\n\t"
        : "=r"(r->v[0]), "=r"(r->v[1]), "=r"(r->v[2]), "=r"(r->v[3]),
          "=r"(r->v[4]), "=r"(r->v[5]), "=r"(r->v[6]), "=r"(r->v[7])
        : "r"(a->v[0]), "r"(a->v[1]), "r"(a->v[2]), "r"(a->v[3]),
          "r"(a->v[4]), "r"(a->v[5]), "r"(a->v[6]), "r"(a->v[7]),
          "r"(b->v[0]), "r"(b->v[1]), "r"(b->v[2]), "r"(b->v[3]),
          "r"(b->v[4]), "r"(b->v[5]), "r"(b->v[6]), "r"(b->v[7])
    );
}

__device__ __forceinline__ void u256_sub(u256_t *r, const u256_t *a, const u256_t *b) {
    asm volatile(
        "sub.cc.u32      %0, %8, %16;\n\t"
        "subc.cc.u32     %1, %9, %17;\n\t"
        "subc.cc.u32     %2, %10, %18;\n\t"
        "subc.cc.u32     %3, %11, %19;\n\t"
        "subc.cc.u32     %4, %12, %20;\n\t"
        "subc.cc.u32     %5, %13, %21;\n\t"
        "subc.cc.u32     %6, %14, %22;\n\t"
        "subc.u32        %7, %15, %23;\n\t"
        : "=r"(r->v[0]), "=r"(r->v[1]), "=r"(r->v[2]), "=r"(r->v[3]),
          "=r"(r->v[4]), "=r"(r->v[5]), "=r"(r->v[6]), "=r"(r->v[7])
        : "r"(a->v[0]), "r"(a->v[1]), "r"(a->v[2]), "r"(a->v[3]),
          "r"(a->v[4]), "r"(a->v[5]), "r"(a->v[6]), "r"(a->v[7]),
          "r"(b->v[0]), "r"(b->v[1]), "r"(b->v[2]), "r"(b->v[3]),
          "r"(b->v[4]), "r"(b->v[5]), "r"(b->v[6]), "r"(b->v[7])
    );
}

__device__ __forceinline__ void u256_shl1(u256_t *r, const u256_t *a) {
    uint32_t carry = 0;
    #pragma unroll
    for (int i = 0; i < 8; i++) {
        uint32_t next_carry = a->v[i] >> 31;
        r->v[i] = (a->v[i] << 1) | carry;
        carry = next_carry;
    }
}

__device__ __forceinline__ void mod_add(u256_t *r, const u256_t *a, const u256_t *b) {
    u256_t temp; 
    u256_add(&temp, a, b);
    if (u256_cmp(&temp, (const u256_t*)SECP256K1_P) >= 0) {
        u256_sub(r, &temp, (const u256_t*)SECP256K1_P);
    } else {
        *r = temp;
    }
}

__device__ __forceinline__ void mod_sub(u256_t *r, const u256_t *a, const u256_t *b) {
    if (u256_cmp(a, b) < 0) {
        u256_t temp; 
        u256_add(&temp, a, (const u256_t*)SECP256K1_P); 
        u256_sub(r, &temp, b);
    } else {
        u256_sub(r, a, b);
    }
}

__device__ void mod_mul(u256_t *r, const u256_t *a, const u256_t *b) {
    u256_t res = {0}; 
    u256_t temp = *a;
    int max_bit = 255;
    
    while (max_bit >= 0 && ((b->v[max_bit / 32] >> (max_bit % 32)) & 1) == 0) {
        max_bit--;
    }
    
    for (int i = 0; i <= max_bit; i++) {
        if ((b->v[i / 32] >> (i % 32)) & 1) mod_add(&res, &res, &temp);
        if (i < max_bit) {
            u256_shl1(&temp, &temp);
            if (u256_cmp(&temp, (const u256_t*)SECP256K1_P) >= 0) {
                u256_sub(&temp, &temp, (const u256_t*)SECP256K1_P);
            }
        }
    }
    *r = res;
}

__device__ void mod_inv(u256_t *r, const u256_t *a) {
    u256_t res = {1,0,0,0,0,0,0,0}; 
    u256_t base = *a;
    
    // Fermat's Little Theorem inversion implementation
    #pragma unroll 4
    for (int i = 0; i < 256; i++) {
        if ((SECP256K1_P_MINUS_2[i / 32] >> (i % 32)) & 1) {
            mod_mul(&res, &res, &base);
        }
        mod_mul(&base, &base, &base);
    }
    *r = res;
}

// =====================================================================
// --- JACOBIAN ALGEBRA ---
// =====================================================================
__device__ void point_add_mixed(point_jacobian_t *r, const point_jacobian_t *p1, const point_affine_t *p2) {
    u256_t z1z1, u2, s2, h, i, j, r_val, v, tmp;

    mod_mul(&z1z1, &p1->z, &p1->z);          
    mod_mul(&u2, &p2->x, &z1z1);             
    mod_mul(&tmp, &p1->z, &z1z1);
    mod_mul(&s2, &p2->y, &tmp);              
    
    mod_sub(&h, &u2, &p1->x);                
    mod_sub(&r_val, &s2, &p1->y);            
    
    mod_mul(&i, &h, &h);                     
    mod_mul(&j, &h, &i);                     
    mod_mul(&v, &p1->x, &i);                 
    
    mod_mul(&tmp, &r_val, &r_val);           
    mod_sub(&tmp, &tmp, &j);                 
    mod_sub(&tmp, &tmp, &v);
    mod_sub(&r->x, &tmp, &v);                
    
    mod_sub(&tmp, &v, &r->x);
    mod_mul(&tmp, &tmp, &r_val);
    mod_mul(&i, &p1->y, &j);
    mod_sub(&r->y, &tmp, &i);                
    
    mod_add(&tmp, &p1->z, &h);
    mod_mul(&tmp, &tmp, &tmp);
    mod_sub(&tmp, &tmp, &z1z1);
    mod_sub(&r->z, &tmp, &i);                
}

// =====================================================================
// --- BRANCHLESS SCALAR MULTIPLICATION PIPELINE ---
// =====================================================================
__device__ void secp256k1_mul(point_affine_t *pubKey, const u256_t *privKey) {
    point_jacobian_t res;
    
    uint8_t b[9];
    b[0] = (uint8_t)(privKey->v[0] & 0xFF);
    b[1] = (uint8_t)((privKey->v[0] >> 8) & 0xFF);
    b[2] = (uint8_t)((privKey->v[0] >> 16) & 0xFF);
    b[3] = (uint8_t)((privKey->v[0] >> 24) & 0xFF);
    b[4] = (uint8_t)(privKey->v[1] & 0xFF);
    b[5] = (uint8_t)((privKey->v[1] >> 8) & 0xFF);
    b[6] = (uint8_t)((privKey->v[1] >> 16) & 0xFF);
    b[7] = (uint8_t)((privKey->v[1] >> 24) & 0xFF);
    b[8] = (uint8_t)(privKey->v[2] & 0x7F);

    res.x = G_TABLE[0][b[0]].x;
    res.y = G_TABLE[0][b[0]].y;
    res.z.v[0] = 1; 
    
    #pragma unroll
    for(int i = 1; i < 8; i++) res.z.v[i] = 0;

    // Fully unrolled architectural execution
    point_add_mixed(&res, &res, &G_TABLE[1][b[1]]);
    point_add_mixed(&res, &res, &G_TABLE[2][b[2]]);
    point_add_mixed(&res, &res, &G_TABLE[3][b[3]]);
    point_add_mixed(&res, &res, &G_TABLE[4][b[4]]);
    point_add_mixed(&res, &res, &G_TABLE[5][b[5]]);
    point_add_mixed(&res, &res, &G_TABLE[6][b[6]]);
    point_add_mixed(&res, &res, &G_TABLE[7][b[7]]);
    point_add_mixed(&res, &res, &G_TABLE[8][b[8]]);

    // Jacobian to Affine coordinate conversion
    u256_t z_inv, z_inv2, z_inv3;
    mod_inv(&z_inv, &res.z);
    mod_mul(&z_inv2, &z_inv, &z_inv);
    mod_mul(&z_inv3, &z_inv2, &z_inv);
    
    mod_mul(&pubKey->x, &res.x, &z_inv2);
    mod_mul(&pubKey->y, &res.y, &z_inv3);
}

// =====================================================================
// --- PIPELINE VALIDATION MODULE ---
// =====================================================================
__device__ bool check_target_address(const point_affine_t *pubKey, uint32_t target_x_part) {
    return (pubKey->x.v[0] == target_x_part);
}

#endif // MATH_CORE_SECP256K1_H