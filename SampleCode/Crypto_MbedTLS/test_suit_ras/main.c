/**************************************************************************//**
 * @file     test_suit_des.c
 * @version  V1.00
 * $Date: 15/05/06 9:54a $
 * @brief    mbedtls DES test suit
 *
 * @note
 * Copyright (C) 2017 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include "mbedtls/platform.h"

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
#endif

#include <stdint.h>
#include <string.h>

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
#endif

#include <stdint.h>
#include <string.h>

#include "nuc980.h"
#include "sys.h"
#include "etimer.h"


/*----------------------------------------------------------------------------*/
/* Constants */

#define DEPENDENCY_SUPPORTED          0
#define DEPENDENCY_NOT_SUPPORTED      1

#define KEY_VALUE_MAPPING_FOUND       0
#define KEY_VALUE_MAPPING_NOT_FOUND   -1

#define DISPATCH_TEST_SUCCESS         0
#define DISPATCH_TEST_FN_NOT_FOUND    1
#define DISPATCH_INVALID_TEST_DATA    2
#define DISPATCH_UNSUPPORTED_SUITE    3


/*----------------------------------------------------------------------------*/
/* Macros */

#define TEST_ASSERT( TEST )                         \
    do {                                            \
        if( ! (TEST) )                              \
        {                                           \
            test_fail( #TEST, __LINE__, __FILE__ ); \
            goto exit;                              \
        }                                           \
    } while( 0 )

#define assert(a) if( !( a ) )                              \
{                                                           \
    printf("Assertion Failed at %s:%d - %s\n",           \
                             __FILE__, __LINE__, #a );      \
    while ( 1 );                                      \
}

/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n,b,i)                            \
{                                                       \
    (n) = ( (uint32_t) (b)[(i)    ] << 24 )             \
        | ( (uint32_t) (b)[(i) + 1] << 16 )             \
        | ( (uint32_t) (b)[(i) + 2] <<  8 )             \
        | ( (uint32_t) (b)[(i) + 3]       );            \
}
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)                            \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
}
#endif


/*----------------------------------------------------------------------------*/
/* Global variables */

UINT8     *file_base_ptr;
UINT32    g_file_idx, g_file_size;


static char  g_line_buff[4096];

static int test_errors = 0;

extern UINT32  VectorBase_RSA, VectorLimit_RSA;

int  open_test_vector(int vector_no)
{
    g_file_idx = 0;

    if (vector_no == 1)
    {
        printf("\n\nOpen test vector test_suite_rsa.data.\n");
        file_base_ptr = (UINT8 *)&VectorBase_RSA;
        g_file_size = (UINT32)&VectorLimit_RSA - (UINT32)&VectorBase_RSA;
        return 0;
    }
    return -1;
}

static int  read_file(UINT8 *pu8Buff, int i32Len)
{
    if (g_file_idx+1 >= g_file_size)
        return -1;
    memcpy(pu8Buff, &file_base_ptr[g_file_idx], i32Len);
    g_file_idx += i32Len;
    return 0;
}

int  is_eof(void)
{
    if (g_file_idx+1 >= g_file_size)
    {
        //printf("EOF.\n");
        return 1;     /* is EOF  */
    }
    else
        return 0;     /* not EOF */
}

int  get_line(void)
{
    int         i;
    uint8_t     ch;

    if (g_file_idx+1 >= g_file_size)
    {
        //printf("EOF.\n");
        return -1;
    }

    memset(g_line_buff, 0, sizeof(g_line_buff));

    for (i = 0;;)
    {
        if (read_file(&ch, 1) < 0)
            return 0;

        if ((ch == 0x0D) || (ch == 0x0A))
            break;

        g_line_buff[i++] = ch;
    }

    while (1)
    {
        if (read_file(&ch, 1) < 0)
            return 0;

        if ((ch != 0x0D) && (ch != 0x0A))
            break;
    }
    g_file_idx--;
    return 0;
}


static int unhexify( unsigned char *obuf, const char *ibuf )
{
    unsigned char c, c2;
    int len = strlen( ibuf ) / 2;
    assert( strlen( ibuf ) % 2 == 0 ); /* must be even number of bytes */

    while( *ibuf != 0 )
    {
        c = *ibuf++;
        if( c >= '0' && c <= '9' )
            c -= '0';
        else if( c >= 'a' && c <= 'f' )
            c -= 'a' - 10;
        else if( c >= 'A' && c <= 'F' )
            c -= 'A' - 10;
        else
            assert( 0 );

        c2 = *ibuf++;
        if( c2 >= '0' && c2 <= '9' )
            c2 -= '0';
        else if( c2 >= 'a' && c2 <= 'f' )
            c2 -= 'a' - 10;
        else if( c2 >= 'A' && c2 <= 'F' )
            c2 -= 'A' - 10;
        else
            assert( 0 );

        *obuf++ = ( c << 4 ) | c2;
    }

    return len;
}

static void hexify( unsigned char *obuf, const unsigned char *ibuf, int len )
{
    unsigned char l, h;

    while( len != 0 )
    {
        h = *ibuf / 16;
        l = *ibuf % 16;

        if( h < 10 )
            *obuf++ = '0' + h;
        else
            *obuf++ = 'a' + h - 10;

        if( l < 10 )
            *obuf++ = '0' + l;
        else
            *obuf++ = 'a' + l - 10;

        ++ibuf;
        len--;
    }
}

void print_hex(uint8_t *hex, int len)
{
    int  i;

    for (i = 0; i < len; i++)
        printf("%x%x ", (hex[i]>>4) & 0xf, hex[i]&0xf);
    printf("\n");
}


/**
 * This function just returns data from rand().
 * Although predictable and often similar on multiple
 * runs, this does not result in identical random on
 * each run. So do not use this if the results of a
 * test depend on the random data that is generated.
 *
 * rng_state shall be NULL.
 */
static int rnd_std_rand( void *rng_state, unsigned char *output, size_t len )
{
#if !defined(__OpenBSD__)
    size_t i;

    if( rng_state != NULL )
        rng_state  = NULL;

    for( i = 0; i < len; ++i )
        output[i] = rand();
#else
    if( rng_state != NULL )
        rng_state = NULL;

    arc4random_buf( output, len );
#endif /* !OpenBSD */

    return( 0 );
}

/**
 * This function only returns zeros
 *
 * rng_state shall be NULL.
 */
static int rnd_zero_rand( void *rng_state, unsigned char *output, size_t len )
{
    if( rng_state != NULL )
        rng_state  = NULL;

    memset( output, 0, len );

    return( 0 );
}


/**
 * Info structure for the pseudo random function
 *
 * Key should be set at the start to a test-unique value.
 * Do not forget endianness!
 * State( v0, v1 ) should be set to zero.
 */
typedef struct
{
    uint32_t key[16];
    uint32_t v0, v1;
} rnd_pseudo_info;

/**
 * This function returns random based on a pseudo random function.
 * This means the results should be identical on all systems.
 * Pseudo random is based on the XTEA encryption algorithm to
 * generate pseudorandom.
 *
 * rng_state shall be a pointer to a rnd_pseudo_info structure.
 */
