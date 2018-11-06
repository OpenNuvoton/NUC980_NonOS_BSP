/*
 * *** THIS FILE HAS BEEN MACHINE GENERATED ***
 *
 * This file has been machine generated using the script: scripts/generate_code.pl
 *
 * Test file      : test_suite_pkcs1_v15.c
 *
 * The following files were used to create this file.
 *
 *      Main code file  : suites/main_test.function
 *      Helper file     : suites/helpers.function
 *      Test suite file : suites/test_suite_pkcs1_v15.function
 *      Test suite data : suites/test_suite_pkcs1_v15.data
 *
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include "mbedtls/config.h"
#include "mbedtls/platform.h"

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
#endif

#include <stdint.h>
#include <string.h>

/*----------------------------------------------------------------------------*/
/* Constants */

#define DEPENDENCY_SUPPORTED        0
#define DEPENDENCY_NOT_SUPPORTED    1

#define KEY_VALUE_MAPPING_FOUND     0
#define KEY_VALUE_MAPPING_NOT_FOUND -1

#define DISPATCH_TEST_SUCCESS       0
#define DISPATCH_TEST_FN_NOT_FOUND  1
#define DISPATCH_INVALID_TEST_DATA  2
#define DISPATCH_UNSUPPORTED_SUITE  3


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
    mbedtls_exit( 1 );                                      \
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


int       pass_cnt;
UINT8     *file_base_ptr;
UINT32    g_file_idx, g_file_size;


static char  g_line_buff[4096];

static int test_errors = 0;

extern UINT32  VectorBase_PKCS1_V15, VectorLimit_PKCS1_V15;

