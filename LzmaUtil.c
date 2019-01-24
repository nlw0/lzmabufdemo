#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lzma/LzmaDec.h"
#include "lzma/Alloc.h"
#include "lzma/7zFile.h"

#define IN_BUF_SIZE (1 << 10)
#define OUT_BUF_SIZE (1 << 10)

static int Decode2(CLzmaDec *state, FILE * file)
{
  Byte inBuf[IN_BUF_SIZE];
  Byte outBuf[OUT_BUF_SIZE];
  size_t inPos = 0, inSize = 0;
  LzmaDec_Init(state);
  for (;;)
  {
    if (inPos == inSize)
    {
      printf("\n[IN] --- read new input\n", inSize);      
      inSize = IN_BUF_SIZE;
      fread(inBuf, 1, inSize, file);
      inPos = 0;
    }
    {
      SizeT inProcessed = inSize - inPos;
      SizeT outProcessed = OUT_BUF_SIZE;
      ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
      ELzmaStatus status;
      LzmaDec_DecodeToBuf(state, outBuf, &outProcessed, inBuf + inPos, &inProcessed, finishMode, &status);

      printf("\n[OUT] **** inPos %d outProcessed %d\n", inPos, outProcessed);
      fwrite(outBuf, 1, outProcessed, stdout);

      inPos += inProcessed;
      /* outPos += outProcessed; */
    }
  }
}

static SRes Decode(FILE * file)
{
  int i;
  SRes res = 0;

  CLzmaDec state;

  /* header: 5 bytes of LZMA properties and 8 bytes of uncompressed size */
  unsigned char header[LZMA_PROPS_SIZE + 8];

  /* Read and parse header */
  fread(header, sizeof(char), sizeof(header), file);

  UInt64 unpackSize;
  unpackSize = 0;
  for (i = 0; i < 8; i++)
    unpackSize += (UInt64)header[LZMA_PROPS_SIZE + i] << (i * 8);
  printf("size %d", unpackSize);
  
  LzmaDec_Construct(&state);
  RINOK(LzmaDec_Allocate(&state, header, LZMA_PROPS_SIZE, &g_Alloc));
  Decode2(&state, file);
  LzmaDec_Free(&state, &g_Alloc);
}

int main(int argc, const char *argv[])
{
  FILE * file;
  file = fopen("raven.txt.lzma", "r");  
  Decode(file);
  fclose(file);
}