static int rnd_pseudo_rand( void *rng_state, unsigned char *output, size_t len )
{
    rnd_pseudo_info *info = (rnd_pseudo_info *) rng_state;
    uint32_t i, *k, sum, delta=0x9E3779B9;
    unsigned char result[4], *out = output;

    if( rng_state == NULL )
        return( rnd_std_rand( NULL, output, len ) );

    k = info->key;

    while( len > 0 )
    {
        size_t use_len = ( len > 4 ) ? 4 : len;
        sum = 0;

        for( i = 0; i < 32; i++ )
        {
            info->v0 += ( ( ( info->v1 << 4 ) ^ ( info->v1 >> 5 ) )
                          + info->v1 ) ^ ( sum + k[sum & 3] );
            sum += delta;
            info->v1 += ( ( ( info->v0 << 4 ) ^ ( info->v0 >> 5 ) )
                          + info->v0 ) ^ ( sum + k[( sum>>11 ) & 3] );
        }

        PUT_UINT32_BE( info->v0, result, 0 );
        memcpy( out, result, use_len );
        len -= use_len;
        out += 4;
    }

    return( 0 );
}

static void test_fail( const char *test, int line_no, const char* filename )
{
    test_errors++;
    if( test_errors == 1 )
        printf("FAILED\n" );
    printf("  %s\n  at line %d, %s\n", test, line_no, filename );
    while (1);
}


/*----------------------------------------------------------------------------*/
/* Test Suite Code */

#if defined(MBEDTLS_RSA_C)
#if defined(MBEDTLS_BIGNUM_C)
#if defined(MBEDTLS_GENPRIME)

#include "mbedtls/rsa.h"
#include "mbedtls/md2.h"
#include "mbedtls/md4.h"
#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#endif /* defined(MBEDTLS_RSA_C) */
#endif /* defined(MBEDTLS_BIGNUM_C) */
#endif /* defined(MBEDTLS_GENPRIME) */


#if defined(MBEDTLS_RSA_C)
#if defined(MBEDTLS_BIGNUM_C)
#if defined(MBEDTLS_GENPRIME)

#define TEST_SUITE_ACTIVE

int verify_string( char **str )
{
    if( (*str)[0] != '"' ||
            (*str)[strlen( *str ) - 1] != '"' )
    {
        printf("Expected string (with \"\") for parameter and got: %s\n", *str );
        return( -1 );
    }

    (*str)++;
    (*str)[strlen( *str ) - 1] = '\0';

    return( 0 );
}

