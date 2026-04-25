// Keccak-f[1600] permutation for CryptoNight (WebGPU/WGSL)
// Translated from src/backend/opencl/cl/cn/keccak.cl

const KECCAK_RNDC: array<u64, 24> = array<u64, 24>(
    0x0000000000000001uL, 0x0000000000008082uL, 0x800000000000808auL,
    0x8000000080008000uL, 0x000000000000808buL, 0x0000000080000001uL,
    0x8000000080008081uL, 0x8000000000008009uL, 0x000000000000008auL,
    0x0000000000000088uL, 0x0000000080008009uL, 0x000000008000000auL,
    0x000000008000808buL, 0x800000000000008buL, 0x8000000000008089uL,
    0x8000000000008003uL, 0x8000000000008002uL, 0x8000000000000080uL,
    0x000000000000800auL, 0x800000008000000auL, 0x8000000080008081uL,
    0x8000000000008080uL, 0x0000000080000001uL, 0x8000000080008008uL
);

const KECCAK_ROTC: array<u32, 24> = array<u32, 24>(
    1u,  3u,  6u,  10u, 15u, 21u, 28u, 36u, 45u, 55u, 2u,  14u,
    27u, 41u, 56u, 8u,  25u, 43u, 62u, 18u, 39u, 61u, 20u, 44u
);

const KECCAK_PILN: array<u32, 24> = array<u32, 24>(
    10u, 7u,  11u, 17u, 18u, 3u, 5u,  16u, 8u,  21u, 24u, 4u,
    15u, 23u, 19u, 13u, 12u, 2u, 20u, 14u, 22u, 9u,  6u,  1u
);

fn rotl64(x: u64, n: u32) -> u64 {
    return (x << n) | (x >> (64u - n));
}

fn bitselect64(a: u64, b: u64, c: u64) -> u64 {
    // OpenCL bitselect(a,b,c) = c ? b : a  (actually: (a & ~c) | (b & c))
    // WGSL select(false, true, cond)
    return select(a, b, (c & 1uL) != 0uL);
}

// Keccak-f[1600] on a private u64[25] array (used by cn0 host-side init)
fn keccakf1600_1(st: ptr<function, array<u64, 25>>) {
    var bc: array<u64, 5>;
    var t: u64;

    for (var round: i32 = 0; round < 24; round = round + 1) {
        // Theta
        bc[0] = (*st)[0] ^ (*st)[5] ^ (*st)[10] ^ (*st)[15] ^ (*st)[20];
        bc[1] = (*st)[1] ^ (*st)[6] ^ (*st)[11] ^ (*st)[16] ^ (*st)[21];
        bc[2] = (*st)[2] ^ (*st)[7] ^ (*st)[12] ^ (*st)[17] ^ (*st)[22];
        bc[3] = (*st)[3] ^ (*st)[8] ^ (*st)[13] ^ (*st)[18] ^ (*st)[23];
        bc[4] = (*st)[4] ^ (*st)[9] ^ (*st)[14] ^ (*st)[19] ^ (*st)[24];

        for (var i: i32 = 0; i < 5; i = i + 1) {
            t = bc[(i + 4) % 5] ^ rotl64(bc[(i + 1) % 5], 1u);
            (*st)[i     ] ^= t;
            (*st)[i +  5] ^= t;
            (*st)[i + 10] ^= t;
            (*st)[i + 15] ^= t;
            (*st)[i + 20] ^= t;
        }

        // Rho Pi
        t = (*st)[1];
        for (var i: i32 = 0; i < 24; i = i + 1) {
            bc[0] = (*st)[KECCAK_PILN[i]];
            (*st)[KECCAK_PILN[i]] = rotl64(t, KECCAK_ROTC[i]);
            t = bc[0];
        }

        // Chi
        for (var i: i32 = 0; i < 25; i = i + 5) {
            var tmp: array<u64, 5>;
            for (var x: i32 = 0; x < 5; x = x + 1) {
                tmp[x] = bitselect64((*st)[i + x] ^ (*st)[i + ((x + 2) % 5)], (*st)[i + x], (*st)[i + ((x + 1) % 5)]);
            }
            for (var x: i32 = 0; x < 5; x = x + 1) {
                (*st)[i + x] = tmp[x];
            }
        }

        // Iota
        (*st)[0] ^= KECCAK_RNDC[round];
    }
}

