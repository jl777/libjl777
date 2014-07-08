#include "crypto.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <alloca.h>
#include <string.h>

#include "hash-ops.h"
#include "crypto-ops.h"
#include "random.h"


static void random_scalar(struct ec_scalar *res)
{
    unsigned char tmp[64];
    generate_random_bytes(64, tmp);
    sc_reduce(tmp);
    memcpy(res, tmp, sizeof(*res));
}

static void hash_to_scalar(const void *data, size_t length, struct ec_scalar *res)
{
    cn_fast_hash(data, length, (struct hash*)(res));
    sc_reduce32(res->data);
}

void generate_keys(struct public_key *pub, struct secret_key *sec)
{
    ge_p3 point;
    random_scalar(&sec->s);
    ge_scalarmult_base(&point, sec->s.data);
    ge_p3_tobytes(pub->p.data, &point);
}

bool check_key(const struct public_key *key)
{
    ge_p3 point;
    return ge_frombytes_vartime(&point, key->p.data) == 0;
}

bool secret_key_to_public_key(const struct secret_key *sec,
                              struct public_key *pub)
{
    ge_p3 point;
    if (sc_check(sec->s.data) != 0)
        return false;

    ge_scalarmult_base(&point, sec->s.data);
    ge_p3_tobytes(pub->p.data, &point);
    return true;
}

bool generate_key_derivation(const struct public_key *key1,
                             const struct secret_key *key2,
                             struct key_derivation *derivation)
{
    ge_p3 point;
    ge_p2 point2;
    ge_p1p1 point3;
    assert(sc_check(key2->s.data) == 0);
    if (ge_frombytes_vartime(&point, key1->p.data) != 0)
        return false;

    ge_scalarmult(&point2, key2->s.data, &point);
    ge_mul8(&point3, &point2);
    ge_p1p1_to_p2(&point2, &point3);
    ge_tobytes(derivation->p.data, &point2);
    return true;
}

static void write_varint(char** dest, size_t i)
{
    while (i >= 0x80) {
        *(*dest)++ = ((char)(i) & 0x7f) | 0x80;
        i >>= 7;
    }
    *(*dest)++ = (char)(i);
}

static void derivation_to_scalar(const struct key_derivation *derivation,
                                 size_t output_index,
                                 struct ec_scalar *res)
{
    struct {
        struct key_derivation derivation;
        char output_index[(sizeof(size_t) * 8 + 6) / 7];
    } buf;
    char *end = buf.output_index;
    buf.derivation = *derivation;
    write_varint(&end, output_index);
    assert(end <= buf.output_index + sizeof(buf.output_index));
    hash_to_scalar(&buf, end - (char*)(&buf), res);
}

bool derive_public_key(const struct key_derivation *derivation,
                       size_t output_index,
                       const struct public_key *base,
                       struct public_key *derived_key)
{
    struct ec_scalar scalar;
    ge_p3 point1;
    ge_p3 point2;
    ge_cached point3;
    ge_p1p1 point4;
    ge_p2 point5;
    if (ge_frombytes_vartime(&point1, base->p.data) != 0)
        return false;

    derivation_to_scalar(derivation, output_index, &scalar);
    ge_scalarmult_base(&point2, scalar.data);
    ge_p3_to_cached(&point3, &point2);
    ge_add(&point4, &point1, &point3);
    ge_p1p1_to_p2(&point5, &point4);
    ge_tobytes(derived_key->p.data, &point5);
    return true;
}

void derive_secret_key(const struct key_derivation *derivation,
                       size_t output_index,
                       const struct secret_key *base,
                       struct secret_key *derived_key)
{
    struct ec_scalar scalar;
    assert(sc_check(base->s.data) == 0);
    derivation_to_scalar(derivation, output_index, &scalar);
    sc_add(derived_key->s.data, base->s.data, scalar.data);
}

struct s_comm {
    struct hash h;
    struct ec_point key;
    struct ec_point comm;
};