int verify_int( char *str, int *value )
{
    size_t i;
    int minus = 0;
    int digits = 1;
    int hex = 0;

    for( i = 0; i < strlen( str ); i++ )
    {
        if( i == 0 && str[i] == '-' )
        {
            minus = 1;
            continue;
        }

        if( ( ( minus && i == 2 ) || ( !minus && i == 1 ) ) &&
                str[i - 1] == '0' && str[i] == 'x' )
        {
            hex = 1;
            continue;
        }

        if( ! ( ( str[i] >= '0' && str[i] <= '9' ) ||
                ( hex && ( ( str[i] >= 'a' && str[i] <= 'f' ) ||
                           ( str[i] >= 'A' && str[i] <= 'F' ) ) ) ) )
        {
            digits = 0;
            break;
        }
    }

    if( digits )
    {
        if( hex )
            *value = strtol( str, NULL, 16 );
        else
            *value = strtol( str, NULL, 10 );

        return( 0 );
    }

    if( strcmp( str, "MBEDTLS_MD_SHA256" ) == 0 )
    {
        *value = ( MBEDTLS_MD_SHA256 );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_ERR_RSA_PUBLIC_FAILED + MBEDTLS_ERR_MPI_BAD_INPUT_DATA" ) == 0 )
    {
        *value = ( MBEDTLS_ERR_RSA_PUBLIC_FAILED + MBEDTLS_ERR_MPI_BAD_INPUT_DATA );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_MD_MD4" ) == 0 )
    {
        *value = ( MBEDTLS_MD_MD4 );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_ERR_RSA_BAD_INPUT_DATA" ) == 0 )
    {
        *value = ( MBEDTLS_ERR_RSA_BAD_INPUT_DATA );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_ERR_RSA_OUTPUT_TOO_LARGE" ) == 0 )
    {
        *value = ( MBEDTLS_ERR_RSA_OUTPUT_TOO_LARGE );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_ERR_RSA_PRIVATE_FAILED + MBEDTLS_ERR_MPI_BAD_INPUT_DATA" ) == 0 )
    {
        *value = ( MBEDTLS_ERR_RSA_PRIVATE_FAILED + MBEDTLS_ERR_MPI_BAD_INPUT_DATA );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_MD_SHA224" ) == 0 )
    {
        *value = ( MBEDTLS_MD_SHA224 );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_MD_SHA512" ) == 0 )
    {
        *value = ( MBEDTLS_MD_SHA512 );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_MD_MD5" ) == 0 )
    {
        *value = ( MBEDTLS_MD_MD5 );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_ERR_RSA_INVALID_PADDING" ) == 0 )
    {
        *value = ( MBEDTLS_ERR_RSA_INVALID_PADDING );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_ERR_RSA_KEY_CHECK_FAILED" ) == 0 )
    {
        *value = ( MBEDTLS_ERR_RSA_KEY_CHECK_FAILED );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_RSA_PKCS_V15" ) == 0 )
    {
        *value = ( MBEDTLS_RSA_PKCS_V15 );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_ERR_RSA_RNG_FAILED" ) == 0 )
    {
        *value = ( MBEDTLS_ERR_RSA_RNG_FAILED );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_MD_SHA384" ) == 0 )
    {
        *value = ( MBEDTLS_MD_SHA384 );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_ERR_RSA_VERIFY_FAILED" ) == 0 )
    {
        *value = ( MBEDTLS_ERR_RSA_VERIFY_FAILED );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_MD_MD2" ) == 0 )
    {
        *value = ( MBEDTLS_MD_MD2 );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_MD_SHA1" ) == 0 )
    {
        *value = ( MBEDTLS_MD_SHA1 );
        return( KEY_VALUE_MAPPING_FOUND );
    }

    printf("Expected integer for parameter and got: %s\n", str );
    return( KEY_VALUE_MAPPING_NOT_FOUND );
}


/*----------------------------------------------------------------------------*/
/* Test Case code */

void test_suite_mbedtls_rsa_pkcs1_sign( char *message_hex_string, int padding_mode, int digest,
                                        int mod, int radix_P, char *input_P, int radix_Q,
                                        char *input_Q, int radix_N, char *input_N, int radix_E,
                                        char *input_E, char *result_hex_str, int result )
{
    unsigned char message_str[1000];
    unsigned char hash_result[1000];
    unsigned char output[1000];
    unsigned char output_str[1000];
    mbedtls_rsa_context ctx;
    mbedtls_mpi P1, Q1, H, G;
    int msg_len;
    rnd_pseudo_info rnd_info;

    mbedtls_mpi_init( &P1 );
    mbedtls_mpi_init( &Q1 );
    mbedtls_mpi_init( &H );
    mbedtls_mpi_init( &G );
    mbedtls_rsa_init( &ctx, padding_mode, 0 );

    memset( message_str, 0x00, 1000 );
    memset( hash_result, 0x00, 1000 );
    memset( output, 0x00, 1000 );
    memset( output_str, 0x00, 1000 );
    memset( &rnd_info, 0, sizeof( rnd_pseudo_info ) );

    RSA_claim_bit_length(mod);
    ctx.len = mod / 8;

    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.P, radix_P, input_P ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.Q, radix_Q, input_Q ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.E, radix_E, input_E ) == 0 );
    TEST_ASSERT( mbedtls_mpi_sub_int( &P1, &ctx.P, 1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_sub_int( &Q1, &ctx.Q, 1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_mul_mpi( &H, &P1, &Q1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_gcd( &G, &ctx.E, &H  ) == 0 );
    TEST_ASSERT( mbedtls_mpi_inv_mod( &ctx.D, &ctx.E, &H  ) == 0 );
    TEST_ASSERT( mbedtls_mpi_mod_mpi( &ctx.DP, &ctx.D, &P1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_mod_mpi( &ctx.DQ, &ctx.D, &Q1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_inv_mod( &ctx.QP, &ctx.Q, &ctx.P ) == 0 );

    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == 0 );

    msg_len = unhexify( message_str, message_hex_string );

    if( mbedtls_md_info_from_type( (mbedtls_md_type_t)digest ) != NULL )
        TEST_ASSERT( mbedtls_md( mbedtls_md_info_from_type( (mbedtls_md_type_t)digest ), message_str, msg_len, hash_result ) == 0 );
    TEST_ASSERT( mbedtls_rsa_pkcs1_sign( &ctx, &rnd_pseudo_rand, &rnd_info, MBEDTLS_RSA_PRIVATE, (mbedtls_md_type_t)digest, 0, hash_result, output ) == result );
    if( result == 0 )
    {
        hexify( output_str, output, ctx.len );

        TEST_ASSERT( strcasecmp( (char *) output_str, result_hex_str ) == 0 );
    }

exit:
    mbedtls_mpi_free( &P1 );
    mbedtls_mpi_free( &Q1 );
    mbedtls_mpi_free( &H );
    mbedtls_mpi_free( &G );
    mbedtls_rsa_free( &ctx );
}

void test_suite_mbedtls_rsa_pkcs1_verify( char *message_hex_string, int padding_mode, int digest,
        int mod, int radix_N, char *input_N, int radix_E,
        char *input_E, char *result_hex_str, int result )
{
    unsigned char message_str[1000];
    unsigned char hash_result[1000];
    unsigned char result_str[1000];
    mbedtls_rsa_context ctx;
    int msg_len;

    mbedtls_rsa_init( &ctx, padding_mode, 0 );
    memset( message_str, 0x00, 1000 );
    memset( hash_result, 0x00, 1000 );
    memset( result_str, 0x00, 1000 );

    RSA_claim_bit_length(mod);
    ctx.len = mod / 8;
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == 0 );

    msg_len = unhexify( message_str, message_hex_string );
    unhexify( result_str, result_hex_str );

    if( mbedtls_md_info_from_type( (mbedtls_md_type_t)digest ) != NULL )
        TEST_ASSERT( mbedtls_md( mbedtls_md_info_from_type( (mbedtls_md_type_t)digest ), message_str, msg_len, hash_result ) == 0 );

    TEST_ASSERT( mbedtls_rsa_pkcs1_verify( &ctx, NULL, NULL, MBEDTLS_RSA_PUBLIC, (mbedtls_md_type_t)digest, 0, hash_result, result_str ) == result );

exit:
    mbedtls_rsa_free( &ctx );
}

void test_suite_rsa_pkcs1_sign_raw( char *message_hex_string, char *hash_result_string,
                                    int padding_mode, int mod, int radix_P, char *input_P,
                                    int radix_Q, char *input_Q, int radix_N,
                                    char *input_N, int radix_E, char *input_E,
                                    char *result_hex_str )
{
    unsigned char message_str[1000];
    unsigned char hash_result[1000];
    unsigned char output[1000];
    unsigned char output_str[1000];
    mbedtls_rsa_context ctx;
    mbedtls_mpi P1, Q1, H, G;
    int hash_len;
    rnd_pseudo_info rnd_info;

    mbedtls_mpi_init( &P1 );
    mbedtls_mpi_init( &Q1 );
    mbedtls_mpi_init( &H );
    mbedtls_mpi_init( &G );
    mbedtls_rsa_init( &ctx, padding_mode, 0 );

    memset( message_str, 0x00, 1000 );
    memset( hash_result, 0x00, 1000 );
    memset( output, 0x00, 1000 );
    memset( output_str, 0x00, 1000 );
    memset( &rnd_info, 0, sizeof( rnd_pseudo_info ) );

    RSA_claim_bit_length(mod);
    ctx.len = mod / 8;
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.P, radix_P, input_P ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.Q, radix_Q, input_Q ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_mpi_sub_int( &P1, &ctx.P, 1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_sub_int( &Q1, &ctx.Q, 1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_mul_mpi( &H, &P1, &Q1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_gcd( &G, &ctx.E, &H  ) == 0 );
    TEST_ASSERT( mbedtls_mpi_inv_mod( &ctx.D, &ctx.E, &H  ) == 0 );
    TEST_ASSERT( mbedtls_mpi_mod_mpi( &ctx.DP, &ctx.D, &P1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_mod_mpi( &ctx.DQ, &ctx.D, &Q1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_inv_mod( &ctx.QP, &ctx.Q, &ctx.P ) == 0 );

    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == 0 );

    unhexify( message_str, message_hex_string );
    hash_len = unhexify( hash_result, hash_result_string );

    TEST_ASSERT( mbedtls_rsa_pkcs1_sign( &ctx, &rnd_pseudo_rand, &rnd_info, MBEDTLS_RSA_PRIVATE, MBEDTLS_MD_NONE, hash_len, hash_result, output ) == 0 );

    hexify( output_str, output, ctx.len );

    TEST_ASSERT( strcasecmp( (char *) output_str, result_hex_str ) == 0 );

    /* For PKCS#1 v1.5, there is an alternative way to generate signatures */
    if( padding_mode == MBEDTLS_RSA_PKCS_V15 )
    {
        memset( output, 0x00, 1000 );
        memset( output_str, 0x00, 1000 );

        TEST_ASSERT( mbedtls_rsa_rsaes_pkcs1_v15_encrypt( &ctx,
                     &rnd_pseudo_rand, &rnd_info, MBEDTLS_RSA_PRIVATE,
                     hash_len, hash_result, output ) == 0 );

        hexify( output_str, output, ctx.len );

        TEST_ASSERT( strcasecmp( (char *) output_str, result_hex_str ) == 0 );
    }

exit:
    mbedtls_mpi_free( &P1 );
    mbedtls_mpi_free( &Q1 );
    mbedtls_mpi_free( &H );
    mbedtls_mpi_free( &G );
    mbedtls_rsa_free( &ctx );
}

void test_suite_rsa_pkcs1_verify_raw( char *message_hex_string, char *hash_result_string,
                                      int padding_mode, int mod, int radix_N,
                                      char *input_N, int radix_E, char *input_E,
                                      char *result_hex_str, int correct )
{
    unsigned char message_str[1000];
    unsigned char hash_result[1000];
    unsigned char result_str[1000];
    unsigned char output[1000];
    mbedtls_rsa_context ctx;
    size_t hash_len, olen;

    mbedtls_rsa_init( &ctx, padding_mode, 0 );
    memset( message_str, 0x00, 1000 );
    memset( hash_result, 0x00, 1000 );
    memset( result_str, 0x00, 1000 );
    memset( output, 0x00, sizeof( output ) );

    RSA_claim_bit_length(mod);
    ctx.len = mod / 8;
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == 0 );

    unhexify( message_str, message_hex_string );
    hash_len = unhexify( hash_result, hash_result_string );
    unhexify( result_str, result_hex_str );

    TEST_ASSERT( mbedtls_rsa_pkcs1_verify( &ctx, NULL, NULL, MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_NONE, hash_len, hash_result, result_str ) == correct );

    /* For PKCS#1 v1.5, there is an alternative way to verify signatures */
    if( padding_mode == MBEDTLS_RSA_PKCS_V15 )
    {
        int ok;

        TEST_ASSERT( mbedtls_rsa_rsaes_pkcs1_v15_decrypt( &ctx,
                     NULL, NULL, MBEDTLS_RSA_PUBLIC,
                     &olen, result_str, output, sizeof( output ) ) == 0 );

        ok = olen == hash_len && memcmp( output, hash_result, olen ) == 0;
        if( correct == 0 )
            TEST_ASSERT( ok == 1 );
        else
            TEST_ASSERT( ok == 0 );
    }

exit:
    mbedtls_rsa_free( &ctx );
}

void test_suite_mbedtls_rsa_pkcs1_encrypt( char *message_hex_string, int padding_mode, int mod,
        int radix_N, char *input_N, int radix_E, char *input_E,
        char *result_hex_str, int result )
{
    unsigned char message_str[1000];
    unsigned char output[1000];
    unsigned char output_str[1000];
    mbedtls_rsa_context ctx;
    size_t msg_len;
    rnd_pseudo_info rnd_info;

    memset( &rnd_info, 0, sizeof( rnd_pseudo_info ) );

    mbedtls_rsa_init( &ctx, padding_mode, 0 );
    memset( message_str, 0x00, 1000 );
    memset( output, 0x00, 1000 );
    memset( output_str, 0x00, 1000 );

    RSA_claim_bit_length(mod);
    ctx.len = mod / 8;
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == 0 );

    msg_len = unhexify( message_str, message_hex_string );

    TEST_ASSERT( mbedtls_rsa_pkcs1_encrypt( &ctx, &rnd_pseudo_rand, &rnd_info, MBEDTLS_RSA_PUBLIC, msg_len, message_str, output ) == result );
    if( result == 0 )
    {
        hexify( output_str, output, ctx.len );

        TEST_ASSERT( strcasecmp( (char *) output_str, result_hex_str ) == 0 );
    }

exit:
    mbedtls_rsa_free( &ctx );
}

void test_suite_rsa_pkcs1_encrypt_bad_rng( char *message_hex_string, int padding_mode,
        int mod, int radix_N, char *input_N,
        int radix_E, char *input_E,
        char *result_hex_str, int result )
{
    unsigned char message_str[1000];
    unsigned char output[1000];
    unsigned char output_str[1000];
    mbedtls_rsa_context ctx;
    size_t msg_len;

    mbedtls_rsa_init( &ctx, padding_mode, 0 );
    memset( message_str, 0x00, 1000 );
    memset( output, 0x00, 1000 );
    memset( output_str, 0x00, 1000 );

    RSA_claim_bit_length(mod);
    ctx.len = mod / 8;
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == 0 );

    msg_len = unhexify( message_str, message_hex_string );

    TEST_ASSERT( mbedtls_rsa_pkcs1_encrypt( &ctx, &rnd_zero_rand, NULL, MBEDTLS_RSA_PUBLIC, msg_len, message_str, output ) == result );
    if( result == 0 )
    {
        hexify( output_str, output, ctx.len );

        TEST_ASSERT( strcasecmp( (char *) output_str, result_hex_str ) == 0 );
    }

exit:
    mbedtls_rsa_free( &ctx );
}

void test_suite_mbedtls_rsa_pkcs1_decrypt( char *message_hex_string, int padding_mode, int mod,
        int radix_P, char *input_P, int radix_Q, char *input_Q,
        int radix_N, char *input_N, int radix_E, char *input_E,
        int max_output, char *result_hex_str, int result )
{
    unsigned char message_str[1000];
    unsigned char output[1000];
    unsigned char output_str[1000];
    mbedtls_rsa_context ctx;
    mbedtls_mpi P1, Q1, H, G;
    size_t output_len;
    rnd_pseudo_info rnd_info;

    mbedtls_mpi_init( &P1 );
    mbedtls_mpi_init( &Q1 );
    mbedtls_mpi_init( &H );
    mbedtls_mpi_init( &G );
    mbedtls_rsa_init( &ctx, padding_mode, 0 );

    memset( message_str, 0x00, 1000 );
    memset( output, 0x00, 1000 );
    memset( output_str, 0x00, 1000 );
    memset( &rnd_info, 0, sizeof( rnd_pseudo_info ) );

    RSA_claim_bit_length(mod);
    ctx.len = mod / 8;
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.P, radix_P, input_P ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.Q, radix_Q, input_Q ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_mpi_sub_int( &P1, &ctx.P, 1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_sub_int( &Q1, &ctx.Q, 1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_mul_mpi( &H, &P1, &Q1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_gcd( &G, &ctx.E, &H  ) == 0 );
    TEST_ASSERT( mbedtls_mpi_inv_mod( &ctx.D, &ctx.E, &H  ) == 0 );
    TEST_ASSERT( mbedtls_mpi_mod_mpi( &ctx.DP, &ctx.D, &P1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_mod_mpi( &ctx.DQ, &ctx.D, &Q1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_inv_mod( &ctx.QP, &ctx.Q, &ctx.P ) == 0 );

    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == 0 );

    unhexify( message_str, message_hex_string );
    output_len = 0;

    TEST_ASSERT( mbedtls_rsa_pkcs1_decrypt( &ctx, rnd_pseudo_rand, &rnd_info, MBEDTLS_RSA_PRIVATE, &output_len, message_str, output, max_output ) == result );
    if( result == 0 )
    {
        hexify( output_str, output, ctx.len );

        TEST_ASSERT( strncasecmp( (char *) output_str, result_hex_str, strlen( result_hex_str ) ) == 0 );
    }

exit:
    mbedtls_mpi_free( &P1 );
    mbedtls_mpi_free( &Q1 );
    mbedtls_mpi_free( &H );
    mbedtls_mpi_free( &G );
    mbedtls_rsa_free( &ctx );
}

void test_suite_mbedtls_rsa_public( char *message_hex_string, int mod, int radix_N, char *input_N,
                                    int radix_E, char *input_E, char *result_hex_str, int result )
{
    unsigned char message_str[1000];
    unsigned char output[1000];
    unsigned char output_str[1000];
    mbedtls_rsa_context ctx, ctx2; /* Also test mbedtls_rsa_copy() while at it */

    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, 0 );
    mbedtls_rsa_init( &ctx2, MBEDTLS_RSA_PKCS_V15, 0 );
    memset( message_str, 0x00, 1000 );
    memset( output, 0x00, 1000 );
    memset( output_str, 0x00, 1000 );

    RSA_claim_bit_length(mod);
    ctx.len = mod / 8;
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == 0 );

    unhexify( message_str, message_hex_string );

    TEST_ASSERT( mbedtls_rsa_public( &ctx, message_str, output ) == result );
    if( result == 0 )
    {
        hexify( output_str, output, ctx.len );

        TEST_ASSERT( strcasecmp( (char *) output_str, result_hex_str ) == 0 );
    }

    /* And now with the copy */
    TEST_ASSERT( mbedtls_rsa_copy( &ctx2, &ctx ) == 0 );
    /* clear the original to be sure */
    mbedtls_rsa_free( &ctx );

    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx2 ) == 0 );

    memset( output, 0x00, 1000 );
    memset( output_str, 0x00, 1000 );
    TEST_ASSERT( mbedtls_rsa_public( &ctx2, message_str, output ) == result );
    if( result == 0 )
    {
        hexify( output_str, output, ctx2.len );

        TEST_ASSERT( strcasecmp( (char *) output_str, result_hex_str ) == 0 );
    }

exit:
    mbedtls_rsa_free( &ctx );
    mbedtls_rsa_free( &ctx2 );
}

void test_suite_mbedtls_rsa_private( char *message_hex_string, int mod, int radix_P, char *input_P,
                                     int radix_Q, char *input_Q, int radix_N, char *input_N,
                                     int radix_E, char *input_E, char *result_hex_str, int result )
{
    unsigned char message_str[1000];
    unsigned char output[1000];
    unsigned char output_str[1000];
    mbedtls_rsa_context ctx, ctx2; /* Also test mbedtls_rsa_copy() while at it */
    mbedtls_mpi P1, Q1, H, G;
    rnd_pseudo_info rnd_info;
    int i;

    mbedtls_mpi_init( &P1 );
    mbedtls_mpi_init( &Q1 );
    mbedtls_mpi_init( &H );
    mbedtls_mpi_init( &G );
    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, 0 );
    mbedtls_rsa_init( &ctx2, MBEDTLS_RSA_PKCS_V15, 0 );

    memset( message_str, 0x00, 1000 );
    memset( &rnd_info, 0, sizeof( rnd_pseudo_info ) );

    RSA_claim_bit_length(mod);
    ctx.len = mod / 8;
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.P, radix_P, input_P ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.Q, radix_Q, input_Q ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_mpi_sub_int( &P1, &ctx.P, 1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_sub_int( &Q1, &ctx.Q, 1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_mul_mpi( &H, &P1, &Q1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_gcd( &G, &ctx.E, &H  ) == 0 );
    TEST_ASSERT( mbedtls_mpi_inv_mod( &ctx.D, &ctx.E, &H  ) == 0 );
    TEST_ASSERT( mbedtls_mpi_mod_mpi( &ctx.DP, &ctx.D, &P1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_mod_mpi( &ctx.DQ, &ctx.D, &Q1 ) == 0 );
    TEST_ASSERT( mbedtls_mpi_inv_mod( &ctx.QP, &ctx.Q, &ctx.P ) == 0 );

    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == 0 );

    unhexify( message_str, message_hex_string );

    /* repeat three times to test updating of blinding values */
    for( i = 0; i < 3; i++ )
    {
        memset( output, 0x00, 1000 );
        memset( output_str, 0x00, 1000 );
        TEST_ASSERT( mbedtls_rsa_private( &ctx, rnd_pseudo_rand, &rnd_info,
                                          message_str, output ) == result );
        if( result == 0 )
        {
            hexify( output_str, output, ctx.len );

            TEST_ASSERT( strcasecmp( (char *) output_str,
                                     result_hex_str ) == 0 );
        }
    }

    /* And now one more time with the copy */
    TEST_ASSERT( mbedtls_rsa_copy( &ctx2, &ctx ) == 0 );
    /* clear the original to be sure */
    mbedtls_rsa_free( &ctx );

    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx2 ) == 0 );

    memset( output, 0x00, 1000 );
    memset( output_str, 0x00, 1000 );
    TEST_ASSERT( mbedtls_rsa_private( &ctx2, rnd_pseudo_rand, &rnd_info,
                                      message_str, output ) == result );
    if( result == 0 )
    {
        hexify( output_str, output, ctx2.len );

        TEST_ASSERT( strcasecmp( (char *) output_str,
                                 result_hex_str ) == 0 );
    }

exit:
    mbedtls_mpi_free( &P1 );
    mbedtls_mpi_free( &Q1 );
    mbedtls_mpi_free( &H );
    mbedtls_mpi_free( &G );
    mbedtls_rsa_free( &ctx );
    mbedtls_rsa_free( &ctx2 );
}

void test_suite_rsa_check_privkey_null()
{
    mbedtls_rsa_context ctx;
    memset( &ctx, 0x00, sizeof( mbedtls_rsa_context ) );

    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == MBEDTLS_ERR_RSA_KEY_CHECK_FAILED );

exit:
    return;
}

void test_suite_mbedtls_rsa_check_pubkey( int radix_N, char *input_N, int radix_E, char *input_E,
        int result )
{
    mbedtls_rsa_context ctx;

    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, 0 );

    if( strlen( input_N ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &ctx.N, radix_N, input_N ) == 0 );
    }
    if( strlen( input_E ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &ctx.E, radix_E, input_E ) == 0 );
    }

    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == result );

exit:
    mbedtls_rsa_free( &ctx );
}

void test_suite_mbedtls_rsa_check_privkey( int mod, int radix_P, char *input_P, int radix_Q,
        char *input_Q, int radix_N, char *input_N,
        int radix_E, char *input_E, int radix_D, char *input_D,
        int radix_DP, char *input_DP, int radix_DQ,
        char *input_DQ, int radix_QP, char *input_QP,
        int result )
{
    mbedtls_rsa_context ctx;

    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, 0 );

    RSA_claim_bit_length(mod);
    ctx.len = mod / 8;
    if( strlen( input_P ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &ctx.P, radix_P, input_P ) == 0 );
    }
    if( strlen( input_Q ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &ctx.Q, radix_Q, input_Q ) == 0 );
    }
    if( strlen( input_N ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &ctx.N, radix_N, input_N ) == 0 );
    }
    if( strlen( input_E ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &ctx.E, radix_E, input_E ) == 0 );
    }
    if( strlen( input_D ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &ctx.D, radix_D, input_D ) == 0 );
    }
    if( strlen( input_DP ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &ctx.DP, radix_DP, input_DP ) == 0 );
    }
    if( strlen( input_DQ ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &ctx.DQ, radix_DQ, input_DQ ) == 0 );
    }
    if( strlen( input_QP ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &ctx.QP, radix_QP, input_QP ) == 0 );
    }

    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == result );

exit:
    mbedtls_rsa_free( &ctx );
}

