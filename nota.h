#ifndef NOTA_H
#define NOTA_H

#define NOTA_BLOB 0x00 // C 0 0 0
#define NOTA_TEXT 0x10 // 
#define NOTA_ARR 0x20 // 0 1 0
#define NOTA_REC 0x30 // C 0 1 1
#define NOTA_FLOAT 0x40 // C 1 0
#define NOTA_INT 0x60 // C 1 1 0
#define NOTA_SYM 0x70 // C 1 1 1

#define NOTA_FALSE 0x00
#define NOTA_TRUE 0x01
#define NOTA_NULL 0x02
#define NOTA_INF 0x03
#define NOTA_PRIVATE 0x08
#define NOTA_SYSTEM 0x09

// Returns the type NOTA_ of the byte at *nota
int nota_type(char *nota);

// Functions take a pointer to a buffer *nota, read or write the value, and then return a pointer to the next byte of the stream

// Pass NULL into the read in variable to skip over it

char *nota_read_blob(long long *len, char *nota);
// ALLOCATES! Uses strdup to return it via the text pointer
char *nota_read_text(char **text, char *nota);
char *nota_read_array(long long *len, char *nota);
char *nota_read_record(long long *len, char *nota);
char *nota_read_float(double *d, char *nota);
char *nota_read_int(long long *l, char *nota);
char *nota_read_sym(int *sym, char *nota);

char *nota_write_blob(unsigned long long n, char *nota);
char *nota_write_text(const char *s, char *nota);
char *nota_write_array(unsigned long long n, char *nota);
char *nota_write_record(unsigned long long n, char *nota);
char *nota_write_number(double n, char *nota);
char *nota_write_sym(int sym, char *nota);

void print_nota_hex(char *nota);

#ifdef NOTA_IMPLEMENTATION

#include "stdio.h"
#include "math.h"
#include "string.h"
#include "stdlib.h"
#include "limits.h"
#include "kim.h"

#define NOTA_CONT 0x80
#define NOTA_DATA 0x7f
#define NOTA_INT_DATA 0x07
#define NOTA_INT_SIGN(CHAR) (CHAR & (1<<3))
#define NOTA_SIG_SIGN(CHAR) (CHAR & (1<<3))
#define NOTA_EXP_SIGN(CHAR) (CHAR & (1<<4))
#define NOTA_TYPE 0x70
#define NOTA_HEAD_DATA 0x0f
#define CONTINUE(CHAR) (CHAR>>7)

#define UTF8_DATA 0x3f

/* define this to use native string instead of kim. Bytes are encoded instead of runes */
#define NOTA_UTF8

int nota_type(char *nota) { return *nota & NOTA_TYPE; }

char *nota_skip(char *nota)
{
  while (CONTINUE(*nota))
    nota++;

  return nota+1;
}

char *nota_read_num(long long *n, char *nota)
{
  if (!n)
    return nota_skip(nota);
    
  *n = 0;
  *n |= (*nota) & NOTA_HEAD_DATA;
  
  while (CONTINUE(*(nota++)))
    *n = (*n<<7) | (*nota) & NOTA_DATA;

  return nota;
}

// Given a number n, and bits used in the first char sb, how many bits are needed
int nota_bits(long long n, int sb)
{
  if (n == 0) return sb;
  int bits = sizeof(n)*CHAR_BIT - __builtin_clzll(n);
  bits-=sb;
  int needed = ((bits + 6) / 7)*7 + sb;
  return needed;
}

// write a number from n into *nota, with sb bits in the first char
char *nota_continue_num(long long n, char *nota, int sb)
{
  int bits = nota_bits(n, sb);
  bits -= sb;
  if (bits > 0)
    nota[0] |= NOTA_CONT;
  else
    nota[0] &= ~NOTA_CONT;

  int shex = (~0) << sb;
  nota[0] &= shex; /* clear shex bits */
  nota[0] |= (~shex) & (n>>bits);

  int i = 1;
  while (bits > 0) {
    bits -= 7;
    int head = bits == 0 ? 0 : NOTA_CONT;
    nota[i] = head | (NOTA_DATA & (n >> bits));
    i++;
  }

  return &nota[i];
}


void print_nota_hex(char *nota)
{
  do {
    printf("%02X ", (unsigned char)(*nota));
  } while(CONTINUE(*(nota++)));
  printf("\n");
  
  return;
  long long chars = 0;
  if (!((*nota>>4 & 0x07) ^ NOTA_TEXT>>4))
    nota_read_num(&chars, nota);

  if ((*nota>>5) == 2 || (*nota>>5) == 6)
    chars = 1;

  for (int i = 0; i < chars+1; i++) {
    do {
      printf("%02X ", (unsigned char)(*nota));
    } while(CONTINUE(*(nota++)));
  }
    
  printf("\n");
}

char *nota_write_int(long long n, char *nota)
{
  char sign = 0;
  
  if (n < 0) {
    sign = 0x08;
    n *= -1;
  }
  
  *nota = NOTA_INT | sign;  

  return nota_continue_num(n, nota, 3);
}

