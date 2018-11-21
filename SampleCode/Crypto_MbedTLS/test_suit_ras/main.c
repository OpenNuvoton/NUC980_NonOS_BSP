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


/* Type for Hex parameters */
typedef struct data_tag
{
    uint8_t *   x;
    uint32_t    len;
} data_t;

/*----------------------------------------------------------------------------*/
/* Status and error constants */

#define DEPENDENCY_SUPPORTED            0   /* Dependency supported by build */
#define KEY_VALUE_MAPPING_FOUND         0   /* Integer expression found */
#define DISPATCH_TEST_SUCCESS           0   /* Test dispatch successful */

#define KEY_VALUE_MAPPING_NOT_FOUND     -1  /* Integer expression not found */
#define DEPENDENCY_NOT_SUPPORTED        -2  /* Dependency not supported */
#define DISPATCH_TEST_FN_NOT_FOUND      -3  /* Test function not found */
#define DISPATCH_INVALID_TEST_DATA      -4  /* Invalid test parameter type.
                                               Only int, string, binary data
                                               and integer expressions are
                                               allowed */
#define DISPATCH_UNSUPPORTED_SUITE      -5  /* Test suite not supported by the
                                               build */

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

int  read_file_line(char *buf, int len)
{
    int         i;
    uint8_t     ch;

    if (g_file_idx+1 >= g_file_size)
    {
        //printf("EOF.\n");
        return -1;
    }

    memset(buf, 0, len);

    for (i = 0;;)
    {
        if (read_file(&ch, 1) < 0)
            return 0;

        if ((ch == 0x0D) || (ch == 0x0A))
            break;

        buf[i++] = ch;
    }

    while (1)
    {
        if (read_file(&ch, 1) < 0)
            return 0;

        if ((ch != 0x0D) && (ch != 0x0A))
            break;
    }
    g_file_idx--;
    //printf("LINE: %s\n", buf);
    return 0;
}

/**
 * \brief       Read a line from the passed file pointer.
 *
 * \param buf   Pointer to memory to hold read line.
 * \param len   Length of the buf.
 *
 * \return      0 if success else -1 for EOF
 */
int get_line(char *buf, size_t len)
{
    char *ret;
    int i = 0, str_len = 0, has_string = 0;

    /* Read until we get a valid line */
    do
    {
        if (read_file_line( buf, len ) != 0)
            return( -1 );

        str_len = strlen( buf );

        /* Skip empty line and comment */
        if ( str_len == 0 || buf[0] == '#' )
            continue;
        has_string = 0;
        for ( i = 0; i < str_len; i++ )
        {
            char c = buf[i];
            if ( c != ' ' && c != '\t' && c != '\n' &&
                 c != '\v' && c != '\f' && c != '\r' )
            {
                has_string = 1;
                break;
            }
        }
    } while( !has_string );

    /* Strip new line and carriage return */
    ret = buf + strlen( buf );
    if( ret-- > buf && *ret == '\n' )
        *ret = '\0';
    if( ret-- > buf && *ret == '\r' )
        *ret = '\0';

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

int hexcmp( uint8_t * a, uint8_t * b, uint32_t a_len, uint32_t b_len )
{
    int ret = 0;
    uint32_t i = 0;

    if ( a_len != b_len )
        return( -1 );

    for( i = 0; i < a_len; i++ )
    {
        if ( a[i] != b[i] )
        {
            ret = -1;
            break;
        }
    }
    return ret;
}


/*----------------------------------------------------------------------------*/
/* Test Suite Code */


#define TEST_SUITE_ACTIVE

#if defined(MBEDTLS_RSA_C)
#if defined(MBEDTLS_BIGNUM_C)
#if defined(MBEDTLS_GENPRIME)
#include "mbedtls/rsa.h"
#include "mbedtls/rsa_internal.h"
#include "mbedtls/md2.h"
#include "mbedtls/md4.h"
#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

void test_mbedtls_rsa_pkcs1_sign( data_t * message_str, int padding_mode,
                             int digest, int mod, int radix_P, char * input_P,
                             int radix_Q, char * input_Q, int radix_N,
                             char * input_N, int radix_E, char * input_E,
                             data_t * result_hex_str, int result )
{
    unsigned char hash_result[1000];
    unsigned char output[1000];
    mbedtls_rsa_context ctx;
    mbedtls_mpi N, P, Q, E;
    rnd_pseudo_info rnd_info;

    RSA_claim_bit_length(mod);

    mbedtls_mpi_init( &N ); mbedtls_mpi_init( &P );
    mbedtls_mpi_init( &Q ); mbedtls_mpi_init( &E );
    mbedtls_rsa_init( &ctx, padding_mode, 0 );

    memset( hash_result, 0x00, 1000 );
    memset( output, 0x00, 1000 );
    memset( &rnd_info, 0, sizeof( rnd_pseudo_info ) );

    TEST_ASSERT( mbedtls_mpi_read_string( &P, radix_P, input_P ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &Q, radix_Q, input_Q ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_import( &ctx, &N, &P, &Q, NULL, &E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_get_len( &ctx ) == (size_t) ( mod / 8 ) );
    TEST_ASSERT( mbedtls_rsa_complete( &ctx ) == 0 );
    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == 0 );


    if( mbedtls_md_info_from_type((mbedtls_md_type_t) digest ) != NULL )
        TEST_ASSERT( mbedtls_md( mbedtls_md_info_from_type((mbedtls_md_type_t) digest ), message_str->x, message_str->len, hash_result ) == 0 );

    TEST_ASSERT( mbedtls_rsa_pkcs1_sign( &ctx, &rnd_pseudo_rand, &rnd_info,
                                         MBEDTLS_RSA_PRIVATE, (mbedtls_md_type_t)digest, 0,
                                         hash_result, output ) == result );
    if( result == 0 )
    {

        TEST_ASSERT( hexcmp( output, result_hex_str->x, ctx.len, result_hex_str->len ) == 0 );
    }

exit:
    mbedtls_mpi_free( &N ); mbedtls_mpi_free( &P );
    mbedtls_mpi_free( &Q ); mbedtls_mpi_free( &E );
    mbedtls_rsa_free( &ctx );
}

void test_mbedtls_rsa_pkcs1_sign_wrapper( void ** params )
{
    data_t data0;
    data_t data13;
    
    data0.x = (uint8_t *) params[0];
    data0.len = *( (uint32_t *) params[1] );
    data13.x = (uint8_t *) params[13];
    data13.len = *( (uint32_t *) params[14]);
    
    test_mbedtls_rsa_pkcs1_sign( &data0, *( (int *) params[2] ), 
                 *( (int *) params[3] ), *( (int *) params[4] ), *( (int *) params[5] ), (char *) params[6], 
                 *( (int *) params[7] ), (char *) params[8], *( (int *) params[9] ), 
                 (char *) params[10],  *( (int *) params[11] ), (char *) params[12], 
                 &data13, *( (int *) params[15] ) );
}


//void test_mbedtls_rsa_pkcs1_sign( data_t * message_str, int padding_mode,
//                             int digest, int mod, int radix_P, char * input_P,
//                             int radix_Q, char * input_Q, int radix_N,
//                             char * input_N, int radix_E, char * input_E,
//                             data_t * result_hex_str, int result )


void test_mbedtls_rsa_pkcs1_verify( data_t * message_str, int padding_mode,
                               int digest, int mod, int radix_N,
                               char * input_N, int radix_E, char * input_E,
                               data_t * result_str, int result )
{
    unsigned char hash_result[1000];
    mbedtls_rsa_context ctx;

    mbedtls_mpi N, E;

    RSA_claim_bit_length(mod);

    mbedtls_mpi_init( &N ); mbedtls_mpi_init( &E );
    mbedtls_rsa_init( &ctx, padding_mode, 0 );
    memset( hash_result, 0x00, 1000 );

    TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_import( &ctx, &N, NULL, NULL, NULL, &E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_get_len( &ctx ) == (size_t) ( mod / 8 ) );
    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == 0 );


    if( mbedtls_md_info_from_type((mbedtls_md_type_t) digest ) != NULL )
        TEST_ASSERT( mbedtls_md( mbedtls_md_info_from_type((mbedtls_md_type_t) digest ), message_str->x, message_str->len, hash_result ) == 0 );

    TEST_ASSERT( mbedtls_rsa_pkcs1_verify( &ctx, NULL, NULL, MBEDTLS_RSA_PUBLIC, (mbedtls_md_type_t)digest, 0, hash_result, result_str->x ) == result );

exit:
    mbedtls_mpi_free( &N ); mbedtls_mpi_free( &E );
    mbedtls_rsa_free( &ctx );
}

void test_mbedtls_rsa_pkcs1_verify_wrapper( void ** params )
{
    data_t data0;
    data_t data9;
    
    data0.x = (uint8_t *) params[0];
    data0.len =  *( (uint32_t *) params[1] );
    data9.x = (uint8_t *) params[9];
    data9.len = *( (uint32_t *) params[10] );

    test_mbedtls_rsa_pkcs1_verify( &data0, *( (int *) params[2] ), *( (int *) params[3] ), *( (int *) params[4] ), *( (int *) params[5] ), (char *) params[6], *( (int *) params[7] ), (char *) params[8], &data9, *( (int *) params[11] ) );
}

void test_rsa_pkcs1_sign_raw( data_t * hash_result,
                         int padding_mode, int mod, int radix_P,
                         char * input_P, int radix_Q, char * input_Q,
                         int radix_N, char * input_N, int radix_E,
                         char * input_E, data_t * result_hex_str )
{
    unsigned char output[1000];
    mbedtls_rsa_context ctx;
    mbedtls_mpi N, P, Q, E;
    rnd_pseudo_info rnd_info;

    mbedtls_rsa_init( &ctx, padding_mode, 0 );
    mbedtls_mpi_init( &N ); mbedtls_mpi_init( &P );
    mbedtls_mpi_init( &Q ); mbedtls_mpi_init( &E );

    memset( output, 0x00, 1000 );
    memset( &rnd_info, 0, sizeof( rnd_pseudo_info ) );

    TEST_ASSERT( mbedtls_mpi_read_string( &P, radix_P, input_P ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &Q, radix_Q, input_Q ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_import( &ctx, &N, &P, &Q, NULL, &E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_get_len( &ctx ) == (size_t) ( mod / 8 ) );
    TEST_ASSERT( mbedtls_rsa_complete( &ctx ) == 0 );
    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == 0 );


    TEST_ASSERT( mbedtls_rsa_pkcs1_sign( &ctx, &rnd_pseudo_rand, &rnd_info,
                                         MBEDTLS_RSA_PRIVATE, MBEDTLS_MD_NONE,
                                         hash_result->len, hash_result->x,
                                         output ) == 0 );


    TEST_ASSERT( hexcmp( output, result_hex_str->x, ctx.len, result_hex_str->len ) == 0 );

#if defined(MBEDTLS_PKCS1_V15)
    /* For PKCS#1 v1.5, there is an alternative way to generate signatures */
    if( padding_mode == MBEDTLS_RSA_PKCS_V15 )
    {
        int res;
        memset( output, 0x00, 1000 );

        res = mbedtls_rsa_rsaes_pkcs1_v15_encrypt( &ctx,
                    &rnd_pseudo_rand, &rnd_info, MBEDTLS_RSA_PRIVATE,
                    hash_result->len, hash_result->x, output );

#if !defined(MBEDTLS_RSA_ALT)
        TEST_ASSERT( res == 0 );
#else
        TEST_ASSERT( ( res == 0 ) ||
                     ( res == MBEDTLS_ERR_RSA_UNSUPPORTED_OPERATION ) );
#endif

        if( res == 0 )
        {
            TEST_ASSERT( hexcmp( output, result_hex_str->x, ctx.len, result_hex_str->len ) == 0 );
        }
    }
#endif /* MBEDTLS_PKCS1_V15 */

exit:
    mbedtls_mpi_free( &N ); mbedtls_mpi_free( &P );
    mbedtls_mpi_free( &Q ); mbedtls_mpi_free( &E );

    mbedtls_rsa_free( &ctx );
}

void test_rsa_pkcs1_sign_raw_wrapper( void ** params )
{
    data_t data0;
    data_t data12;
    
    data0.x = (uint8_t *) params[0];
    data0.len = *( (uint32_t *) params[1] );
    data12.x = (uint8_t *) params[12];
    data12.len = *( (uint32_t *) params[13] );

    test_rsa_pkcs1_sign_raw( &data0, *( (int *) params[2] ), *( (int *) params[3] ), *( (int *) params[4] ), (char *) params[5], *( (int *) params[6] ), (char *) params[7], *( (int *) params[8] ), (char *) params[9], *( (int *) params[10] ), (char *) params[11], &data12 );
}

void test_rsa_pkcs1_verify_raw( data_t * hash_result,
                           int padding_mode, int mod, int radix_N,
                           char * input_N, int radix_E, char * input_E,
                           data_t * result_str, int correct )
{
    unsigned char output[1000];
    mbedtls_rsa_context ctx;

    mbedtls_mpi N, E;
    mbedtls_mpi_init( &N ); mbedtls_mpi_init( &E );

    mbedtls_rsa_init( &ctx, padding_mode, 0 );
    memset( output, 0x00, sizeof( output ) );

    TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_import( &ctx, &N, NULL, NULL, NULL, &E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_get_len( &ctx ) == (size_t) ( mod / 8 ) );
    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == 0 );


    TEST_ASSERT( mbedtls_rsa_pkcs1_verify( &ctx, NULL, NULL, MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_NONE, hash_result->len, hash_result->x, result_str->x ) == correct );

#if defined(MBEDTLS_PKCS1_V15)
    /* For PKCS#1 v1.5, there is an alternative way to verify signatures */
    if( padding_mode == MBEDTLS_RSA_PKCS_V15 )
    {
        int res;
        int ok;
        size_t olen;

        res = mbedtls_rsa_rsaes_pkcs1_v15_decrypt( &ctx,
                    NULL, NULL, MBEDTLS_RSA_PUBLIC,
                    &olen, result_str->x, output, sizeof( output ) );

#if !defined(MBEDTLS_RSA_ALT)
        TEST_ASSERT( res == 0 );
#else
        TEST_ASSERT( ( res == 0 ) ||
                     ( res == MBEDTLS_ERR_RSA_UNSUPPORTED_OPERATION ) );
#endif

        if( res == 0 )
        {
            ok = olen == hash_result->len && memcmp( output, hash_result->x, olen ) == 0;
            if( correct == 0 )
                TEST_ASSERT( ok == 1 );
            else
                TEST_ASSERT( ok == 0 );
        }
    }
#endif /* MBEDTLS_PKCS1_V15 */

exit:
    mbedtls_mpi_free( &N ); mbedtls_mpi_free( &E );
    mbedtls_rsa_free( &ctx );
}

void test_rsa_pkcs1_verify_raw_wrapper( void ** params )
{
    data_t data0;
    data_t data8;
    
    data0.x = (uint8_t *) params[0];
    data0.len = *( (uint32_t *) params[1] );
    data8.x = (uint8_t *) params[8];
    data8.len = *( (uint32_t *) params[9] );

    test_rsa_pkcs1_verify_raw( &data0, *( (int *) params[2] ), *( (int *) params[3] ), *( (int *) params[4] ), (char *) params[5], *( (int *) params[6] ), (char *) params[7], &data8, *( (int *) params[10] ) );
}

void test_mbedtls_rsa_pkcs1_encrypt( data_t * message_str, int padding_mode,
                                int mod, int radix_N, char * input_N,
                                int radix_E, char * input_E,
                                data_t * result_hex_str, int result )
{
    unsigned char output[1000];
    mbedtls_rsa_context ctx;
    rnd_pseudo_info rnd_info;

    mbedtls_mpi N, E;
    mbedtls_mpi_init( &N ); mbedtls_mpi_init( &E );

    memset( &rnd_info, 0, sizeof( rnd_pseudo_info ) );

    mbedtls_rsa_init( &ctx, padding_mode, 0 );
    memset( output, 0x00, 1000 );

    TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_import( &ctx, &N, NULL, NULL, NULL, &E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_get_len( &ctx ) == (size_t) ( mod / 8 ) );
    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == 0 );


    TEST_ASSERT( mbedtls_rsa_pkcs1_encrypt( &ctx, &rnd_pseudo_rand, &rnd_info,
                                            MBEDTLS_RSA_PUBLIC, message_str->len,
                                            message_str->x, output ) == result );
    if( result == 0 )
    {

        TEST_ASSERT( hexcmp( output, result_hex_str->x, ctx.len, result_hex_str->len ) == 0 );
    }

exit:
    mbedtls_mpi_free( &N ); mbedtls_mpi_free( &E );
    mbedtls_rsa_free( &ctx );
}