void test_suite_rsa_check_pubpriv( int mod, int radix_Npub, char *input_Npub,
                                   int radix_Epub, char *input_Epub,
                                   int radix_P, char *input_P, int radix_Q,
                                   char *input_Q, int radix_N, char *input_N,
                                   int radix_E, char *input_E, int radix_D, char *input_D,
                                   int radix_DP, char *input_DP, int radix_DQ,
                                   char *input_DQ, int radix_QP, char *input_QP,
                                   int result )
{
    mbedtls_rsa_context pub, prv;

    mbedtls_rsa_init( &pub, MBEDTLS_RSA_PKCS_V15, 0 );
    mbedtls_rsa_init( &prv, MBEDTLS_RSA_PKCS_V15, 0 );

    pub.len = mod / 8;
    prv.len = mod / 8;

    if( strlen( input_Npub ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &pub.N, radix_Npub, input_Npub ) == 0 );
    }
    if( strlen( input_Epub ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &pub.E, radix_Epub, input_Epub ) == 0 );
    }

    if( strlen( input_P ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &prv.P, radix_P, input_P ) == 0 );
    }
    if( strlen( input_Q ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &prv.Q, radix_Q, input_Q ) == 0 );
    }
    if( strlen( input_N ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &prv.N, radix_N, input_N ) == 0 );
    }
    if( strlen( input_E ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &prv.E, radix_E, input_E ) == 0 );
    }
    if( strlen( input_D ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &prv.D, radix_D, input_D ) == 0 );
    }
    if( strlen( input_DP ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &prv.DP, radix_DP, input_DP ) == 0 );
    }
    if( strlen( input_DQ ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &prv.DQ, radix_DQ, input_DQ ) == 0 );
    }
    if( strlen( input_QP ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &prv.QP, radix_QP, input_QP ) == 0 );
    }

    TEST_ASSERT( mbedtls_rsa_check_pub_priv( &pub, &prv ) == result );

