// CryptoNight Variant 0 (CN_0) compute shader for WebGPU/WGSL
// Translated from src/backend/opencl/cl/cn/cryptonight.cl
//
// MINIMAL PROOF-OF-CONCEPT — CN_0 only.
// Branch hashes are deferred to host CPU.

// ============================================================================
// Compile-time constants (CN_0)
// ============================================================================
const MEMORY: u32 = 2097152u;       // 2 MB scratchpad
const ITERATIONS: u32 = 524288u;    // 0x80000
const MASK: u32 = 2097136u;         // 0x1FFFF0

// ============================================================================
// Keccak-f[1600] constants
// ============================================================================
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
    1u, 3u, 6u, 10u, 15u, 21u, 28u, 36u, 45u, 55u, 2u, 14u,
    27u, 41u, 56u, 8u, 25u, 43u, 62u, 18u, 39u, 61u, 20u, 44u
);
const KECCAK_PILN: array<u32, 24> = array<u32, 24>(
    10u, 7u, 11u, 17u, 18u, 3u, 5u, 16u, 8u, 21u, 24u, 4u,
    15u, 23u, 19u, 13u, 12u, 2u, 20u, 14u, 22u, 9u, 6u, 1u
);

fn rotl64(x: u64, n: u32) -> u64 {
    return (x << n) | (x >> (64u - n));
}

fn bitselect64(a: u64, b: u64, c: u64) -> u64 {
    return (a & ~c) | (b & c);
}

fn keccakf1600_local(st_base: u32, state_buf: ptr<workgroup, array<u64, 200>>) {
    var bc: array<u64, 5>;
    var t: u64;
    for (var round: i32 = 0; round < 24; round = round + 1) {
        bc[0] = (*state_buf)[st_base + 0] ^ (*state_buf)[st_base + 5] ^ (*state_buf)[st_base + 10] ^ (*state_buf)[st_base + 15] ^ (*state_buf)[st_base + 20];
        bc[1] = (*state_buf)[st_base + 1] ^ (*state_buf)[st_base + 6] ^ (*state_buf)[st_base + 11] ^ (*state_buf)[st_base + 16] ^ (*state_buf)[st_base + 21];
        bc[2] = (*state_buf)[st_base + 2] ^ (*state_buf)[st_base + 7] ^ (*state_buf)[st_base + 12] ^ (*state_buf)[st_base + 17] ^ (*state_buf)[st_base + 22];
        bc[3] = (*state_buf)[st_base + 3] ^ (*state_buf)[st_base + 8] ^ (*state_buf)[st_base + 13] ^ (*state_buf)[st_base + 18] ^ (*state_buf)[st_base + 23];
        bc[4] = (*state_buf)[st_base + 4] ^ (*state_buf)[st_base + 9] ^ (*state_buf)[st_base + 14] ^ (*state_buf)[st_base + 19] ^ (*state_buf)[st_base + 24];

        (*state_buf)[st_base + 0]  ^= bc[4]; (*state_buf)[st_base + 5]  ^= bc[4]; (*state_buf)[st_base + 10] ^= bc[4]; (*state_buf)[st_base + 15] ^= bc[4]; (*state_buf)[st_base + 20] ^= bc[4];
        (*state_buf)[st_base + 1]  ^= bc[0]; (*state_buf)[st_base + 6]  ^= bc[0]; (*state_buf)[st_base + 11] ^= bc[0]; (*state_buf)[st_base + 16] ^= bc[0]; (*state_buf)[st_base + 21] ^= bc[0];
        (*state_buf)[st_base + 2]  ^= bc[1]; (*state_buf)[st_base + 7]  ^= bc[1]; (*state_buf)[st_base + 12] ^= bc[1]; (*state_buf)[st_base + 17] ^= bc[1]; (*state_buf)[st_base + 22] ^= bc[1];
        (*state_buf)[st_base + 3]  ^= bc[2]; (*state_buf)[st_base + 8]  ^= bc[2]; (*state_buf)[st_base + 13] ^= bc[2]; (*state_buf)[st_base + 18] ^= bc[2]; (*state_buf)[st_base + 23] ^= bc[2];
        (*state_buf)[st_base + 4]  ^= bc[3]; (*state_buf)[st_base + 9]  ^= bc[3]; (*state_buf)[st_base + 14] ^= bc[3]; (*state_buf)[st_base + 19] ^= bc[3]; (*state_buf)[st_base + 24] ^= bc[3];

        t = (*state_buf)[st_base + 1];
        for (var i: i32 = 0; i < 24; i = i + 1) {
            bc[0] = (*state_buf)[st_base + KECCAK_PILN[i]];
            (*state_buf)[st_base + KECCAK_PILN[i]] = rotl64(t, KECCAK_ROTC[i]);
            t = bc[0];
        }

        for (var i: i32 = 0; i < 25; i = i + 5) {
            let tmp1: u64 = (*state_buf)[st_base + i];
            let tmp2: u64 = (*state_buf)[st_base + i + 1];
            (*state_buf)[st_base + i]     = bitselect64((*state_buf)[st_base + i]     ^ (*state_buf)[st_base + i + 2], (*state_buf)[st_base + i],     (*state_buf)[st_base + i + 1]);
            (*state_buf)[st_base + i + 1] = bitselect64((*state_buf)[st_base + i + 1] ^ (*state_buf)[st_base + i + 3], (*state_buf)[st_base + i + 1], (*state_buf)[st_base + i + 2]);
            (*state_buf)[st_base + i + 2] = bitselect64((*state_buf)[st_base + i + 2] ^ (*state_buf)[st_base + i + 4], (*state_buf)[st_base + i + 2], (*state_buf)[st_base + i + 3]);
            (*state_buf)[st_base + i + 3] = bitselect64((*state_buf)[st_base + i + 3] ^ tmp1,                         (*state_buf)[st_base + i + 3], (*state_buf)[st_base + i + 4]);
            (*state_buf)[st_base + i + 4] = bitselect64((*state_buf)[st_base + i + 4] ^ tmp2,                         (*state_buf)[st_base + i + 4], tmp1);
        }
        (*state_buf)[st_base + 0] ^= KECCAK_RNDC[round];
    }
}