void test_mbedtls_rsa_pkcs1_encrypt_wrapper( void ** params )
{
    data_t data0;
    data_t data8;
    
    data0.x = (uint8_t *) params[0];
    data0.len = *( (uint32_t *) params[1] );
    data8.x = (uint8_t *) params[8];
    data8.len = *( (uint32_t *) params[9] );

    test_mbedtls_rsa_pkcs1_encrypt( &data0, *( (int *) params[2] ), *( (int *) params[3] ), *( (int *) params[4] ), (char *) params[5], *( (int *) params[6] ), (char *) params[7], &data8, *( (int *) params[10] ) );
}

void test_rsa_pkcs1_encrypt_bad_rng( data_t * message_str, int padding_mode,
                                int mod, int radix_N, char * input_N,
                                int radix_E, char * input_E,
                                data_t * result_hex_str, int result )
{
    unsigned char output[1000];
    mbedtls_rsa_context ctx;

    mbedtls_mpi N, E;

    mbedtls_mpi_init( &N ); mbedtls_mpi_init( &E );
    mbedtls_rsa_init( &ctx, padding_mode, 0 );
    memset( output, 0x00, 1000 );

    TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_import( &ctx, &N, NULL, NULL, NULL, &E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_get_len( &ctx ) == (size_t) ( mod / 8 ) );
    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == 0 );


    TEST_ASSERT( mbedtls_rsa_pkcs1_encrypt( &ctx, &rnd_zero_rand, NULL,
                                            MBEDTLS_RSA_PUBLIC, message_str->len,
                                            message_str->x, output ) == result );
    if( result == 0 )
    {

        TEST_ASSERT( hexcmp( output, result_hex_str->x, ctx.len, result_hex_str->len ) == 0 );
    }

exit:
    mbedtls_mpi_free( &N ); mbedtls_mpi_free( &E );
    mbedtls_rsa_free( &ctx );
}

void test_rsa_pkcs1_encrypt_bad_rng_wrapper( void ** params )
{
    data_t data0;
    data_t data8;
    
    data0.x = (uint8_t *) params[0];
    data0.len =  *( (uint32_t *) params[1] );
    data8.x = (uint8_t *) params[8];
    data8.len = *( (uint32_t *) params[9] );

    test_rsa_pkcs1_encrypt_bad_rng( &data0, *( (int *) params[2] ), *( (int *) params[3] ), *( (int *) params[4] ), (char *) params[5], *( (int *) params[6] ), (char *) params[7], &data8, *( (int *) params[10] ) );
}

