#ifndef KIM_H
#define KIM_H

// write number of runes from a kim stream int a utf8 stream
void utf8_to_kim(char **utf, char **kim);

// write number of runes from a kim stream int a utf8 stream
void kim_to_utf8(char **kim, char **utf, int runes);

// Return the number of bytes a given utf-8 rune will have
int utf8_bytes(char c);

// Return the number of runes in a utf8 string
int utf8_count(char *utf8);

#ifdef KIM_IMPLEMENTATION

#define KIM_CONT 0x80
#define KIM_DATA 0x7f
#define CONTINUE(CHAR) (CHAR>>7)

int decode_utf8(char **s);
void encode_utf8(char **s, int code);
void encode_kim(char **s, int code);
int decode_kim(char **s);

int utf8_bytes(char c)
{
  int bytes = __builtin_clz(~(c));
  if (!bytes) return 1;
  return bytes-24;
}

int utf8_count(char *utf8)
{
  int count = 0;
  char *p = utf8;

  while(*s) {
    count++;
    utf8 += utf8_bytes(*utf8);
  }
  
  return count;
}

// decode and advance s, returning the character rune
int decode_utf8(char **s) {
  int k = **s ? __builtin_clz(~(**s << 24)) : 0;  // Count # of leading 1 bits.
  int mask = (1 << (8 - k)) - 1;                  // All 1's with k leading 0's.
  int value = **s & mask;
  for (++(*s), --k; k > 0 && **s; --k, ++(*s)) {  // Note that k = #total bytes, or 0.
    value <<= 6;
    value += (**s & 0x3F);
  }
  return value;
}

// Write and advance s with rune in utf-8
void encode_utf8(char **s, int rune) {
  char val[4];
  int lead_byte_max = 0x7F;
  int val_index = 0;
  while (rune > lead_byte_max) {
    val[val_index++] = (rune & 0x3F) | 0x80;
    rune >>= 6;
    lead_byte_max >>= (val_index == 1 ? 2 : 1);
  }
  val[val_index++] = (rune & lead_byte_max) | (~lead_byte_max << 1);
  while (val_index--) {
    **s = val[val_index];
    (*s)++;
  }
}

// write and advance s with rune in kim
static inline void encode_kim(char **s, int rune)
{
  if (rune < KIM_CONT) {
    **s = 0 | (KIM_DATA & rune);
    (*s)++;
    return;
  }
  
  int bits = ((32 - __builtin_clz(rune) + 6) / 7) * 7;

  while (bits > 7) {
    bits -= 7;
    **s = KIM_CONT | KIM_DATA & (rune >> bits);
    (*s)++;
  }
  **s = KIM_DATA & rune;
  (*s)++;
}

// decode and advance s, returning the character rune
int decode_kim(char **s)
{
  int rune = **s & KIM_DATA;
  while (CONTINUE(**s)) {
    rune <<= 7;
    (*s)++;
    rune |= **s & KIM_DATA;
  }
  (*s)++;
  return rune;
}

void utf8_to_kim(char **utf, char **kim)
{
  while (**utf)
    encode_kim(kim, decode_utf8(utf));
}

void kim_to_utf8(char **kim, char **utf, int runes)
{
  for (int i = 0; i < runes; i++)
    encode_utf8(utf, decode_kim(kim));
}

#endif
#endif