// ============================================================================
// AES helpers
// ============================================================================
const AES0_C: array<u32, 256> = array<u32, 256>(
    0xA56363C6u, 0x847C7CF8u, 0x997777EEu, 0x8D7B7BF6u, 0x0DF2F2FFu, 0xBD6B6BD6u, 0xB16F6FDEu, 0x54C5C591u,
    0x50303060u, 0x03010102u, 0xA96767CEu, 0x7D2B2B56u, 0x19FEFEE7u, 0x62D7D7B5u, 0xE6ABAB4Du, 0x9A7676ECu,
    0x45CACA8Fu, 0x9D82821Fu, 0x40C9C989u, 0x877D7DFAu, 0x15FAFAEFu, 0xEB5959B2u, 0xC947478Eu, 0x0BF0F0FBu,
    0xECADAD41u, 0x67D4D4B3u, 0xFDA2A25Fu, 0xEAAFAF45u, 0xBF9C9C23u, 0xF7A4A453u, 0x967272E4u, 0x5BC0C09Bu,
    0xC2B7B775u, 0x1CFDFDE1u, 0xAE93933Du, 0x6A26264Cu, 0x5A36366Cu, 0x413F3F7Eu, 0x02F7F7F5u, 0x4FCCCC83u,
    0x5C343468u, 0xF4A5A551u, 0x34E5E5D1u, 0x08F1F1F9u, 0x937171E2u, 0x73D8D8ABu, 0x53313162u, 0x3F15152Au,
    0x0C040408u, 0x52C7C795u, 0x65232346u, 0x5EC3C39Du, 0x28181830u, 0xA1969637u, 0x0F05050Au, 0xB59A9A2Fu,
    0x0907070Eu, 0x36121224u, 0x9B80801Bu, 0x3DE2E2DFu, 0x26EBEBCDu, 0x6927274Eu, 0xCDB2B27Fu, 0x9F7575EAu,
    0x1B090912u, 0x9E83831Du, 0x742C2C58u, 0x2E1A1A34u, 0x2D1B1B36u, 0xB26E6EDCu, 0xEE5A5AB4u, 0xFBA0A05Bu,
    0xF65252A4u, 0x4D3B3B76u, 0x61D6D6B7u, 0xCEB3B37Du, 0x7B292952u, 0x3EE3E3DDu, 0x712F2F5Eu, 0x97848413u,
    0xF55353A6u, 0x68D1D1B9u, 0x00000000u, 0x2CEDEDC1u, 0x60202040u, 0x1FFCFCF3u, 0xC8B1B179u, 0xED5B5BB6u,
    0xBE6A6AD4u, 0x46CBCB8Du, 0xD9BEBE67u, 0x4B393972u, 0xDE4A4A94u, 0xD44C4C98u, 0xE85858B0u, 0x4ACFCF85u,
    0x6BD0D0BBu, 0x2AEFEFC5u, 0xE5AAAA4Fu, 0x16FBFBEDu, 0xC5434386u, 0xD74D4D9Au, 0x55333366u, 0x94858511u,
    0xCF45458Au, 0x10F9F9E9u, 0x06020204u, 0x817F7FFEu, 0xF05050A0u, 0x443C3C78u, 0xBA9F9F25u, 0xE3A8A84Bu,
    0xF35151A2u, 0xFEA3A35Du, 0xC0404080u, 0x8A8F8F05u, 0xAD92923Fu, 0xBC9D9D21u, 0x48383870u, 0x04F5F5F1u,
    0xDFBCBC63u, 0xC1B6B677u, 0x75DADAAFu, 0x63212142u, 0x30101020u, 0x1AFFFFE5u, 0x0EF3F3FDu, 0x6DD2D2BFu,
    0x4CCDCD81u, 0x140C0C18u, 0x35131326u, 0x2FECECC3u, 0xE15F5FBEu, 0xA2979735u, 0xCC444488u, 0x3917172Eu,
    0x57C4C493u, 0xF2A7A755u, 0x827E7EFCu, 0x473D3D7Au, 0xAC6464C8u, 0xE75D5DBAu, 0x2B191932u, 0x957373E6u,
    0xA06060C0u, 0x98818119u, 0xD14F4F9Eu, 0x7FDCDCA3u, 0x66222244u, 0x7E2A2A54u, 0xAB90903Bu, 0x8388880Bu,
    0xCA46468Cu, 0x29EEEEC7u, 0xD3B8B86Bu, 0x3C141428u, 0x79DEDEA7u, 0xE25E5EBCu, 0x1D0B0B16u, 0x76DBDBADu,
    0x3BE0E0DBu, 0x56323264u, 0x4E3A3A74u, 0x1E0A0A14u, 0xDB494992u, 0x0A06060Cu, 0x6C242448u, 0xE45C5CB8u,
    0x5DC2C29Fu, 0x6ED3D3BDu, 0xEFACAC43u, 0xA66262C4u, 0xA8919139u, 0xA4959531u, 0x37E4E4D3u, 0x8B7979F2u,
    0x32E7E7D5u, 0x43C8C88Bu, 0x5937376Eu, 0xB76D6DDAu, 0x8C8D8D01u, 0x64D5D5B1u, 0xD24E4E9Cu, 0xE0A9A949u,
    0xB46C6CD8u, 0xFA5656ACu, 0x07F4F4F3u, 0x25EAEACFu, 0xAF6565CAu, 0x8E7A7AF4u, 0xE9AEAE47u, 0x18080810u,
    0xD5BABA6Fu, 0x887878F0u, 0x6F25254Au, 0x722E2E5Cu, 0x241C1C38u, 0xF1A6A657u, 0xC7B4B473u, 0x51C6C697u,
    0x23E8E8CBu, 0x7CDDDDA1u, 0x9C7474E8u, 0x211F1F3Eu, 0xDD4B4B96u, 0xDCBDBD61u, 0x868B8B0Du, 0x858A8A0Fu,
    0x907070E0u, 0x423E3E7Cu, 0xC4B5B571u, 0xAA6666CCu, 0xD8484890u, 0x05030306u, 0x01F6F6F7u, 0x120E0E1Cu,
    0xA36161C2u, 0x5F35356Au, 0xF95757AEu, 0xD0B9B969u, 0x91868617u, 0x58C1C199u, 0x271D1D3Au, 0xB99E9E27u,
    0x38E1E1D9u, 0x13F8F8EBu, 0xB398982Bu, 0x33111122u, 0xBB6969D2u, 0x70D9D9A9u, 0x898E8E07u, 0xA7949433u,
    0xB69B9B2Du, 0x221E1E3Cu, 0x92878715u, 0x20E9E9C9u, 0x49CECE87u, 0xFF5555AAu, 0x78282850u, 0x7ADFDFA5u,
    0x8F8C8C03u, 0xF8A1A159u, 0x80898909u, 0x170D0D1Au, 0xDABFBF65u, 0x31E6E6D7u, 0xC6424284u, 0xB86868D0u,
    0xC3414182u, 0xB0999929u, 0x772D2D5Au, 0x110F0F1Eu, 0xCBB0B07Bu, 0xFC5454A8u, 0xD6BBBB6Du, 0x3A16162Cu
);