void test_mbedtls_rsa_pkcs1_decrypt( data_t * message_str, int padding_mode,
                                int mod, int radix_P, char * input_P,
                                int radix_Q, char * input_Q, int radix_N,
                                char * input_N, int radix_E, char * input_E,
                                int max_output, data_t * result_hex_str,
                                int result )
{
    unsigned char output[1000];
    mbedtls_rsa_context ctx;
    size_t output_len;
    rnd_pseudo_info rnd_info;
    mbedtls_mpi N, P, Q, E;

    mbedtls_mpi_init( &N ); mbedtls_mpi_init( &P );
    mbedtls_mpi_init( &Q ); mbedtls_mpi_init( &E );

    mbedtls_rsa_init( &ctx, padding_mode, 0 );

    memset( output, 0x00, 1000 );
    memset( &rnd_info, 0, sizeof( rnd_pseudo_info ) );


    TEST_ASSERT( mbedtls_mpi_read_string( &P, radix_P, input_P ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &Q, radix_Q, input_Q ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_import( &ctx, &N, &P, &Q, NULL, &E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_get_len( &ctx ) == (size_t) ( mod / 8 ) );
    TEST_ASSERT( mbedtls_rsa_complete( &ctx ) == 0 );
    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == 0 );

    output_len = 0;

    TEST_ASSERT( mbedtls_rsa_pkcs1_decrypt( &ctx, rnd_pseudo_rand, &rnd_info, MBEDTLS_RSA_PRIVATE, &output_len, message_str->x, output, max_output ) == result );
    if( result == 0 )
    {

        TEST_ASSERT( hexcmp( output, result_hex_str->x, output_len, result_hex_str->len ) == 0 );
    }

exit:
    mbedtls_mpi_free( &N ); mbedtls_mpi_free( &P );
    mbedtls_mpi_free( &Q ); mbedtls_mpi_free( &E );
    mbedtls_rsa_free( &ctx );
}

void test_mbedtls_rsa_pkcs1_decrypt_wrapper( void ** params )
{
    data_t data0;
    data_t data13;

    data0.x = (uint8_t *) params[0];
    data0.len =  *( (uint32_t *) params[1] );
    data13.x = (uint8_t *) params[13];
    data13.len = *( (uint32_t *) params[14] );

    test_mbedtls_rsa_pkcs1_decrypt( &data0, *( (int *) params[2] ), *( (int *) params[3] ), *( (int *) params[4] ), (char *) params[5], *( (int *) params[6] ), (char *) params[7], *( (int *) params[8] ), (char *) params[9], *( (int *) params[10] ), (char *) params[11], *( (int *) params[12] ), &data13, *( (int *) params[15] ) );
}

void test_mbedtls_rsa_public( data_t * message_str, int mod, int radix_N,
                         char * input_N, int radix_E, char * input_E,
                         data_t * result_hex_str, int result )
{
    unsigned char output[1000];
    mbedtls_rsa_context ctx, ctx2; /* Also test mbedtls_rsa_copy() while at it */

    mbedtls_mpi N, E;

    mbedtls_mpi_init( &N ); mbedtls_mpi_init( &E );
    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, 0 );
    mbedtls_rsa_init( &ctx2, MBEDTLS_RSA_PKCS_V15, 0 );
    memset( output, 0x00, 1000 );

    TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_import( &ctx, &N, NULL, NULL, NULL, &E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_get_len( &ctx ) == (size_t) ( mod / 8 ) );
    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == 0 );


    TEST_ASSERT( mbedtls_rsa_public( &ctx, message_str->x, output ) == result );
    if( result == 0 )
    {

        TEST_ASSERT( hexcmp( output, result_hex_str->x, ctx.len, result_hex_str->len ) == 0 );
    }

    /* And now with the copy */
    TEST_ASSERT( mbedtls_rsa_copy( &ctx2, &ctx ) == 0 );
    /* clear the original to be sure */
    mbedtls_rsa_free( &ctx );

    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx2 ) == 0 );

    memset( output, 0x00, 1000 );
    TEST_ASSERT( mbedtls_rsa_public( &ctx2, message_str->x, output ) == result );
    if( result == 0 )
    {

        TEST_ASSERT( hexcmp( output, result_hex_str->x, ctx.len, result_hex_str->len ) == 0 );
    }

exit:
    mbedtls_mpi_free( &N ); mbedtls_mpi_free( &E );
    mbedtls_rsa_free( &ctx );
    mbedtls_rsa_free( &ctx2 );
}

void test_mbedtls_rsa_public_wrapper( void ** params )
{
    data_t data0;
    data_t data7;

    data0.x = (uint8_t *) params[0];
    data0.len =  *( (uint32_t *) params[1] );
    data7.x = (uint8_t *) params[7];
    data7.len = *( (uint32_t *) params[8] );

    test_mbedtls_rsa_public( &data0, *( (int *) params[2] ), *( (int *) params[3] ), (char *) params[4], *( (int *) params[5] ), (char *) params[6], &data7, *( (int *) params[9] ) );
}

void test_mbedtls_rsa_private( data_t * message_str, int mod, int radix_P,
                          char * input_P, int radix_Q, char * input_Q,
                          int radix_N, char * input_N, int radix_E,
                          char * input_E, data_t * result_hex_str,
                          int result )
{
    unsigned char output[1000];
    mbedtls_rsa_context ctx, ctx2; /* Also test mbedtls_rsa_copy() while at it */
    mbedtls_mpi N, P, Q, E;
    rnd_pseudo_info rnd_info;
    int i;

    mbedtls_mpi_init( &N ); mbedtls_mpi_init( &P );
    mbedtls_mpi_init( &Q ); mbedtls_mpi_init( &E );
    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, 0 );
    mbedtls_rsa_init( &ctx2, MBEDTLS_RSA_PKCS_V15, 0 );

    memset( &rnd_info, 0, sizeof( rnd_pseudo_info ) );

    TEST_ASSERT( mbedtls_mpi_read_string( &P, radix_P, input_P ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &Q, radix_Q, input_Q ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_import( &ctx, &N, &P, &Q, NULL, &E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_get_len( &ctx ) == (size_t) ( mod / 8 ) );
    TEST_ASSERT( mbedtls_rsa_complete( &ctx ) == 0 );
    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == 0 );


    /* repeat three times to test updating of blinding values */
    for( i = 0; i < 3; i++ )
    {
        memset( output, 0x00, 1000 );
        TEST_ASSERT( mbedtls_rsa_private( &ctx, rnd_pseudo_rand, &rnd_info,
                                  message_str->x, output ) == result );
        if( result == 0 )
        {

            TEST_ASSERT( hexcmp( output, result_hex_str->x, ctx.len, result_hex_str->len ) == 0 );
        }
    }

    /* And now one more time with the copy */
    TEST_ASSERT( mbedtls_rsa_copy( &ctx2, &ctx ) == 0 );
    /* clear the original to be sure */
    mbedtls_rsa_free( &ctx );

    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx2 ) == 0 );

    memset( output, 0x00, 1000 );
    TEST_ASSERT( mbedtls_rsa_private( &ctx2, rnd_pseudo_rand, &rnd_info,
                              message_str->x, output ) == result );
    if( result == 0 )
    {

        TEST_ASSERT( hexcmp( output, result_hex_str->x, ctx2.len, result_hex_str->len ) == 0 );
    }

exit:
    mbedtls_mpi_free( &N ); mbedtls_mpi_free( &P );
    mbedtls_mpi_free( &Q ); mbedtls_mpi_free( &E );

    mbedtls_rsa_free( &ctx ); mbedtls_rsa_free( &ctx2 );
}

void test_mbedtls_rsa_private_wrapper( void ** params )
{
    data_t data0;
    data_t data11;

    data0.x = (uint8_t *) params[0];
    data0.len =  *( (uint32_t *) params[1] );
    data11.x = (uint8_t *) params[11];
    data11.len = *( (uint32_t *) params[12] );

    test_mbedtls_rsa_private( &data0, *( (int *) params[2] ), *( (int *) params[3] ), (char *) params[4], *( (int *) params[5] ), (char *) params[6], *( (int *) params[7] ), (char *) params[8], *( (int *) params[9] ), (char *) params[10], &data11, *( (int *) params[13] ) );
}

void test_rsa_check_privkey_null(  )
{
    mbedtls_rsa_context ctx;
    memset( &ctx, 0x00, sizeof( mbedtls_rsa_context ) );

    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == MBEDTLS_ERR_RSA_KEY_CHECK_FAILED );
exit:
    ;
}

void test_rsa_check_privkey_null_wrapper( void ** params )
{
    (void)params;

    test_rsa_check_privkey_null(  );
}

void test_mbedtls_rsa_check_pubkey( int radix_N, char * input_N, int radix_E,
                               char * input_E, int result )
{
    mbedtls_rsa_context ctx;
    mbedtls_mpi N, E;

    mbedtls_mpi_init( &N ); mbedtls_mpi_init( &E );
    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, 0 );

    if( strlen( input_N ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );
    }
    if( strlen( input_E ) )
    {
        TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );
    }

    TEST_ASSERT( mbedtls_rsa_import( &ctx, &N, NULL, NULL, NULL, &E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == result );

exit:
    mbedtls_mpi_free( &N ); mbedtls_mpi_free( &E );
    mbedtls_rsa_free( &ctx );
}

void test_mbedtls_rsa_check_pubkey_wrapper( void ** params )
{

    test_mbedtls_rsa_check_pubkey( *( (int *) params[0] ), (char *) params[1], *( (int *) params[2] ), (char *) params[3], *( (int *) params[4] ) );
}

void test_mbedtls_rsa_check_privkey( int mod, int radix_P, char * input_P,
                                int radix_Q, char * input_Q, int radix_N,
                                char * input_N, int radix_E, char * input_E,
                                int radix_D, char * input_D, int radix_DP,
                                char * input_DP, int radix_DQ,
                                char * input_DQ, int radix_QP,
                                char * input_QP, int result )
{
    mbedtls_rsa_context ctx;

    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, 0 );

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
#if !defined(MBEDTLS_RSA_NO_CRT)
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
#else
    ((void) radix_DP); ((void) input_DP);
    ((void) radix_DQ); ((void) input_DQ);
    ((void) radix_QP); ((void) input_QP);
#endif

    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == result );

exit:
    mbedtls_rsa_free( &ctx );
}

void test_mbedtls_rsa_check_privkey_wrapper( void ** params )
{

    test_mbedtls_rsa_check_privkey( *( (int *) params[0] ), *( (int *) params[1] ), (char *) params[2], *( (int *) params[3] ), (char *) params[4], *( (int *) params[5] ), (char *) params[6], *( (int *) params[7] ), (char *) params[8], *( (int *) params[9] ), (char *) params[10], *( (int *) params[11] ), (char *) params[12], *( (int *) params[13] ), (char *) params[14], *( (int *) params[15] ), (char *) params[16], *( (int *) params[17] ) );
}