int  open_test_vector(int vector_no)
{
    g_file_idx = 0;

    if (vector_no == 1)
    {
        printf("\n\nOpen test vector test_suite_pkcs1_v15.data.\n");
        file_base_ptr = (UINT8 *)&VectorBase_PKCS1_V15;
        g_file_size = (UINT32)&VectorLimit_PKCS1_V15 - (UINT32)&VectorBase_PKCS1_V15;
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
    printf("LINE: %s\n", g_line_buff);
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

typedef struct
{
    unsigned char *buf;
    size_t length;
} rnd_buf_info;


/**
 * This function returns random based on a buffer it receives.
 *
 * rng_state shall be a pointer to a rnd_buf_info structure.
 *
 * The number of bytes released from the buffer on each call to
 * the random function is specified by per_call. (Can be between
 * 1 and 4)
 *
 * After the buffer is empty it will return rand();
 */
static int rnd_buffer_rand( void *rng_state, unsigned char *output, size_t len )
{
    rnd_buf_info *info = (rnd_buf_info *) rng_state;
    size_t use_len;

    if( rng_state == NULL )
        return( rnd_std_rand( NULL, output, len ) );

    use_len = len;
    if( len > info->length )
        use_len = info->length;

    if( use_len )
    {
        memcpy( output, info->buf, use_len );
        info->buf += use_len;
        info->length -= use_len;
    }

    if( len - use_len > 0 )
        return( rnd_std_rand( NULL, output + use_len, len - use_len ) );

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

#if defined(MBEDTLS_PKCS1_V15)
#if defined(MBEDTLS_RSA_C)
#if defined(MBEDTLS_SHA1_C)

#include "mbedtls/rsa.h"
#include "mbedtls/md.h"

#endif /* defined(MBEDTLS_PKCS1_V15) */
#endif /* defined(MBEDTLS_RSA_C) */
#endif /* defined(MBEDTLS_SHA1_C) */


#if defined(MBEDTLS_PKCS1_V15)
#if defined(MBEDTLS_RSA_C)
#if defined(MBEDTLS_SHA1_C)

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

    if( strcmp( str, "MBEDTLS_ERR_RSA_INVALID_PADDING" ) == 0 )
    {
        *value = ( MBEDTLS_ERR_RSA_INVALID_PADDING );
        return( KEY_VALUE_MAPPING_FOUND );
    }
    if( strcmp( str, "MBEDTLS_ERR_RSA_BAD_INPUT_DATA" ) == 0 )
    {
        *value = ( MBEDTLS_ERR_RSA_BAD_INPUT_DATA );
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

void test_suite_pkcs1_rsaes_v15_encrypt( int mod, int radix_N, char *input_N, int radix_E,
        char *input_E, int hash,
        char *message_hex_string, char *seed,
        char *result_hex_str, int result )
{
    unsigned char message_str[1000];
    unsigned char output[1000];
    unsigned char output_str[1000];
    unsigned char rnd_buf[1000];
    mbedtls_rsa_context ctx;
    size_t msg_len;
    rnd_buf_info info;

    info.length = unhexify( rnd_buf, seed );
    info.buf = rnd_buf;

    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, hash );
    memset( message_str, 0x00, 1000 );
    memset( output, 0x00, 1000 );
    memset( output_str, 0x00, 1000 );

    ctx.len = mod / 8 + ( ( mod % 8 ) ? 1 : 0 );
    RSA_claim_bit_length(mod);
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &ctx.E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == 0 );

    msg_len = unhexify( message_str, message_hex_string );

    TEST_ASSERT( mbedtls_rsa_pkcs1_encrypt( &ctx, &rnd_buffer_rand, &info, MBEDTLS_RSA_PUBLIC, msg_len, message_str, output ) == result );
    if( result == 0 )
    {
        hexify( output_str, output, ctx.len );

        TEST_ASSERT( strcasecmp( (char *) output_str, result_hex_str ) == 0 );
    }

exit:
    mbedtls_rsa_free( &ctx );
}

void test_suite_pkcs1_rsaes_v15_decrypt( int mod, int radix_P, char *input_P,
        int radix_Q, char *input_Q, int radix_N,
        char *input_N, int radix_E, char *input_E,
        int hash, char *result_hex_str, char *seed,
        char *message_hex_string, int result )
{
    unsigned char message_str[1000];
    unsigned char output[1000];
    unsigned char output_str[1000];
    mbedtls_rsa_context ctx;
    mbedtls_mpi P1, Q1, H, G;
    size_t output_len;
    rnd_pseudo_info rnd_info;
    ((void) seed);

    mbedtls_mpi_init( &P1 );
    mbedtls_mpi_init( &Q1 );
    mbedtls_mpi_init( &H );
    mbedtls_mpi_init( &G );
    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, hash );

    memset( message_str, 0x00, 1000 );
    memset( output, 0x00, 1000 );
    memset( output_str, 0x00, 1000 );
    memset( &rnd_info, 0, sizeof( rnd_pseudo_info ) );

    ctx.len = mod / 8 + ( ( mod % 8 ) ? 1 : 0 );
    RSA_claim_bit_length(mod);
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

    TEST_ASSERT( mbedtls_rsa_pkcs1_decrypt( &ctx, &rnd_pseudo_rand, &rnd_info, MBEDTLS_RSA_PRIVATE, &output_len, message_str, output, 1000 ) == result );
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

void test_suite_pkcs1_rsassa_v15_sign( int mod, int radix_P, char *input_P, int radix_Q,
                                       char *input_Q, int radix_N, char *input_N,
                                       int radix_E, char *input_E, int digest, int hash,
                                       char *message_hex_string, char *salt,
                                       char *result_hex_str, int result )
{
    unsigned char message_str[1000];
    unsigned char hash_result[1000];
    unsigned char output[1000];
    unsigned char output_str[1000];
    unsigned char rnd_buf[1000];
    mbedtls_rsa_context ctx;
    mbedtls_mpi P1, Q1, H, G;
    size_t msg_len;
    rnd_buf_info info;

    info.length = unhexify( rnd_buf, salt );
    info.buf = rnd_buf;

    mbedtls_mpi_init( &P1 );
    mbedtls_mpi_init( &Q1 );
    mbedtls_mpi_init( &H );
    mbedtls_mpi_init( &G );
    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, hash );

    memset( message_str, 0x00, 1000 );
    memset( hash_result, 0x00, 1000 );
    memset( output, 0x00, 1000 );
    memset( output_str, 0x00, 1000 );

    ctx.len = mod / 8 + ( ( mod % 8 ) ? 1 : 0 );
    RSA_claim_bit_length(mod);
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

    TEST_ASSERT( mbedtls_rsa_pkcs1_sign( &ctx, &rnd_buffer_rand, &info, MBEDTLS_RSA_PRIVATE, (mbedtls_md_type_t)digest, 0, hash_result, output ) == result );
    if( result == 0 )
    {
        hexify( output_str, output, ctx.len);

        TEST_ASSERT( strcasecmp( (char *) output_str, result_hex_str ) == 0 );
    }

exit:
    mbedtls_mpi_free( &P1 );
    mbedtls_mpi_free( &Q1 );
    mbedtls_mpi_free( &H );
    mbedtls_mpi_free( &G );
    mbedtls_rsa_free( &ctx );
}

void test_suite_pkcs1_rsassa_v15_verify( int mod, int radix_N, char *input_N, int radix_E,
        char *input_E, int digest, int hash,
        char *message_hex_string, char *salt,
        char *result_hex_str, int result )
{
    unsigned char message_str[1000];
    unsigned char hash_result[1000];
    unsigned char result_str[1000];
    mbedtls_rsa_context ctx;
    size_t msg_len;
    ((void) salt);

    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, hash );
    memset( message_str, 0x00, 1000 );
    memset( hash_result, 0x00, 1000 );
    memset( result_str, 0x00, 1000 );

    ctx.len = mod / 8 + ( ( mod % 8 ) ? 1 : 0 );
    RSA_claim_bit_length(mod);
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


#endif /* defined(MBEDTLS_PKCS1_V15) */
#endif /* defined(MBEDTLS_RSA_C) */
#endif /* defined(MBEDTLS_SHA1_C) */



/*----------------------------------------------------------------------------*/
/* Test dispatch code */

int dep_check( char *str )
{
    if( str == NULL )
        return( 1 );

    return( DEPENDENCY_NOT_SUPPORTED );
}

int dispatch_test(int cnt, char *params[50])
{
    int ret;
    ((void) cnt);
    ((void) params);

#if defined(TEST_SUITE_ACTIVE)
    ret = DISPATCH_TEST_SUCCESS;

    // Cast to void to avoid compiler warnings
    (void)ret;

    if( strcmp( params[0], "pkcs1_rsaes_v15_encrypt" ) == 0 )
    {

        int param1;
        int param2;
        char *param3 = params[3];
        int param4;
        char *param5 = params[5];
        int param6;
        char *param7 = params[7];
        char *param8 = params[8];
        char *param9 = params[9];
        int param10;

        if( cnt != 11 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 11 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_int( params[1], &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[2], &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param3 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[4], &param4 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param5 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[6], &param6 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param7 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param8 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param9 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[10], &param10 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_pkcs1_rsaes_v15_encrypt( param1, param2, param3, param4, param5, param6, param7, param8, param9, param10 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "pkcs1_rsaes_v15_decrypt" ) == 0 )
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
        char *param12 = params[12];
        char *param13 = params[13];
        int param14;

        if( cnt != 15 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 15 );
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
        if( verify_string( &param12 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param13 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[14], &param14 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_pkcs1_rsaes_v15_decrypt( param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12, param13, param14 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "pkcs1_rsassa_v15_sign" ) == 0 )
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
        int param11;
        char *param12 = params[12];
        char *param13 = params[13];
        char *param14 = params[14];
        int param15;

        if( cnt != 16 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 16 );
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
        if( verify_int( params[11], &param11 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param12 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param13 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param14 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[15], &param15 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_pkcs1_rsassa_v15_sign( param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12, param13, param14, param15 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "pkcs1_rsassa_v15_verify" ) == 0 )
    {

        int param1;
        int param2;
        char *param3 = params[3];
        int param4;
        char *param5 = params[5];
        int param6;
        int param7;
        char *param8 = params[8];
        char *param9 = params[9];
        char *param10 = params[10];
        int param11;

        if( cnt != 12 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 12 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_int( params[1], &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[2], &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param3 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[4], &param4 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param5 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[6], &param6 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[7], &param7 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param8 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param9 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param10 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_int( params[11], &param11 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_pkcs1_rsassa_v15_verify( param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else

    {
        printf("FAILED\nSkipping unknown test function '%s'\n", params[0] );
        ret = DISPATCH_TEST_FN_NOT_FOUND;
    }
#else
    ret = DISPATCH_UNSUPPORTED_SUITE;
#endif
    return( ret );
}


/*----------------------------------------------------------------------------*/
/* Main Test code */


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

/*-----------------------------------------------------------------------------*/
int main(void)
{
    char  *params[50];
    int   cnt, vector_no;
    int   is_eof, ret;

    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();

    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | (1<<23));   /* Enable Crypto clock */

    printf("+---------------------------------------------+\n");
    printf("|   Crypto mbedtls RSA PKCS1 V1.5 test suit   |\n");
    printf("+---------------------------------------------+\n");

    sysInstallISR(IRQ_LEVEL_1, IRQ_CRYPTO, (PVOID)CRYPTO_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_CRYPTO);

    SHA_ENABLE_INT(CRPT);

    /* Now begin to execute the tests in the testfiles */
    for (vector_no = 1; ; vector_no++)
    {
        if (open_test_vector(vector_no) != 0)
            break;

        pass_cnt = 0;

        while (1)
        {
            is_eof = get_line();
            if (is_eof) break;


            is_eof = get_line();
            if (is_eof) break;

            cnt = parse_arguments(g_line_buff, strlen(g_line_buff), params);

            if (strcmp( params[0], "depends_on" ) == 0)
            {
                is_eof = get_line();
                if (is_eof) break;

                cnt = parse_arguments(g_line_buff, strlen(g_line_buff), params);
            }

            dispatch_test( cnt, params );
            pass_cnt++;

            if ((ret == DISPATCH_TEST_SUCCESS) && (test_errors == 0))
            {
                printf("[PASS]\n" );
            }
            else if( ret == DISPATCH_INVALID_TEST_DATA )
            {
                printf("FAILED: FATAL PARSE ERROR\n" );
                while (1);
            }
        }
    }

    printf("\n----------------------------------------------------------------------------\n\n");
    printf("%d pattern PASSED", pass_cnt );

    while (1);
}


