#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "random.h"
#include "hash.h"
#include "crypto.h"


void print_hex(const unsigned char* data, size_t len)
{
    for (size_t i = 0; i < len; ++i)
        printf("%02x", data[i]);
}

void print_data(const char* msg, const unsigned char data[32])
{
    printf("%s", msg);
    print_hex(data, 32);
    printf("\n");
}

void print_signature(const struct signature* sig, size_t count)
{
    printf("Signature: ");
    print_hex((unsigned char*)sig, count * sizeof(struct signature));
    printf("\n");
}

void print_result(bool r)
{
    printf("Valid signature: %s\n", r ? "True" : "False");
}


struct key_pair
{
    struct public_key pub;
    struct secret_key sec;
};


int main(int argc, char* argv[])
{
    init_random();
    atexit(deinit_random);

    struct key_pair Alice;
    struct key_pair Bob;
    struct key_pair Carol;

    // === Generate keys ===

    generate_keys(&Alice.pub, &Alice.sec);
    generate_keys(&Bob.pub,   &Bob.sec);
    generate_keys(&Carol.pub, &Carol.sec);

    print_data("Alice pub_key: ", Alice.pub.p.data);
    print_data("Alice sec_key: ", Alice.sec.s.data);
    print_data("Bob   pub_key: ", Bob.pub.p.data);
    print_data("Bob   sec_key: ", Bob.sec.s.data);
    print_data("Carol pub_key: ", Carol.pub.p.data);
    print_data("Carol sec_key: ", Carol.sec.s.data);

    assert(check_key(&Alice.pub));
    assert(check_key(&Bob.pub));
    assert(check_key(&Carol.pub));


    struct public_key eph_pub;
    struct secret_key eph_sec;
    struct key_image key_image;

    // === Previous transaction: Alice -> Bob ===
    {
        struct key_pair tx_key;
        generate_keys(&tx_key.pub, &tx_key.sec);
        struct key_derivation deriv;
        assert( generate_key_derivation(&tx_key.pub, &Bob.sec, &deriv) );
        assert( derive_public_key(&deriv, 1, &Bob.pub, &eph_pub) );
        derive_secret_key(&deriv, 1, &Bob.sec, &eph_sec);
        generate_key_image(&eph_pub, &eph_sec, &key_image);
    }
    print_data("Key image: ", key_image.p.data);

    // === New transaction: Bob -> Carol ===

    const char* message = "Message to sign...";

    // === Calc message hash ===

    struct hash hash;
    cn_fast_hash(message, strlen(message), &hash);
    print_data("Message hash: ", hash.data);

    // === Generate signature ===

    const struct public_key* pubs[3] = { &Alice.pub, &Bob.pub, &Carol.pub };
    const size_t pub_count = 3;

    struct secret_key* sec;
    size_t sec_index = 1;  // Bob

    pubs[sec_index] = &eph_pub;
    sec = &eph_sec;

    struct signature *sig = malloc(pub_count * sizeof(struct signature));

    generate_ring_signature(&hash, &key_image, pubs, pub_count, sec, sec_index, sig);

    printf("Signature: ");
    print_hex((unsigned char*)sig, pub_count * sizeof(struct signature));
    printf("\n\n");

    // === Check signature ===

    // 1. Valid signature
    bool r = check_ring_signature(&hash, &key_image, pubs, pub_count, sig);
    print_result(r);  // Expected: True

    // 2. Not valid signature: message being signed is modified
    const char* broken_message = "Not the original message";
    struct hash hash2;
    cn_fast_hash(broken_message, strlen(broken_message), &hash2);
    r = check_ring_signature(&hash2, &key_image, pubs, pub_count, sig);
    print_result(r);  // Expected: False

    // 3. Not valid signature: Carol's public key is absent
    struct key_pair Dave;
    generate_keys(&Dave.pub, &Dave.sec);
    const struct public_key* pubs2[3] = { &Alice.pub, &Bob.pub, &Dave.pub };
    const size_t pub_count2 = 3;
    pubs2[sec_index] = &eph_pub;

    r = check_ring_signature(&hash, &key_image, pubs2, pub_count2, sig);
    print_result(r);  // Expected: False

    free(sig);
    return 0;
}