// Keccak-f[1600] on a workgroup-local u64[25] array (used inside cn0/cn2)
// In WGSL we pass the base index into the workgroup array.
fn keccakf1600_2(st_base: u32, state_buf: ptr<workgroup, array<u64, 200>>) {
    var bc: array<u64, 5>;
    var t: u64;

    for (var round: i32 = 0; round < 24; round = round + 1) {
        bc[0] = (*state_buf)[st_base + 0] ^ (*state_buf)[st_base + 5] ^ (*state_buf)[st_base + 10] ^ (*state_buf)[st_base + 15] ^ (*state_buf)[st_base + 20];
        bc[1] = (*state_buf)[st_base + 1] ^ (*state_buf)[st_base + 6] ^ (*state_buf)[st_base + 11] ^ (*state_buf)[st_base + 16] ^ (*state_buf)[st_base + 21];
        bc[2] = (*state_buf)[st_base + 2] ^ (*state_buf)[st_base + 7] ^ (*state_buf)[st_base + 12] ^ (*state_buf)[st_base + 17] ^ (*state_buf)[st_base + 22];
        bc[3] = (*state_buf)[st_base + 3] ^ (*state_buf)[st_base + 8] ^ (*state_buf)[st_base + 13] ^ (*state_buf)[st_base + 18] ^ (*state_buf)[st_base + 23];
        bc[4] = (*state_buf)[st_base + 4] ^ (*state_buf)[st_base + 9] ^ (*state_buf)[st_base + 14] ^ (*state_buf)[st_base + 19] ^ (*state_buf)[st_base + 24];

        (*state_buf)[st_base + 0]  ^= bc[4];
        (*state_buf)[st_base + 5]  ^= bc[4];
        (*state_buf)[st_base + 10] ^= bc[4];
        (*state_buf)[st_base + 15] ^= bc[4];
        (*state_buf)[st_base + 20] ^= bc[4];

        (*state_buf)[st_base + 1]  ^= bc[0];
        (*state_buf)[st_base + 6]  ^= bc[0];
        (*state_buf)[st_base + 11] ^= bc[0];
        (*state_buf)[st_base + 16] ^= bc[0];
        (*state_buf)[st_base + 21] ^= bc[0];

        (*state_buf)[st_base + 2]  ^= bc[1];
        (*state_buf)[st_base + 7]  ^= bc[1];
        (*state_buf)[st_base + 12] ^= bc[1];
        (*state_buf)[st_base + 17] ^= bc[1];
        (*state_buf)[st_base + 22] ^= bc[1];

        (*state_buf)[st_base + 3]  ^= bc[2];
        (*state_buf)[st_base + 8]  ^= bc[2];
        (*state_buf)[st_base + 13] ^= bc[2];
        (*state_buf)[st_base + 18] ^= bc[2];
        (*state_buf)[st_base + 23] ^= bc[2];

        (*state_buf)[st_base + 4]  ^= bc[3];
        (*state_buf)[st_base + 9]  ^= bc[3];
        (*state_buf)[st_base + 14] ^= bc[3];
        (*state_buf)[st_base + 19] ^= bc[3];
        (*state_buf)[st_base + 24] ^= bc[3];

        // Rho Pi
        t = (*state_buf)[st_base + 1];
        for (var i: i32 = 0; i < 24; i = i + 1) {
            bc[0] = (*state_buf)[st_base + KECCAK_PILN[i]];
            (*state_buf)[st_base + KECCAK_PILN[i]] = rotl64(t, KECCAK_ROTC[i]);
            t = bc[0];
        }

        // Chi
        for (var i: i32 = 0; i < 25; i = i + 5) {
            var tmp1: u64 = (*state_buf)[st_base + i];
            var tmp2: u64 = (*state_buf)[st_base + i + 1];

            (*state_buf)[st_base + i]     = bitselect64((*state_buf)[st_base + i]     ^ (*state_buf)[st_base + i + 2], (*state_buf)[st_base + i],     (*state_buf)[st_base + i + 1]);
            (*state_buf)[st_base + i + 1] = bitselect64((*state_buf)[st_base + i + 1] ^ (*state_buf)[st_base + i + 3], (*state_buf)[st_base + i + 1], (*state_buf)[st_base + i + 2]);
            (*state_buf)[st_base + i + 2] = bitselect64((*state_buf)[st_base + i + 2] ^ (*state_buf)[st_base + i + 4], (*state_buf)[st_base + i + 2], (*state_buf)[st_base + i + 3]);
            (*state_buf)[st_base + i + 3] = bitselect64((*state_buf)[st_base + i + 3] ^ tmp1,                         (*state_buf)[st_base + i + 3], (*state_buf)[st_base + i + 4]);
            (*state_buf)[st_base + i + 4] = bitselect64((*state_buf)[st_base + i + 4] ^ tmp2,                         (*state_buf)[st_base + i + 4], tmp1);
        }

        // Iota
        (*state_buf)[st_base + 0] ^= KECCAK_RNDC[round];
    }
}