void generate_signature(const struct hash *prefix_hash,
                        const struct public_key *pub,
                        const struct secret_key *sec,
                        struct signature *sig)
{
    ge_p3 tmp3;
    struct ec_scalar k;
    struct s_comm buf;
#if !defined(NDEBUG)
    {
        ge_p3 t;
        struct public_key t2;
        assert(sc_check(sec->s.data) == 0);
        ge_scalarmult_base(&t, sec->s.data);
        ge_p3_tobytes(t2.p.data, &t);
        assert(memcmp(pub, &t2, sizeof(t2)) == 0);
    }
#endif
    buf.h = *prefix_hash;
    buf.key = pub->p;
    random_scalar(&k);
    ge_scalarmult_base(&tmp3, k.data);
    ge_p3_tobytes(buf.comm.data, &tmp3);
    hash_to_scalar(&buf, sizeof(buf), &sig->c);
    sc_mulsub(sig->r.data, sig->c.data, sec->s.data, k.data);
}

bool check_signature(const struct hash *prefix_hash,
                     const struct public_key *pub,
                     const struct signature *sig)
{
    ge_p2 tmp2;
    ge_p3 tmp3;
    struct ec_scalar c;
    struct s_comm buf;
    assert(check_key(pub));
    buf.h = *prefix_hash;
    buf.key = pub->p;
    if (ge_frombytes_vartime(&tmp3, pub->p.data) != 0)
        abort();

    if (sc_check(sig->c.data) != 0 || sc_check(sig->r.data) != 0)
        return false;

    ge_double_scalarmult_base_vartime(&tmp2, sig->c.data, &tmp3, sig->r.data);
    ge_tobytes(buf.comm.data, &tmp2);
    hash_to_scalar(&buf, sizeof(buf), &c);
    sc_sub(c.data, c.data, sig->c.data);
    return sc_isnonzero(c.data) == 0;
}


static void hash_to_ec(const struct public_key *key, ge_p3 *res)
{
    struct hash h;
    ge_p2 point;
    ge_p1p1 point2;
    cn_fast_hash(key, sizeof(struct public_key), &h);
    ge_fromfe_frombytes_vartime(&point, (const unsigned char *)(&h));
    ge_mul8(&point2, &point);
    ge_p1p1_to_p3(res, &point2);
}

void generate_key_image(const struct public_key *pub,
                        const struct secret_key *sec,
                        struct key_image *image)
{
    ge_p3 point;
    ge_p2 point2;
    assert(sc_check(sec->s.data) == 0);
    hash_to_ec(pub, &point);
    ge_scalarmult(&point2, sec->s.data, &point);
    ge_tobytes(image->p.data, &point2);
}


struct rs_comm_ab
{
    struct ec_point a;
    struct ec_point b;
};

struct rs_comm
{
    struct hash h;
    struct rs_comm_ab ab[];
};

static inline size_t rs_comm_size(size_t pubs_count)
{
    return sizeof(struct rs_comm) + pubs_count * sizeof(struct rs_comm_ab);
}

void generate_ring_signature(const struct hash *prefix_hash,
                             const struct key_image *image,
                             const struct public_key *const pubs[],
                             size_t pubs_count,
                             const struct secret_key *sec,
                             size_t sec_index,
                             struct signature *sig)
{
    ge_p3 image_unp;
    ge_dsmp image_pre;
    struct ec_scalar sum, k, h;
    struct rs_comm *const buf = alloca(rs_comm_size(pubs_count));
    assert(sec_index < pubs_count);
#if !defined(NDEBUG)
    {
        ge_p3 t;
        struct public_key t2;
        struct key_image t3;
        assert(sc_check(sec->s.data) == 0);
        ge_scalarmult_base(&t, sec->s.data);
        ge_p3_tobytes(t2.p.data, &t);
        assert(memcmp(pubs[sec_index], &t2, sizeof(t2)) == 0);
        generate_key_image(pubs[sec_index], sec, &t3);
        assert(memcmp(image, &t3, sizeof(t3)) == 0);
        for (size_t i = 0; i < pubs_count; i++) {
            assert(check_key(pubs[i]));
        }
    }
#endif
    if (ge_frombytes_vartime(&image_unp, image->p.data) != 0)
        abort();

