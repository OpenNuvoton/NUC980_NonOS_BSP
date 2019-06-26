/*
 * *** THIS FILE HAS BEEN MACHINE GENERATED ***
 *
 * This file has been machine generated using the script:
 * generate_test_code.py
 *
 * Test file      : ./test_suite_pkcs1_v15.c
 *
 * The following files were used to create this file.
 *
 *      Main code file      : suites/main_test.function
 *      Platform code file  : suites/host_test.function
 *      Helper file         : suites/helpers.function
 *      Test suite file     : suites/test_suite_pkcs1_v15.function
 *      Test suite data     : suites/test_suite_pkcs1_v15.data
 *
 *
 *  This file is part of Mbed TLS (https://tls.mbed.org)
 */

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
    }
    while( !has_string );

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
    size_t i;

    if( rng_state != NULL )
        rng_state  = NULL;

    for( i = 0; i < len; ++i )
        output[i] = rand();

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

#if defined(MBEDTLS_PKCS1_V15)
#if defined(MBEDTLS_RSA_C)
#if defined(MBEDTLS_SHA1_C)
#include "mbedtls/rsa.h"
#include "mbedtls/md.h"
void test_pkcs1_rsaes_v15_encrypt( int mod, int radix_N, char * input_N,
                                   int radix_E, char * input_E, int hash,
                                   data_t * message_str, data_t * rnd_buf,
                                   data_t * result_hex_str, int result )
{
    unsigned char output[1000];
    mbedtls_rsa_context ctx;
    rnd_buf_info info;
    mbedtls_mpi N, E;

    RSA_claim_bit_length(mod);

    info.buf = rnd_buf->x;
    info.length = rnd_buf->len;

    mbedtls_mpi_init( &N );
    mbedtls_mpi_init( &E );
    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, hash );
    memset( output, 0x00, 1000 );

    TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_import( &ctx, &N, NULL, NULL, NULL, &E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_get_len( &ctx ) == (size_t) ( ( mod + 7 ) / 8 ) );
    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == 0 );


    TEST_ASSERT( mbedtls_rsa_pkcs1_encrypt( &ctx, &rnd_buffer_rand, &info, MBEDTLS_RSA_PUBLIC, message_str->len, message_str->x, output ) == result );
    if( result == 0 )
    {

        TEST_ASSERT( hexcmp( output, result_hex_str->x, ctx.len, result_hex_str->len ) == 0 );
    }

exit:
    mbedtls_mpi_free( &N );
    mbedtls_mpi_free( &E );
    mbedtls_rsa_free( &ctx );
}

void test_pkcs1_rsaes_v15_encrypt_wrapper( void ** params )
{
    data_t   data6, data8, data10;

    data6.x = (uint8_t *) params[6];
    data6.len = *( (uint32_t *) params[7] );

    data8.x = (uint8_t *) params[8];
    data8.len = *( (uint32_t *) params[9] );

    data10.x = (uint8_t *) params[10];
    data10.len = *( (uint32_t *) params[11] );

    test_pkcs1_rsaes_v15_encrypt( *( (int *) params[0] ), *( (int *) params[1] ), (char *) params[2], *( (int *) params[3] ), (char *) params[4], *( (int *) params[5] ), &data6, &data8, &data10, *( (int *) params[12] ) );
}

void test_pkcs1_rsaes_v15_decrypt( int mod, int radix_P, char * input_P,
                                   int radix_Q, char * input_Q, int radix_N,
                                   char * input_N, int radix_E, char * input_E,
                                   int hash, data_t * result_hex_str,
                                   char * seed, data_t * message_str,
                                   int result )
{
    unsigned char output[1000];
    mbedtls_rsa_context ctx;
    size_t output_len;
    rnd_pseudo_info rnd_info;
    mbedtls_mpi N, P, Q, E;
    ((void) seed);

    RSA_claim_bit_length(mod);

    mbedtls_mpi_init( &N );
    mbedtls_mpi_init( &P );
    mbedtls_mpi_init( &Q );
    mbedtls_mpi_init( &E );
    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, hash );

    memset( output, 0x00, 1000 );
    memset( &rnd_info, 0, sizeof( rnd_pseudo_info ) );

    TEST_ASSERT( mbedtls_mpi_read_string( &P, radix_P, input_P ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &Q, radix_Q, input_Q ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_import( &ctx, &N, &P, &Q, NULL, &E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_get_len( &ctx ) == (size_t) ( ( mod + 7 ) / 8 ) );
    TEST_ASSERT( mbedtls_rsa_complete( &ctx ) == 0 );
    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == 0 );


    TEST_ASSERT( mbedtls_rsa_pkcs1_decrypt( &ctx, &rnd_pseudo_rand, &rnd_info, MBEDTLS_RSA_PRIVATE, &output_len, message_str->x, output, 1000 ) == result );
    if( result == 0 )
    {

        TEST_ASSERT( hexcmp( output, result_hex_str->x, output_len, result_hex_str->len) == 0 );
    }

exit:
    mbedtls_mpi_free( &N );
    mbedtls_mpi_free( &P );
    mbedtls_mpi_free( &Q );
    mbedtls_mpi_free( &E );
    mbedtls_rsa_free( &ctx );
}