exit:
    mbedtls_rsa_free( &pub );
    mbedtls_rsa_free( &prv );
}

#ifdef MBEDTLS_CTR_DRBG_C
#ifdef MBEDTLS_ENTROPY_C
void test_suite_mbedtls_rsa_gen_key( int nrbits, int exponent, int result)
{
    mbedtls_rsa_context ctx;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "test_suite_rsa";

    mbedtls_ctr_drbg_init( &ctr_drbg );

    mbedtls_entropy_init( &entropy );
    TEST_ASSERT( mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                                        (const unsigned char *) pers, strlen( pers ) ) == 0 );

    mbedtls_rsa_init( &ctx, 0, 0 );

    TEST_ASSERT( mbedtls_rsa_gen_key( &ctx, mbedtls_ctr_drbg_random, &ctr_drbg, nrbits, exponent ) == result );
    if( result == 0 )
    {
        TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == 0 );
        TEST_ASSERT( mbedtls_mpi_cmp_mpi( &ctx.P, &ctx.Q ) > 0 );
    }

exit:
    mbedtls_rsa_free( &ctx );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );
}
#endif /* MBEDTLS_CTR_DRBG_C */
#endif /* MBEDTLS_ENTROPY_C */

#ifdef MBEDTLS_SELF_TEST
void test_suite_rsa_selftest()
{
    TEST_ASSERT( mbedtls_rsa_self_test( 1 ) == 0 );

exit:
    return;
}
#endif /* MBEDTLS_SELF_TEST */


