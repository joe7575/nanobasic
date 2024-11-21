#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "TINY.h"

#define MAX_STR_LEN 160

void compiler(char *filename)
{
  FILE *fp = fopen(filename, "r");
  if(fp == NULL)
  {
    printf("Error: could not open file %s\n", filename);
    return;
  }

  char line[MAX_STR_LEN];
  while(fgets(line, MAX_STR_LEN, fp) != NULL)
  {
    char *p = line;
    char token[MAX_STR_LEN];
    while(p != NULL)
    {
      p = TINY_Scanner(p, token);
      printf("%s\n", token);
    }
  }

  fclose(fp);
}

int main(void)
{
  compiler("../test.bas");
  return 0;
}
