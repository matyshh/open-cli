#ifndef OPENCLI_CRYPTO_UTILS_H
#define OPENCLI_CRYPTO_UTILS_H

#include <stdint.h>
#include <stdbool.h>

#define SHA256_BLOCK_SIZE 32
#define SHA256_DIGEST_LENGTH 32

typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
} SHA256_CTX;

/**
 * Initialize SHA256 context
 */
void sha256_init(SHA256_CTX *ctx);

/**
 * Update SHA256 context with new data
 */
void sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len);

/**
 * Finalize SHA256 hash and output digest
 */
void sha256_final(SHA256_CTX *ctx, uint8_t hash[]);

/**
 * Calculate SHA256 hash of file
 */
bool calculate_file_sha256(const char *filepath, uint8_t hash[SHA256_DIGEST_LENGTH]);

/**
 * Verify file against expected SHA256 hash
 */
bool verify_file_sha256(const char *filepath, const char *expected_hash_hex);

/**
 * Convert binary hash to hex string
 */
void hash_to_hex_string(const uint8_t hash[SHA256_DIGEST_LENGTH], char hex_string[65]);

#endif /* OPENCLI_CRYPTO_UTILS_H */