fn byte_val(x: u32, y: u32) -> u32 {
    return (x >> (y * 8u)) & 0xFFu;
}

fn rotl32(x: u32, n: u32) -> u32 {
    return (x << n) | (x >> (32u - n));
}

fn aes_round(
    aes0: ptr<workgroup, array<u32, 256>>,
    aes1: ptr<workgroup, array<u32, 256>>,
    aes2: ptr<workgroup, array<u32, 256>>,
    aes3: ptr<workgroup, array<u32, 256>>,
    X: vec4<u32>,
    key: vec4<u32>
) -> vec4<u32> {
    var k = key;
    k.x ^= (*aes0)[byte_val(X.x, 0)] ^ (*aes1)[byte_val(X.y, 1)] ^ (*aes2)[byte_val(X.z, 2)] ^ (*aes3)[byte_val(X.w, 3)];
    k.y ^= (*aes0)[byte_val(X.y, 0)] ^ (*aes1)[byte_val(X.z, 1)] ^ (*aes2)[byte_val(X.w, 2)] ^ (*aes3)[byte_val(X.x, 3)];
    k.z ^= (*aes0)[byte_val(X.z, 0)] ^ (*aes1)[byte_val(X.w, 1)] ^ (*aes2)[byte_val(X.x, 2)] ^ (*aes3)[byte_val(X.y, 3)];
    k.w ^= (*aes0)[byte_val(X.w, 0)] ^ (*aes1)[byte_val(X.x, 1)] ^ (*aes2)[byte_val(X.y, 2)] ^ (*aes3)[byte_val(X.z, 3)];
    return k;
}