void test_rsa_check_pubpriv( int mod, int radix_Npub, char * input_Npub,
                        int radix_Epub, char * input_Epub, int radix_P,
                        char * input_P, int radix_Q, char * input_Q,
                        int radix_N, char * input_N, int radix_E,
                        char * input_E, int radix_D, char * input_D,
                        int radix_DP, char * input_DP, int radix_DQ,
                        char * input_DQ, int radix_QP, char * input_QP,
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
#if !defined(MBEDTLS_RSA_NO_CRT)
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
#else
    ((void) radix_DP); ((void) input_DP);
    ((void) radix_DQ); ((void) input_DQ);
    ((void) radix_QP); ((void) input_QP);
#endif

    TEST_ASSERT( mbedtls_rsa_check_pub_priv( &pub, &prv ) == result );

exit:
    mbedtls_rsa_free( &pub );
    mbedtls_rsa_free( &prv );
}

void test_rsa_check_pubpriv_wrapper( void ** params )
{

    test_rsa_check_pubpriv( *( (int *) params[0] ), *( (int *) params[1] ), (char *) params[2], *( (int *) params[3] ), (char *) params[4], *( (int *) params[5] ), (char *) params[6], *( (int *) params[7] ), (char *) params[8], *( (int *) params[9] ), (char *) params[10], *( (int *) params[11] ), (char *) params[12], *( (int *) params[13] ), (char *) params[14], *( (int *) params[15] ), (char *) params[16], *( (int *) params[17] ), (char *) params[18], *( (int *) params[19] ), (char *) params[20], *( (int *) params[21] ) );
}
#if defined(MBEDTLS_CTR_DRBG_C)
#if defined(MBEDTLS_ENTROPY_C)
#if defined(ENTROPY_HAVE_STRONG)

void test_mbedtls_rsa_gen_key( int nrbits, int exponent, int result)
{
    mbedtls_rsa_context ctx;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "test_suite_rsa";

    mbedtls_ctr_drbg_init( &ctr_drbg );
    mbedtls_entropy_init( &entropy );
    mbedtls_rsa_init ( &ctx, 0, 0 );

    TEST_ASSERT( mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func,
                                        &entropy, (const unsigned char *) pers,
                                        strlen( pers ) ) == 0 );

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

void test_mbedtls_rsa_gen_key_wrapper( void ** params )
{

    test_mbedtls_rsa_gen_key( *( (int *) params[0] ), *( (int *) params[1] ), *( (int *) params[2] ) );
}
#endif /* ENTROPY_HAVE_STRONG */
#endif /* MBEDTLS_ENTROPY_C */
#endif /* MBEDTLS_CTR_DRBG_C */
#if defined(MBEDTLS_CTR_DRBG_C)
#if defined(MBEDTLS_ENTROPY_C)

void test_mbedtls_rsa_deduce_primes( int radix_N, char *input_N,
                                int radix_D, char *input_D,
                                int radix_E, char *input_E,
                                int radix_P, char *output_P,
                                int radix_Q, char *output_Q,
                                int corrupt, int result )
{
    mbedtls_mpi N, P, Pp, Q, Qp, D, E;

    mbedtls_mpi_init( &N );
    mbedtls_mpi_init( &P );  mbedtls_mpi_init( &Q  );
    mbedtls_mpi_init( &Pp ); mbedtls_mpi_init( &Qp );
    mbedtls_mpi_init( &D ); mbedtls_mpi_init( &E );

    TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &D, radix_D, input_D ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &Qp, radix_P, output_P ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &Pp, radix_Q, output_Q ) == 0 );

    if( corrupt )
        TEST_ASSERT( mbedtls_mpi_add_int( &D, &D, 2 ) == 0 );

    /* Try to deduce P, Q from N, D, E only. */
    TEST_ASSERT( mbedtls_rsa_deduce_primes( &N, &D, &E, &P, &Q ) == result );

    if( !corrupt )
    {
        /* Check if (P,Q) = (Pp, Qp) or (P,Q) = (Qp, Pp) */
        TEST_ASSERT( ( mbedtls_mpi_cmp_mpi( &P, &Pp ) == 0 && mbedtls_mpi_cmp_mpi( &Q, &Qp ) == 0 ) ||
                     ( mbedtls_mpi_cmp_mpi( &P, &Qp ) == 0 && mbedtls_mpi_cmp_mpi( &Q, &Pp ) == 0 ) );
    }

exit:
    mbedtls_mpi_free( &N );
    mbedtls_mpi_free( &P  ); mbedtls_mpi_free( &Q  );
    mbedtls_mpi_free( &Pp ); mbedtls_mpi_free( &Qp );
    mbedtls_mpi_free( &D ); mbedtls_mpi_free( &E );
}

void test_mbedtls_rsa_deduce_primes_wrapper( void ** params )
{

    test_mbedtls_rsa_deduce_primes( *( (int *) params[0] ), (char *) params[1], *( (int *) params[2] ), (char *) params[3], *( (int *) params[4] ), (char *) params[5], *( (int *) params[6] ), (char *) params[7], *( (int *) params[8] ), (char *) params[9], *( (int *) params[10] ), *( (int *) params[11] ) );
}
#endif /* MBEDTLS_ENTROPY_C */
#endif /* MBEDTLS_CTR_DRBG_C */

void test_mbedtls_rsa_deduce_private_exponent( int radix_P, char *input_P,
                                          int radix_Q, char *input_Q,
                                          int radix_E, char *input_E,
                                          int radix_D, char *output_D,
                                          int corrupt, int result )
{
    mbedtls_mpi P, Q, D, Dp, E, R, Rp;

    mbedtls_mpi_init( &P ); mbedtls_mpi_init( &Q );
    mbedtls_mpi_init( &D ); mbedtls_mpi_init( &Dp );
    mbedtls_mpi_init( &E );
    mbedtls_mpi_init( &R ); mbedtls_mpi_init( &Rp );

    TEST_ASSERT( mbedtls_mpi_read_string( &P, radix_P, input_P ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &Q, radix_Q, input_Q ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &Dp, radix_D, output_D ) == 0 );

    if( corrupt )
    {
        /* Make E even */
        TEST_ASSERT( mbedtls_mpi_set_bit( &E, 0, 0 ) == 0 );
    }

    /* Try to deduce D from N, P, Q, E. */
    TEST_ASSERT( mbedtls_rsa_deduce_private_exponent( &P, &Q,
                                                      &E, &D ) == result );

    if( !corrupt )
    {
        /*
         * Check that D and Dp agree modulo LCM(P-1, Q-1).
         */

        /* Replace P,Q by P-1, Q-1 */
        TEST_ASSERT( mbedtls_mpi_sub_int( &P, &P, 1 ) == 0 );
        TEST_ASSERT( mbedtls_mpi_sub_int( &Q, &Q, 1 ) == 0 );

        /* Check D == Dp modulo P-1 */
        TEST_ASSERT( mbedtls_mpi_mod_mpi( &R,  &D,  &P ) == 0 );
        TEST_ASSERT( mbedtls_mpi_mod_mpi( &Rp, &Dp, &P ) == 0 );
        TEST_ASSERT( mbedtls_mpi_cmp_mpi( &R,  &Rp )     == 0 );

        /* Check D == Dp modulo Q-1 */
        TEST_ASSERT( mbedtls_mpi_mod_mpi( &R,  &D,  &Q ) == 0 );
        TEST_ASSERT( mbedtls_mpi_mod_mpi( &Rp, &Dp, &Q ) == 0 );
        TEST_ASSERT( mbedtls_mpi_cmp_mpi( &R,  &Rp )     == 0 );
    }

exit:

    mbedtls_mpi_free( &P ); mbedtls_mpi_free( &Q  );
    mbedtls_mpi_free( &D ); mbedtls_mpi_free( &Dp );
    mbedtls_mpi_free( &E );
    mbedtls_mpi_free( &R ); mbedtls_mpi_free( &Rp );
}

void test_mbedtls_rsa_deduce_private_exponent_wrapper( void ** params )
{

    test_mbedtls_rsa_deduce_private_exponent( *( (int *) params[0] ), (char *) params[1], *( (int *) params[2] ), (char *) params[3], *( (int *) params[4] ), (char *) params[5], *( (int *) params[6] ), (char *) params[7], *( (int *) params[8] ), *( (int *) params[9] ) );
}
#if defined(MBEDTLS_CTR_DRBG_C)
#if defined(MBEDTLS_ENTROPY_C)
#if defined(ENTROPY_HAVE_STRONG)

