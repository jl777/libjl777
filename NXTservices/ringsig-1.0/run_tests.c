#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Include C sources right here
#include "random.c"
#include "crypto.c"

void fatal_error(const char* msg)
{
    printf("\nFATAL ERROR: %s\n", msg);
    abort();
}

void setup_test_random()
{
    memset(&state, 42, sizeof(union hash_state));
}

bool scalar_eq(const struct ec_scalar *a, const struct ec_scalar *b)
{
  return memcmp(a, b, sizeof(struct ec_scalar)) == 0;
}

bool point_eq(const struct ec_point *a, const struct ec_point *b)
{
  return memcmp(a, b, sizeof(struct ec_point)) == 0;
}

bool public_key_eq(const struct public_key *a, const struct public_key *b)
{
    return point_eq(&a->p, &b->p);
}

bool secret_key_eq(const struct secret_key *a, const struct secret_key *b)
{
    return scalar_eq(&a->s, &b->s);
}

bool key_derivation_eq(const struct key_derivation *a, const struct key_derivation *b)
{
    return point_eq(&a->p, &b->p);
}

bool key_image_eq(const struct key_image *a, const struct key_image *b)
{
    return point_eq(&a->p, &b->p);
}

bool signature_eq(const struct signature *a, const struct signature *b)
{
    return scalar_eq(&a->c, &b->c) && scalar_eq(&a->r, &b->r);
}

bool multi_signature_eq(const struct signature a[], const struct signature b[], size_t count)
{
    for (size_t i = 0; i < count; ++i)
        if (!signature_eq(&a[i], &b[i]))
            return false;
    return true;
}


FILE* input_file = NULL;

static void read_size_t(size_t *s)
{
    if (fscanf(input_file, "%u", s) != 1)
        fatal_error("read_size_t()");
}

static bool read_string(char s[32])
{
    if (fscanf(input_file, "%32s", s) == 1) {
        return true;
    } else {
        if (!feof(input_file))
            fatal_error("read_string()");
        return false;
    }
}

static void read_bool(bool *b)
{
    char s[32];
    read_string(s);
    if (strcmp(s, "false") == 0)
        *b = false;
    else if (strcmp(s, "true") == 0)
        *b = true;
    else
        fatal_error("read_bool()");
}

static size_t read_data(unsigned char buf[32])
{
    char hex[65];
    if (fscanf(input_file, "%64s", hex) != 1)
        fatal_error("read_data()");

    size_t size = strnlen(hex, 64) / 2;
    for (size_t i = 0; i < size; ++i) {
        unsigned u;
        if (sscanf(&hex[i*2], "%2x", &u) != 1)
            fatal_error("read_data()");
        buf[i] = u;
    }
    return size;
}

static void read_32hex(unsigned char buf[32])
{
    for (size_t i = 0; i < 32; ++i) {
        unsigned u;
        if (fscanf(input_file, "%2x", &u) != 1)
            fatal_error("read_32hex()");
        buf[i] = u;
    }
}

static void read_scalar(struct ec_scalar *s)
{
    read_32hex(s->data);
}

static void read_point(struct ec_point *p)
{
    read_32hex(p->data);
}

static void read_public_key(struct public_key *p)
{
    read_point(&p->p);
}

static void read_secret_key(struct secret_key *s)
{
    read_scalar(&s->s);
}

static void read_key_derivation(struct key_derivation *d)
{
    read_point(&d->p);
}

static void read_key_image(struct key_image *i)
{
    read_point(&i->p);
}

static void read_signature(struct signature *s)
{
    read_scalar(&s->c);
    read_scalar(&s->r);
}

static void read_hash(struct hash *s)
{
    read_32hex(s->data);
}



static bool check_scalar(const struct ec_scalar *scalar)
{
    return sc_check(scalar->data) == 0;
}

static void hash_to_point(const struct hash *h, struct ec_point *res)
{
    ge_p2 point;
    ge_fromfe_frombytes_vartime(&point, h->data);
    ge_tobytes(res->data, &point);
}

static void _hash_to_ec(const struct public_key *key, struct ec_point *res)
{
    ge_p3 tmp;
    hash_to_ec(key, &tmp);
    ge_p3_tobytes(res->data, &tmp);
}


