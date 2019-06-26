/* Dummy header for u-boot */
extern int printf(const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));
extern int (getchar)(void);
extern int (putchar)(int /*c*/);
extern int sprintf(char * __restrict /*s*/, const char * __restrict /*format*/, ...) __attribute__((__nonnull__(1,2)));