void test_mbedtls_rsa_import( int radix_N, char *input_N,
                         int radix_P, char *input_P,
                         int radix_Q, char *input_Q,
                         int radix_D, char *input_D,
                         int radix_E, char *input_E,
                         int successive,
                         int is_priv,
                         int res_check,
                         int res_complete )
{
    mbedtls_mpi N, P, Q, D, E;
    mbedtls_rsa_context ctx;

    /* Buffers used for encryption-decryption test */
    unsigned char *buf_orig = NULL;
    unsigned char *buf_enc  = NULL;
    unsigned char *buf_dec  = NULL;

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "test_suite_rsa";

    const int have_N = ( strlen( input_N ) > 0 );
    const int have_P = ( strlen( input_P ) > 0 );
    const int have_Q = ( strlen( input_Q ) > 0 );
    const int have_D = ( strlen( input_D ) > 0 );
    const int have_E = ( strlen( input_E ) > 0 );

    mbedtls_ctr_drbg_init( &ctr_drbg );
    mbedtls_entropy_init( &entropy );
    mbedtls_rsa_init( &ctx, 0, 0 );

    mbedtls_mpi_init( &N );
    mbedtls_mpi_init( &P ); mbedtls_mpi_init( &Q );
    mbedtls_mpi_init( &D ); mbedtls_mpi_init( &E );

    TEST_ASSERT( mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                                (const unsigned char *) pers, strlen( pers ) ) == 0 );

    if( have_N )
        TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );

    if( have_P )
        TEST_ASSERT( mbedtls_mpi_read_string( &P, radix_P, input_P ) == 0 );

    if( have_Q )
        TEST_ASSERT( mbedtls_mpi_read_string( &Q, radix_Q, input_Q ) == 0 );

    if( have_D )
        TEST_ASSERT( mbedtls_mpi_read_string( &D, radix_D, input_D ) == 0 );

    if( have_E )
        TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );

    if( !successive )
    {
        TEST_ASSERT( mbedtls_rsa_import( &ctx,
                             have_N ? &N : NULL,
                             have_P ? &P : NULL,
                             have_Q ? &Q : NULL,
                             have_D ? &D : NULL,
                             have_E ? &E : NULL ) == 0 );
    }
    else
    {
        /* Import N, P, Q, D, E separately.
         * This should make no functional difference. */

        TEST_ASSERT( mbedtls_rsa_import( &ctx,
                               have_N ? &N : NULL,
                               NULL, NULL, NULL, NULL ) == 0 );

        TEST_ASSERT( mbedtls_rsa_import( &ctx,
                               NULL,
                               have_P ? &P : NULL,
                               NULL, NULL, NULL ) == 0 );

        TEST_ASSERT( mbedtls_rsa_import( &ctx,
                               NULL, NULL,
                               have_Q ? &Q : NULL,
                               NULL, NULL ) == 0 );

        TEST_ASSERT( mbedtls_rsa_import( &ctx,
                               NULL, NULL, NULL,
                               have_D ? &D : NULL,
                               NULL ) == 0 );

        TEST_ASSERT( mbedtls_rsa_import( &ctx,
                               NULL, NULL, NULL, NULL,
                               have_E ? &E : NULL ) == 0 );
    }

    TEST_ASSERT( mbedtls_rsa_complete( &ctx ) == res_complete );

    /* On expected success, perform some public and private
     * key operations to check if the key is working properly. */
    if( res_complete == 0 )
    {
        if( is_priv )
            TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == res_check );
        else
            TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == res_check );

        if( res_check != 0 )
            goto exit;

        buf_orig = mbedtls_calloc( 1, mbedtls_rsa_get_len( &ctx ) );
        buf_enc  = mbedtls_calloc( 1, mbedtls_rsa_get_len( &ctx ) );
        buf_dec  = mbedtls_calloc( 1, mbedtls_rsa_get_len( &ctx ) );
        if( buf_orig == NULL || buf_enc == NULL || buf_dec == NULL )
            goto exit;

        TEST_ASSERT( mbedtls_ctr_drbg_random( &ctr_drbg,
                              buf_orig, mbedtls_rsa_get_len( &ctx ) ) == 0 );

        /* Make sure the number we're generating is smaller than the modulus */
        buf_orig[0] = 0x00;

        TEST_ASSERT( mbedtls_rsa_public( &ctx, buf_orig, buf_enc ) == 0 );

        if( is_priv )
        {
            TEST_ASSERT( mbedtls_rsa_private( &ctx, mbedtls_ctr_drbg_random,
                                              &ctr_drbg, buf_enc,
                                              buf_dec ) == 0 );

            TEST_ASSERT( memcmp( buf_orig, buf_dec,
                                 mbedtls_rsa_get_len( &ctx ) ) == 0 );
        }
    }

exit:

    mbedtls_free( buf_orig );
    mbedtls_free( buf_enc  );
    mbedtls_free( buf_dec  );

    mbedtls_rsa_free( &ctx );

    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );

    mbedtls_mpi_free( &N );
    mbedtls_mpi_free( &P ); mbedtls_mpi_free( &Q );
    mbedtls_mpi_free( &D ); mbedtls_mpi_free( &E );
}

void test_mbedtls_rsa_import_wrapper( void ** params )
{

    test_mbedtls_rsa_import( *( (int *) params[0] ), (char *) params[1], *( (int *) params[2] ), (char *) params[3], *( (int *) params[4] ), (char *) params[5], *( (int *) params[6] ), (char *) params[7], *( (int *) params[8] ), (char *) params[9], *( (int *) params[10] ), *( (int *) params[11] ), *( (int *) params[12] ), *( (int *) params[13] ) );
}
#endif /* ENTROPY_HAVE_STRONG */
#endif /* MBEDTLS_ENTROPY_C */
#endif /* MBEDTLS_CTR_DRBG_C */

void test_mbedtls_rsa_export( int radix_N, char *input_N,
                         int radix_P, char *input_P,
                         int radix_Q, char *input_Q,
                         int radix_D, char *input_D,
                         int radix_E, char *input_E,
                         int is_priv,
                         int successive )
{
    /* Original MPI's with which we set up the RSA context */
    mbedtls_mpi N, P, Q, D, E;

    /* Exported MPI's */
    mbedtls_mpi Ne, Pe, Qe, De, Ee;

    const int have_N = ( strlen( input_N ) > 0 );
    const int have_P = ( strlen( input_P ) > 0 );
    const int have_Q = ( strlen( input_Q ) > 0 );
    const int have_D = ( strlen( input_D ) > 0 );
    const int have_E = ( strlen( input_E ) > 0 );

    mbedtls_rsa_context ctx;

    mbedtls_rsa_init( &ctx, 0, 0 );

    mbedtls_mpi_init( &N );
    mbedtls_mpi_init( &P ); mbedtls_mpi_init( &Q );
    mbedtls_mpi_init( &D ); mbedtls_mpi_init( &E );

    mbedtls_mpi_init( &Ne );
    mbedtls_mpi_init( &Pe ); mbedtls_mpi_init( &Qe );
    mbedtls_mpi_init( &De ); mbedtls_mpi_init( &Ee );

    /* Setup RSA context */

    if( have_N )
        TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );

    if( have_P )
        TEST_ASSERT( mbedtls_mpi_read_string( &P, radix_P, input_P ) == 0 );

    if( have_Q )
        TEST_ASSERT( mbedtls_mpi_read_string( &Q, radix_Q, input_Q ) == 0 );

    if( have_D )
        TEST_ASSERT( mbedtls_mpi_read_string( &D, radix_D, input_D ) == 0 );

    if( have_E )
        TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_import( &ctx,
                                     strlen( input_N ) ? &N : NULL,
                                     strlen( input_P ) ? &P : NULL,
                                     strlen( input_Q ) ? &Q : NULL,
                                     strlen( input_D ) ? &D : NULL,
                                     strlen( input_E ) ? &E : NULL ) == 0 );

    TEST_ASSERT( mbedtls_rsa_complete( &ctx ) == 0 );

    /*
     * Export parameters and compare to original ones.
     */

    /* N and E must always be present. */
    if( !successive )
    {
        TEST_ASSERT( mbedtls_rsa_export( &ctx, &Ne, NULL, NULL, NULL, &Ee ) == 0 );
    }
    else
    {
        TEST_ASSERT( mbedtls_rsa_export( &ctx, &Ne, NULL, NULL, NULL, NULL ) == 0 );
        TEST_ASSERT( mbedtls_rsa_export( &ctx, NULL, NULL, NULL, NULL, &Ee ) == 0 );
    }
    TEST_ASSERT( mbedtls_mpi_cmp_mpi( &N, &Ne ) == 0 );
    TEST_ASSERT( mbedtls_mpi_cmp_mpi( &E, &Ee ) == 0 );

    /* If we were providing enough information to setup a complete private context,
     * we expect to be able to export all core parameters. */

    if( is_priv )
    {
        if( !successive )
        {
            TEST_ASSERT( mbedtls_rsa_export( &ctx, NULL, &Pe, &Qe,
                                             &De, NULL ) == 0 );
        }
        else
        {
            TEST_ASSERT( mbedtls_rsa_export( &ctx, NULL, &Pe, NULL,
                                             NULL, NULL ) == 0 );
            TEST_ASSERT( mbedtls_rsa_export( &ctx, NULL, NULL, &Qe,
                                             NULL, NULL ) == 0 );
            TEST_ASSERT( mbedtls_rsa_export( &ctx, NULL, NULL, NULL,
                                             &De, NULL ) == 0 );
        }

        if( have_P )
            TEST_ASSERT( mbedtls_mpi_cmp_mpi( &P, &Pe ) == 0 );

        if( have_Q )
            TEST_ASSERT( mbedtls_mpi_cmp_mpi( &Q, &Qe ) == 0 );

        if( have_D )
            TEST_ASSERT( mbedtls_mpi_cmp_mpi( &D, &De ) == 0 );

        /* While at it, perform a sanity check */
        TEST_ASSERT( mbedtls_rsa_validate_params( &Ne, &Pe, &Qe, &De, &Ee,
                                                       NULL, NULL ) == 0 );
    }

exit:

    mbedtls_rsa_free( &ctx );

    mbedtls_mpi_free( &N );
    mbedtls_mpi_free( &P ); mbedtls_mpi_free( &Q );
    mbedtls_mpi_free( &D ); mbedtls_mpi_free( &E );

    mbedtls_mpi_free( &Ne );
    mbedtls_mpi_free( &Pe ); mbedtls_mpi_free( &Qe );
    mbedtls_mpi_free( &De ); mbedtls_mpi_free( &Ee );
}

void test_mbedtls_rsa_export_wrapper( void ** params )
{

    test_mbedtls_rsa_export( *( (int *) params[0] ), (char *) params[1], *( (int *) params[2] ), (char *) params[3], *( (int *) params[4] ), (char *) params[5], *( (int *) params[6] ), (char *) params[7], *( (int *) params[8] ), (char *) params[9], *( (int *) params[10] ), *( (int *) params[11] ) );
}
#if defined(MBEDTLS_ENTROPY_C)
#if defined(ENTROPY_HAVE_STRONG)