int run_tests()
{
    size_t test_index = 0;
    bool error = false;


    input_file = fopen("tests.txt", "r");
    if (!input_file) fatal_error("fopen()");

    for (;;) {
        char cmd[32];
        if (!read_string(cmd))
            break;
        cmd[31] = '\0';

        bool test_ok = false;
        ++test_index;

        if (strcmp(cmd, "check_scalar") == 0) {
            struct ec_scalar scalar;
            bool expected, actual;
            read_scalar(&scalar);
            read_bool(&expected);
            actual = check_scalar(&scalar);
            test_ok = (expected == actual);
        } else if (strcmp(cmd, "random_scalar") == 0) {
            struct ec_scalar expected, actual;
            read_scalar(&expected);
            random_scalar(&actual);
            test_ok = scalar_eq(&expected, &actual);
        } else if (strcmp(cmd, "hash_to_scalar") == 0) {
            unsigned char data[32];
            struct ec_scalar expected, actual;
            size_t size = read_data(data);
            read_scalar(&expected);
            hash_to_scalar(data, size, &actual);
            test_ok = scalar_eq(&expected, &actual);
        } else if (strcmp(cmd, "generate_keys") == 0) {
            struct public_key expected1, actual1;
            struct secret_key expected2, actual2;
            read_public_key(&expected1);
            read_secret_key(&expected2);
            generate_keys(&actual1, &actual2);
            test_ok = public_key_eq(&expected1, &actual1) && secret_key_eq(&expected2, &actual2);
        } else if (strcmp(cmd, "check_key") == 0) {
            struct public_key key;
            bool expected, actual;
            read_public_key(&key);
            read_bool(&expected);
            actual = check_key(&key);
            test_ok = (expected == actual);
        } else if (strcmp(cmd, "secret_key_to_public_key") == 0) {
            struct secret_key sec;
            bool expected1, actual1;
            struct public_key expected2, actual2;
            read_secret_key(&sec);
            read_bool(&expected1);
            if (expected1)
                read_public_key(&expected2);
            actual1 = secret_key_to_public_key(&sec, &actual2);
            test_ok = ! (expected1 != actual1 || (expected1 && !public_key_eq(&expected2, &actual2)));
        } else if (strcmp(cmd, "generate_key_derivation") == 0) {
            struct public_key key1;
            struct secret_key key2;
            bool expected1, actual1;
            struct key_derivation expected2, actual2;
            read_public_key(&key1);
            read_secret_key(&key2);
            read_bool(&expected1);
            if (expected1)
                read_key_derivation(&expected2);
            actual1 = generate_key_derivation(&key1, &key2, &actual2);
            test_ok = ! (expected1 != actual1 || (expected1 && !key_derivation_eq(&expected2, &actual2)));
        } else if (strcmp(cmd, "derive_public_key") == 0) {
            struct key_derivation derivation;
            size_t output_index;
            struct public_key base;
            bool expected1, actual1;
            struct public_key expected2, actual2;

            read_key_derivation(&derivation);
            read_size_t(&output_index);
            read_public_key(&base);
            read_bool(&expected1);

            if (expected1)
                read_public_key(&expected2);

            actual1 = derive_public_key(&derivation, output_index, &base, &actual2);
            test_ok = ! (expected1 != actual1 || (expected1 && !public_key_eq(&expected2, &actual2)));
        } else if (strcmp(cmd, "derive_secret_key") == 0) {
            struct key_derivation derivation;
            size_t output_index;
            struct secret_key base;
            struct secret_key expected, actual;
            read_key_derivation(&derivation);
            read_size_t(&output_index);
            read_secret_key(&base);
            read_secret_key(&expected);
            derive_secret_key(&derivation, output_index, &base, &actual);
            test_ok = secret_key_eq(&expected, &actual);
        } else if (strcmp(cmd, "generate_signature") == 0) {
            struct hash prefix_hash;
            struct public_key pub;
            struct secret_key sec;
            struct signature expected, actual;
            read_hash(&prefix_hash);
            read_public_key(&pub);
            read_secret_key(&sec);
            read_signature(&expected);
            generate_signature(&prefix_hash, &pub, &sec, &actual);
            test_ok = signature_eq(&expected, &actual);
        } else if (strcmp(cmd, "check_signature") == 0) {
            struct hash prefix_hash;
            struct public_key pub;
            struct signature sig;
            bool expected, actual;
            read_hash(&prefix_hash);
            read_public_key(&pub);
            read_signature(&sig);
            read_bool(&expected);
            actual = check_signature(&prefix_hash, &pub, &sig);
            test_ok = (expected == actual);
        } else if (strcmp(cmd, "hash_to_point") == 0) {
            struct hash h;
            struct ec_point expected, actual;
            read_hash(&h);
            read_point(&expected);
            hash_to_point(&h, &actual);
            test_ok = point_eq(&expected, &actual);
        } else if (strcmp(cmd, "hash_to_ec") == 0) {
            struct public_key key;
            struct ec_point expected, actual;
            read_public_key(&key);
            read_point(&expected);
            _hash_to_ec(&key, &actual);
            test_ok = point_eq(&expected, &actual);
        } else if (strcmp(cmd, "generate_key_image") == 0) {
            struct public_key pub;
            struct secret_key sec;
            struct key_image expected, actual;
            read_public_key(&pub);
            read_secret_key(&sec);
            read_key_image(&expected);
            generate_key_image(&pub, &sec, &actual);
            test_ok = key_image_eq(&expected, &actual);
        } else if (strcmp(cmd, "generate_ring_signature") == 0) {
            struct hash prefix_hash;
            struct key_image image;
            size_t pubs_count;
            read_hash(&prefix_hash);
            read_key_image(&image);
            read_size_t(&pubs_count);
            struct public_key* pubs = malloc(pubs_count * sizeof(struct public_key));
            const struct public_key** pub_ptrs = malloc(pubs_count * sizeof(struct public_key*));
            for (size_t i = 0; i < pubs_count; i++) {
                read_public_key(&pubs[i]);
                pub_ptrs[i] = &pubs[i];
            }
            struct secret_key sec;
            size_t sec_index;
            read_secret_key(&sec);
            read_size_t(&sec_index);
            struct signature* expected = malloc(pubs_count * sizeof(struct signature));
            for (size_t i = 0; i < pubs_count; i++)
                read_signature(&expected[i]);
            struct signature* actual = malloc(pubs_count * sizeof(struct signature));
            generate_ring_signature(&prefix_hash, &image, pub_ptrs, pubs_count,
                                    &sec, sec_index, actual);
            test_ok = multi_signature_eq(expected, actual, pubs_count);
            free(actual);
            free(expected);
            free(pub_ptrs);
            free(pubs);
        } else if (strcmp(cmd, "check_ring_signature") == 0) {
            struct hash prefix_hash;
            struct key_image image;
            size_t pubs_count;
            bool expected, actual;
            read_hash(&prefix_hash);
            read_key_image(&image);
            read_size_t(&pubs_count);
            struct public_key* pubs = malloc(pubs_count * sizeof(struct public_key));
            const struct public_key** pub_ptrs = malloc(pubs_count * sizeof(struct public_key*));
            for (size_t i = 0; i < pubs_count; i++) {
                read_public_key(&pubs[i]);
                pub_ptrs[i] = &pubs[i];
            }
            struct signature* sigs = malloc(pubs_count * sizeof(struct signature));
            for (size_t i = 0; i < pubs_count; i++)
                read_signature(&sigs[i]);
            read_bool(&expected);
            actual = check_ring_signature(&prefix_hash, &image,
                                          pub_ptrs, pubs_count,
                                          sigs);
            test_ok = (expected == actual);
            free(sigs);
            free(pub_ptrs);
            free(pubs);
        } else {
            fatal_error("Bad command");
        }

        if (test_ok) {
            printf("Test %u: %s: ok\n", test_index, cmd);
        } else {
            printf("Test %u: %s: FAIL\n", test_index, cmd);
            error = true;
        }
    }
    fclose(input_file);
    if (error)
        printf("One or more tests failed\n");
    else
        printf("All tests passed\n");
    return error ? 1 : 0;
}

int main(int argc, char* argv[])
{
    init_random();
    atexit(deinit_random);
    setup_test_random();

    return run_tests();
}