#define NOTA_DBL_PREC 6
#define xstr(s) str(s)
#define str(s) #s

#include <stdio.h>
#include <math.h>

void extract_mantissa_coefficient(double num, long *mantissa, long* coefficient) {
    char buf[64];
    char *p, *dec_point;
    int exp = 0, coeff = 0;

    // Convert double to string with maximum precision
    snprintf(buf, sizeof(buf), "%.17g", num);

    // Find if 'e' or 'E' is present (scientific notation)
    p = strchr(buf, 'e');
    if (!p) p = strchr(buf, 'E');
    if (p) {
        // There is an exponent part
        exp = atol(p + 1);
        *p = '\0'; // Remove exponent part from the string
    }

    // Find decimal point
    dec_point = strchr(buf, '.');
    if (dec_point) {
        // Count number of digits after decimal point
        int digits_after_point = strlen(dec_point + 1);
        coeff = digits_after_point;
        // Remove decimal point by shifting characters
        memmove(dec_point, dec_point + 1, strlen(dec_point));
    } else
        coeff = 0;

    // Adjust coefficient with exponent from scientific notation
    coeff -= exp;

    // Copy the mantissa
    *mantissa = atol(buf);

    // Set coefficient
    *coefficient = coeff;
}

char *nota_write_float(double n, char *nota)
{
  int neg = n < 0;
  long digits;
  long coef;
  extract_mantissa_coefficient(n, &digits, &coef);

  printf("Values of %g are %ld e %ld\n", n, digits, coef);
  if (coef == 0)
    return nota_write_int(coef * (neg ? -1 : 1), nota);

  int expsign = coef < 0 ? ~0 : 0;
  coef = llabs(coef);

  nota[0] = NOTA_FLOAT;
  nota[0] |= 0x10 & expsign;
  nota[0] |= 0x08 & neg;

  char *c = nota_continue_num(coef, nota, 3);

  return nota_continue_num(digits, c, 7);
}

char *nota_read_float(double *d, char *nota)
{
  long long sig = 0;
  long long e = 0;

  char *c = nota;
  e = (*c) & NOTA_INT_DATA; /* first three bits */

  while (CONTINUE(*c)) {
    e = (e<<7) | (*c) & NOTA_DATA;
    c++;
  }
  
  c++;

  do 
    sig = (sig<<7) | *c & NOTA_DATA;
  while (CONTINUE(*(c++)));

  if (NOTA_SIG_SIGN(*nota)) sig *= -1;
  if (NOTA_EXP_SIGN(*nota)) e *= -1;

  *d = (double)sig * pow(10.0, e);
  return c;
}

char *nota_write_number(double n, char *nota)
{
  if (n < (double)INT64_MIN || n > (double)INT64_MAX) return nota_write_float(n, nota);
  if (floor(n) == n) 
    return nota_write_int(n, nota);
  return nota_write_float(n, nota);
}

char *nota_read_int(long long *n, char *nota)
{
  if (!n)
    return nota_skip(nota);

  *n = 0;
  char *c = nota;
  *n |= (*c) & NOTA_INT_DATA; /* first three bits */
  while (CONTINUE(*(c++)))
    *n = (*n<<7) | (*c) & NOTA_DATA;

  if (NOTA_INT_SIGN(*nota)) *n *= -1;

  return c;
}

/* n is the number of bits */
char *nota_write_blob(unsigned long long n, char *nota)
{
  nota[0] = NOTA_BLOB;
  return nota_continue_num(n, nota, 4);
}

char *nota_write_array(unsigned long long n, char *nota)
{
  nota[0] = NOTA_ARR;
  return nota_continue_num(n, nota, 4);
}

char *nota_read_array(long long *len, char *nota)
{
  if (!len) return nota;
  return nota_read_num(len, nota);
}

char *nota_read_record(long long *len, char *nota)
{
  if (!len) return nota;
  return nota_read_num(len, nota);
}

char *nota_read_blob(long long *len, char *nota)
{
  if (!len) return nota;
  return nota_read_num(len, nota);
}

char *nota_write_record(unsigned long long n, char *nota)
{
  nota[0] = NOTA_REC;
  return nota_continue_num(n, nota, 4);
}

char *nota_write_sym(int sym, char *nota)
{
  *nota = NOTA_SYM | sym;
  return nota+1;
}

char *nota_read_sym(int *sym, char *nota)
{
  if (*sym) *sym = (*nota) & 0x0f;
  return nota+1;
}

char *nota_read_text(char **text, char *nota)
{
  long long chars;
  nota = nota_read_num(&chars, nota);

  char utf[chars*4]; // enough for the worst case scenario
  char *pp = utf;
  kim_to_utf8(&nota, &pp, chars);
  *pp = 0;
  *text = strdup(utf);  
  
  return nota;
}

char *nota_write_text(const char *s, char *nota)
{
  nota[0] = NOTA_TEXT;
  long long n = utf8_count(s);
  nota = nota_continue_num(n,nota,4);  
  utf8_to_kim(&s, &nota);
  return nota;
}

#endif
#endif