fn aes_round_two_tables(
    aes0: ptr<workgroup, array<u32, 256>>,
    aes1: ptr<workgroup, array<u32, 256>>,
    X: vec4<u32>,
    key: vec4<u32>
) -> vec4<u32> {
    var k = key;
    k.x ^= (*aes0)[byte_val(X.x, 0)] ^ (*aes1)[byte_val(X.y, 1)] ^ rotl32((*aes0)[byte_val(X.z, 2)] ^ (*aes1)[byte_val(X.w, 3)], 16u);
    k.y ^= (*aes0)[byte_val(X.y, 0)] ^ (*aes1)[byte_val(X.z, 1)] ^ rotl32((*aes0)[byte_val(X.w, 2)] ^ (*aes1)[byte_val(X.x, 3)], 16u);
    k.z ^= (*aes0)[byte_val(X.z, 0)] ^ (*aes1)[byte_val(X.w, 1)] ^ rotl32((*aes0)[byte_val(X.x, 2)] ^ (*aes1)[byte_val(X.y, 3)], 16u);
    k.w ^= (*aes0)[byte_val(X.w, 0)] ^ (*aes1)[byte_val(X.x, 1)] ^ rotl32((*aes0)[byte_val(X.y, 2)] ^ (*aes1)[byte_val(X.z, 3)], 16u);
    return k;
}