#endif /* defined(MBEDTLS_RSA_C) */
#endif /* defined(MBEDTLS_BIGNUM_C) */
#endif /* defined(MBEDTLS_GENPRIME) */


/*----------------------------------------------------------------------------*/
/* Test dispatch code */

int dep_check( char *str )
{
    if( str == NULL )
        return( 1 );

    if( strcmp( str, "MBEDTLS_MD5_C" ) == 0 )
    {
#if defined(MBEDTLS_MD5_C)
        return( DEPENDENCY_SUPPORTED );
#else
        return( DEPENDENCY_NOT_SUPPORTED );
#endif
    }
    if( strcmp( str, "MBEDTLS_PKCS1_V15" ) == 0 )
    {
#if defined(MBEDTLS_PKCS1_V15)
        return( DEPENDENCY_SUPPORTED );
#else
        return( DEPENDENCY_NOT_SUPPORTED );
#endif
    }
    if( strcmp( str, "MBEDTLS_SHA1_C" ) == 0 )
    {
#if defined(MBEDTLS_SHA1_C)
        return( DEPENDENCY_SUPPORTED );
#else
        return( DEPENDENCY_NOT_SUPPORTED );
#endif
    }
    if( strcmp( str, "MBEDTLS_SHA256_C" ) == 0 )
    {
#if defined(MBEDTLS_SHA256_C)
        return( DEPENDENCY_SUPPORTED );
#else
        return( DEPENDENCY_NOT_SUPPORTED );
#endif
    }
    if( strcmp( str, "MBEDTLS_SELF_TEST" ) == 0 )
    {
#if defined(MBEDTLS_SELF_TEST)
        return( DEPENDENCY_SUPPORTED );
#else
        return( DEPENDENCY_NOT_SUPPORTED );
#endif
    }
    if( strcmp( str, "MBEDTLS_SHA512_C" ) == 0 )
    {
#if defined(MBEDTLS_SHA512_C)
        return( DEPENDENCY_SUPPORTED );
#else
        return( DEPENDENCY_NOT_SUPPORTED );
#endif
    }
    if( strcmp( str, "MBEDTLS_MD4_C" ) == 0 )
    {
#if defined(MBEDTLS_MD4_C)
        return( DEPENDENCY_SUPPORTED );
#else
        return( DEPENDENCY_NOT_SUPPORTED );
#endif
    }
    if( strcmp( str, "MBEDTLS_MD2_C" ) == 0 )
    {
#if defined(MBEDTLS_MD2_C)
        return( DEPENDENCY_SUPPORTED );
#else
        return( DEPENDENCY_NOT_SUPPORTED );
#endif
    }

    return( DEPENDENCY_NOT_SUPPORTED );
}

