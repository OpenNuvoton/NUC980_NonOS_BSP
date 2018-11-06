/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * $Date: 15/05/06 9:54a $
 * @brief    mbedtls SHA test suit
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include "nuc980.h"
#include "sys.h"
#include "crypto.h"

#include "mbedtls/config.h"
#include "mbedtls/platform.h"

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
#endif

#include <stdint.h>
#include <string.h>

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
    do                                              \
    {                                               \
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
    while (1);                                      \
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


int        pass_cnt;
uint8_t    *file_base_ptr;
uint32_t   g_file_idx, g_file_size;

static char  g_line_buff[4096];

static int test_errors = 0;

extern uint32_t  VectorBase_SHA, VectorLimit_SHA;

int  open_test_vector(int vector_no)
{
    g_file_idx = 0;

    if (vector_no == 1)
    {
        printf("\n\nOpen test vector test_suite_shax.data.\n");
        file_base_ptr = (uint8_t *)&VectorBase_SHA;
        g_file_size = (uint32_t)&VectorLimit_SHA - (uint32_t)&VectorBase_SHA;
        printf("File size: %d\n", g_file_size);
        return 0;
    }
    return -1;
}

static int  read_file(uint8_t *pu8Buff, int i32Len)
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
    uint8_t     ch[2];

    if (g_file_idx+1 >= g_file_size)
    {
        //printf("EOF.\n");
        return -1;
    }

    memset(g_line_buff, 0, sizeof(g_line_buff));

    for (i = 0; i < sizeof(g_line_buff);)
    {
        if (read_file(ch, 1) < 0)
            return 0;

        if ((ch[0] == 0x0D) || (ch[0] == 0x0A))
            break;

        g_line_buff[i++] = ch[0];
    }

    while (1)
    {
        if (read_file(ch, 1) < 0)
            return 0;

        if ((ch[0] != 0x0D) && (ch[0] != 0x0A))
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


#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"


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
    printf("Expected integer for parameter and got: %s\n", str );
    return( KEY_VALUE_MAPPING_NOT_FOUND );
}


void test_suite_mbedtls_sha1( char *hex_src_string, char *hex_hash_string )
{
    unsigned char src_str[10000];
    unsigned char hash_str[10000];
    unsigned char output[41];
    int src_len;

    memset(src_str, 0x00, 10000);
    memset(hash_str, 0x00, 10000);
    memset(output, 0x00, 41);

    src_len = unhexify( src_str, hex_src_string );

    mbedtls_sha1( src_str, src_len, output );
    hexify( hash_str, output, 20 );

    //printf("hash_str:        %s\n", hash_str);
    //printf("hex_hash_string: %s\n", hex_hash_string);

    TEST_ASSERT( strcmp( (char *) hash_str, hex_hash_string ) == 0 );

exit:
    return;
}

void test_suite_sha224(char *hex_src_string, char *hex_hash_string )
{
    unsigned char src_str[10000];
    unsigned char hash_str[10000];
    unsigned char output[57];
    int src_len;

    memset(src_str, 0x00, 10000);
    memset(hash_str, 0x00, 10000);
    memset(output, 0x00, 57);

    src_len = unhexify( src_str, hex_src_string );

    mbedtls_sha256( src_str, src_len, output, 1 );
    hexify( hash_str, output, 28 );

    TEST_ASSERT( strcmp( (char *) hash_str, hex_hash_string ) == 0 );

exit:
    return;
}

void test_suite_mbedtls_sha256(char *hex_src_string, char *hex_hash_string )
{
    unsigned char src_str[10000];
    unsigned char hash_str[10000];
    unsigned char output[65];
    int src_len;

    memset(src_str, 0x00, 10000);
    memset(hash_str, 0x00, 10000);
    memset(output, 0x00, 65);

    src_len = unhexify( src_str, hex_src_string );

    mbedtls_sha256( src_str, src_len, output, 0 );
    hexify( hash_str, output, 32 );

    TEST_ASSERT( strcmp( (char *) hash_str, hex_hash_string ) == 0 );

exit:
    return;
}

void test_suite_sha384(char *hex_src_string, char *hex_hash_string )
{
    unsigned char src_str[10000];
    unsigned char hash_str[10000];
    unsigned char output[97];
    int src_len;

    memset(src_str, 0x00, 10000);
    memset(hash_str, 0x00, 10000);
    memset(output, 0x00, 97);

    src_len = unhexify( src_str, hex_src_string );

    mbedtls_sha512( src_str, src_len, output, 1 );
    hexify( hash_str, output, 48 );

    TEST_ASSERT( strcmp( (char *) hash_str, hex_hash_string ) == 0 );

exit:
    return;
}

void test_suite_mbedtls_sha512(char *hex_src_string, char *hex_hash_string )
{
    unsigned char src_str[10000];
    unsigned char hash_str[10000];
    unsigned char output[129];
    int src_len;

    memset(src_str, 0x00, 10000);
    memset(hash_str, 0x00, 10000);
    memset(output, 0x00, 129);

    src_len = unhexify( src_str, hex_src_string );

    mbedtls_sha512( src_str, src_len, output, 0);
    hexify( hash_str, output, 64 );

    TEST_ASSERT( strcmp( (char *) hash_str, hex_hash_string ) == 0 );

exit:
    return;
}

void test_suite_sha1_selftest()
{
    TEST_ASSERT( mbedtls_sha1_self_test( 1 ) == 0 );

exit:
    return;
}

void test_suite_sha256_selftest()
{
    TEST_ASSERT( mbedtls_sha256_self_test( 1 ) == 0 );

exit:
    return;
}

void test_suite_sha512_selftest()
{
    TEST_ASSERT( mbedtls_sha512_self_test( 1 ) == 0 );

exit:
    return;
}


/*----------------------------------------------------------------------------*/
/* Test dispatch code */

int dep_check( char *str )
{
    if( str == NULL )
        return( 1 );

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

    if( strcmp( params[0], "mbedtls_sha1" ) == 0 )
    {
        char *param1 = params[1];
        char *param2 = params[2];

        if( cnt != 3 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 3 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_string( &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_mbedtls_sha1( param1, param2 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "sha224" ) == 0 )
    {
        char *param1 = params[1];
        char *param2 = params[2];

        if( cnt != 3 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 3 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_string( &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_sha224( param1, param2 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "mbedtls_sha256" ) == 0 )
    {
        char *param1 = params[1];
        char *param2 = params[2];

        if( cnt != 3 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 3 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_string( &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_mbedtls_sha256( param1, param2 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "sha384" ) == 0 )
    {
        char *param1 = params[1];
        char *param2 = params[2];

        if( cnt != 3 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 3 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_string( &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_sha384( param1, param2 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "mbedtls_sha512" ) == 0 )
    {
        char *param1 = params[1];
        char *param2 = params[2];

        if( cnt != 3 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 3 );
            return( DISPATCH_INVALID_TEST_DATA );
        }

        if( verify_string( &param1 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );
        if( verify_string( &param2 ) != 0 ) return( DISPATCH_INVALID_TEST_DATA );

        test_suite_mbedtls_sha512( param1, param2 );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "sha1_selftest" ) == 0 )
    {
        if( cnt != 1 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 1 );
            return( DISPATCH_INVALID_TEST_DATA );
        }


        test_suite_sha1_selftest(  );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "sha256_selftest" ) == 0 )
    {
        if( cnt != 1 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 1 );
            return( DISPATCH_INVALID_TEST_DATA );
        }


        test_suite_sha256_selftest(  );
        return ( DISPATCH_TEST_SUCCESS );
    }
    else if( strcmp( params[0], "sha512_selftest" ) == 0 )
    {
        if( cnt != 1 )
        {
            printf("\nIncorrect argument count (%d != %d)\n", cnt, 1 );
            return( DISPATCH_INVALID_TEST_DATA );
        }


        test_suite_sha512_selftest(  );
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
}

/*-----------------------------------------------------------------------------*/
int main(void)
{
    char  *params[50];
    int   cnt, vector_no;
    int   is_eof;

    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    UART_Init();

    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | (1<<23));   /* Enable Crypto clock */

    printf("+---------------------------------------+\n");
    printf("|     Crypto mbedtls SHAx test suit      |\n");
    printf("+---------------------------------------+\n");

    sysInstallISR(IRQ_LEVEL_1, IRQ_CRYPTO, (PVOID)CRYPTO_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(IRQ_CRYPTO);

    SHA_ENABLE_INT(CRPT);

    for (vector_no = 1; ; vector_no++)
    {
        if (open_test_vector(vector_no) != 0)
            break;

        pass_cnt = 0;

        while (1)
        {
            while (1)
            {
                is_eof = get_line();
                if (is_eof)
                    break;

                printf("LINE: %s\n", g_line_buff);
                cnt = parse_arguments(g_line_buff, sizeof(g_line_buff), params);

                if (strcmp( params[0], "depends_on" ) == 0)   /* ignore */
                    continue;

                //printf("[%d] LINE: %s\n", cnt, g_line_buff);

                if (cnt > 2)
                    break;
            }
            if (is_eof)
                break;

            dispatch_test(cnt, params);
            pass_cnt++;
        }
        printf("PASS count: %d\n", pass_cnt);
    }
    printf("All test file done.\n");
    while (1);
}