fn aes_expand_key_256(keybuf: ptr<function, array<u32, 40>>) {
    const sbox: array<u32, 256> = array<u32, 256>(
        0x63u,0x7Cu,0x77u,0x7Bu,0xF2u,0x6Bu,0x6Fu,0xC5u,0x30u,0x01u,0x67u,0x2Bu,0xFEu,0xD7u,0xABu,0x76u,
        0xCAu,0x82u,0xC9u,0x7Du,0xFAu,0x59u,0x47u,0xF0u,0xADu,0xD4u,0xA2u,0xAFu,0x9Cu,0xA4u,0x72u,0xC0u,
        0xB7u,0xFDu,0x93u,0x26u,0x36u,0x3Fu,0xF7u,0xCCu,0x34u,0xA5u,0xE5u,0xF1u,0x71u,0xD8u,0x31u,0x15u,
        0x04u,0xC7u,0x23u,0xC3u,0x18u,0x96u,0x05u,0x9Au,0x07u,0x12u,0x80u,0xE2u,0xEBu,0x27u,0xB2u,0x75u,
        0x09u,0x83u,0x2Cu,0x1Au,0x1Bu,0x6Eu,0x5Au,0xA0u,0x52u,0x3Bu,0xD6u,0xB3u,0x29u,0xE3u,0x2Fu,0x84u,
        0x53u,0xD1u,0x00u,0xEDu,0x20u,0xFCu,0xB1u,0x5Bu,0x6Au,0xCBu,0xBEu,0x39u,0x4Au,0x4Cu,0x58u,0xCFu,
        0xD0u,0xEFu,0xAAu,0xFBu,0x43u,0x4Du,0x33u,0x85u,0x45u,0xF9u,0x02u,0x7Fu,0x50u,0x3Cu,0x9Fu,0xA8u,
        0x51u,0xA3u,0x40u,0x8Fu,0x92u,0x9Du,0x38u,0xF5u,0xBCu,0xB6u,0xDAu,0x21u,0x10u,0xFFu,0xF3u,0xD2u,
        0xCDu,0x0Cu,0x13u,0xECu,0x5Fu,0x97u,0x44u,0x17u,0xC4u,0xA7u,0x7Eu,0x3Du,0x64u,0x5Du,0x19u,0x73u,
        0x60u,0x81u,0x4Fu,0xDCu,0x22u,0x2Au,0x90u,0x88u,0x46u,0xEEu,0xB8u,0x14u,0xDEu,0x5Eu,0x0Bu,0xDBu,
        0xE0u,0x32u,0x3Au,0x0Au,0x49u,0x06u,0x24u,0x5Cu,0xC2u,0xD3u,0xACu,0x62u,0x91u,0x95u,0xE4u,0x79u,
        0xE7u,0xC8u,0x37u,0x6Du,0x8Du,0xD5u,0x4Eu,0xA9u,0x6Cu,0x56u,0xF4u,0xEAu,0x65u,0x7Au,0xAEu,0x08u,
        0xBAu,0x78u,0x25u,0x2Eu,0x1Cu,0xA6u,0xB4u,0xC6u,0xE8u,0xDDu,0x74u,0x1Fu,0x4Bu,0xBDu,0x8Bu,0x8Au,
        0x70u,0x3Eu,0xB5u,0x66u,0x48u,0x03u,0xF6u,0x0Eu,0x61u,0x35u,0x57u,0xB9u,0x86u,0xC1u,0x1Du,0x9Eu,
        0xE1u,0xF8u,0x98u,0x11u,0x69u,0xD9u,0x8Eu,0x94u,0x9Bu,0x1Eu,0x87u,0xE9u,0xCEu,0x55u,0x28u,0xDFu,
        0x8Cu,0xA1u,0x89u,0x0Du,0xBFu,0xE6u,0x42u,0x68u,0x41u,0x99u,0x2Du,0x0Fu,0xB0u,0x54u,0xBBu,0x16u
    );
    const rcon: array<u32, 8> = array<u32, 8>(0x8du, 0x01u, 0x02u, 0x04u, 0x08u, 0x10u, 0x20u, 0x40u);
    var i: u32 = 1u;
    for (var c: u32 = 8u; c < 40u; c = c + 1u) {
        var t: u32 = (*keybuf)[c - 1u];
        if ((c & 7u) == 0u) {
            t = (sbox[(t >> 24u) & 0xFFu] << 24u) | (sbox[(t >> 16u) & 0xFFu] << 16u) | (sbox[(t >> 8u) & 0xFFu] << 8u) | sbox[t & 0xFFu];
            t = rotl32(t, 24u) ^ rcon[i];
            i = i + 1u;
        } else if ((c & 7u) == 4u) {
            t = (sbox[(t >> 24u) & 0xFFu] << 24u) | (sbox[(t >> 16u) & 0xFFu] << 16u) | (sbox[(t >> 8u) & 0xFFu] << 8u) | sbox[t & 0xFFu];
        }
        (*keybuf)[c] = (*keybuf)[c - 8u] ^ t;
    }
}

