#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lzma/LzmaDec.h"
#include "lzma/Alloc.h"
#include "lzma/7zFile.h"

#define IN_BUF_SIZE (1 << 13)
#define OUT_BUF_SIZE (1 << 15)

static int Decode2(CLzmaDec* state, size_t output_size, FILE* file)
{
    Byte inBuf[IN_BUF_SIZE];
    Byte outBuf[OUT_BUF_SIZE];
    size_t inPos = 0, inSize = 0, outPos = 0;
    LzmaDec_Init(state);
    char ring[20];
    while (outPos < output_size)
    {
        if (inPos == inSize)
        {
//            printf("\n[IN] --- read new input\n", inSize);
            inSize = IN_BUF_SIZE;
            fread(inBuf, 1, inSize, file);
            inPos = 0;
        }
        {
            SizeT inProcessed = inSize - inPos;
            SizeT outProcessed = (OUT_BUF_SIZE > (output_size - outPos)) ? OUT_BUF_SIZE : output_size - outPos;
            ELzmaFinishMode finishMode = (OUT_BUF_SIZE > (output_size - outPos)) ? LZMA_FINISH_ANY : LZMA_FINISH_END;

            ELzmaStatus status;
            LzmaDec_DecodeToBuf(state, outBuf, &outProcessed, inBuf + inPos, &inProcessed, finishMode, &status);

//            fwrite(outBuf, 1, outProcessed, stdout);

            for (int i = 0; i < outProcessed; i++)
                ring[(i+ outPos)%20] = outBuf[i];

            inPos += inProcessed;
            outPos += outProcessed;
//            printf("\n[OUT] **** inPos %d outProcessed %d outPos %d\n", inPos, outProcessed, outPos);
        }
    }
    for (int i = 1; i < 21; i++)
        printf("%02x ", (255 & ring[(outPos + i)%20]));
    printf("\n");
    return 0;
}

static SRes Decode(FILE* file)
{
    int i;

    CLzmaDec state;

    /* header: 5 bytes of LZMA properties and 8 bytes of uncompressed size */
    unsigned char header[LZMA_PROPS_SIZE + 8];

    /* Read and parse header */
    fread(header, sizeof(char), sizeof(header), file);

    size_t unpackSize;
    unpackSize = 0;
    for (i = 0; i < 8; i++)
        unpackSize += header[LZMA_PROPS_SIZE + i] << (i * 8);

    LzmaDec_Construct(&state);
    RINOK(LzmaDec_Allocate(&state, header, LZMA_PROPS_SIZE, &g_Alloc));
    Decode2(&state, unpackSize, file);
    LzmaDec_Free(&state, &g_Alloc);
}

int main(int argc, const char* argv[])
{
    FILE* file;
    file = fopen("raven.txt.lzma", "r");
    Decode(file);
    printf("\n");
    fclose(file);
}