    ge_dsm_precomp(image_pre, &image_unp);
    sc_0(sum.data);
    buf->h = *prefix_hash;
    for (size_t i = 0; i < pubs_count; i++) {
        ge_p2 tmp2;
        ge_p3 tmp3;
        if (i == sec_index) {
            random_scalar(&k);
            ge_scalarmult_base(&tmp3, k.data);
            ge_p3_tobytes(buf->ab[i].a.data, &tmp3);
            hash_to_ec(pubs[i], &tmp3);
            ge_scalarmult(&tmp2, k.data, &tmp3);
            ge_tobytes(buf->ab[i].b.data, &tmp2);
        } else {
            random_scalar(&sig[i].c);
            random_scalar(&sig[i].r);
            if (ge_frombytes_vartime(&tmp3, pubs[i]->p.data) != 0)
                abort();
            ge_double_scalarmult_base_vartime(&tmp2, sig[i].c.data, &tmp3,
                                              sig[i].r.data);
            ge_tobytes(buf->ab[i].a.data, &tmp2);
            hash_to_ec(pubs[i], &tmp3);
            ge_double_scalarmult_precomp_vartime(&tmp2, sig[i].r.data, &tmp3,
                                                 sig[i].c.data, image_pre);
            ge_tobytes(buf->ab[i].b.data, &tmp2);
            sc_add(sum.data, sum.data, sig[i].c.data);
        }
    }
    hash_to_scalar(buf, rs_comm_size(pubs_count), &h);
    sc_sub(sig[sec_index].c.data, h.data, sum.data);
    sc_mulsub(sig[sec_index].r.data, sig[sec_index].c.data, sec->s.data, k.data);
}

bool check_ring_signature(const struct hash *prefix_hash,
                          const struct key_image *image,
                          const struct public_key *const pubs[],
                          size_t pubs_count,
                          const struct signature *sig)
{
    ge_p3 image_unp;
    ge_dsmp image_pre;
    struct ec_scalar sum, h;
    struct rs_comm *const buf = alloca(rs_comm_size(pubs_count));
#if !defined(NDEBUG)
    for (size_t i = 0; i < pubs_count; i++)
        assert(check_key(pubs[i]));
#endif
    if (ge_frombytes_vartime(&image_unp, image->p.data) != 0)
        return false;

    ge_dsm_precomp(image_pre, &image_unp);
    sc_0(sum.data);
    buf->h = *prefix_hash;
    for (size_t i = 0; i < pubs_count; i++) {
        ge_p2 tmp2;
        ge_p3 tmp3;
        if (sc_check(sig[i].c.data) != 0 || sc_check(sig[i].r.data) != 0)
            return false;
        if (ge_frombytes_vartime(&tmp3, pubs[i]->p.data) != 0)
            abort();
        ge_double_scalarmult_base_vartime(&tmp2, sig[i].c.data, &tmp3,
                                          sig[i].r.data);
        ge_tobytes(buf->ab[i].a.data, &tmp2);
        hash_to_ec(pubs[i], &tmp3);
        ge_double_scalarmult_precomp_vartime(&tmp2, sig[i].r.data, &tmp3,
                                             sig[i].c.data, image_pre);
        ge_tobytes(buf->ab[i].b.data, &tmp2);
        sc_add(sum.data, sum.data, sig[i].c.data);
    }
    hash_to_scalar(buf, rs_comm_size(pubs_count), &h);
    sc_sub(h.data, h.data, sum.data);
    return sc_isnonzero(h.data) == 0;
}