fn mul_hi_u64(a: u64, b: u64) -> u64 {
    let a_lo: u32 = u32(a & 0xFFFFFFFFuL);
    let a_hi: u32 = u32(a >> 32u);
    let b_lo: u32 = u32(b & 0xFFFFFFFFuL);
    let b_hi: u32 = u32(b >> 32u);

    let p0: u64 = u64(a_lo) * u64(b_lo);
    let p1: u64 = u64(a_lo) * u64(b_hi);
    let p2: u64 = u64(a_hi) * u64(b_lo);
    let p3: u64 = u64(a_hi) * u64(b_hi);

    let mid: u64 = (p0 >> 32u) + (p1 & 0xFFFFFFFFuL) + (p2 & 0xFFFFFFFFuL);
    let hi: u64 = p3 + (p1 >> 32u) + (p2 >> 32u) + (mid >> 32u);
    return hi;
}

// ============================================================================
// Module-scope workgroup memory (shared by all entry points)
// ============================================================================
var<workgroup> wg_aes0: array<u32, 256>;
var<workgroup> wg_aes1: array<u32, 256>;
var<workgroup> wg_aes2: array<u32, 256>;
var<workgroup> wg_aes3: array<u32, 256>;
var<workgroup> wg_state: array<u64, 200>;

// ============================================================================
// Storage buffers
// ============================================================================
@group(0) @binding(0) var<storage, read> input_buf: array<u64>;
@group(0) @binding(1) var<storage, read_write> scratchpad: array<u32>;
@group(0) @binding(2) var<storage, read_write> states: array<u64>;
@group(0) @binding(3) var<storage, read_write> output_buf: array<u32>;

// ============================================================================
// cn0 kernel
// ============================================================================
@compute @workgroup_size(8, 8, 1)
fn cn0(@builtin(global_invocation_id) gid: vec3<u32>,
       @builtin(local_invocation_id) lid: vec3<u32>) {
    var ExpandedKey1: array<u32, 40>;
    var text: vec4<u32>;

    let gIdx: u32 = gid.x;
    let llid: u32 = lid.y * 8u + lid.x;

    for (var i: u32 = llid; i < 256u; i = i + 64u) {
        let tmp: u32 = AES0_C[i];
        wg_aes0[i] = tmp;
        wg_aes1[i] = rotl32(tmp, 8u);
        wg_aes2[i] = rotl32(tmp, 16u);
        wg_aes3[i] = rotl32(tmp, 24u);
    }
    workgroupBarrier();

    if (lid.y == 0u) {
        let st_base: u32 = lid.x * 25u;
        for (var i: u32 = 0u; i < 25u; i = i + 1u) {
            wg_state[st_base + i] = 0uL;
        }

        for (var j: u32 = 0u; j < 17u; j = j + 1u) {
            wg_state[st_base + j] ^= input_buf[j];
        }

        var tmp9: u64 = wg_state[st_base + 9];
        var tmp10: u64 = wg_state[st_base + 10];
        let nonce_low: u32 = gid.x & 0xFFu;
        let nonce_high: u32 = gid.x >> 8u;
        tmp9 = (tmp9 & 0xFFFFFFFF00FFFFFFuL) | (u64(nonce_low) << 24u);
        tmp10 = (tmp10 & 0xFFFFFFFFFF000000uL) | u64(nonce_high);
        wg_state[st_base + 9] = tmp9;
        wg_state[st_base + 10] = tmp10;

        keccakf1600_local(st_base, &wg_state);

        let gstate_base: u32 = gIdx * 25u;
        for (var i: u32 = 0u; i < 25u; i = i + 1u) {
            states[gstate_base + i] = wg_state[st_base + i];
        }
    }
    workgroupBarrier();

    let gstate_base: u32 = gIdx * 25u;
    text = vec4<u32>(u32(states[gstate_base + 4 + lid.y]),
                     u32(states[gstate_base + 4 + lid.y] >> 32u),
                     u32(states[gstate_base + 5 + lid.y]),
                     u32(states[gstate_base + 5 + lid.y] >> 32u));

    for (var i: u32 = 0u; i < 4u; i = i + 1u) {
        ExpandedKey1[i] = u32(states[gstate_base + i]);
        ExpandedKey1[i + 4] = u32(states[gstate_base + i] >> 32u);
    }
    aes_expand_key_256(&ExpandedKey1);

    let sp_base: u32 = gIdx * (MEMORY >> 4u);
    for (var i: u32 = 0u; i < (MEMORY >> 4u); i = i + 8u) {
        for (var j: u32 = 0u; j < 10u; j = j + 1u) {
            let t: vec4<u32> = vec4<u32>(ExpandedKey1[j * 4],
                                          ExpandedKey1[j * 4 + 1],
                                          ExpandedKey1[j * 4 + 2],
                                          ExpandedKey1[j * 4 + 3]);
            text = aes_round(&wg_aes0, &wg_aes1, &wg_aes2, &wg_aes3, text, t);
        }
        let idx: u32 = sp_base + i + lid.y;
        scratchpad[idx * 4 + 0] = text.x;
        scratchpad[idx * 4 + 1] = text.y;
        scratchpad[idx * 4 + 2] = text.z;
        scratchpad[idx * 4 + 3] = text.w;
    }
}

