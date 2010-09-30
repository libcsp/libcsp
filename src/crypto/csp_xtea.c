/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 GomSpace ApS (gomspace.com)
Copyright (C) 2010 AAUSAT3 Project (aausat3.space.aau.dk)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/* Simple implementation of XTEA in CBC mode with zero padding */

#include <stdint.h>
#include <string.h>

/* CSP includes */
#include <csp/csp.h>
#include <csp/csp_config.h>

#include "csp_xtea.h"

#if CSP_ENABLE_XTEA

#define XTEA_BLOCKSIZE 	8
#define XTEA_ROUNDS 	32

/* Helper function for printing */
void print_blocks(uint8_t buf[], uint32_t len) {
    int i;    
    for (i = 0; i < len; i++) {
        printf("%02x", buf[i]);
        if (!((i+1) & 7))
            printf(" ");
    }
    printf("\r\n");
}

/* These functions take 64 bits of data in block[0] and block[1] and the 128 bits key in key[0] - key[3] */
static inline void xtea_encrypt_block(uint32_t block[2], uint32_t const key[4]) {
    uint32_t i, v0 = block[0], v1 = block[1], delta = 0x9E3779B9, sum = 0;
    for (i = 0; i < XTEA_ROUNDS; i++) {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
        sum += delta;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
    }
    block[0] = v0; block[1] = v1;
}
 
static inline void xtea_decrypt_block(uint32_t block[2], uint32_t const key[4]) {
    uint32_t i, v0 = block[0], v1 = block[1], delta = 0x9E3779B9, sum = delta * XTEA_ROUNDS;
    for (i = 0; i < XTEA_ROUNDS; i++) {
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
        sum -= delta;
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
    }
    block[0] = v0; block[1] = v1;
}

/* Encrypt plain text */
int xtea_encrypt(uint8_t * plain, const uint32_t len, uint32_t const key[4], uint32_t const iv[2]) {

    int i;
    uint32_t * p = (uint32_t *)plain;
    uint32_t * bp0, * bp1;
    uint32_t zerobuf[2];

    uint32_t blocks = (len + XTEA_BLOCKSIZE - 1)/ XTEA_BLOCKSIZE;
    uint32_t remain;    

    for (i = 0; i < blocks; i++) {
        /* Calculate remaining bytes */
        remain = len - i * XTEA_BLOCKSIZE;

        /* Zero pad incomplete blocks */
        if (remain < XTEA_BLOCKSIZE) {
            memset((uint8_t *)zerobuf, 0, 2 * sizeof(zerobuf[0]));
            memcpy((uint8_t *)zerobuf, &p[i*2], remain);
            bp0 = &zerobuf[0];
            bp1 = &zerobuf[1];
        } else {
            bp0 = &p[i*2];
            bp1 = &p[i*2+1];
        }

        /* Cipher Block Chaining */
        if (i == 0) {
            *bp0 ^= iv[0];
            *bp1 ^= iv[1];
        } else {
            *bp0 ^= p[i*2-2];
            *bp1 ^= p[i*2-1];
        }

        /* Encrypt Block */
        xtea_encrypt_block(&p[i*2], key);
    }    

    return 0;
}

/* Decrypt cipher text */
int xtea_decrypt(uint8_t * cipher, const uint32_t len, uint32_t const key[4], uint32_t const iv[2]) {

    int i;
    uint32_t * p = (uint32_t *)cipher;
    uint32_t * bp0, * bp1;
    uint32_t zerobuf[2];

    uint32_t blocks = (len + XTEA_BLOCKSIZE - 1)/ XTEA_BLOCKSIZE;
    uint32_t remain;

    for (i = blocks - 1; i >= 0; i--) {
        /* Calculate remaining bytes */
        remain = len - i * XTEA_BLOCKSIZE;

        /* Zero pad incomplete blocks */
        if (remain < XTEA_BLOCKSIZE) {
            memset((uint8_t *)zerobuf, 0, 2 * sizeof(zerobuf[0]));
            memcpy((uint8_t *)zerobuf, &p[i*2], remain);
            bp0 = &zerobuf[0];
            bp1 = &zerobuf[1];
        } else {
            bp0 = &p[i*2];
            bp1 = &p[i*2+1];
        }

        /* Decrypt block */
        xtea_decrypt_block(&p[i*2], key);
        
        /* Cipher Block Chaining */
        if (i == 0) {
            *bp0 ^= iv[0];
            *bp1 ^= iv[1];
        } else {
            *bp0 ^= p[i*2-2];
            *bp1 ^= p[i*2-1];
        }
    }
    
    return 0;
}

#if 0 // This needs an implementation of RAND_bytes to work
int xtea_test(int argc, char **argv) {
    /* Key and initialization vector */
    uint32_t key[4];
    uint32_t iv[2];

    printf("XTEA encryption/decryption test (CBC mode with zero padding)\r\n");

    /* Create random key and IV */
    if (RAND_bytes((unsigned char *)key, 4 * sizeof(uint32_t)) != 1) {
        printf("Failed to create random key\r\n");
        exit(EXIT_FAILURE);
    }
    if (RAND_bytes((unsigned char *)iv, 2 * sizeof(uint32_t)) != 1) {
        printf("Failed to create random IV\r\n");
        exit(EXIT_FAILURE);
    }

    printf("KEY: %08x%08x%08x%08x\r\n", key[0], key[1], key[2], key[3]);
    printf("IV : %08x%08x\r\n", iv[0], iv[1]);

    /* Create data buffer with random data */
    uint8_t enc_data[DATASIZ] __attribute__ ((aligned(4)));
    uint8_t dec_data[DATASIZ] __attribute__ ((aligned(4)));

    if (RAND_bytes((unsigned char *)enc_data, DATASIZ) != 1) {
        printf("Failed to randomize buffer\r\n");
        exit(EXIT_FAILURE);
    }
    memcpy(&dec_data[0], &enc_data[0], DATASIZ);

    printf("ORG: ");
    print_blocks(enc_data, DATASIZ);
    printf("CPY: ");
    print_blocks(dec_data, DATASIZ);

    /* Encrypt data */
    xtea_encrypt(enc_data, DATASIZ, key, iv);
    printf("ENC: ");
    print_blocks(enc_data, DATASIZ);

    /* Decrypt data */
    xtea_decrypt(enc_data, DATASIZ, key, iv);
    printf("DEC: ");
    print_blocks(enc_data, DATASIZ);

    if (memcmp(enc_data, dec_data, DATASIZ)) {
        printf("Failed to properly decrypt data\r\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Successfully decrypted data\r\n");
        exit(EXIT_SUCCESS);
    }
}
#endif // 0

#endif // CSP_ENABLE_XTEA
