#pragma once

#include <stddef.h>
#include <stdbool.h>
#include "hash.h"

#pragma pack(push, 1)
struct ec_point
{
    unsigned char data[32];
};

struct ec_scalar
{
    unsigned char data[32];
};

struct public_key
{
    struct ec_point p;
};

struct secret_key
{
    struct ec_scalar s;
};

struct key_derivation
{
    struct ec_point p;
};

struct key_image
{
    struct ec_point p;
};

struct signature
{
    struct ec_scalar c;
    struct ec_scalar r;
};
#pragma pack(pop)

_Static_assert(sizeof(struct ec_point) == 32 &&
               sizeof(struct ec_scalar) == 32 &&
               sizeof(struct public_key) == 32 &&
               sizeof(struct secret_key) == 32 &&
               sizeof(struct key_derivation) == 32 &&
               sizeof(struct key_image) == 32 &&
               sizeof(struct signature) == 64, "Invalid structure size");


/* Generate a new key pair
 */
void generate_keys(struct public_key *pub,
                   struct secret_key *sec);

/* Check a public key. Returns true if it is valid, false otherwise.
 */
bool check_key(const struct public_key *key);

/* Checks a private key and computes the corresponding public key.
 */
bool secret_key_to_public_key(const struct secret_key *sec,
                              struct public_key *pub);

/* To generate an ephemeral key used to send money to:
 * * The sender generates a new key pair, which becomes the transaction key.
 *   The public transaction key is included in "extra" field.
 * * Both the sender and the receiver generate key derivation from
 *   the transaction key, the receivers' "view" key and the output index.
 * * The sender uses key derivation and the receivers' "spend" key to derive
 *   an ephemeral public key.
 * * The receiver can either derive the public key (to check that
 *   the transaction is addressed to him) or the private key (to spend the money).
 */
bool generate_key_derivation(const struct public_key *key1,
                             const struct secret_key *key2,
                             struct key_derivation *derivation);

bool derive_public_key(const struct key_derivation *derivation,
                       size_t output_index,
                       const struct public_key *base,
                       struct public_key *derived_key);

void derive_secret_key(const struct key_derivation *derivation,
                       size_t output_index,
                       const struct secret_key *base,
                       struct secret_key *derived_key);

/* Generation and checking of a standard signature.
 */
void generate_signature(const struct hash *prefix_hash,
                        const struct public_key *pub,
                        const struct secret_key *sec,
                        struct signature *sig);

bool check_signature(const struct hash *prefix_hash,
                     const struct public_key *pub,
                     const struct signature *sig);

/* To send money to a key:
 * * The sender generates an ephemeral key and includes it in transaction output.
 * * To spend the money, the receiver generates a key image from it.
 * * Then he selects a bunch of outputs, including the one he spends, and uses
 *   them to generate a ring signature.
 * To check the signature, it is necessary to collect all the keys that were
 * used to generate it. To detect double spends, it is necessary to check that
 * each key image is used at most once.
 */
void generate_key_image(const struct public_key *pub,
                        const struct secret_key *sec,
                        struct key_image *image);

void generate_ring_signature(const struct hash *prefix_hash,
                             const struct key_image *image,
                             const struct public_key *const pubs[],
                             size_t pubs_count,
                             const struct secret_key *sec,
                             size_t sec_index,
                             struct signature *sig);

bool check_ring_signature(const struct hash *prefix_hash,
                          const struct key_image *image,
                          const struct public_key *const pubs[],
                          size_t pubs_count,
                          const struct signature *sig);
