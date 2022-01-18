#include <erl_nif.h>
#include <hashids.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#define EHASHIDS_MAX_NUMBERS 65536
/* branch prediction hinting */
#ifndef __has_builtin
#   define __has_builtin(x) (0)
#endif
#if defined(__builtin_expect) || __has_builtin(__builtin_expect)
#   define EHASHIDS_LIKELY(x)        (__builtin_expect(!!(x), 1))
#   define EHASHIDS_UNLIKELY(x)      (__builtin_expect(!!(x), 0))
#else
#   define EHASHIDS_LIKELY(x)        (x)
#   define EHASHIDS_UNLIKELY(x)      (x)
#endif


static ErlNifResourceType *hashids_type = NULL;

static ERL_NIF_TERM hashids_init_nif(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM hashids_estimate_encoded_size_nif(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM hashids_encode_nif(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM hashids_decode_nif(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM hashids_compile_nif(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM hashids_new_from_compiled_nif(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);

static int handle_load(ErlNifEnv *env, void **priv, ERL_NIF_TERM load_info);
static void garbage_collect_hashids(ErlNifEnv *env, void *p);
static int handle_upgrade(ErlNifEnv* env, void** priv_data, void** old_priv_data, ERL_NIF_TERM load_info);

static ERL_NIF_TERM make_error_tuple_from_string(ErlNifEnv* env, const char *error);
static ERL_NIF_TERM make_error_tuple(ErlNifEnv* env, ERL_NIF_TERM error);
static ERL_NIF_TERM make_atom(ErlNifEnv* env, const char* atom);

/* ERL_NIF >= 2.8 */
#if ERL_NIF_MAJOR_VERSION > 2 || \
    (ERL_NIF_MAJOR_VERSION == 2 && \
    (ERL_NIF_MINOR_VERSION > 8 || (ERL_NIF_MINOR_VERSION == 8)))

static ErlNifFunc nif_funcs[] = {
    {"new",                   3, hashids_init_nif,                  0},
    {"new",                   2, hashids_init_nif,                  0},
    {"new",                   1, hashids_init_nif,                  0},
    {"new",                   0, hashids_init_nif,                  0},
    {"estimate_encoded_size", 2, hashids_estimate_encoded_size_nif, 0},
    {"encode",                2, hashids_encode_nif,                0},
    {"decode",                2, hashids_decode_nif,                0},
    {"compile",               1, hashids_compile_nif,               0},
    {"from_compiled",         1, hashids_new_from_compiled_nif,     0}
};
#else
static ErlNifFunc nif_funcs[] = {
    {"new",                   3, hashids_init_nif},
    {"new",                   2, hashids_init_nif},
    {"new",                   1, hashids_init_nif},
    {"new",                   0, hashids_init_nif},
    {"estimate_encoded_size", 2, hashids_estimate_encoded_size_nif},
    {"encode",                2, hashids_encode_nif},
    {"decode",                2, hashids_decode_nif},
    {"compile",               1, hashids_compile_nif},
    {"from_compiled",         1, hashids_new_from_compiled_nif}
};
#endif

static void garbage_collect_hashids(ErlNifEnv *env, void *p)
{
    hashids_t *hashids = *(hashids_t **)p;
    hashids_free(hashids);
}

static int handle_load(ErlNifEnv *env, void **priv, ERL_NIF_TERM load_info)
{
    ErlNifResourceType *rt;
    ErlNifResourceFlags rf;

    rt = enif_open_resource_type(
        env,
        NULL,
        "hashids_type",
        garbage_collect_hashids,
        ERL_NIF_RT_CREATE,
        &rf
    );

    if(EHASHIDS_UNLIKELY(rt == NULL)) return -1;

    hashids_type = rt;

    return 0;
}

static int handle_upgrade(ErlNifEnv* env, void** priv_data, void** old_priv_data, ERL_NIF_TERM load_info)
{
    return 0;
}


static ERL_NIF_TERM hashids_init_nif(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    hashids_t *hashids;
    hashids_t **resource_ref;
    int res;
    char *salt = NULL;
    char *alphabet = NULL;
    ErlNifBinary salt_bin;
    ErlNifBinary alphabet_bin;
    ERL_NIF_TERM resource;
    unsigned int min_hash_length;

    if(argc > 0){
        res = enif_inspect_binary(env, argv[0], &salt_bin);
        if(res == 0){
            return make_error_tuple_from_string(env, "salt");
        }
        salt = (char *)enif_alloc(salt_bin.size + 1);
        if(salt == NULL){
            return make_error_tuple_from_string(env, "alloc");
        }
        (void)memcpy(salt, salt_bin.data, salt_bin.size);
        salt[salt_bin.size] = '\0';
    }

    if(argc > 1){
        res = enif_get_uint(env, argv[1], &min_hash_length);
        if(res == 0){
            enif_free(salt);
            return make_error_tuple_from_string(env, "max_hash_length");
        }
    }

    if(argc > 2){
        res = enif_inspect_binary(env, argv[2], &alphabet_bin);
        if(EHASHIDS_UNLIKELY(res == 0)){
            enif_free(salt);
            return make_error_tuple_from_string(env, "alphabet");
        }
        alphabet = (char *)enif_alloc(alphabet_bin.size + 1);
        if(EHASHIDS_UNLIKELY(alphabet == NULL)){
            enif_free(salt);
            return make_error_tuple_from_string(env, "alphabet_alloc");
        }
        (void)memcpy(alphabet, alphabet_bin.data, alphabet_bin.size);
        alphabet[alphabet_bin.size] = '\0';
    }

    switch(argc){
    case 0:
    case 1:    
        hashids = hashids_init(salt);
        break;
    case 2:
        hashids = hashids_init2(salt, min_hash_length);
        break;
    case 3:
        hashids = hashids_init3(salt, min_hash_length, alphabet);
        break;
    default:
        if(salt) enif_free(salt);
        if(alphabet) enif_free(alphabet);
        return make_error_tuple_from_string(env, "arity");
    }

    if (EHASHIDS_UNLIKELY(!hashids)) {
        switch (hashids_errno) {
            case HASHIDS_ERROR_ALLOC:
                if (salt) enif_free(salt);
                if (alphabet) enif_free(alphabet);
                return make_error_tuple_from_string(env, "alloc");

            case HASHIDS_ERROR_ALPHABET_LENGTH:
                if (salt) enif_free(salt);
                if (alphabet) enif_free(alphabet);
                return make_error_tuple_from_string(env, "alphabet_length");

            case HASHIDS_ERROR_ALPHABET_SPACE:
                if (salt) enif_free(salt);
                if (alphabet) enif_free(alphabet);
                return make_error_tuple_from_string(env, "alphabet_space");

            default:
                if (salt) enif_free(salt);
                if (alphabet) enif_free(alphabet);
                return make_error_tuple_from_string(env, "init");
        }
    }

    resource_ref = (hashids_t **)enif_alloc_resource(hashids_type, sizeof(hashids_t *));
    *resource_ref = hashids;

    if(salt) enif_free(salt);
    if(alphabet) enif_free(alphabet);

    resource = enif_make_resource(env, resource_ref);
    enif_release_resource(resource_ref);
    
    return resource;
}

static ERL_NIF_TERM hashids_estimate_encoded_size_nif(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    hashids_t **hashids = NULL;
    ERL_NIF_TERM head;
    ERL_NIF_TERM tail;
    ERL_NIF_TERM tmp;
    unsigned int len;
    ErlNifUInt64 *numbers;
    size_t res;

    assert(sizeof(unsigned long long) == sizeof(ErlNifUInt64));

    if(EHASHIDS_UNLIKELY(argc != 2)) return make_error_tuple_from_string(env, "arity");

    if(EHASHIDS_UNLIKELY(!enif_get_resource(env, argv[0], hashids_type, (void **)&hashids))) {
        return make_error_tuple_from_string(env, "bad_resource");
    }

    if(EHASHIDS_UNLIKELY(!enif_get_list_length(env, argv[1], &len))){
        return make_error_tuple_from_string(env, "numbers");
    }

    // +1 if the list is empty because we don't want strange behaviour here
    numbers = (ErlNifUInt64 *)enif_alloc(sizeof(ErlNifUInt64) * (len + 1));
    if(EHASHIDS_UNLIKELY(!numbers)){
        return make_error_tuple_from_string(env, "alloc");
    }

    tmp = argv[1];
    for(unsigned int i = 0; i < len; i++, tmp = tail){
        if(EHASHIDS_UNLIKELY(!enif_get_list_cell(env, tmp, &head, &tail))){
            enif_free(numbers);
            return make_error_tuple_from_string(env, "numbers");
        }
        if(EHASHIDS_UNLIKELY(!enif_get_uint64(env, head, numbers + i))){
            enif_free(numbers);
            return make_error_tuple_from_string(env, "numbers");
        }
    }

    res = hashids_estimate_encoded_size(*hashids, (size_t)len, (unsigned long long *)numbers);
    enif_free(numbers);
    return enif_make_tuple2(env, make_atom(env, "ok"), enif_make_uint64(env, res));
}

static ERL_NIF_TERM hashids_encode_nif(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    hashids_t **hashids = NULL;
    ERL_NIF_TERM head;
    ERL_NIF_TERM tail;
    ERL_NIF_TERM tmp;
    unsigned int len;
    ErlNifUInt64 *numbers;
    size_t res;
    size_t buffer_size;
    ErlNifBinary out_bin;
    ERL_NIF_TERM final_bin;

    assert(sizeof(unsigned long long) == sizeof(ErlNifUInt64));

    if(EHASHIDS_UNLIKELY(argc != 2)) return make_error_tuple_from_string(env, "arity");

    if(EHASHIDS_UNLIKELY(!enif_get_resource(env, argv[0], hashids_type, (void **)&hashids))) {
        return make_error_tuple_from_string(env, "bad_resource");
    }

    if(EHASHIDS_UNLIKELY(!enif_get_list_length(env, argv[1], &len))){
        return make_error_tuple_from_string(env, "numbers");
    }

    // +1 if the list is empty because we don't want strange behaviour here
    numbers = (ErlNifUInt64 *)enif_alloc(sizeof(ErlNifUInt64) * (len + 1));
    if(EHASHIDS_UNLIKELY(!numbers)){
        return make_error_tuple_from_string(env, "alloc");
    }

    tmp = argv[1];
    for(unsigned int i = 0; i < len; i++, tmp = tail){
        if(EHASHIDS_UNLIKELY(!enif_get_list_cell(env, tmp, &head, &tail))){
            enif_free(numbers);
            return make_error_tuple_from_string(env, "numbers");
        }
        if(EHASHIDS_UNLIKELY(!enif_get_uint64(env, head, numbers + i))){
            enif_free(numbers);
            return make_error_tuple_from_string(env, "numbers");
        }
    }
    buffer_size = hashids_estimate_encoded_size(*hashids, (size_t)len, (unsigned long long *)numbers);
    if(EHASHIDS_UNLIKELY(enif_alloc_binary(buffer_size, &out_bin) == 0)){
        enif_free(numbers);
        return make_error_tuple_from_string(env, "alloc");
    }

    res = hashids_encode(*hashids, (char *)out_bin.data, (size_t)len, (unsigned long long *)numbers);
    if(EHASHIDS_UNLIKELY(res == 0)){
        enif_free(numbers);
        enif_release_binary(&out_bin);
        return make_error_tuple_from_string(env, "numbers");
    }

    // Fix up because C gives us zero terminated strings and Erlang does not
    out_bin.size = strlen((char *)out_bin.data);
    final_bin = enif_make_binary(env, &out_bin);
    enif_free(numbers);
    return enif_make_tuple2(env, make_atom(env, "ok"), final_bin);
}

static ERL_NIF_TERM hashids_decode_nif(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    hashids_t **hashids = NULL;
    size_t ret;
    int res;
    char *encoded_id = NULL;
    ErlNifBinary encoded_id_bin;
    unsigned long long *numbers;
    size_t numbers_len = 8;
    ERL_NIF_TERM *arr = NULL;
    ERL_NIF_TERM result;

    if(EHASHIDS_UNLIKELY(argc != 2)) return make_error_tuple_from_string(env, "arity");

    if(EHASHIDS_UNLIKELY(!enif_get_resource(env, argv[0], hashids_type, (void **)&hashids))) {
        return make_error_tuple_from_string(env, "bad_resource");
    }

    res = enif_inspect_binary(env, argv[1], &encoded_id_bin);
    if(EHASHIDS_UNLIKELY(res == 0)){
        return make_error_tuple_from_string(env, "id");
    }
    encoded_id = (char *)enif_alloc(encoded_id_bin.size + 1);
    if(EHASHIDS_UNLIKELY(encoded_id == NULL)){
        return make_error_tuple_from_string(env, "alloc");
    }
    (void)memcpy(encoded_id, encoded_id_bin.data, encoded_id_bin.size);
    encoded_id[encoded_id_bin.size] = '\0';

    do {
        numbers = (unsigned long long *)enif_alloc(numbers_len * sizeof(unsigned long long));
        ret = hashids_decode(*hashids, encoded_id, numbers, numbers_len);
        if(EHASHIDS_UNLIKELY(ret == 0)){
            switch(hashids_errno){
                case HASHIDS_ERROR_INVALID_HASH:
                    enif_free(encoded_id);
                    enif_free(numbers);
                    return make_error_tuple_from_string(env, "invalid_hash");
                case HASHIDS_ERROR_ALLOC:
                    enif_free(encoded_id);
                    enif_free(numbers);
                    return make_error_tuple_from_string(env, "alloc");
                default:
                    enif_free(encoded_id);
                    enif_free(numbers);
                    return make_error_tuple_from_string(env, "unknown");
            }
        }
        if(EHASHIDS_UNLIKELY(ret == numbers_len && numbers_len <= EHASHIDS_MAX_NUMBERS)){
            enif_free(numbers);
            numbers_len <<= 1;
        } else break;
    }while(1);

    arr = (ERL_NIF_TERM *)enif_alloc(sizeof(ERL_NIF_TERM) * ret);
    for(size_t i = 0; i < ret; i++){
        arr[i] = enif_make_uint64(env, (ErlNifUInt64)numbers[i]);
    }
    enif_free(numbers);
    result = enif_make_list_from_array(env, arr, (unsigned int)ret);
    enif_free(arr);
    return enif_make_tuple2(env, make_atom(env, "ok"), result);
}

static ERL_NIF_TERM hashids_new_from_compiled_nif(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    hashids_t *hashids;
    hashids_t **resource_ref;
    ERL_NIF_TERM resource;
    ErlNifBinary alphabet_bin;
    ErlNifBinary alphabet_copy1_bin;
    ErlNifBinary alphabet_copy2_bin;
    ErlNifBinary salt_bin;
    ErlNifBinary separators_bin;
    ErlNifBinary guards_bin;
    const ERL_NIF_TERM *arr;
    int arity;
    int res;
    ErlNifUInt64 alphabet_length;
    ErlNifUInt64 salt_length;
    ErlNifUInt64 separators_count;
    ErlNifUInt64 guards_count;
    ErlNifUInt64 min_hash_length;

    if(EHASHIDS_UNLIKELY(argc != 1)) return make_error_tuple_from_string(env, "arity");
    hashids = (hashids_t *)calloc(1, sizeof(hashids_t));

    if(EHASHIDS_UNLIKELY(enif_get_tuple(env, argv[0], &arity, &arr) == 0)){
        free(hashids);
        return make_error_tuple_from_string(env, "badarg");
    }

    if(EHASHIDS_UNLIKELY(arity != 11)){
        free(hashids);
        return make_error_tuple_from_string(env, "badarg");
    }

    res = enif_inspect_binary(env, arr[0], &alphabet_bin);
    if(EHASHIDS_UNLIKELY(res == 0)){
        free(hashids);
        return make_error_tuple_from_string(env, "badarg");
    }

    res = enif_inspect_binary(env, arr[1], &alphabet_copy1_bin);
    if(EHASHIDS_UNLIKELY(res == 0)){
        free(hashids);
        return make_error_tuple_from_string(env, "badarg");
    }

    res = enif_inspect_binary(env, arr[2], &alphabet_copy2_bin);
    if(EHASHIDS_UNLIKELY(res == 0)){
        free(hashids);
        return make_error_tuple_from_string(env, "badarg");
    }

    res = enif_get_uint64(env, arr[3], &alphabet_length);
    if(EHASHIDS_UNLIKELY(res == 0)){
        free(hashids);
        return make_error_tuple_from_string(env, "badarg");
    }
    hashids->alphabet_length = (size_t)alphabet_length;

    res = enif_inspect_binary(env, arr[4], &salt_bin);
    if(EHASHIDS_UNLIKELY(res == 0)){
        free(hashids);
        return make_error_tuple_from_string(env, "badarg");
    }

    res = enif_get_uint64(env, arr[5], &salt_length);
    if(EHASHIDS_UNLIKELY(res == 0)){
        free(hashids);
        return make_error_tuple_from_string(env, "badarg");
    }
    hashids->salt_length = (size_t)salt_length;

    res = enif_inspect_binary(env, arr[6], &separators_bin);
    if(EHASHIDS_UNLIKELY(res == 0)){
        free(hashids);
        return make_error_tuple_from_string(env, "badarg");
    }

    res = enif_get_uint64(env, arr[7], &separators_count);
    if(EHASHIDS_UNLIKELY(res == 0)){
        free(hashids);
        return make_error_tuple_from_string(env, "badarg");
    }
    hashids->separators_count = (size_t)separators_count;

    res = enif_inspect_binary(env, arr[8], &guards_bin);
    if(EHASHIDS_UNLIKELY(res == 0)){
        free(hashids);
        return make_error_tuple_from_string(env, "badarg");
    }

    res = enif_get_uint64(env, arr[9], &guards_count);
    if(EHASHIDS_UNLIKELY(res == 0)){
        free(hashids);
        return make_error_tuple_from_string(env, "badarg");
    }
    hashids->guards_count = (size_t)guards_count;

    res = enif_get_uint64(env, arr[10], &min_hash_length);
    if(EHASHIDS_UNLIKELY(res == 0)){
        free(hashids);
        return make_error_tuple_from_string(env, "badarg");
    }
    hashids->min_hash_length = (size_t)min_hash_length;

    hashids->alphabet = (char *)calloc(1, alphabet_bin.size);
    if(EHASHIDS_UNLIKELY(hashids->alphabet == NULL)){
        free(hashids);
        return make_error_tuple_from_string(env, "alloc");
    }

    hashids->alphabet_copy_1 = (char *)calloc(1, alphabet_copy1_bin.size);
    if(EHASHIDS_UNLIKELY(hashids->alphabet_copy_1 == NULL)){
        free(hashids->alphabet);
        free(hashids);
        return make_error_tuple_from_string(env, "alloc");
    }

    hashids->alphabet_copy_2 = (char *)calloc(1, alphabet_copy2_bin.size);
    if(EHASHIDS_UNLIKELY(hashids->alphabet_copy_2 == NULL)){
        free(hashids->alphabet_copy_1);
        free(hashids->alphabet);
        free(hashids);
        return make_error_tuple_from_string(env, "alloc");
    }

    hashids->salt = (char *)calloc(1, salt_bin.size);
    if(EHASHIDS_UNLIKELY(hashids->salt == NULL)){
        free(hashids->alphabet_copy_2);
        free(hashids->alphabet_copy_1);
        free(hashids->alphabet);
        free(hashids);
        return make_error_tuple_from_string(env, "alloc");
    }

    hashids->separators = (char *)calloc(1, separators_bin.size);
    if(EHASHIDS_UNLIKELY(hashids->separators == NULL)){
        free(hashids->salt);
        free(hashids->alphabet_copy_2);
        free(hashids->alphabet_copy_1);
        free(hashids->alphabet);
        free(hashids);
        return make_error_tuple_from_string(env, "alloc");
    }

    hashids->guards = (char *)calloc(1, guards_bin.size);
    if(EHASHIDS_UNLIKELY(hashids->guards == NULL)){
        free(hashids->separators);
        free(hashids->salt);
        free(hashids->alphabet_copy_2);
        free(hashids->alphabet_copy_1);
        free(hashids->alphabet);
        free(hashids);
        return make_error_tuple_from_string(env, "alloc");
    }

    (void)memcpy(hashids->alphabet, alphabet_bin.data, alphabet_bin.size);
    (void)memcpy(hashids->alphabet_copy_1, alphabet_copy1_bin.data, alphabet_copy1_bin.size);
    (void)memcpy(hashids->alphabet_copy_2, alphabet_copy2_bin.data, alphabet_copy2_bin.size);
    (void)memcpy(hashids->salt, salt_bin.data, salt_bin.size);
    (void)memcpy(hashids->separators, separators_bin.data, separators_bin.size);
    (void)memcpy(hashids->guards, guards_bin.data, guards_bin.size);

    resource_ref = (hashids_t **)enif_alloc_resource(hashids_type, sizeof(hashids_t *));
    *resource_ref = hashids;

    resource = enif_make_resource(env, resource_ref);
    enif_release_resource(resource_ref);

    return resource;
}

static ERL_NIF_TERM hashids_compile_nif(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    hashids_t **hashids = NULL;
    hashids_t *h;
    ErlNifBinary alphabet_bin;
    ERL_NIF_TERM alphabet_final_bin;
    ErlNifBinary alphabet_copy1_bin;
    ERL_NIF_TERM alphabet_copy1_final_bin;
    ErlNifBinary alphabet_copy2_bin;
    ERL_NIF_TERM alphabet_copy2_final_bin;
    size_t alphabet_length;
    ErlNifBinary salt_bin;
    ERL_NIF_TERM salt_final_bin;
    size_t salt_length;
    ErlNifBinary separators_bin;
    ERL_NIF_TERM separators_final_bin;
    size_t separators_count;
    ErlNifBinary guards_bin;
    ERL_NIF_TERM guards_final_bin;
    size_t guards_count;
    size_t min_hash_length;
    ERL_NIF_TERM arr[11];

    if(EHASHIDS_UNLIKELY(argc != 1)) return make_error_tuple_from_string(env, "arity");

    if(EHASHIDS_UNLIKELY(!enif_get_resource(env, argv[0], hashids_type, (void **)&hashids))) {
        return make_error_tuple_from_string(env, "bad_resource");
    }

    h = *hashids;

    alphabet_length = h->alphabet_length;
    salt_length = h->salt_length;
    separators_count = h->separators_count;
    guards_count = h->guards_count;
    min_hash_length = h->min_hash_length;

    if(EHASHIDS_UNLIKELY(enif_alloc_binary(alphabet_length + 1, &alphabet_bin) == 0)){
        return make_error_tuple_from_string(env, "alloc");
    }
    (void)memcpy(alphabet_bin.data, h->alphabet, alphabet_length + 1);

    if(EHASHIDS_UNLIKELY(enif_alloc_binary(alphabet_length + 1, &alphabet_copy1_bin) == 0)){
        enif_release_binary(&alphabet_bin);
        return make_error_tuple_from_string(env, "alloc");
    }
    (void)memcpy(alphabet_copy1_bin.data, h->alphabet_copy_1 , alphabet_length + 1);

    if(EHASHIDS_UNLIKELY(enif_alloc_binary(alphabet_length + 1, &alphabet_copy2_bin) == 0)){
        enif_release_binary(&alphabet_bin);
        enif_release_binary(&alphabet_copy1_bin);
        return make_error_tuple_from_string(env, "alloc");
    }
    (void)memcpy(alphabet_copy2_bin.data, h->alphabet_copy_2, alphabet_length + 1);

    if(EHASHIDS_UNLIKELY(enif_alloc_binary(salt_length + 1, &salt_bin) == 0)){
        enif_release_binary(&alphabet_bin);
        enif_release_binary(&alphabet_copy1_bin);
        enif_release_binary(&alphabet_copy2_bin);
        return make_error_tuple_from_string(env, "alloc");
    }
    (void)memcpy(salt_bin.data, h->salt, salt_length + 1);

    if(EHASHIDS_UNLIKELY(enif_alloc_binary(separators_count + 1, &separators_bin) == 0)){
        enif_release_binary(&alphabet_bin);
        enif_release_binary(&alphabet_copy1_bin);
        enif_release_binary(&alphabet_copy2_bin);
        enif_release_binary(&salt_bin);
        return make_error_tuple_from_string(env, "alloc");
    }
    (void)memcpy(separators_bin.data, h->separators, separators_count + 1);

    if(EHASHIDS_UNLIKELY(enif_alloc_binary(guards_count + 1, &guards_bin) == 0)){
        enif_release_binary(&alphabet_bin);
        enif_release_binary(&alphabet_copy1_bin);
        enif_release_binary(&alphabet_copy2_bin);
        enif_release_binary(&salt_bin);
        enif_release_binary(&separators_bin);
        return make_error_tuple_from_string(env, "alloc");
    }
    (void)memcpy(guards_bin.data, h->guards, guards_count + 1);

    alphabet_final_bin = enif_make_binary(env, &alphabet_bin);
    alphabet_copy1_final_bin = enif_make_binary(env, &alphabet_copy1_bin);
    alphabet_copy2_final_bin = enif_make_binary(env, &alphabet_copy2_bin);
    salt_final_bin = enif_make_binary(env, &salt_bin);
    separators_final_bin = enif_make_binary(env, &separators_bin);
    guards_final_bin = enif_make_binary(env, &guards_bin);

    arr[0] = alphabet_final_bin;
    arr[1] = alphabet_copy1_final_bin;
    arr[2] = alphabet_copy2_final_bin;
    arr[3] = enif_make_uint64(env, (uint64_t)alphabet_length);
    arr[4] = salt_final_bin;
    arr[5] = enif_make_uint64(env, (uint64_t)salt_length);
    arr[6] = separators_final_bin;
    arr[7] = enif_make_uint64(env, (uint64_t)separators_count);
    arr[8] = guards_final_bin;
    arr[9] = enif_make_uint64(env, (uint64_t)guards_count);
    arr[10] = enif_make_uint64(env, (uint64_t)min_hash_length);

    return enif_make_tuple2(
        env,
        make_atom(env, "ok"),
        enif_make_tuple_from_array(env, arr, 11)
    );
}

static ERL_NIF_TERM make_atom(ErlNifEnv* env, const char* atom)
{
    ERL_NIF_TERM ret;

    if(!enif_make_existing_atom(env, atom, &ret, ERL_NIF_LATIN1)){
        return enif_make_atom(env, atom);
    }

    return ret;
}

static ERL_NIF_TERM make_error_tuple_from_string(ErlNifEnv* env, const char *error)
{
    return make_error_tuple(env, make_atom(env, error));
}

static ERL_NIF_TERM make_error_tuple(ErlNifEnv* env, ERL_NIF_TERM error)
{
    return enif_make_tuple2(env, make_atom(env, "error"), error);
}

ERL_NIF_INIT(ehashids, nif_funcs, handle_load, NULL, handle_upgrade, NULL);