void test_pkcs1_rsaes_v15_decrypt_wrapper( void ** params )
{
    data_t  data10, data13;

    data10.x = (uint8_t *) params[10];
    data10.len = *( (uint32_t *) params[11] );

    data13.x = (uint8_t *) params[13];
    data13.len = *( (uint32_t *) params[14] );

    test_pkcs1_rsaes_v15_decrypt( *( (int *) params[0] ), *( (int *) params[1] ), (char *) params[2], *( (int *) params[3] ), (char *) params[4], *( (int *) params[5] ), (char *) params[6], *( (int *) params[7] ), (char *) params[8], *( (int *) params[9] ), &data10, (char *) params[12], &data13, *( (int *) params[15] ) );
}

void test_pkcs1_rsassa_v15_sign( int mod, int radix_P, char * input_P, int radix_Q,
                                 char * input_Q, int radix_N, char * input_N,
                                 int radix_E, char * input_E, int digest, int hash,
                                 data_t * message_str, data_t * rnd_buf,
                                 data_t * result_hex_str, int result )
{
    unsigned char hash_result[1000];
    unsigned char output[1000];
    mbedtls_rsa_context ctx;
    mbedtls_mpi N, P, Q, E;
    rnd_buf_info info;

    RSA_claim_bit_length(mod);

    info.buf = rnd_buf->x;
    info.length = rnd_buf->len;

    mbedtls_mpi_init( &N );
    mbedtls_mpi_init( &P );
    mbedtls_mpi_init( &Q );
    mbedtls_mpi_init( &E );
    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, hash );

    memset( hash_result, 0x00, 1000 );
    memset( output, 0x00, 1000 );

    TEST_ASSERT( mbedtls_mpi_read_string( &P, radix_P, input_P ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &Q, radix_Q, input_Q ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );

    TEST_ASSERT( mbedtls_rsa_import( &ctx, &N, &P, &Q, NULL, &E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_get_len( &ctx ) == (size_t) ( ( mod + 7 ) / 8 ) );
    TEST_ASSERT( mbedtls_rsa_complete( &ctx ) == 0 );
    TEST_ASSERT( mbedtls_rsa_check_privkey( &ctx ) == 0 );


    if( mbedtls_md_info_from_type((mbedtls_md_type_t) digest ) != NULL )
        TEST_ASSERT( mbedtls_md( mbedtls_md_info_from_type( (mbedtls_md_type_t)digest ), message_str->x, message_str->len, hash_result ) == 0 );

    TEST_ASSERT( mbedtls_rsa_pkcs1_sign( &ctx, &rnd_buffer_rand, &info, MBEDTLS_RSA_PRIVATE, (mbedtls_md_type_t)digest, 0, hash_result, output ) == result );
    if( result == 0 )
    {

        TEST_ASSERT( hexcmp( output, result_hex_str->x, ctx.len, result_hex_str->len ) == 0 );
    }

exit:
    mbedtls_mpi_free( &N );
    mbedtls_mpi_free( &P );
    mbedtls_mpi_free( &Q );
    mbedtls_mpi_free( &E );
    mbedtls_rsa_free( &ctx );
}

void test_pkcs1_rsassa_v15_sign_wrapper( void ** params )
{
    data_t  data11, data13, data15;

    data11.x = (uint8_t *) params[11];
    data11.len = *( (uint32_t *) params[12] );

    data13.x = (uint8_t *) params[13];
    data13.len = *( (uint32_t *) params[14] );

    data15.x = (uint8_t *) params[15];
    data15.len = *( (uint32_t *) params[16] );

    test_pkcs1_rsassa_v15_sign( *( (int *) params[0] ), *( (int *) params[1] ), (char *) params[2], *( (int *) params[3] ), (char *) params[4], *( (int *) params[5] ), (char *) params[6], *( (int *) params[7] ), (char *) params[8], *( (int *) params[9] ), *( (int *) params[10] ), &data11, &data13, &data15, *( (int *) params[17] ) );
}

void test_pkcs1_rsassa_v15_verify( int mod, int radix_N, char * input_N,
                                   int radix_E, char * input_E, int digest,
                                   int hash, data_t * message_str, char * salt,
                                   data_t * result_str, int result )
{
    unsigned char hash_result[1000];
    mbedtls_rsa_context ctx;
    mbedtls_mpi N, E;
    ((void) salt);

    RSA_claim_bit_length(mod);

    mbedtls_mpi_init( &N );
    mbedtls_mpi_init( &E );
    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, hash );
    memset( hash_result, 0x00, 1000 );

    TEST_ASSERT( mbedtls_mpi_read_string( &N, radix_N, input_N ) == 0 );
    TEST_ASSERT( mbedtls_mpi_read_string( &E, radix_E, input_E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_import( &ctx, &N, NULL, NULL, NULL, &E ) == 0 );
    TEST_ASSERT( mbedtls_rsa_get_len( &ctx ) == (size_t) ( ( mod + 7 ) / 8 ) );
    TEST_ASSERT( mbedtls_rsa_check_pubkey( &ctx ) == 0 );


    if( mbedtls_md_info_from_type( (mbedtls_md_type_t)digest ) != NULL )
        TEST_ASSERT( mbedtls_md( mbedtls_md_info_from_type( (mbedtls_md_type_t)digest ), message_str->x, message_str->len, hash_result ) == 0 );

    TEST_ASSERT( mbedtls_rsa_pkcs1_verify( &ctx, NULL, NULL, MBEDTLS_RSA_PUBLIC, (mbedtls_md_type_t)digest, 0, hash_result, result_str->x ) == result );

exit:
    mbedtls_mpi_free( &N );
    mbedtls_mpi_free( &E );
    mbedtls_rsa_free( &ctx );
}

void test_pkcs1_rsassa_v15_verify_wrapper( void ** params )
{
    data_t  data7, data10;

    data7.x = (uint8_t *) params[7];
    data7.len = *( (uint32_t *) params[8] );

    data10.x = (uint8_t *) params[10];
    data10.len = *( (uint32_t *) params[11] );

    test_pkcs1_rsassa_v15_verify( *( (int *) params[0] ), *( (int *) params[1] ), (char *) params[2], *( (int *) params[3] ), (char *) params[4], *( (int *) params[5] ), *( (int *) params[6] ), &data7, (char *) params[9], &data10, *( (int *) params[12] ) );
}
#endif /* MBEDTLS_SHA1_C */
#endif /* MBEDTLS_RSA_C */
#endif /* MBEDTLS_PKCS1_V15 */


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

#if defined(MBEDTLS_PKCS1_V15) && defined(MBEDTLS_RSA_C) && defined(MBEDTLS_SHA1_C)

    case 0:
    {
        *out_value = MBEDTLS_MD_SHA1;
    }
    break;
    case 1:
    {
        *out_value = MBEDTLS_ERR_RSA_BAD_INPUT_DATA;
    }
    break;
    case 2:
    {
        *out_value = MBEDTLS_ERR_RSA_INVALID_PADDING;
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

#if defined(MBEDTLS_PKCS1_V15) && defined(MBEDTLS_RSA_C) && defined(MBEDTLS_SHA1_C)

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

#if defined(MBEDTLS_PKCS1_V15) && defined(MBEDTLS_RSA_C) && defined(MBEDTLS_SHA1_C)
    test_pkcs1_rsaes_v15_encrypt_wrapper,
#else
    NULL,
#endif
    /* Function Id: 1 */

#if defined(MBEDTLS_PKCS1_V15) && defined(MBEDTLS_RSA_C) && defined(MBEDTLS_SHA1_C)
    test_pkcs1_rsaes_v15_decrypt_wrapper,
#else
    NULL,
#endif
    /* Function Id: 2 */

#if defined(MBEDTLS_PKCS1_V15) && defined(MBEDTLS_RSA_C) && defined(MBEDTLS_SHA1_C)
    test_pkcs1_rsassa_v15_sign_wrapper,
#else
    NULL,
#endif
    /* Function Id: 3 */

#if defined(MBEDTLS_PKCS1_V15) && defined(MBEDTLS_RSA_C) && defined(MBEDTLS_SHA1_C)
    test_pkcs1_rsassa_v15_verify_wrapper,
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
            if ( get_expression ( exp_id, (int32_t *) int_params_store ) == 0 )
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

        }
    }

    printf("\n----------------------------------------------------------------------------\n\n");
    if( total_errors == 0 )
        printf("PASSED" );
    else
        printf("FAILED" );

    printf(" (%d / %d tests (%d skipped))\n",
           total_tests - total_errors, total_tests, total_skipped );

    fflush(stdout);

    while (1);
}