// ============================================================================
// cn1 kernel
// ============================================================================
@compute @workgroup_size(64, 1, 1)
fn cn1(@builtin(global_invocation_id) gid: vec3<u32>,
       @builtin(local_invocation_id) lid: vec3<u32>) {
    var a: array<u64, 2>;
    var b: array<u64, 2>;

    let gIdx: u32 = gid.x;
    let llid: u32 = lid.x;

    for (var i: u32 = llid; i < 256u; i = i + 64u) {
        let tmp: u32 = AES0_C[i];
        wg_aes0[i] = tmp;
        wg_aes1[i] = rotl32(tmp, 8u);
    }
    workgroupBarrier();

    let gstate_base: u32 = gIdx * 25u;
    let sp_base: u32 = gIdx * (MEMORY >> 4u);

    a[0] = states[gstate_base + 0] ^ states[gstate_base + 4];
    b[0] = states[gstate_base + 2] ^ states[gstate_base + 6];
    a[1] = states[gstate_base + 1] ^ states[gstate_base + 5];
    b[1] = states[gstate_base + 3] ^ states[gstate_base + 7];

    var b_x: vec4<u32> = vec4<u32>(u32(b[0]), u32(b[0] >> 32u), u32(b[1]), u32(b[1] >> 32u));
    var idx0: u32 = u32(a[0]);

    for (var i: u32 = 0u; i < ITERATIONS; i = i + 1u) {
        let sp_idx1: u32 = sp_base + ((idx0 & MASK) >> 4u);
        var c: vec4<u32> = vec4<u32>(scratchpad[sp_idx1 * 4 + 0],
                                      scratchpad[sp_idx1 * 4 + 1],
                                      scratchpad[sp_idx1 * 4 + 2],
                                      scratchpad[sp_idx1 * 4 + 3]);

        let a_vec: vec4<u32> = vec4<u32>(u32(a[0]), u32(a[0] >> 32u), u32(a[1]), u32(a[1] >> 32u));
        c = aes_round_two_tables(&wg_aes0, &wg_aes1, c, a_vec);

        b_x ^= c;
        scratchpad[sp_idx1 * 4 + 0] = b_x.x;
        scratchpad[sp_idx1 * 4 + 1] = b_x.y;
        scratchpad[sp_idx1 * 4 + 2] = b_x.z;
        scratchpad[sp_idx1 * 4 + 3] = b_x.w;

        let c0: u64 = u64(c.x) | (u64(c.y) << 32u);
        let sp_idx2: u32 = sp_base + ((u32(c0) & MASK) >> 4u);
        var tmp: vec4<u32> = vec4<u32>(scratchpad[sp_idx2 * 4 + 0],
                                        scratchpad[sp_idx2 * 4 + 1],
                                        scratchpad[sp_idx2 * 4 + 2],
                                        scratchpad[sp_idx2 * 4 + 3]);
        let tmp0: u64 = u64(tmp.x) | (u64(tmp.y) << 32u);

        a[1] += c0 * tmp0;
        a[0] += mul_hi_u64(c0, tmp0);

        let a_vec_new: vec4<u32> = vec4<u32>(u32(a[0]), u32(a[0] >> 32u), u32(a[1]), u32(a[1] >> 32u));
        scratchpad[sp_idx2 * 4 + 0] = a_vec_new.x;
        scratchpad[sp_idx2 * 4 + 1] = a_vec_new.y;
        scratchpad[sp_idx2 * 4 + 2] = a_vec_new.z;
        scratchpad[sp_idx2 * 4 + 3] = a_vec_new.w;

        a[0] ^= tmp0;
        idx0 = u32(a[0]);
        b_x = c;
    }
}