void test_mbedtls_rsa_validate_params( int radix_N, char *input_N,
                                  int radix_P, char *input_P,
                                  int radix_Q, char *input_Q,
                                  int radix_D, char *input_D,
                                  int radix_E, char *input_E,
                                  int prng, int result )
{
    /* Original MPI's with which we set up the RSA context */
    mbedtls_mpi N, P, Q, D, E;

    const int have_N = ( strlen( input_N ) > 0 );
    const int have_P = ( strlen( input_P ) > 0 );
    const int have_Q = ( strlen( input_Q ) > 0 );
    const int have_D = ( strlen( input_D ) > 0 );
    const int have_E = ( strlen( input_E ) > 0 );

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "test_suite_rsa";

    mbedtls_mpi_init( &N );
    mbedtls_mpi_init( &P ); mbedtls_mpi_init( &Q );
    mbedtls_mpi_init( &D ); mbedtls_mpi_init( &E );

    mbedtls_ctr_drbg_init( &ctr_drbg );
    mbedtls_entropy_init( &entropy );
    TEST_ASSERT( mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func,
                                        &entropy, (const unsigned char *) pers,
                                        strlen( pers ) ) == 0 );

    if( have_N )
        TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );

    if( have_P )
        TEST_ASSERT( mbedtls_mpi_read_string( &P, radix_P, input_P ) == 0 );

    if( have_Q )
        TEST_ASSERT( mbedtls_mpi_read_string( &Q, radix_Q, input_Q ) == 0 );

    if( have_D )
        TEST_ASSERT( mbedtls_mpi_read_string( &D, radix_D, input_D ) == 0 );

    if( have_E )
        TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_validate_params( have_N ? &N : NULL,
                                        have_P ? &P : NULL,
                                        have_Q ? &Q : NULL,
                                        have_D ? &D : NULL,
                                        have_E ? &E : NULL,
                                        prng ? mbedtls_ctr_drbg_random : NULL,
                                        prng ? &ctr_drbg : NULL ) == result );
exit:

    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );

    mbedtls_mpi_free( &N );
    mbedtls_mpi_free( &P ); mbedtls_mpi_free( &Q );
    mbedtls_mpi_free( &D ); mbedtls_mpi_free( &E );
}

void test_mbedtls_rsa_validate_params_wrapper( void ** params )
{

    test_mbedtls_rsa_validate_params( *( (int *) params[0] ), (char *) params[1], *( (int *) params[2] ), (char *) params[3], *( (int *) params[4] ), (char *) params[5], *( (int *) params[6] ), (char *) params[7], *( (int *) params[8] ), (char *) params[9], *( (int *) params[10] ), *( (int *) params[11] ) );
}
#endif /* ENTROPY_HAVE_STRONG */
#endif /* MBEDTLS_ENTROPY_C */
#if defined(MBEDTLS_CTR_DRBG_C)
#if defined(MBEDTLS_ENTROPY_C)

void test_mbedtls_rsa_export_raw( data_t *input_N, data_t *input_P,
                             data_t *input_Q, data_t *input_D,
                             data_t *input_E, int is_priv,
                             int successive )
{
    /* Exported buffers */
    unsigned char bufNe[1000];
    unsigned char bufPe[1000];
    unsigned char bufQe[1000];
    unsigned char bufDe[1000];
    unsigned char bufEe[1000];

    mbedtls_rsa_context ctx;

    mbedtls_rsa_init( &ctx, 0, 0 );

    /* Setup RSA context */
    TEST_ASSERT( mbedtls_rsa_import_raw( &ctx,
                               input_N->len ? input_N->x : NULL, input_N->len,
                               input_P->len ? input_P->x : NULL, input_P->len,
                               input_Q->len ? input_Q->x : NULL, input_Q->len,
                               input_D->len ? input_D->x : NULL, input_D->len,
                               input_E->len ? input_E->x : NULL, input_E->len ) == 0 );

    TEST_ASSERT( mbedtls_rsa_complete( &ctx ) == 0 );

    /*
     * Export parameters and compare to original ones.
     */

    /* N and E must always be present. */
    if( !successive )
    {
        TEST_ASSERT( mbedtls_rsa_export_raw( &ctx, bufNe, input_N->len,
                                             NULL, 0, NULL, 0, NULL, 0,
                                             bufEe, input_E->len ) == 0 );
    }
    else
    {
        TEST_ASSERT( mbedtls_rsa_export_raw( &ctx, bufNe, input_N->len,
                                             NULL, 0, NULL, 0, NULL, 0,
                                             NULL, 0 ) == 0 );
        TEST_ASSERT( mbedtls_rsa_export_raw( &ctx, NULL, 0,
                                             NULL, 0, NULL, 0, NULL, 0,
                                             bufEe, input_E->len ) == 0 );
    }
    TEST_ASSERT( memcmp( input_N->x, bufNe, input_N->len ) == 0 );
    TEST_ASSERT( memcmp( input_E->x, bufEe, input_E->len ) == 0 );

    /* If we were providing enough information to setup a complete private context,
     * we expect to be able to export all core parameters. */

    if( is_priv )
    {
        if( !successive )
        {
            TEST_ASSERT( mbedtls_rsa_export_raw( &ctx, NULL, 0,
                                         bufPe, input_P->len ? input_P->len : sizeof( bufPe ),
                                         bufQe, input_Q->len ? input_Q->len : sizeof( bufQe ),
                                         bufDe, input_D->len ? input_D->len : sizeof( bufDe ),
                                         NULL, 0 ) == 0 );
        }
        else
        {
            TEST_ASSERT( mbedtls_rsa_export_raw( &ctx, NULL, 0,
                                         bufPe, input_P->len ? input_P->len : sizeof( bufPe ),
                                         NULL, 0, NULL, 0,
                                         NULL, 0 ) == 0 );

            TEST_ASSERT( mbedtls_rsa_export_raw( &ctx, NULL, 0, NULL, 0,
                                         bufQe, input_Q->len ? input_Q->len : sizeof( bufQe ),
                                         NULL, 0, NULL, 0 ) == 0 );

            TEST_ASSERT( mbedtls_rsa_export_raw( &ctx, NULL, 0, NULL, 0, NULL, 0,
                                         bufDe, input_D->len ? input_D->len : sizeof( bufDe ),
                                         NULL, 0 ) == 0 );
        }

        if( input_P->len )
            TEST_ASSERT( memcmp( input_P->x, bufPe, input_P->len ) == 0 );

        if( input_Q->len )
            TEST_ASSERT( memcmp( input_Q->x, bufQe, input_Q->len ) == 0 );

        if( input_D->len )
            TEST_ASSERT( memcmp( input_D->x, bufDe, input_D->len ) == 0 );

    }

exit:
    mbedtls_rsa_free( &ctx );
}

void test_mbedtls_rsa_export_raw_wrapper( void ** params )
{
    data_t data0;
    data_t data2;
    data_t data4;
    data_t data6;
    data_t data8;

    data0.x = (uint8_t *) params[0];
    data0.len =  *( (uint32_t *) params[1] );
    data2.x = (uint8_t *) params[2];
    data2.len = *( (uint32_t *) params[3] );
    data4.x = (uint8_t *) params[4];
    data4.len =  *( (uint32_t *) params[5] );
    data6.x = (uint8_t *) params[6];
    data6.len = *( (uint32_t *) params[7] );
    data8.x = (uint8_t *) params[8];
    data8.len =  *( (uint32_t *) params[9] );



    test_mbedtls_rsa_export_raw( &data0, &data2, &data4, &data6, &data8, *( (int *) params[10] ), *( (int *) params[11] ) );
}
#endif /* MBEDTLS_ENTROPY_C */
#endif /* MBEDTLS_CTR_DRBG_C */
#if defined(MBEDTLS_CTR_DRBG_C)
#if defined(MBEDTLS_ENTROPY_C)
#if defined(ENTROPY_HAVE_STRONG)

void test_mbedtls_rsa_import_raw( data_t *input_N,
                             data_t *input_P, data_t *input_Q,
                             data_t *input_D, data_t *input_E,
                             int successive,
                             int is_priv,
                             int res_check,
                             int res_complete )
{
    /* Buffers used for encryption-decryption test */
    unsigned char *buf_orig = NULL;
    unsigned char *buf_enc  = NULL;
    unsigned char *buf_dec  = NULL;

    mbedtls_rsa_context ctx;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

    const char *pers = "test_suite_rsa";

    mbedtls_ctr_drbg_init( &ctr_drbg );
    mbedtls_entropy_init( &entropy );
    mbedtls_rsa_init( &ctx, 0, 0 );

    TEST_ASSERT( mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func,
                                        &entropy, (const unsigned char *) pers,
                                        strlen( pers ) ) == 0 );

    if( !successive )
    {
        TEST_ASSERT( mbedtls_rsa_import_raw( &ctx,
                               ( input_N->len > 0 ) ? input_N->x : NULL, input_N->len,
                               ( input_P->len > 0 ) ? input_P->x : NULL, input_P->len,
                               ( input_Q->len > 0 ) ? input_Q->x : NULL, input_Q->len,
                               ( input_D->len > 0 ) ? input_D->x : NULL, input_D->len,
                               ( input_E->len > 0 ) ? input_E->x : NULL, input_E->len ) == 0 );
    }
    else
    {
        /* Import N, P, Q, D, E separately.
         * This should make no functional difference. */

        TEST_ASSERT( mbedtls_rsa_import_raw( &ctx,
                               ( input_N->len > 0 ) ? input_N->x : NULL, input_N->len,
                               NULL, 0, NULL, 0, NULL, 0, NULL, 0 ) == 0 );

        TEST_ASSERT( mbedtls_rsa_import_raw( &ctx,
                               NULL, 0,
                               ( input_P->len > 0 ) ? input_P->x : NULL, input_P->len,
                               NULL, 0, NULL, 0, NULL, 0 ) == 0 );

        TEST_ASSERT( mbedtls_rsa_import_raw( &ctx,
                               NULL, 0, NULL, 0,
                               ( input_Q->len > 0 ) ? input_Q->x : NULL, input_Q->len,
                               NULL, 0, NULL, 0 ) == 0 );

        TEST_ASSERT( mbedtls_rsa_import_raw( &ctx,
                               NULL, 0, NULL, 0, NULL, 0,
                               ( input_D->len > 0 ) ? input_D->x : NULL, input_D->len,
                               NULL, 0 ) == 0 );

        TEST_ASSERT( mbedtls_rsa_import_raw( &ctx,
                               NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                               ( input_E->len > 0 ) ? input_E->x : NULL, input_E->len ) == 0 );
    }

    TEST_ASSERT( mbedtls_rsa_complete( &ctx ) == res_complete );

    /* On expected success, perform some public and private
     * key operations to check if the key is working properly. */
    if( res_complete == 0 )
    {
        if( is_priv )
            TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == res_check );
        else
            TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == res_check );

        if( res_check != 0 )
            goto exit;

        buf_orig = mbedtls_calloc( 1, mbedtls_rsa_get_len( &ctx ) );
        buf_enc  = mbedtls_calloc( 1, mbedtls_rsa_get_len( &ctx ) );
        buf_dec  = mbedtls_calloc( 1, mbedtls_rsa_get_len( &ctx ) );
        if( buf_orig == NULL || buf_enc == NULL || buf_dec == NULL )
            goto exit;

        TEST_ASSERT( mbedtls_ctr_drbg_random( &ctr_drbg,
                              buf_orig, mbedtls_rsa_get_len( &ctx ) ) == 0 );

        /* Make sure the number we're generating is smaller than the modulus */
        buf_orig[0] = 0x00;

        TEST_ASSERT( mbedtls_rsa_public( &ctx, buf_orig, buf_enc ) == 0 );

        if( is_priv )
        {
            TEST_ASSERT( mbedtls_rsa_private( &ctx, mbedtls_ctr_drbg_random,
                                              &ctr_drbg, buf_enc,
                                              buf_dec ) == 0 );

            TEST_ASSERT( memcmp( buf_orig, buf_dec,
                                 mbedtls_rsa_get_len( &ctx ) ) == 0 );
        }
    }

