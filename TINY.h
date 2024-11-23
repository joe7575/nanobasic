#define IXX_BOOL unsigned char
#define IXX_U8  unsigned char
#define IXX_U16 unsigned short
#define IXX_U32 unsigned int

IXX_U8 *TINY_Compiler(char *filename, IXX_U16 *pu16_len, IXX_U8 *pu8_num_vars);
void *TINY_Create(IXX_U8 u8_num_vars);
void TINY_SetVar(void *pv_vm, IXX_U8 u8_var, IXX_U32 u32_val);
IXX_U16 TINY_Run(void *pv_vm, IXX_U8* pu8_programm, IXX_U16 u16_len);
void TINY_Destroy(void * pv_vm);
void TINY_Hex2Bin(const char* pc8_in, IXX_U16 u16_len, IXX_U8* pu8_out);
void TINY_DumpCode(IXX_U8 *code, IXX_U16 size);
void TINY_OutputSymbolTable(void);
