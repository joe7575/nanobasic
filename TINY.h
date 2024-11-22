#define IXX_U8 unsigned char
#define IXX_U16 unsigned short
#define IXX_U32 unsigned int

IXX_U8 *TINY_Compiler(char *filename, IXX_U16 *pu16_len);
IXX_U16 TINY_Run(IXX_U8* pu8_programm);
void TINY_Hex2Bin(const char* pc8_in, IXX_U16 u16_len, IXX_U8* pu8_out);