exit:

    mbedtls_free( buf_orig );
    mbedtls_free( buf_enc  );
    mbedtls_free( buf_dec  );

    mbedtls_rsa_free( &ctx );

    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );

}

void test_mbedtls_rsa_import_raw_wrapper( void ** params )
{
    data_t data0;
    data_t data2;
    data_t data4;
    data_t data6;
    data_t data8;

    data0.x = (uint8_t *) params[0];
    data0.len =  *( (uint32_t *) params[1] );
    data2.x = (uint8_t *) params[2];
    data2.len = *( (uint32_t *) params[3] );
    data4.x = (uint8_t *) params[4];
    data4.len =  *( (uint32_t *) params[5] );
    data6.x = (uint8_t *) params[6];
    data6.len = *( (uint32_t *) params[7] );
    data8.x = (uint8_t *) params[8];
    data8.len =  *( (uint32_t *) params[9] );

    test_mbedtls_rsa_import_raw( &data0, &data2, &data4, &data6, &data8, *( (int *) params[10] ), *( (int *) params[11] ), *( (int *) params[12] ), *( (int *) params[13] ) );
}
#endif /* ENTROPY_HAVE_STRONG */
#endif /* MBEDTLS_ENTROPY_C */
#endif /* MBEDTLS_CTR_DRBG_C */
#if defined(MBEDTLS_SELF_TEST)

void test_rsa_selftest(  )
{
    TEST_ASSERT( mbedtls_rsa_self_test( 1 ) == 0 );
exit:
    ;
}

void test_rsa_selftest_wrapper( void ** params )
{
    (void)params;

    test_rsa_selftest(  );
}
#endif /* MBEDTLS_SELF_TEST */
#endif /* MBEDTLS_GENPRIME */
#endif /* MBEDTLS_BIGNUM_C */
#endif /* MBEDTLS_RSA_C */


/*----------------------------------------------------------------------------*/
/* Test dispatch code */


/**
 * \brief       Evaluates an expression/macro into its literal integer value.
 *              For optimizing space for embedded targets each expression/macro
 *              is identified by a unique identifier instead of string literals.
 *              Identifiers and evaluation code is generated by script:
 *              generate_test_code.py
 *
 * \param exp_id    Expression identifier.
 * \param out_value Pointer to int to hold the integer.
 *
 * \return       0 if exp_id is found. 1 otherwise.
 */
int get_expression( int32_t exp_id, int32_t * out_value )
{
    int ret = KEY_VALUE_MAPPING_FOUND;

    (void) exp_id;
    (void) out_value;

    switch( exp_id )
    {

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)

        case 0:
            {
                *out_value = MBEDTLS_RSA_PKCS_V15;
            }
            break;
        case 1:
            {
                *out_value = MBEDTLS_MD_SHA1;
            }
            break;
        case 2:
            {
                *out_value = MBEDTLS_ERR_RSA_VERIFY_FAILED;
            }
            break;
        case 3:
            {
                *out_value = MBEDTLS_MD_SHA224;
            }
            break;
        case 4:
            {
                *out_value = MBEDTLS_MD_SHA256;
            }
            break;
        case 5:
            {
                *out_value = MBEDTLS_MD_SHA384;
            }
            break;
        case 6:
            {
                *out_value = MBEDTLS_MD_SHA512;
            }
            break;
        case 7:
            {
                *out_value = MBEDTLS_MD_MD2;
            }
            break;
        case 8:
            {
                *out_value = MBEDTLS_MD_MD4;
            }
            break;
        case 9:
            {
                *out_value = MBEDTLS_MD_MD5;
            }
            break;
        case 10:
            {
                *out_value = MBEDTLS_ERR_RSA_BAD_INPUT_DATA;
            }
            break;
        case 11:
            {
                *out_value = MBEDTLS_ERR_RSA_INVALID_PADDING;
            }
            break;
        case 12:
            {
                *out_value = MBEDTLS_ERR_RSA_PRIVATE_FAILED + MBEDTLS_ERR_MPI_BAD_INPUT_DATA;
            }
            break;
        case 13:
            {
                *out_value = MBEDTLS_ERR_RSA_OUTPUT_TOO_LARGE;
            }
            break;
        case 14:
            {
                *out_value = MBEDTLS_ERR_RSA_KEY_CHECK_FAILED;
            }
            break;
        case 15:
            {
                *out_value = MBEDTLS_ERR_RSA_PUBLIC_FAILED + MBEDTLS_ERR_MPI_BAD_INPUT_DATA;
            }
            break;
        case 16:
            {
                *out_value = MBEDTLS_ERR_MPI_NOT_ACCEPTABLE;
            }
            break;
        case 17:
            {
                *out_value = MBEDTLS_ERR_MPI_BAD_INPUT_DATA;
            }
            break;
        case 18:
            {
                *out_value = MBEDTLS_ERR_RSA_RNG_FAILED;
            }
            break;
#endif

        default:
           {
                ret = KEY_VALUE_MAPPING_NOT_FOUND;
           }
           break;
    }
    return( ret );
}


/**
 * \brief       Checks if the dependency i.e. the compile flag is set.
 *              For optimizing space for embedded targets each dependency
 *              is identified by a unique identifier instead of string literals.
 *              Identifiers and check code is generated by script:
 *              generate_test_code.py
 *
 * \param exp_id    Dependency identifier.
 *
 * \return       DEPENDENCY_SUPPORTED if set else DEPENDENCY_NOT_SUPPORTED
 */
