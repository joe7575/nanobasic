#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "TINY.h"

#define is_alpha(x)   (Ascii[x & 0x7F] & 0x01)
#define is_digit(x)   (Ascii[x & 0x7F] & 0x02)
#define is_wspace(x)  (Ascii[x & 0x7F] & 0x04)
#define is_alnum(x)   (Ascii[x & 0x7F] & 0x03)
#define is_comp(x)    (Ascii[x & 0x7F] & 0x08)
#define is_arith(x)   (Ascii[x & 0x7F] & 0x10)

static char Ascii[] = {
//  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F  
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, // 0x00 - 0x0F
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00, // 0x10 - 0x1F
  0x04, 0x10, 0x00, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x10, 0x10, 0x00, 0x10, 0x00, 0x10, // 0x20 - 0x2F
  0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x08, 0x08, 0x08, 0x00, // 0x30 - 0x3F
  0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, // 0x40 - 0x4F
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, // 0x50 - 0x5F
  0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, // 0x60 - 0x6F
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x70 - 0x7F
};

char *TINY_Scanner(char *pc8_in, char *pc8_out)
{
  char c8;

  while(is_wspace(*pc8_in))
  {
    pc8_in++;
  }

  while((c8 = *pc8_in) != 0)
  {
    if(is_alpha(c8))
    {
      *pc8_out++ = c8;
      pc8_in++;
      while(is_alnum(*pc8_in))
      {
        *pc8_out++ = *pc8_in++;
      }
      *pc8_out++ = '\0';
      return pc8_in;
    }

    if(is_digit(c8))
    {
      *pc8_out++ = c8;
      pc8_in++;
      while(is_digit(*pc8_in))
      {
        *pc8_out++ = *pc8_in++;
      }
      *pc8_out++ = '\0';
      return pc8_in;
    }

    if(is_comp(c8))
    {
      *pc8_out++ = c8;
      pc8_in++;
      while(is_comp(*pc8_in))
      {
        *pc8_out++ = *pc8_in++;
      }
      *pc8_out++ = '\0';
      return pc8_in;
    }

    if(is_arith(c8))
    {
      *pc8_out++ = c8;
      pc8_in++;
      while(is_arith(*pc8_in))
      {
        *pc8_out++ = *pc8_in++;
      }
      *pc8_out++ = '\0';
      return pc8_in;
    }

    if(c8 == '\"')
    {
      *pc8_out++ = c8;
      pc8_in++;
      while((c8 = *pc8_in) != '\"')
      {
        *pc8_out++ = c8;
        pc8_in++;
      }
      *pc8_out++ = c8;
      pc8_in++;
      *pc8_out++ = '\0';
      return pc8_in;
    }
    {
      pc8_in++;
    }
  }

  // End of string
  if((c8 == '\n') || (c8 == '\r') || (c8 == '\0'))
  {
    *pc8_out = '\0';
    return NULL;
  }

  // Single character
  *pc8_out++ = c8;
  pc8_in++;
  *pc8_out++ = '\0';
  return pc8_in;
}

#ifdef TEST
int main(void)
{
  char s[] = "LET A = 1234 * 2 - 1";
  char t[80];
  char *p = s;

  while(*p != 0)
  {
    p = TINY_Scanner(p,t);
    printf("%s\n",t);
  }

  strcpy(s,"\"Hello World\"");
  p = s;
  while(*p != 0)
  {
    p = TINY_Scanner(p,t);
    printf("%s\n",t);
  }
  
  strcpy(s,"A=1234*2-1");
  p = s;
  while(*p != 0)
  {
    p = TINY_Scanner(p,t);
    printf("%s\n",t);
  }

  return 0;
}
#endif