// ============================================================================
// cn2 kernel
// ============================================================================
@compute @workgroup_size(8, 8, 1)
fn cn2(@builtin(global_invocation_id) gid: vec3<u32>,
       @builtin(local_invocation_id) lid: vec3<u32>) {
    var ExpandedKey2: array<u32, 40>;
    var text: vec4<u32>;

    let gIdx: u32 = gid.x;
    let llid: u32 = lid.y * 8u + lid.x;

    for (var i: u32 = llid; i < 256u; i = i + 64u) {
        let tmp: u32 = AES0_C[i];
        wg_aes0[i] = tmp;
        wg_aes1[i] = rotl32(tmp, 8u);
        wg_aes2[i] = rotl32(tmp, 16u);
        wg_aes3[i] = rotl32(tmp, 24u);
    }
    workgroupBarrier();

    let gstate_base: u32 = gIdx * 25u;
    let sp_base: u32 = gIdx * (MEMORY >> 4u);

    text = vec4<u32>(u32(states[gstate_base + 4 + lid.y]),
                     u32(states[gstate_base + 4 + lid.y] >> 32u),
                     u32(states[gstate_base + 5 + lid.y]),
                     u32(states[gstate_base + 5 + lid.y] >> 32u));

    for (var i: u32 = 0u; i < 8u; i = i + 1u) {
        ExpandedKey2[i] = u32(states[gstate_base + i + 1]);
        ExpandedKey2[i + 8] = u32(states[gstate_base + i + 1] >> 32u);
    }
    aes_expand_key_256(&ExpandedKey2);

    workgroupBarrier();

    for (var i: u32 = 0u; i < (MEMORY >> 7u); i = i + 1u) {
        let sp_idx: u32 = sp_base + ((i << 3u) + lid.y);
        let t: vec4<u32> = vec4<u32>(scratchpad[sp_idx * 4 + 0],
                                      scratchpad[sp_idx * 4 + 1],
                                      scratchpad[sp_idx * 4 + 2],
                                      scratchpad[sp_idx * 4 + 3]);
        text ^= t;
        for (var j: u32 = 0u; j < 10u; j = j + 1u) {
            let k: vec4<u32> = vec4<u32>(ExpandedKey2[j * 4],
                                          ExpandedKey2[j * 4 + 1],
                                          ExpandedKey2[j * 4 + 2],
                                          ExpandedKey2[j * 4 + 3]);
            text = aes_round(&wg_aes0, &wg_aes1, &wg_aes2, &wg_aes3, text, k);
        }
    }

    states[gstate_base + 4 + lid.y] = u64(text.x) | (u64(text.y) << 32u);
    states[gstate_base + 5 + lid.y] = u64(text.z) | (u64(text.w) << 32u);

    workgroupBarrier();

    if (lid.y == 0u) {
        let st_base: u32 = lid.x * 25u;
        for (var i: u32 = 0u; i < 25u; i = i + 1u) {
            wg_state[st_base + i] = states[gstate_base + i];
        }
        keccakf1600_local(st_base, &wg_state);
        for (var i: u32 = 0u; i < 25u; i = i + 1u) {
            states[gstate_base + i] = wg_state[st_base + i];
        }

        if (lid.x == 0u) {
            output_buf[gIdx * 2 + 0] = u32(wg_state[st_base + 0]);
            output_buf[gIdx * 2 + 1] = u32(wg_state[st_base + 0] >> 32u);
        }
    }
}