int dep_check( int dep_id )
{
    int ret = DEPENDENCY_NOT_SUPPORTED;

    (void) dep_id;

    switch( dep_id )
    {

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)

        case 0:
            {
#if defined(MBEDTLS_SHA1_C)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;
        case 1:
            {
#if defined(MBEDTLS_PKCS1_V15)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;
        case 2:
            {
#if defined(MBEDTLS_SHA256_C)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;
        case 3:
            {
#if defined(MBEDTLS_SHA512_C)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;
        case 4:
            {
#if defined(MBEDTLS_MD2_C)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;
        case 5:
            {
#if defined(MBEDTLS_MD4_C)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;
        case 6:
            {
#if defined(MBEDTLS_MD5_C)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;
        case 7:
            {
#if !defined(MBEDTLS_RSA_NO_CRT)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;
        case 8:
            {
#if defined(MBEDTLS_SELF_TEST)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;
#endif

        default:
            break;
    }
    return( ret );
}


/**
 * \brief       Function pointer type for test function wrappers.
 *
 *
 * \param void **   Pointer to void pointers. Represents an array of test
 *                  function parameters.
 *
 * \return       void
 */
typedef void (*TestWrapper_t)( void ** );


/**
 * \brief       Table of test function wrappers. Used by dispatch_test().
 *              This table is populated by script:
 *              generate_test_code.py
 *
 */
TestWrapper_t test_funcs[] =
{
/* Function Id: 0 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)
    test_mbedtls_rsa_pkcs1_sign_wrapper,
#else
    NULL,
#endif
/* Function Id: 1 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)
    test_mbedtls_rsa_pkcs1_verify_wrapper,
#else
    NULL,
#endif
/* Function Id: 2 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)
    test_rsa_pkcs1_sign_raw_wrapper,
#else
    NULL,
#endif
/* Function Id: 3 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)
    test_rsa_pkcs1_verify_raw_wrapper,
#else
    NULL,
#endif
/* Function Id: 4 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)
    test_mbedtls_rsa_pkcs1_encrypt_wrapper,
#else
    NULL,
#endif
/* Function Id: 5 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)
    test_rsa_pkcs1_encrypt_bad_rng_wrapper,
#else
    NULL,
#endif
/* Function Id: 6 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)
    test_mbedtls_rsa_pkcs1_decrypt_wrapper,
#else
    NULL,
#endif
/* Function Id: 7 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)
    test_mbedtls_rsa_public_wrapper,
#else
    NULL,
#endif
/* Function Id: 8 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)
    test_mbedtls_rsa_private_wrapper,
#else
    NULL,
#endif
/* Function Id: 9 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)
    test_rsa_check_privkey_null_wrapper,
#else
    NULL,
#endif
/* Function Id: 10 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)
    test_mbedtls_rsa_check_pubkey_wrapper,
#else
    NULL,
#endif
/* Function Id: 11 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)
    test_mbedtls_rsa_check_privkey_wrapper,
#else
    NULL,
#endif
/* Function Id: 12 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)
    test_rsa_check_pubpriv_wrapper,
#else
    NULL,
#endif
/* Function Id: 13 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME) && defined(MBEDTLS_CTR_DRBG_C) && defined(MBEDTLS_ENTROPY_C) && defined(ENTROPY_HAVE_STRONG)
    test_mbedtls_rsa_gen_key_wrapper,
#else
    NULL,
#endif
/* Function Id: 14 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME) && defined(MBEDTLS_CTR_DRBG_C) && defined(MBEDTLS_ENTROPY_C)
    test_mbedtls_rsa_deduce_primes_wrapper,
#else
    NULL,
#endif
/* Function Id: 15 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)
    test_mbedtls_rsa_deduce_private_exponent_wrapper,
#else
    NULL,
#endif
/* Function Id: 16 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME) && defined(MBEDTLS_CTR_DRBG_C) && defined(MBEDTLS_ENTROPY_C) && defined(ENTROPY_HAVE_STRONG)
    test_mbedtls_rsa_import_wrapper,
#else
    NULL,
#endif
/* Function Id: 17 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME)
    test_mbedtls_rsa_export_wrapper,
#else
    NULL,
#endif
/* Function Id: 18 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME) && defined(MBEDTLS_ENTROPY_C) && defined(ENTROPY_HAVE_STRONG)
    test_mbedtls_rsa_validate_params_wrapper,
#else
    NULL,
#endif
/* Function Id: 19 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME) && defined(MBEDTLS_CTR_DRBG_C) && defined(MBEDTLS_ENTROPY_C)
    test_mbedtls_rsa_export_raw_wrapper,
#else
    NULL,
#endif
/* Function Id: 20 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME) && defined(MBEDTLS_CTR_DRBG_C) && defined(MBEDTLS_ENTROPY_C) && defined(ENTROPY_HAVE_STRONG)
    test_mbedtls_rsa_import_raw_wrapper,
#else
    NULL,
#endif
/* Function Id: 21 */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_GENPRIME) && defined(MBEDTLS_SELF_TEST)
    test_rsa_selftest_wrapper,
#else
    NULL,
#endif

};


/**
 * \brief       Dispatches test functions based on function index.
 *
 * \param exp_id    Test function index.
 *
 * \return       DISPATCH_TEST_SUCCESS if found
 *               DISPATCH_TEST_FN_NOT_FOUND if not found
 *               DISPATCH_UNSUPPORTED_SUITE if not compile time enabled.
 */
int dispatch_test( int func_idx, void ** params )
{
    int ret = DISPATCH_TEST_SUCCESS;
    TestWrapper_t fp = NULL;

    if ( func_idx < (int)( sizeof( test_funcs ) / sizeof( TestWrapper_t ) ) )
    {
        fp = test_funcs[func_idx];
        if ( fp )
            fp( params );
        else
            ret = DISPATCH_UNSUPPORTED_SUITE;
    }
    else
    {
        ret = DISPATCH_TEST_FN_NOT_FOUND;
    }

    return( ret );
}


/**
 * \brief       Checks if test function is supported
 *
 * \param exp_id    Test function index.
 *
 * \return       DISPATCH_TEST_SUCCESS if found
 *               DISPATCH_TEST_FN_NOT_FOUND if not found
 *               DISPATCH_UNSUPPORTED_SUITE if not compile time enabled.
 */
int check_test( int func_idx )
{
    int ret = DISPATCH_TEST_SUCCESS;
    TestWrapper_t fp = NULL;

    if ( func_idx < (int)( sizeof(test_funcs)/sizeof( TestWrapper_t ) ) )
    {
        fp = test_funcs[func_idx];
        if ( fp == NULL )
            ret = DISPATCH_UNSUPPORTED_SUITE;
    }
    else
    {
        ret = DISPATCH_TEST_FN_NOT_FOUND;
    }

    return( ret );
}


/**
 * \brief       Verifies that string is in string parameter format i.e. "<str>"
 *              It also strips enclosing '"' from the input string.
 *
 * \param str   String parameter.
 *
 * \return      0 if success else 1
 */
int verify_string( char **str )
{
    if( ( *str )[0] != '"' ||
        ( *str )[strlen( *str ) - 1] != '"' )
    {
        printf("Expected string (with \"\") for parameter and got: %s\n", *str );
        return( -1 );
    }

    ( *str )++;
    ( *str )[strlen( *str ) - 1] = '\0';

    return( 0 );
}

/**
 * \brief       Verifies that string is an integer. Also gives the converted
 *              integer value.
 *
 * \param str   Input string.
 * \param value Pointer to int for output value.
 *
 * \return      0 if success else 1
 */
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
            str[i - 1] == '0' && ( str[i] == 'x' || str[i] == 'X' ) )
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

    printf("Expected integer for parameter and got: %s\n", str );
    return( KEY_VALUE_MAPPING_NOT_FOUND );
}

/**
 * \brief       Splits string delimited by ':'. Ignores '\:'.
 *
 * \param buf           Input string
 * \param len           Input string length
 * \param params        Out params found
 * \param params_len    Out params array len
 *
 * \return      Count of strings found.
 */
static int parse_arguments( char *buf, size_t len, char **params,
                            size_t params_len )
{
    size_t cnt = 0, i;
    char *cur = buf;
    char *p = buf, *q;

    params[cnt++] = cur;

    while( *p != '\0' && p < ( buf + len ) )
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
                assert( cnt < params_len );
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
            if( *p == '\\' && *( p + 1 ) == 'n' )
            {
                p += 2;
                *( q++ ) = '\n';
            }
            else if( *p == '\\' && *( p + 1 ) == ':' )
            {
                p += 2;
                *( q++ ) = ':';
            }
            else if( *p == '\\' && *( p + 1 ) == '?' )
            {
                p += 2;
                *( q++ ) = '?';
            }
            else
                *( q++ ) = *( p++ );
        }
        *q = '\0';
    }

    return( cnt );
}

/**
 * \brief       Converts parameters into test function consumable parameters.
 *              Example: Input:  {"int", "0", "char*", "Hello",
 *                                "hex", "abef", "exp", "1"}
 *                      Output:  {
 *                                0,                // Verified int
 *                                "Hello",          // Verified string
 *                                2, { 0xab, 0xef },// Converted len,hex pair
 *                                9600              // Evaluated expression
 *                               }
 *
 *
 * \param cnt               Parameter array count.
 * \param params            Out array of found parameters.
 * \param int_params_store  Memory for storing processed integer parameters.
 *
 * \return      0 for success else 1
 */
static int convert_params( size_t cnt , char ** params , int * int_params_store )
{
    char ** cur = params;
    char ** out = params;
    int ret = DISPATCH_TEST_SUCCESS;

    while ( cur < params + cnt )
    {
        char * type = *cur++;
        char * val = *cur++;

        if ( strcmp( type, "char*" ) == 0 )
        {
            if ( verify_string( &val ) == 0 )
            {
              *out++ = val;
            }
            else
            {
                ret = ( DISPATCH_INVALID_TEST_DATA );
                break;
            }
        }
        else if ( strcmp( type, "int" ) == 0 )
        {
            if ( verify_int( val, int_params_store ) == 0 )
            {
              *out++ = (char *) int_params_store++;
            }
            else
            {
                ret = ( DISPATCH_INVALID_TEST_DATA );
                break;
            }
        }
        else if ( strcmp( type, "hex" ) == 0 )
        {
            if ( verify_string( &val ) == 0 )
            {
                *int_params_store = unhexify( (unsigned char *) val, val );
                *out++ = val;
                *out++ = (char *)(int_params_store++);
            }
            else
            {
                ret = ( DISPATCH_INVALID_TEST_DATA );
                break;
            }
        }
        else if ( strcmp( type, "exp" ) == 0 )
        {
            int exp_id = strtol( val, NULL, 10 );
            if ( get_expression ( exp_id, int_params_store ) == 0 )
            {
              *out++ = (char *)int_params_store++;
            }
            else
            {
              ret = ( DISPATCH_INVALID_TEST_DATA );
              break;
            }
        }
        else
        {
          ret = ( DISPATCH_INVALID_TEST_DATA );
          break;
        }
    }
    return( ret );
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
    int   i, cnt, pass_cnt = 0, vector_no;
    int   function_id = 0;
    int   ret, total_tests=0, total_errors=0, total_skipped=0;
    char  *params[50];
    int   int_params[50];
    char  buf[5000];

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
            
            total_tests++;

            if (get_line(buf, sizeof(buf)) != 0)
                break;

            printf("%s \n", buf );

            if (get_line(buf, sizeof(buf)) != 0)
                break;
            cnt = parse_arguments( buf, strlen( buf ), params,
                                   sizeof( params ) / sizeof( params[0] ) );

            if (strcmp( params[0], "depends_on" ) == 0 )
            {
                for( i = 1; i < cnt; i++ )
                {
                	int dep_id = strtol( params[i], NULL, 10 );
                	
                    if( dep_check( dep_id ) != DEPENDENCY_SUPPORTED )
                    {
                        unmet_dep_count++;
                        break;
                    }
                }

                if (get_line(buf, sizeof(buf)) != 0)
                    break;

                cnt = parse_arguments( buf, strlen( buf ), params,
                                   sizeof( params ) / sizeof( params[0] ) );
            }

            // If there are no unmet dependencies execute the test
            if( unmet_dep_count == 0 )
            {
                function_id = strtol( params[0], NULL, 10 );
                if ( (ret = check_test( function_id )) == DISPATCH_TEST_SUCCESS )
                {
                    ret = convert_params( cnt - 1, params + 1, int_params );
                    if ( DISPATCH_TEST_SUCCESS == ret )
                    {
                        ret = dispatch_test( function_id, (void **)( params + 1 ) );
                    }
                }
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
            if (get_line(buf, sizeof(buf)) != 0)
                break;

            if( strlen(buf) != 0 )
            {
                printf("Should be empty %d\n", (int) strlen(buf) );
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