int dispatch_test(int cnt, char *params[50])
{
    int ret;
    ((void) cnt);
    ((void) params);

    ret = DISPATCH_TEST_SUCCESS;

    // Cast to void to avoid compiler warnings
    (void)ret;

    if( strcmp( params[0], "mbedtls_rsa_pkcs1_sign" ) == 0 )
    {

        char *param1 = params[1];
        int param2;
        int param3;
        int param4;
        int param5;
        char *param6 = params[6];
        int param7;
        char *param8 = params[8];
        int param9;
        char *param10 = params[10];
        int param11;
        char *param12 = params[12];
        char *param13 = params[13];
        int param14;

        if( cnt != 15 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 15 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_string( &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[2], &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[3], &param3 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[4], &param4 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[5], &param5 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param6 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[7], &param7 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param8 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[9], &param9 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param10 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[11], &param11 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param12 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param13 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[14], &param14 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_mbedtls_rsa_pkcs1_sign( param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12, param13, param14 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "mbedtls_rsa_pkcs1_verify" ) == 0 )
    {

        char *param1 = params[1];
        int param2;
        int param3;
        int param4;
        int param5;
        char *param6 = params[6];
        int param7;
        char *param8 = params[8];
        char *param9 = params[9];
        int param10;

        if( cnt != 11 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 11 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_string( &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[2], &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[3], &param3 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[4], &param4 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[5], &param5 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param6 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[7], &param7 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param8 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param9 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[10], &param10 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_mbedtls_rsa_pkcs1_verify( param1, param2, param3, param4, param5, param6, param7, param8, param9, param10 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "rsa_pkcs1_sign_raw" ) == 0 )
    {

        char *param1 = params[1];
        char *param2 = params[2];
        int param3;
        int param4;
        int param5;
        char *param6 = params[6];
        int param7;
        char *param8 = params[8];
        int param9;
        char *param10 = params[10];
        int param11;
        char *param12 = params[12];
        char *param13 = params[13];

        if( cnt != 14 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 14 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_string( &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[3], &param3 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[4], &param4 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[5], &param5 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param6 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[7], &param7 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param8 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[9], &param9 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param10 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[11], &param11 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param12 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param13 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_rsa_pkcs1_sign_raw( param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12, param13 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "rsa_pkcs1_verify_raw" ) == 0 )
    {

        char *param1 = params[1];
        char *param2 = params[2];
        int param3;
        int param4;
        int param5;
        char *param6 = params[6];
        int param7;
        char *param8 = params[8];
        char *param9 = params[9];
        int param10;

        if( cnt != 11 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 11 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_string( &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[3], &param3 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[4], &param4 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[5], &param5 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param6 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[7], &param7 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param8 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param9 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[10], &param10 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_rsa_pkcs1_verify_raw( param1, param2, param3, param4, param5, param6, param7, param8, param9, param10 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "mbedtls_rsa_pkcs1_encrypt" ) == 0 )
    {

        char *param1 = params[1];
        int param2;
        int param3;
        int param4;
        char *param5 = params[5];
        int param6;
        char *param7 = params[7];
        char *param8 = params[8];
        int param9;

        if( cnt != 10 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 10 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_string( &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[2], &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[3], &param3 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[4], &param4 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param5 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[6], &param6 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param7 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param8 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[9], &param9 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_mbedtls_rsa_pkcs1_encrypt( param1, param2, param3, param4, param5, param6, param7, param8, param9 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "rsa_pkcs1_encrypt_bad_rng" ) == 0 )
    {

        char *param1 = params[1];
        int param2;
        int param3;
        int param4;
        char *param5 = params[5];
        int param6;
        char *param7 = params[7];
        char *param8 = params[8];
        int param9;

        if( cnt != 10 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 10 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_string( &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[2], &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[3], &param3 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[4], &param4 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param5 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[6], &param6 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param7 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param8 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[9], &param9 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_rsa_pkcs1_encrypt_bad_rng( param1, param2, param3, param4, param5, param6, param7, param8, param9 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "mbedtls_rsa_pkcs1_decrypt" ) == 0 )
    {

        char *param1 = params[1];
        int param2;
        int param3;
        int param4;
        char *param5 = params[5];
        int param6;
        char *param7 = params[7];
        int param8;
        char *param9 = params[9];
        int param10;
        char *param11 = params[11];
        int param12;
        char *param13 = params[13];
        int param14;

        if( cnt != 15 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 15 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_string( &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[2], &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[3], &param3 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[4], &param4 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param5 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[6], &param6 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param7 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[8], &param8 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param9 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[10], &param10 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param11 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[12], &param12 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param13 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[14], &param14 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_mbedtls_rsa_pkcs1_decrypt( param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12, param13, param14 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "mbedtls_rsa_public" ) == 0 )
    {

        char *param1 = params[1];
        int param2;
        int param3;
        char *param4 = params[4];
        int param5;
        char *param6 = params[6];
        char *param7 = params[7];
        int param8;

        if( cnt != 9 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 9 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_string( &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[2], &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[3], &param3 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param4 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[5], &param5 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param6 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param7 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[8], &param8 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_mbedtls_rsa_public( param1, param2, param3, param4, param5, param6, param7, param8 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "mbedtls_rsa_private" ) == 0 )
    {

        char *param1 = params[1];
        int param2;
        int param3;
        char *param4 = params[4];
        int param5;
        char *param6 = params[6];
        int param7;
        char *param8 = params[8];
        int param9;
        char *param10 = params[10];
        char *param11 = params[11];
        int param12;

        if( cnt != 13 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 13 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_string( &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[2], &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[3], &param3 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param4 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[5], &param5 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param6 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[7], &param7 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param8 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[9], &param9 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param10 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param11 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[12], &param12 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_mbedtls_rsa_private( param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "rsa_check_privkey_null" ) == 0 )
    {


        if( cnt != 1 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 1 );
            return( DISPATCH_INVALID_TEST_DATA );
        }


        test_suite_rsa_check_privkey_null(  );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "mbedtls_rsa_check_pubkey" ) == 0 )
    {

        int param1;
        char *param2 = params[2];
        int param3;
        char *param4 = params[4];
        int param5;

        if( cnt != 6 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 6 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_int( params[1], &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[3], &param3 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param4 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[5], &param5 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_mbedtls_rsa_check_pubkey( param1, param2, param3, param4, param5 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "mbedtls_rsa_check_privkey" ) == 0 )
    {

        int param1;
        int param2;
        char *param3 = params[3];
        int param4;
        char *param5 = params[5];
        int param6;
        char *param7 = params[7];
        int param8;
        char *param9 = params[9];
        int param10;
        char *param11 = params[11];
        int param12;
        char *param13 = params[13];
        int param14;
        char *param15 = params[15];
        int param16;
        char *param17 = params[17];
        int param18;

        if( cnt != 19 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 19 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_int( params[1], &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[2], &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param3 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[4], &param4 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param5 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[6], &param6 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param7 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[8], &param8 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param9 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[10], &param10 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param11 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[12], &param12 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param13 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[14], &param14 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param15 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[16], &param16 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param17 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[18], &param18 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_mbedtls_rsa_check_privkey( param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12, param13, param14, param15, param16, param17, param18 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "rsa_check_pubpriv" ) == 0 )
    {

        int param1;
        int param2;
        char *param3 = params[3];
        int param4;
        char *param5 = params[5];
        int param6;
        char *param7 = params[7];
        int param8;
        char *param9 = params[9];
        int param10;
        char *param11 = params[11];
        int param12;
        char *param13 = params[13];
        int param14;
        char *param15 = params[15];
        int param16;
        char *param17 = params[17];
        int param18;
        char *param19 = params[19];
        int param20;
        char *param21 = params[21];
        int param22;

        if( cnt != 23 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 23 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_int( params[1], &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[2], &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param3 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[4], &param4 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param5 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[6], &param6 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param7 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[8], &param8 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param9 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[10], &param10 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param11 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[12], &param12 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param13 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[14], &param14 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param15 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[16], &param16 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param17 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[18], &param18 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param19 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[20], &param20 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param21 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[22], &param22 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_rsa_check_pubpriv( param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12, param13, param14, param15, param16, param17, param18, param19, param20, param21, param22 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "mbedtls_rsa_gen_key" ) == 0 )
    {
        int param1;
        int param2;
        int param3;

        if( cnt != 4 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 4 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_int( params[1], &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[2], &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[3], &param3 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_mbedtls_rsa_gen_key( param1, param2, param3 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "rsa_selftest" ) == 0 )
    {
        if( cnt != 1 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 1 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        test_suite_rsa_selftest(  );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else

    {
        printf("FAILED\nSkipping unknown test function '%s'\n", params[0] );
        ret = DISPATCH_TEST_FN_NOT_FOUND;
    }
    return( ret );
}

int parse_arguments( char *buf, size_t len, char *params[50] )
{
    int cnt = 0, i;
    char *cur = buf;
    char *p = buf, *q;

    params[cnt++] = cur;

    while( *p != '\0' && p < buf + len )
    {
        if( *p == '\\' )
        {
            p++;
            p++;
            continue;
        }
        if( *p == ':' )
        {
            if( p + 1 < buf + len )
            {
                cur = p + 1;
                params[cnt++] = cur;
            }
            *p = '\0';
        }

        p++;
    }

    /* Replace newlines, question marks and colons in strings */
    for( i = 0; i < cnt; i++ )
    {
        p = params[i];
        q = params[i];

        while( *p != '\0' )
        {
            if( *p == '\\' && *(p + 1) == 'n' )
            {
                p += 2;
                *(q++) = '\n';
            }
            else if( *p == '\\' && *(p + 1) == ':' )
            {
                p += 2;
                *(q++) = ':';
            }
            else if( *p == '\\' && *(p + 1) == '?' )
            {
                p += 2;
                *(q++) = '?';
            }
            else
                *(q++) = *(p++);
        }
        *q = '\0';
    }

    return( cnt );
}

void UART_Init()
{
    /* enable UART0 clock */
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | 0x10000);

    /* GPF11, GPF12 */
    outpw(REG_SYS_GPF_MFPH, (inpw(REG_SYS_GPF_MFPH) & 0xfff00fff) | 0x11000);   // UART0 multi-function

    /* UART0 line configuration for (115200,n,8,1) */
    outpw(REG_UART0_LCR, inpw(REG_UART0_LCR) | 0x07);
    outpw(REG_UART0_BAUD, 0x30000066); /* 12MHz reference clock input, 115200 */
}

volatile int  g_Crypto_Int_done = 0;

void CRYPTO_IRQHandler()
{
    if (SHA_GET_INT_FLAG(CRPT))
    {
        g_Crypto_Int_done = 1;
        SHA_CLR_INT_FLAG(CRPT);
    }
    g_Crypto_Int_done = 1;
}

volatile int  _timer_tick;

void ETMR0_IRQHandler(void)
{
    _timer_tick ++;
    // clear timer interrupt flag
    ETIMER_ClearIntFlag(0);
}

int  get_ticks(void)
{
    return _timer_tick;
}

void Start_ETIMER0(void)
{
    // Enable ETIMER0 engine clock
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | (1 << 8));

    // Set timer frequency to 100 HZ
    ETIMER_Open(0, ETIMER_PERIODIC_MODE, 100);

    // Enable timer interrupt
    ETIMER_EnableInt(0);
    sysInstallISR(IRQ_LEVEL_1, IRQ_TIMER0, (PVOID)ETMR0_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_TIMER0);

    _timer_tick = 0;

    // Start Timer 0
    ETIMER_Start(0);
}

/*-----------------------------------------------------------------------------*/
int main(void)
{
    char  *params[50];
    int   i, cnt, pass_cnt = 0, vector_no;
    int   ret, total_tests=0, total_errors=0, total_skipped=0;

    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();

    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | (1<<23));   /* Enable Crypto clock */

    printf("+---------------------------------------+\n");
    printf("|     Crypto mbedtls RSA test suit      |\n");
    printf("+---------------------------------------+\n");

    sysInstallISR(IRQ_LEVEL_1, IRQ_CRYPTO, (PVOID)CRYPTO_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_CRYPTO);

    SHA_ENABLE_INT(CRPT);

    Start_ETIMER0();

    for (vector_no = 1; ; vector_no++)
    {
        int unmet_dep_count = 0;

        if (open_test_vector(vector_no) != 0)
            break;

        while (!is_eof())
        {
            if( unmet_dep_count > 0 )
            {
                printf("FATAL: Dep count larger than zero at start of loop\n" );
                while (1);
            }
            unmet_dep_count = 0;

            if (get_line() != 0)
                break;

            printf("%s \n", g_line_buff );

            if (get_line() != 0)
                break;

            cnt = parse_arguments(g_line_buff, strlen(g_line_buff), params );

            if (strcmp( params[0], "depends_on" ) == 0 )
            {
                for( i = 1; i < cnt; i++ )
                {
                    if( dep_check( params[i] ) != DEPENDENCY_SUPPORTED )
                    {
                        unmet_dep_count++;
                        break;
                    }
                }

                if (get_line() != 0)
                    break;

                cnt = parse_arguments( g_line_buff, strlen(g_line_buff), params );
            }

            // If there are no unmet dependencies execute the test
            if( unmet_dep_count == 0 )
            {
                total_tests++;
                ret = dispatch_test( cnt, params );
            }

            if( unmet_dep_count > 0 || ret == DISPATCH_UNSUPPORTED_SUITE )
            {
                total_skipped++;
                printf("skip!\n");
                unmet_dep_count = 0;
            }
            else if( ret == DISPATCH_TEST_SUCCESS && test_errors == 0 )
            {
                printf("[PASS] %d\n", ++pass_cnt );
            }
            else if( ret == DISPATCH_INVALID_TEST_DATA )
            {
                printf("FAILED: FATAL PARSE ERROR\n" );
                while( 1 );
            }
            else
                total_errors++;

#if 0
            if (get_line() != 0)
                break;

            if( strlen(g_line_buff) != 0 )
            {
                printf("Should be empty %d\n", (int) strlen(g_line_buff) );
                while (1);
            }
#endif
        }
    }

    printf("\n----------------------------------------------------------------------------\n\n");
    if( total_errors == 0 )
        printf("PASSED" );
    else
        printf("FAILED" );

    printf(" (%d / %d tests (%d skipped))\n",
           total_tests - total_errors, total_tests, total_skipped );


    while (1);
}


