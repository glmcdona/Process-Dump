/* This is a platform independent C89 x86 assembler(in development), disassembler(almost done), emulator(in development) and length disassembler library.

Features:
- Suppport for Intel and AT&T syntax.
- Support for x86(16/32/64).
- No dynamic memory allocation, global variables/state/context or runtime initialization.
- Thread-safe by design.
- Optimized for speed, size and low memory usage.
- The only dependencies are <stdbool.h>, <stdint.h> and <stddef.h>. Check out the 'NMD_ASSEMBLY_NO_INCLUDES' macro.

Setup:
Define the 'NMD_ASSEMBLY_IMPLEMENTATION' macro in one source file before the include statement to instantiate the implementation.
#define NMD_ASSEMBLY_IMPLEMENTATION
#include "nmd_assembly.h"

Using absolutely no dependencies(other headers...):
Define the 'NMD_ASSEMBLY_NO_INCLUDES' macro to tell the library not to include any headers. By doing so it will define the required types.
Be aware: This feature uses platform dependent macros.

Interfaces(a.k.a the functions you call from your application):
- The assembler is represented by the following function:
Assembles an instruction from a string. Returns the number of bytes written to the buffer on success, zero otherwise. Instructions can be separated using either the ';' or '\n' character.
Parameters:
- string         [in]         A pointer to a string that represents a instruction in assembly language.
- buffer         [out]        A pointer to a buffer that receives the encoded instructions.
- bufferSize     [in]         The size of the buffer in bytes.
- runtimeAddress [in]         The instruction's runtime address. You may use 'NMD_X86_INVALID_RUNTIME_ADDRESS'.
- mode           [in]         The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
- count          [in/out/opt] A pointer to a variable that on input is the maximum number of instructions that can be parsed(or zero for unlimited instructions), and on output is the number of instructions parsed. This parameter may be zero.
size_t nmd_x86_assemble(const char* string, void* buffer, size_t bufferSize, uint64_t runtimeAddress, NMD_X86_MODE mode, size_t* const count);

- The disassembler is composed of a decoder and a formatter represented by these two functions respectively:
- Decodes an instruction. Returns true if the instruction is valid, false otherwise.
Parameters:
- buffer      [in]  A pointer to a buffer containing a encoded instruction.
- bufferSize  [in]  The buffer's size in bytes.
- instruction [out] A pointer to a variable of type 'NMD_X86Instruction' that receives information about the instruction.
- mode        [in]  The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
- flags       [in]  A mask of 'NMD_X86_DECODER_FLAGS_XXX' that specifies which features the decoder is allowed to use. If uncertain, use 'NMD_X86_DECODER_FLAGS_MINIMAL'.
bool nmd_x86_decode_buffer(const void* buffer, size_t bufferSize, NMD_X86Instruction* instruction, NMD_X86_MODE mode, uint32_t flags);

- Formats an instruction. This function may cause a crash if you modify 'instruction' manually.
Parameters:
- instruction    [in]  A pointer to a variable of type 'NMD_X86Instruction' describing the instruction to be formatted.
- buffer         [out] A pointer to buffer that receives the string. The buffer's recommended size is 128 bytes.
- runtimeAddress [in]  The instruction's runtime address. You may use 'NMD_X86_INVALID_RUNTIME_ADDRESS'.
- formatFlags    [in]  A mask of 'NMD_X86_FORMAT_FLAGS_XXX' that specifies how the function should format the instruction. If uncertain, use 'NMD_X86_FORMAT_FLAGS_DEFAULT'.
void nmd_x86_format_instruction(const NMD_X86Instruction* instruction, char buffer[], uint64_t runtimeAddress, uint32_t formatFlags);

- The emulator is represented by the following function:
Emulates x86 code according to the cpu's state. You MUST initialize the following variables before calling this
function: 'cpu->mode', 'cpu->physicalMemory', 'cpu->physicalMemorySize', 'cpu->virtualAddress' and 'cpu->rip'.
You may optionally initialize 'cpu->rsp' if a stack is desirable. Below is a short description of each variable:
- 'cpu->mode': The emulator's operating architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
- 'cpu->physicalMemory': A pointer to a buffer used as the emulator's memory.
- 'cpu->physicalMemorySize': The size of the buffer pointer by 'physicalMemory' in bytes.
- 'cpu->virtualAddress': The starting address of the emulator's virtual address space.
- 'cpu->rip': The virtual address where emulation starts.
- 'cpu->rsp': The virtual address of the bottom of the stack.
Parameters:
- cpu      [in] A pointer to a variable of type 'NMD_X86Cpu' that holds the state of the cpu.
- maxCount [in] The maximum number of instructions that can be executed, or zero for unlimited instructions.
bool nmd_x86_emulate(NMD_X86Cpu* cpu, size_t maxCount);

- The length disassembler is represented by the following function:
Returns the instruction's length if it's valid, zero otherwise.
Parameters:
- buffer     [in] A pointer to a buffer containing a encoded instruction.
- bufferSize [in] The buffer's size in bytes.
- mode       [in] The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
size_t nmd_x86_ldisasm(const void* buffer, size_t bufferSize, NMD_X86_MODE mode);

Enabling and disabling features of the decoder:
To dynamically choose which features are used by the decoder, use the 'flags' parameter of nmd_x86_decode_buffer(). The less features specified in the mask, the
faster the decoder runs. By default all features are available, some can be completely disabled at compile time(thus reducing code size and increasing code speed) by defining
the following macros(in the same place the macro 'NMD_ASSEMBLY_IMPLEMENTATION' is defined):
- 'NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK': the decoder does not check if the instruction is invalid.
- 'NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID': the decoder does not fill the 'id' variable.
- 'NMD_ASSEMBLY_DISABLE_DECODER_CPU_FLAGS': the decoder does not fill the 'cpuFlags' variable.
- 'NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS': the decoder does not fill the 'numOperands' and 'operands' variable.
- 'NMD_ASSEMBLY_DISABLE_DECODER_GROUP': the decoder does not fill the 'group' variable.
- 'NMD_ASSEMBLY_DISABLE_DECODER_VEX': the decoder does not support VEX instructions.
- 'NMD_ASSEMBLY_DISABLE_DECODER_EVEX': the decoder does not support EVEX instructions.
- 'NMD_ASSEMBLY_DISABLE_DECODER_3DNOW': the decoder does not support 3DNow! instructions.

Enabling and disabling features of the formatter:
To dynamically choose which features are used by the formatter, use the 'formatFlags' parameter of nmd_x86_format_instruction(). The less features specified in the mask, the
faster the function runs. By default all features are available, some can be completely disabled at compile time(thus reducing code size and increasing code speed) by defining
the following macros(in the same place the macro 'NMD_ASSEMBLY_IMPLEMENTATION' is defined):
- 'NMD_ASSEMBLY_DISABLE_FORMATTER_POINTER_SIZE': the formatter does not support pointer size.
- 'NMD_ASSEMBLY_DISABLE_FORMATTER_BYTES: the formatter does not support instruction bytes. You may define the 'NMD_X86_FORMATTER_NUM_PADDING_BYTES' macro to be the number of bytes used as space padding.
- 'NMD_ASSEMBLY_DISABLE_FORMATTER_ATT_SYNTAX: the formatter does not support AT&T syntax.
- 'NMD_ASSEMBLY_DISABLE_FORMATTER_UPPERCASE: the formatter does not support uppercase.
- 'NMD_ASSEMBLY_DISABLE_FORMATTER_COMMA_SPACES: the formatter does not support comma spaces.
- 'NMD_ASSEMBLY_DISABLE_FORMATTER_OPERATOR_SPACES: the formatter does not support operator spaces.
- 'NMD_ASSEMBLY_DISABLE_FORMATTER_VEX': the formatter does not support VEX instructions.
- 'NMD_ASSEMBLY_DISABLE_FORMATTER_EVEX': the formatter does not support EVEX instructions.
- 'NMD_ASSEMBLY_DISABLE_FORMATTER_3DNOW': the formatter does not support 3DNow! instructions.

Enabling and disabling feature of the length disassembler:
Use the following macros to disable features at compile-time:
- 'NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK': the length disassembler does not check if the instruction is invalid.
- 'NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VEX': the length disassembler does not support VEX instructions.
- 'NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_3DNOW': the length disassembler does not support 3DNow! instructions.

TODO:
Short-Term
- implement instruction set extensions to the decoder : VEX, EVEX, MVEX, 3DNOW, XOP.
- Implement x86 assembler and emulator.
Long-Term
- Implement decompiler.
- Add support for other architectures(ARM, MIPS and PowerPC ?).

References:
- Intel 64 and IA-32 Architectures. Software Developer's Manual Volume 2 (2A, 2B, 2C & 2D): Instruction Set Reference, A-Z.
- Chapter 2 Instruction Format.
- Chapter 3-5 Instruction set reference.
- Appendix A Opcode Map.
- Appendix B.16 Instruction and Formats and Encoding.
- 3DNow! Technology Manual.
- AMD Extensions to the 3DNow! and MMX Instruction Sets Manual.
- Intel Architecture Instruction Set Extensions and Future Features Programming Reference.
- Capstone Engine.
- Zydis Disassembler.
- VIA PadLock Programming Guide.

Contributors(This may not be a complete list):
- Nomade: Founder and maintainer.
- Darkratos: Bug reporting and feature suggesting.
*/

#ifndef NMD_ASSEMBLY_H
#define NMD_ASSEMBLY_H

#ifdef NMD_ASSEMBLY_NO_INCLUDES

#ifndef __cplusplus

#define bool  _Bool
#define false 0
#define true  1

#endif /* __cplusplus */

typedef signed char        int8_t;
typedef unsigned char      uint8_t;

typedef signed short       int16_t;
typedef unsigned short     uint16_t;

typedef signed int         int32_t;
typedef unsigned int       uint32_t;

typedef signed long long   int64_t;
typedef unsigned long long uint64_t;

#if defined(_WIN64) && defined(_MSC_VER)
typedef unsigned __int64 size_t;
typedef __int64          ptrdiff_t;
#elif (defined(_WIN32) || defined(WIN32)) && defined(_MSC_VER)
typedef unsigned __int32 size_t
typedef __int32          ptrdiff_t;
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__x86_64__) || defined(__ppc64__)
typedef unsigned long size_t
typedef long          ptrdiff_t
#else
typedef unsigned int size_t
typedef int          ptrdiff_t
#endif
#else
typedef unsigned long size_t
typedef long          ptrdiff_t
#endif

#else

/* Dependencies */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#endif /* NMD_ASSEMBLY_NO_INCLUDES */

#ifndef NMD_X86_FORMATTER_NUM_PADDING_BYTES
#define NMD_X86_FORMATTER_NUM_PADDING_BYTES 10
#endif /* NMD_X86_FORMATTER_NUM_PADDING_BYTES */

#define NMD_X86_INVALID_RUNTIME_ADDRESS -1
#define NMD_X86_MAXIMUM_INSTRUCTION_LENGTH 15
#define NMD_X86_MAXIMUM_NUM_OPERANDS 4

/* These flags specify how the formatter should work. */
enum NMD_X86_FORMATTER_FLAGS
{
	NMD_X86_FORMAT_FLAGS_HEX = (1 << 0),  /* If set, numbers are displayed in hex base, otherwise they are displayed in decimal base. */
	NMD_X86_FORMAT_FLAGS_POINTER_SIZE = (1 << 1),  /* Pointer sizes(e.g. 'dword ptr', 'byte ptr') are displayed. */
	NMD_X86_FORMAT_FLAGS_ONLY_SEGMENT_OVERRIDE = (1 << 2),  /* If set, only segment overrides using prefixes(e.g. '2EH', '64H') are displayed, otherwise a segment is always present before a memory operand. */
	NMD_X86_FORMAT_FLAGS_COMMA_SPACES = (1 << 3),  /* A space is placed after a comma. */
	NMD_X86_FORMAT_FLAGS_OPERATOR_SPACES = (1 << 4),  /* A space is placed before and after the '+' and '-' characters. */
	NMD_X86_FORMAT_FLAGS_UPPERCASE = (1 << 5),  /* The string is uppercase. */
	NMD_X86_FORMAT_FLAGS_0X_PREFIX = (1 << 6),  /* Hexadecimal numbers have the '0x'('0X' if uppercase) prefix. */
	NMD_X86_FORMAT_FLAGS_H_SUFFIX = (1 << 7),  /* Hexadecimal numbers have the 'h'('H' if uppercase') suffix. */
	NMD_X86_FORMAT_FLAGS_ENFORCE_HEX_ID = (1 << 8),  /* If the HEX flag is set and either the prefix or suffix flag is also set, numbers less than 10 are displayed with preffix or suffix. */
	NMD_X86_FORMAT_FLAGS_HEX_LOWERCASE = (1 << 9),  /* If the HEX flag is set and the UPPERCASE flag is not set, hexadecimal numbers are displayed in lowercase. */
	NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_MEMORY_VIEW = (1 << 10), /* If set, signed numbers are displayed as they are represented in memory(e.g. -1 = 0xFFFFFFFF). */
	NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_HEX = (1 << 11), /* If set and NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_MEMORY_VIEW is also set, the number's hexadecimal representation is displayed in parenthesis. */
	NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_DEC = (1 << 12), /* Same as NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_HEX, but the number is displayed in decimal base. */
	NMD_X86_FORMAT_FLAGS_SCALE_ONE = (1 << 13), /* If set, scale one is displayed. E.g. add byte ptr [eax+eax*1], al. */
	NMD_X86_FORMAT_FLAGS_BYTES = (1 << 14), /* The instruction's bytes are displayed before the instructions. */
	NMD_X86_FORMAT_FLAGS_ATT_SYNTAX = (1 << 15), /* AT&T syntax is used instead of Intel's. */

												 /* The formatter's default formatting style. */
												 NMD_X86_FORMAT_FLAGS_DEFAULT = (NMD_X86_FORMAT_FLAGS_HEX | NMD_X86_FORMAT_FLAGS_H_SUFFIX | NMD_X86_FORMAT_FLAGS_ONLY_SEGMENT_OVERRIDE | NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_MEMORY_VIEW | NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_DEC),
};

enum NMD_X86_DECODER_FLAGS
{
	NMD_X86_DECODER_FLAGS_VALIDITY_CHECK = (1 << 0), /* The decoder checks if the instruction is valid. */
	NMD_X86_DECODER_FLAGS_INSTRUCTION_ID = (1 << 1), /* The decoder fills the 'id' variable. */
	NMD_X86_DECODER_FLAGS_CPU_FLAGS = (1 << 2), /* The decoder fills the 'cpuFlags' variable. */
	NMD_X86_DECODER_FLAGS_OPERANDS = (1 << 3), /* The decoder fills the 'numOperands' and 'operands' variable. */
	NMD_X86_DECODER_FLAGS_GROUP = (1 << 4), /* The decoder fills 'group' variable. */
	NMD_X86_DECODER_FLAGS_VEX = (1 << 5), /* The decoder parses VEX instructions. */
	NMD_X86_DECODER_FLAGS_EVEX = (1 << 6), /* The decoder parses EVEX instructions. */
	NMD_X86_DECODER_FLAGS_3DNOW = (1 << 7), /* The decoder parses 3DNow! instructions. */

											/* These are not actual features, but rather masks of features. */
											NMD_X86_DECODER_FLAGS_NONE = 0,
											NMD_X86_DECODER_FLAGS_MINIMAL = (NMD_X86_DECODER_FLAGS_VALIDITY_CHECK | NMD_X86_DECODER_FLAGS_VEX | NMD_X86_DECODER_FLAGS_EVEX), /* Mask that specifies minimal features to provide acurate results in any environment. */
											NMD_X86_DECODER_FLAGS_ALL = (1 << 8) - 1, /* Mask that specifies all features. */
};

enum NMD_X86_PREFIXES
{
	NMD_X86_PREFIXES_NONE = 0,
	NMD_X86_PREFIXES_ES_SEGMENT_OVERRIDE = (1 << 0),
	NMD_X86_PREFIXES_CS_SEGMENT_OVERRIDE = (1 << 1),
	NMD_X86_PREFIXES_SS_SEGMENT_OVERRIDE = (1 << 2),
	NMD_X86_PREFIXES_DS_SEGMENT_OVERRIDE = (1 << 3),
	NMD_X86_PREFIXES_FS_SEGMENT_OVERRIDE = (1 << 4),
	NMD_X86_PREFIXES_GS_SEGMENT_OVERRIDE = (1 << 5),
	NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE = (1 << 6),
	NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE = (1 << 7),
	NMD_X86_PREFIXES_LOCK = (1 << 8),
	NMD_X86_PREFIXES_REPEAT_NOT_ZERO = (1 << 9),
	NMD_X86_PREFIXES_REPEAT = (1 << 10),
	NMD_X86_PREFIXES_REX_W = (1 << 11),
	NMD_X86_PREFIXES_REX_R = (1 << 12),
	NMD_X86_PREFIXES_REX_X = (1 << 13),
	NMD_X86_PREFIXES_REX_B = (1 << 14)
};

enum NMD_X86_IMM
{
	NMD_X86_IMM_NONE = 0,
	NMD_X86_IMM8 = 1,
	NMD_X86_IMM16 = 2,
	NMD_X86_IMM32 = 4,
	NMD_X86_IMM48 = 6,
	NMD_X86_IMM64 = 8,
	NMD_X86_IMM_ANY = (NMD_X86_IMM8 | NMD_X86_IMM16 | NMD_X86_IMM32 | NMD_X86_IMM64)
};

enum NMD_X86_DISP
{
	NMD_X86_DISP_NONE = 0,
	NMD_X86_DISP8 = 1,
	NMD_X86_DISP16 = 2,
	NMD_X86_DISP32 = 4,
	NMD_X86_DISP64 = 8,
	NMD_X86_DISP_ANY = (NMD_X86_DISP8 | NMD_X86_DISP16 | NMD_X86_DISP32)
};

typedef union NMD_Modrm
{
	struct
	{
		uint8_t rm : 3;
		uint8_t reg : 3;
		uint8_t mod : 2;
	} fields;
	uint8_t modrm;
} NMD_Modrm;

typedef union NMD_SIB
{
	struct
	{
		uint8_t base : 3;
		uint8_t index : 3;
		uint8_t scale : 2;
	} fields;
	uint8_t sib;
} NMD_SIB;

typedef enum NMD_X86_MODE
{
	NMD_X86_MODE_NONE = 0, /* Invalid mode. */
	NMD_X86_MODE_16 = 2,
	NMD_X86_MODE_32 = 4,
	NMD_X86_MODE_64 = 8,
} NMD_X86_MODE;

enum NMD_X86_OPCODE_MAP
{
	NMD_X86_OPCODE_MAP_NONE = 0,
	NMD_X86_OPCODE_MAP_DEFAULT,
	NMD_X86_OPCODE_MAP_0F,
	NMD_X86_OPCODE_MAP_0F38,
	NMD_X86_OPCODE_MAP_0F3A,
	NMD_X86_OPCODE_MAP_0F0F
};

enum NMD_X86_ENCODING
{
	NMD_X86_ENCODING_NONE = 0,
	NMD_X86_ENCODING_LEGACY,  /* Legacy encoding. */
	NMD_X86_ENCODING_VEX,     /* Intel's VEX(vector extensions) coding scheme. */
	NMD_X86_ENCODING_EVEX,    /* Intel's EVEX(Enhanced vector extension) coding scheme. */
	NMD_X86_ENCODING_3DNOW,   /* AMD's 3DNow! extension. */
	NMD_X86_ENCODING_XOP,     /* AMD's XOP(eXtended Operations) instruction set. */
							  /* NMD_X86_ENCODING_MVEX,     MVEX used by Intel's "Xeon Phi" ISA. */
};

typedef struct NMD_X86Vex
{
	bool R : 1;
	bool X : 1;
	bool B : 1;
	bool L : 1;
	bool W : 1;
	uint8_t pp : 2;
	uint8_t m_mmmm : 5;
	uint8_t vvvv : 4;
	uint8_t vex[3]; /* The full vex prefix. vex[0] is either C4h(3-byte VEX) or C5h(2-byte VEX).*/
} NMD_X86Vex;

enum NMD_X86_REG
{
	NMD_X86_REG_NONE = 0,

	NMD_X86_REG_AL,
	NMD_X86_REG_CL,
	NMD_X86_REG_DL,
	NMD_X86_REG_BL,
	NMD_X86_REG_AH,
	NMD_X86_REG_CH,
	NMD_X86_REG_DH,
	NMD_X86_REG_BH,

	NMD_X86_REG_AX,
	NMD_X86_REG_CX,
	NMD_X86_REG_DX,
	NMD_X86_REG_BX,
	NMD_X86_REG_SP,
	NMD_X86_REG_BP,
	NMD_X86_REG_SI,
	NMD_X86_REG_DI,

	NMD_X86_REG_EAX,
	NMD_X86_REG_ECX,
	NMD_X86_REG_EDX,
	NMD_X86_REG_EBX,
	NMD_X86_REG_ESP,
	NMD_X86_REG_EBP,
	NMD_X86_REG_ESI,
	NMD_X86_REG_EDI,

	NMD_X86_REG_RAX,
	NMD_X86_REG_RBX,
	NMD_X86_REG_RCX,
	NMD_X86_REG_RDX,
	NMD_X86_REG_RBP,
	NMD_X86_REG_RSP,
	NMD_X86_REG_RSI,
	NMD_X86_REG_RDI,

	NMD_X86_REG_R8,
	NMD_X86_REG_R9,
	NMD_X86_REG_R10,
	NMD_X86_REG_R11,
	NMD_X86_REG_R12,
	NMD_X86_REG_R13,
	NMD_X86_REG_R14,
	NMD_X86_REG_R15,

	NMD_X86_REG_R8B,
	NMD_X86_REG_R9B,
	NMD_X86_REG_R10B,
	NMD_X86_REG_R11B,
	NMD_X86_REG_R12B,
	NMD_X86_REG_R13B,
	NMD_X86_REG_R14B,
	NMD_X86_REG_R15B,

	NMD_X86_REG_R8D,
	NMD_X86_REG_R9D,
	NMD_X86_REG_R10D,
	NMD_X86_REG_R11D,
	NMD_X86_REG_R12D,
	NMD_X86_REG_R13D,
	NMD_X86_REG_R14D,
	NMD_X86_REG_R15D,

	NMD_X86_REG_ES,
	NMD_X86_REG_CS,
	NMD_X86_REG_SS,
	NMD_X86_REG_DS,
	NMD_X86_REG_FS,
	NMD_X86_REG_GS,

	NMD_X86_REG_CR0,
	NMD_X86_REG_CR1,
	NMD_X86_REG_CR2,
	NMD_X86_REG_CR3,
	NMD_X86_REG_CR4,
	NMD_X86_REG_CR5,
	NMD_X86_REG_CR6,
	NMD_X86_REG_CR7,
	NMD_X86_REG_CR8,
	NMD_X86_REG_CR9,
	NMD_X86_REG_CR10,
	NMD_X86_REG_CR11,
	NMD_X86_REG_CR12,
	NMD_X86_REG_CR13,
	NMD_X86_REG_CR14,
	NMD_X86_REG_CR15,

	NMD_X86_REG_DR0,
	NMD_X86_REG_DR1,
	NMD_X86_REG_DR2,
	NMD_X86_REG_DR3,
	NMD_X86_REG_DR4,
	NMD_X86_REG_DR5,
	NMD_X86_REG_DR6,
	NMD_X86_REG_DR7,
	NMD_X86_REG_DR8,
	NMD_X86_REG_DR9,
	NMD_X86_REG_DR10,
	NMD_X86_REG_DR11,
	NMD_X86_REG_DR12,
	NMD_X86_REG_DR13,
	NMD_X86_REG_DR14,
	NMD_X86_REG_DR15,

	NMD_X86_REG_K0,
	NMD_X86_REG_K1,
	NMD_X86_REG_K2,
	NMD_X86_REG_K3,
	NMD_X86_REG_K4,
	NMD_X86_REG_K5,
	NMD_X86_REG_K6,
	NMD_X86_REG_K7,

	NMD_X86_REG_ST0,
	NMD_X86_REG_ST1,
	NMD_X86_REG_ST2,
	NMD_X86_REG_ST3,
	NMD_X86_REG_ST4,
	NMD_X86_REG_ST5,
	NMD_X86_REG_ST6,
	NMD_X86_REG_ST7,

	NMD_X86_REG_MM0,
	NMD_X86_REG_MM1,
	NMD_X86_REG_MM2,
	NMD_X86_REG_MM3,
	NMD_X86_REG_MM4,
	NMD_X86_REG_MM5,
	NMD_X86_REG_MM6,
	NMD_X86_REG_MM7,

	NMD_X86_REG_XMM0,
	NMD_X86_REG_XMM1,
	NMD_X86_REG_XMM2,
	NMD_X86_REG_XMM3,
	NMD_X86_REG_XMM4,
	NMD_X86_REG_XMM5,
	NMD_X86_REG_XMM6,
	NMD_X86_REG_XMM7,
	NMD_X86_REG_XMM8,
	NMD_X86_REG_XMM9,
	NMD_X86_REG_XMM10,
	NMD_X86_REG_XMM11,
	NMD_X86_REG_XMM12,
	NMD_X86_REG_XMM13,
	NMD_X86_REG_XMM14,
	NMD_X86_REG_XMM15,
	NMD_X86_REG_XMM16,
	NMD_X86_REG_XMM17,
	NMD_X86_REG_XMM18,
	NMD_X86_REG_XMM19,
	NMD_X86_REG_XMM20,
	NMD_X86_REG_XMM21,
	NMD_X86_REG_XMM22,
	NMD_X86_REG_XMM23,
	NMD_X86_REG_XMM24,
	NMD_X86_REG_XMM25,
	NMD_X86_REG_XMM26,
	NMD_X86_REG_XMM27,
	NMD_X86_REG_XMM28,
	NMD_X86_REG_XMM29,
	NMD_X86_REG_XMM30,
	NMD_X86_REG_XMM31,

	NMD_X86_REG_YMM0,
	NMD_X86_REG_YMM1,
	NMD_X86_REG_YMM2,
	NMD_X86_REG_YMM3,
	NMD_X86_REG_YMM4,
	NMD_X86_REG_YMM5,
	NMD_X86_REG_YMM6,
	NMD_X86_REG_YMM7,
	NMD_X86_REG_YMM8,
	NMD_X86_REG_YMM9,
	NMD_X86_REG_YMM10,
	NMD_X86_REG_YMM11,
	NMD_X86_REG_YMM12,
	NMD_X86_REG_YMM13,
	NMD_X86_REG_YMM14,
	NMD_X86_REG_YMM15,
	NMD_X86_REG_YMM16,
	NMD_X86_REG_YMM17,
	NMD_X86_REG_YMM18,
	NMD_X86_REG_YMM19,
	NMD_X86_REG_YMM20,
	NMD_X86_REG_YMM21,
	NMD_X86_REG_YMM22,
	NMD_X86_REG_YMM23,
	NMD_X86_REG_YMM24,
	NMD_X86_REG_YMM25,
	NMD_X86_REG_YMM26,
	NMD_X86_REG_YMM27,
	NMD_X86_REG_YMM28,
	NMD_X86_REG_YMM29,
	NMD_X86_REG_YMM30,
	NMD_X86_REG_YMM31,

	NMD_X86_REG_ZMM0,
	NMD_X86_REG_ZMM1,
	NMD_X86_REG_ZMM2,
	NMD_X86_REG_ZMM3,
	NMD_X86_REG_ZMM4,
	NMD_X86_REG_ZMM5,
	NMD_X86_REG_ZMM6,
	NMD_X86_REG_ZMM7,
	NMD_X86_REG_ZMM8,
	NMD_X86_REG_ZMM9,
	NMD_X86_REG_ZMM10,
	NMD_X86_REG_ZMM11,
	NMD_X86_REG_ZMM12,
	NMD_X86_REG_ZMM13,
	NMD_X86_REG_ZMM14,
	NMD_X86_REG_ZMM15,
	NMD_X86_REG_ZMM16,
	NMD_X86_REG_ZMM17,
	NMD_X86_REG_ZMM18,
	NMD_X86_REG_ZMM19,
	NMD_X86_REG_ZMM20,
	NMD_X86_REG_ZMM21,
	NMD_X86_REG_ZMM22,
	NMD_X86_REG_ZMM23,
	NMD_X86_REG_ZMM24,
	NMD_X86_REG_ZMM25,
	NMD_X86_REG_ZMM26,
	NMD_X86_REG_ZMM27,
	NMD_X86_REG_ZMM28,
	NMD_X86_REG_ZMM29,
	NMD_X86_REG_ZMM30,
	NMD_X86_REG_ZMM31,
};

enum NMD_GROUP {
	NMD_GROUP_NONE = 0, /* The instruction is not part of any group. */

	NMD_GROUP_JUMP = (1 << 0), /* All jump instructions. */
	NMD_GROUP_CALL = (1 << 1), /* Call instruction. */
	NMD_GROUP_RET = (1 << 2), /* Return instruction. */
	NMD_GROUP_INT = (1 << 3), /* Interrupt instruction. */
	NMD_GROUP_PRIVILEGE = (1 << 4), /* Privileged instruction. */
	NMD_GROUP_CONDITIONAL_BRANCH = (1 << 5), /* Conditional branch instruction. */
	NMD_GROUP_UNCONDITIONAL_BRANCH = (1 << 6), /* Unconditional branch instruction. */
	NMD_GROUP_RELATIVE_ADDRESSING = (1 << 7), /* Relative addressing instruction. */

											  /* These are not actual groups, but rather masks of groups. */
											  NMD_GROUP_BRANCH = (NMD_GROUP_CONDITIONAL_BRANCH | NMD_GROUP_UNCONDITIONAL_BRANCH), /* Mask used to check if the instruction is a branch instruction. */
											  NMD_GROUP_ANY = (1 << 8) - 1, /* Mask used to check if the instruction is part of any group. */
};

/*
Credits to the capstone engine:
Some members of the enum are organized in such a way because the instruction's id parsing component of the decoder can take advantage of it.
If an instruction as marked as 'padding', it means that it's being used to fill holes between instructions organized in a special way for optimization reasons.
*/
enum NMD_X86_INSTRUCTION
{
	NMD_X86_INSTRUCTION_INVALID = 0,

	/* Optimized for opcode extension group 1. */
	NMD_X86_INSTRUCTION_ADD,
	NMD_X86_INSTRUCTION_OR,
	NMD_X86_INSTRUCTION_ADC,
	NMD_X86_INSTRUCTION_SBB,
	NMD_X86_INSTRUCTION_AND,
	NMD_X86_INSTRUCTION_SUB,
	NMD_X86_INSTRUCTION_XOR,
	NMD_X86_INSTRUCTION_CMP,

	/* Optimized for opcode extension group 2. */
	NMD_X86_INSTRUCTION_ROL,
	NMD_X86_INSTRUCTION_ROR,
	NMD_X86_INSTRUCTION_RCL,
	NMD_X86_INSTRUCTION_RCR,
	NMD_X86_INSTRUCTION_SHL,
	NMD_X86_INSTRUCTION_SHR,
	NMD_X86_INSTRUCTION_AAA, /* padding */
	NMD_X86_INSTRUCTION_SAR,

	/* Optimized for opcode extension group 3. */
	NMD_X86_INSTRUCTION_TEST,
	NMD_X86_INSTRUCTION_BLSFILL, /* pading */
	NMD_X86_INSTRUCTION_NOT,
	NMD_X86_INSTRUCTION_NEG,
	NMD_X86_INSTRUCTION_MUL,
	NMD_X86_INSTRUCTION_IMUL,
	NMD_X86_INSTRUCTION_DIV,
	NMD_X86_INSTRUCTION_IDIV,

	/* Optimized for opcode extension group 5. */
	NMD_X86_INSTRUCTION_INC,
	NMD_X86_INSTRUCTION_DEC,
	NMD_X86_INSTRUCTION_CALL,
	NMD_X86_INSTRUCTION_LCALL,
	NMD_X86_INSTRUCTION_JMP,
	NMD_X86_INSTRUCTION_LJMP,
	NMD_X86_INSTRUCTION_PUSH,

	/* Optimized for the 7th row of the 1 byte opcode map and the 8th row of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_JO,
	NMD_X86_INSTRUCTION_JNO,
	NMD_X86_INSTRUCTION_JB,
	NMD_X86_INSTRUCTION_JAE,
	NMD_X86_INSTRUCTION_JE,
	NMD_X86_INSTRUCTION_JNE,
	NMD_X86_INSTRUCTION_JBE,
	NMD_X86_INSTRUCTION_JA,
	NMD_X86_INSTRUCTION_JS,
	NMD_X86_INSTRUCTION_JNS,
	NMD_X86_INSTRUCTION_JP,
	NMD_X86_INSTRUCTION_JNP,
	NMD_X86_INSTRUCTION_JL,
	NMD_X86_INSTRUCTION_JGE,
	NMD_X86_INSTRUCTION_JLE,
	NMD_X86_INSTRUCTION_JG,

	/* Optimized for escape opcodes with D8 as first byte. */
	NMD_X86_INSTRUCTION_FADD,
	NMD_X86_INSTRUCTION_FMUL,
	NMD_X86_INSTRUCTION_FCOM,
	NMD_X86_INSTRUCTION_FCOMP,
	NMD_X86_INSTRUCTION_FSUB,
	NMD_X86_INSTRUCTION_FSUBR,
	NMD_X86_INSTRUCTION_FDIV,
	NMD_X86_INSTRUCTION_FDIVR,

	/* Optimized for escape opcodes with D9 as first byte. */
	NMD_X86_INSTRUCTION_FLD,
	NMD_X86_INSTRUCTION_ADOX, /* padding */
	NMD_X86_INSTRUCTION_FST,
	NMD_X86_INSTRUCTION_FSTP,
	NMD_X86_INSTRUCTION_FLDENV,
	NMD_X86_INSTRUCTION_FLDCW,
	NMD_X86_INSTRUCTION_FNSTENV,
	NMD_X86_INSTRUCTION_FNSTCW,

	NMD_X86_INSTRUCTION_FCHS,
	NMD_X86_INSTRUCTION_FABS,
	NMD_X86_INSTRUCTION_AAS, /* padding */
	NMD_X86_INSTRUCTION_ADCX, /* padding */
	NMD_X86_INSTRUCTION_FTST,
	NMD_X86_INSTRUCTION_FXAM,
	NMD_X86_INSTRUCTION_RET, /* padding */
	NMD_X86_INSTRUCTION_ENTER, /* padding */
	NMD_X86_INSTRUCTION_FLD1,
	NMD_X86_INSTRUCTION_FLDL2T,
	NMD_X86_INSTRUCTION_FLDL2E,
	NMD_X86_INSTRUCTION_FLDPI,
	NMD_X86_INSTRUCTION_FLDLG2,
	NMD_X86_INSTRUCTION_FLDLN2,
	NMD_X86_INSTRUCTION_FLDZ,
	NMD_X86_INSTRUCTION_FNOP, /* padding */
	NMD_X86_INSTRUCTION_F2XM1,
	NMD_X86_INSTRUCTION_FYL2X,
	NMD_X86_INSTRUCTION_FPTAN,
	NMD_X86_INSTRUCTION_FPATAN,
	NMD_X86_INSTRUCTION_FXTRACT,
	NMD_X86_INSTRUCTION_FPREM1,
	NMD_X86_INSTRUCTION_FDECSTP,
	NMD_X86_INSTRUCTION_FINCSTP,
	NMD_X86_INSTRUCTION_FPREM,
	NMD_X86_INSTRUCTION_FYL2XP1,
	NMD_X86_INSTRUCTION_FSQRT,
	NMD_X86_INSTRUCTION_FSINCOS,
	NMD_X86_INSTRUCTION_FRNDINT,
	NMD_X86_INSTRUCTION_FSCALE,
	NMD_X86_INSTRUCTION_FSIN,
	NMD_X86_INSTRUCTION_FCOS,

	/* Optimized for escape opcodes with DA as first byte. */
	NMD_X86_INSTRUCTION_FIADD,
	NMD_X86_INSTRUCTION_FIMUL,
	NMD_X86_INSTRUCTION_FICOM,
	NMD_X86_INSTRUCTION_FICOMP,
	NMD_X86_INSTRUCTION_FISUB,
	NMD_X86_INSTRUCTION_FISUBR,
	NMD_X86_INSTRUCTION_FIDIV,
	NMD_X86_INSTRUCTION_FIDIVR,

	NMD_X86_INSTRUCTION_FCMOVB,
	NMD_X86_INSTRUCTION_FCMOVE,
	NMD_X86_INSTRUCTION_FCMOVBE,
	NMD_X86_INSTRUCTION_FCMOVU,

	/* Optimized for escape opcodes with DB/DF as first byte. */
	NMD_X86_INSTRUCTION_FILD,
	NMD_X86_INSTRUCTION_FISTTP,
	NMD_X86_INSTRUCTION_FIST,
	NMD_X86_INSTRUCTION_FISTP,
	NMD_X86_INSTRUCTION_FBLD,
	NMD_X86_INSTRUCTION_AESKEYGENASSIST, /* padding */
	NMD_X86_INSTRUCTION_FBSTP,
	NMD_X86_INSTRUCTION_ANDN, /* padding */

	NMD_X86_INSTRUCTION_FCMOVNB,
	NMD_X86_INSTRUCTION_FCMOVNE,
	NMD_X86_INSTRUCTION_FCMOVNBE,
	NMD_X86_INSTRUCTION_FCMOVNU,
	NMD_X86_INSTRUCTION_FNCLEX,
	NMD_X86_INSTRUCTION_FUCOMI,
	NMD_X86_INSTRUCTION_FCOMI,

	/* Optimized for escape opcodes with DE as first byte. */
	NMD_X86_INSTRUCTION_FADDP,
	NMD_X86_INSTRUCTION_FMULP,
	NMD_X86_INSTRUCTION_MOVAPD, /* padding */
	NMD_X86_INSTRUCTION_BNDCN, /* padding */
	NMD_X86_INSTRUCTION_FSUBRP,
	NMD_X86_INSTRUCTION_FSUBP,
	NMD_X86_INSTRUCTION_FDIVRP,
	NMD_X86_INSTRUCTION_FDIVP,

	/* Optimized for the 15th row of the 1 byte opcode map. */
	NMD_X86_INSTRUCTION_INT1,
	NMD_X86_INSTRUCTION_BSR, /* padding */
	NMD_X86_INSTRUCTION_ADDSUBPD, /* padding */
	NMD_X86_INSTRUCTION_HLT,
	NMD_X86_INSTRUCTION_CMC,
	NMD_X86_INSTRUCTION_ADDSUBPS, /* padding */
	NMD_X86_INSTRUCTION_BLENDVPD, /* padding*/
	NMD_X86_INSTRUCTION_CLC,
	NMD_X86_INSTRUCTION_STC,
	NMD_X86_INSTRUCTION_CLI,
	NMD_X86_INSTRUCTION_STI,
	NMD_X86_INSTRUCTION_CLD,
	NMD_X86_INSTRUCTION_STD,

	/* Optimized for the 13th row of the 1 byte opcode map. */
	NMD_X86_INSTRUCTION_AAM,
	NMD_X86_INSTRUCTION_AAD,
	NMD_X86_INSTRUCTION_BLENDVPS, /* padding*/
	NMD_X86_INSTRUCTION_XLAT,

	/* Optimized for the 14th row of the 1 byte opcode map. */
	NMD_X86_INSTRUCTION_LOOPNE,
	NMD_X86_INSTRUCTION_LOOPE,
	NMD_X86_INSTRUCTION_LOOP,
	NMD_X86_INSTRUCTION_JRCXZ,

	/* Optimized for opcode extension group 6. */
	NMD_X86_INSTRUCTION_SLDT,
	NMD_X86_INSTRUCTION_STR,
	NMD_X86_INSTRUCTION_LLDT,
	NMD_X86_INSTRUCTION_LTR,
	NMD_X86_INSTRUCTION_VERR,
	NMD_X86_INSTRUCTION_VERW,

	/* Optimized for opcode extension group 7. */
	NMD_X86_INSTRUCTION_SGDT,
	NMD_X86_INSTRUCTION_SIDT,
	NMD_X86_INSTRUCTION_LGDT,
	NMD_X86_INSTRUCTION_LIDT,
	NMD_X86_INSTRUCTION_SMSW,
	NMD_X86_INSTRUCTION_CLWB, /* padding */
	NMD_X86_INSTRUCTION_LMSW,
	NMD_X86_INSTRUCTION_INVLPG,

	NMD_X86_INSTRUCTION_VMCALL,
	NMD_X86_INSTRUCTION_VMLAUNCH,
	NMD_X86_INSTRUCTION_VMRESUME,
	NMD_X86_INSTRUCTION_VMXOFF,

	NMD_X86_INSTRUCTION_MONITOR,
	NMD_X86_INSTRUCTION_MWAIT,
	NMD_X86_INSTRUCTION_CLAC,
	NMD_X86_INSTRUCTION_STAC,
	NMD_X86_INSTRUCTION_CBW, /* padding */
	NMD_X86_INSTRUCTION_CMPSB, /* padding */
	NMD_X86_INSTRUCTION_CMPSQ, /* padding */
	NMD_X86_INSTRUCTION_ENCLS,

	NMD_X86_INSTRUCTION_XGETBV,
	NMD_X86_INSTRUCTION_XSETBV,
	NMD_X86_INSTRUCTION_ARPL, /* padding */
	NMD_X86_INSTRUCTION_BEXTR, /* padding */
	NMD_X86_INSTRUCTION_VMFUNC,
	NMD_X86_INSTRUCTION_XEND,
	NMD_X86_INSTRUCTION_XTEST,
	NMD_X86_INSTRUCTION_ENCLU,

	NMD_X86_INSTRUCTION_VMRUN,
	NMD_X86_INSTRUCTION_VMMCALL,
	NMD_X86_INSTRUCTION_VMLOAD,
	NMD_X86_INSTRUCTION_VMSAVE,
	NMD_X86_INSTRUCTION_STGI,
	NMD_X86_INSTRUCTION_CLGI,
	NMD_X86_INSTRUCTION_SKINIT,
	NMD_X86_INSTRUCTION_INVLPGA,

	/* Optimized for the row 0x0 of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_LAR,
	NMD_X86_INSTRUCTION_LSL,
	NMD_X86_INSTRUCTION_BLCFILL, /* padding */
	NMD_X86_INSTRUCTION_SYSCALL,
	NMD_X86_INSTRUCTION_CLTS,
	NMD_X86_INSTRUCTION_SYSRET,
	NMD_X86_INSTRUCTION_INVD,
	NMD_X86_INSTRUCTION_WBINVD,
	NMD_X86_INSTRUCTION_BLCI, /* padding */
	NMD_X86_INSTRUCTION_UD2,
	NMD_X86_INSTRUCTION_PREFETCHW,
	NMD_X86_INSTRUCTION_FEMMS,

	/* Optimized for the row 0x3 of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_WRMSR,
	NMD_X86_INSTRUCTION_RDTSC,
	NMD_X86_INSTRUCTION_RDMSR,
	NMD_X86_INSTRUCTION_RDPMC,
	NMD_X86_INSTRUCTION_SYSENTER,
	NMD_X86_INSTRUCTION_SYSEXIT,
	NMD_X86_INSTRUCTION_BLCIC, /* padding */
	NMD_X86_INSTRUCTION_GETSEC,

	/* Optimized for the row 0x4 of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_CMOVO,
	NMD_X86_INSTRUCTION_CMOVNO,
	NMD_X86_INSTRUCTION_CMOVB,
	NMD_X86_INSTRUCTION_CMOVAE,
	NMD_X86_INSTRUCTION_CMOVE,
	NMD_X86_INSTRUCTION_CMOVNE,
	NMD_X86_INSTRUCTION_CMOVBE,
	NMD_X86_INSTRUCTION_CMOVA,
	NMD_X86_INSTRUCTION_CMOVS,
	NMD_X86_INSTRUCTION_CMOVNS,
	NMD_X86_INSTRUCTION_CMOVP,
	NMD_X86_INSTRUCTION_CMOVNP,
	NMD_X86_INSTRUCTION_CMOVL,
	NMD_X86_INSTRUCTION_CMOVGE,
	NMD_X86_INSTRUCTION_CMOVLE,
	NMD_X86_INSTRUCTION_CMOVG,

	/* Optimized for the row 0x9 of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_SETO,
	NMD_X86_INSTRUCTION_SETNO,
	NMD_X86_INSTRUCTION_SETB,
	NMD_X86_INSTRUCTION_SETAE,
	NMD_X86_INSTRUCTION_SETE,
	NMD_X86_INSTRUCTION_SETNE,
	NMD_X86_INSTRUCTION_SETBE,
	NMD_X86_INSTRUCTION_SETA,
	NMD_X86_INSTRUCTION_SETS,
	NMD_X86_INSTRUCTION_SETNS,
	NMD_X86_INSTRUCTION_SETP,
	NMD_X86_INSTRUCTION_SETNP,
	NMD_X86_INSTRUCTION_SETL,
	NMD_X86_INSTRUCTION_SETGE,
	NMD_X86_INSTRUCTION_SETLE,
	NMD_X86_INSTRUCTION_SETG,

	/* Optimized for the row 0xb of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_LSS,
	NMD_X86_INSTRUCTION_BTR,
	NMD_X86_INSTRUCTION_LFS,
	NMD_X86_INSTRUCTION_LGS,

	NMD_X86_INSTRUCTION_BT,
	NMD_X86_INSTRUCTION_BTC,
	NMD_X86_INSTRUCTION_BTS,

	/* Optimized for the row 0x0 of the 3 byte opcode map(38h). */
	NMD_X86_INSTRUCTION_PSHUFB,
	NMD_X86_INSTRUCTION_PHADDW,
	NMD_X86_INSTRUCTION_PHADDD,
	NMD_X86_INSTRUCTION_PHADDSW,
	NMD_X86_INSTRUCTION_PMADDUBSW,
	NMD_X86_INSTRUCTION_PHSUBW,
	NMD_X86_INSTRUCTION_PHSUBD,
	NMD_X86_INSTRUCTION_PHSUBSW,
	NMD_X86_INSTRUCTION_PSIGNB,
	NMD_X86_INSTRUCTION_PSIGNW,
	NMD_X86_INSTRUCTION_PSIGND,
	NMD_X86_INSTRUCTION_PMULHRSW,

	/* Optimized for the row 0x1 of the 3 byte opcode map(38h). */
	NMD_X86_INSTRUCTION_PABSB,
	NMD_X86_INSTRUCTION_PABSW,
	NMD_X86_INSTRUCTION_PABSD,

	/* Optimized for the row 0x2 of the 3 byte opcode map(38). */
	NMD_X86_INSTRUCTION_PMOVSXBW,
	NMD_X86_INSTRUCTION_PMOVSXBD,
	NMD_X86_INSTRUCTION_PMOVSXBQ,
	NMD_X86_INSTRUCTION_PMOVSXWD,
	NMD_X86_INSTRUCTION_PMOVSXWQ,
	NMD_X86_INSTRUCTION_PMOVZXDQ,
	NMD_X86_INSTRUCTION_CPUID, /* padding */
	NMD_X86_INSTRUCTION_BLCMSK, /* padding */
	NMD_X86_INSTRUCTION_PMULDQ,
	NMD_X86_INSTRUCTION_PCMPEQQ,
	NMD_X86_INSTRUCTION_MOVNTDQA,
	NMD_X86_INSTRUCTION_PACKUSDW,

	/* Optimized for the row 0x3 of the 3 byte opcode map(38h). */
	NMD_X86_INSTRUCTION_PMOVZXBW,
	NMD_X86_INSTRUCTION_PMOVZXBD,
	NMD_X86_INSTRUCTION_PMOVZXBQ,
	NMD_X86_INSTRUCTION_PMOVZXWD,
	NMD_X86_INSTRUCTION_PMOVZXWQ,
	NMD_X86_INSTRUCTION_PMOVSXDQ,
	NMD_X86_INSTRUCTION_BLCS, /* padding */
	NMD_X86_INSTRUCTION_PCMPGTQ,
	NMD_X86_INSTRUCTION_PMINSB,
	NMD_X86_INSTRUCTION_PMINSD,
	NMD_X86_INSTRUCTION_PMINUW,
	NMD_X86_INSTRUCTION_PMINUD,
	NMD_X86_INSTRUCTION_PMAXSB,
	NMD_X86_INSTRUCTION_PMAXSD,
	NMD_X86_INSTRUCTION_PMAXUW,
	NMD_X86_INSTRUCTION_PMAXUD,

	/* Optimized for the row 0x8 of the 3 byte opcode map(38h). */
	NMD_X86_INSTRUCTION_INVEPT,
	NMD_X86_INSTRUCTION_INVVPID,
	NMD_X86_INSTRUCTION_INVPCID,

	/* Optimized for the row 0xc of the 3 byte opcode map(38h). */
	NMD_X86_INSTRUCTION_SHA1NEXTE,
	NMD_X86_INSTRUCTION_SHA1MSG1,
	NMD_X86_INSTRUCTION_SHA1MSG2,
	NMD_X86_INSTRUCTION_SHA256RNDS2,
	NMD_X86_INSTRUCTION_SHA256MSG1,
	NMD_X86_INSTRUCTION_SHA256MSG2,

	/* Optimized for the row 0xd of the 3 byte opcode map(38h). */
	NMD_X86_INSTRUCTION_AESIMC,
	NMD_X86_INSTRUCTION_AESENC,
	NMD_X86_INSTRUCTION_AESENCLAST,
	NMD_X86_INSTRUCTION_AESDEC,
	NMD_X86_INSTRUCTION_AESDECLAST,

	/* Optimized for the row 0x0 of the 3 byte opcode map(3Ah). */
	NMD_X86_INSTRUCTION_ROUNDPS,
	NMD_X86_INSTRUCTION_ROUNDPD,
	NMD_X86_INSTRUCTION_ROUNDSS,
	NMD_X86_INSTRUCTION_ROUNDSD,
	NMD_X86_INSTRUCTION_BLENDPS,
	NMD_X86_INSTRUCTION_BLENDPD,
	NMD_X86_INSTRUCTION_PBLENDW,
	NMD_X86_INSTRUCTION_PALIGNR,

	/* Optimized for the row 0x4 of the 3 byte opcode map(3A). */
	NMD_X86_INSTRUCTION_DPPS,
	NMD_X86_INSTRUCTION_DPPD,
	NMD_X86_INSTRUCTION_MPSADBW,
	NMD_X86_INSTRUCTION_VPCMPGTQ, /* padding */
	NMD_X86_INSTRUCTION_PCLMULQDQ,

	/* Optimized for the row 0x6 of the 3 byte opcode map(3A). */
	NMD_X86_INSTRUCTION_PCMPESTRM,
	NMD_X86_INSTRUCTION_PCMPESTRI,
	NMD_X86_INSTRUCTION_PCMPISTRM,
	NMD_X86_INSTRUCTION_PCMPISTRI,

	/* Optimized for the rows 0xd, 0xe and 0xf of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_PSRLW,
	NMD_X86_INSTRUCTION_PSRLD,
	NMD_X86_INSTRUCTION_PSRLQ,
	NMD_X86_INSTRUCTION_PADDQ,
	NMD_X86_INSTRUCTION_PMULLW,
	NMD_X86_INSTRUCTION_BOUND, /* padding */
	NMD_X86_INSTRUCTION_PMOVMSKB,
	NMD_X86_INSTRUCTION_PSUBUSB,
	NMD_X86_INSTRUCTION_PSUBUSW,
	NMD_X86_INSTRUCTION_PMINUB,
	NMD_X86_INSTRUCTION_PAND,
	NMD_X86_INSTRUCTION_PADDUSB,
	NMD_X86_INSTRUCTION_PADDUSW,
	NMD_X86_INSTRUCTION_PMAXUB,
	NMD_X86_INSTRUCTION_PANDN,
	NMD_X86_INSTRUCTION_PAVGB,
	NMD_X86_INSTRUCTION_PSRAW,
	NMD_X86_INSTRUCTION_PSRAD,
	NMD_X86_INSTRUCTION_PAVGW,
	NMD_X86_INSTRUCTION_PMULHUW,
	NMD_X86_INSTRUCTION_PMULHW,
	NMD_X86_INSTRUCTION_CQO, /* padding */
	NMD_X86_INSTRUCTION_CRC32, /* padding */
	NMD_X86_INSTRUCTION_PSUBSB,
	NMD_X86_INSTRUCTION_PSUBSW,
	NMD_X86_INSTRUCTION_PMINSW,
	NMD_X86_INSTRUCTION_POR,
	NMD_X86_INSTRUCTION_PADDSB,
	NMD_X86_INSTRUCTION_PADDSW,
	NMD_X86_INSTRUCTION_PMAXSW,
	NMD_X86_INSTRUCTION_PXOR,
	NMD_X86_INSTRUCTION_LDDQU,
	NMD_X86_INSTRUCTION_PSLLW,
	NMD_X86_INSTRUCTION_PSLLD,
	NMD_X86_INSTRUCTION_PSLLQ,
	NMD_X86_INSTRUCTION_PMULUDQ,
	NMD_X86_INSTRUCTION_PMADDWD,
	NMD_X86_INSTRUCTION_PSADBW,
	NMD_X86_INSTRUCTION_BSWAP, /* padding */
	NMD_X86_INSTRUCTION_PSUBB,
	NMD_X86_INSTRUCTION_PSUBW,
	NMD_X86_INSTRUCTION_PSUBD,
	NMD_X86_INSTRUCTION_PSUBQ,
	NMD_X86_INSTRUCTION_PADDB,
	NMD_X86_INSTRUCTION_PADDW,
	NMD_X86_INSTRUCTION_PADDD,

	/* Optimized for the row 0xc of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_MOVNTI,
	NMD_X86_INSTRUCTION_PINSRW,
	NMD_X86_INSTRUCTION_PEXTRW,

	/* Optimized for opcode extension group 15. */
	NMD_X86_INSTRUCTION_FXSAVE,
	NMD_X86_INSTRUCTION_FXRSTOR,
	NMD_X86_INSTRUCTION_LDMXCSR,
	NMD_X86_INSTRUCTION_STMXCSR,
	NMD_X86_INSTRUCTION_XSAVE,
	NMD_X86_INSTRUCTION_XRSTOR,
	NMD_X86_INSTRUCTION_XSAVEOPT,
	NMD_X86_INSTRUCTION_CLFLUSH,

	NMD_X86_INSTRUCTION_RDFSBASE,
	NMD_X86_INSTRUCTION_RDGSBASE,
	NMD_X86_INSTRUCTION_WRFSBASE,
	NMD_X86_INSTRUCTION_WRGSBASE,
	NMD_X86_INSTRUCTION_CMPXCHG, /* padding */
	NMD_X86_INSTRUCTION_LFENCE,
	NMD_X86_INSTRUCTION_MFENCE,
	NMD_X86_INSTRUCTION_SFENCE,

	NMD_X86_INSTRUCTION_PCMPEQB,
	NMD_X86_INSTRUCTION_PCMPEQW,
	NMD_X86_INSTRUCTION_PCMPEQD,

	/* Optimized for the row 0x5 of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_MOVMSKPS,
	NMD_X86_INSTRUCTION_SQRTPS,
	NMD_X86_INSTRUCTION_RSQRTPS,
	NMD_X86_INSTRUCTION_RCPPS,
	NMD_X86_INSTRUCTION_ANDPS,
	NMD_X86_INSTRUCTION_ANDNPS,
	NMD_X86_INSTRUCTION_ORPS,
	NMD_X86_INSTRUCTION_XORPS,
	NMD_X86_INSTRUCTION_ADDPS,
	NMD_X86_INSTRUCTION_MULPS,
	NMD_X86_INSTRUCTION_CVTPS2PD,
	NMD_X86_INSTRUCTION_CVTDQ2PS,
	NMD_X86_INSTRUCTION_SUBPS,
	NMD_X86_INSTRUCTION_MINPS,
	NMD_X86_INSTRUCTION_DIVPS,
	NMD_X86_INSTRUCTION_MAXPS,

	NMD_X86_INSTRUCTION_MOVMSKPD,
	NMD_X86_INSTRUCTION_SQRTPD,
	NMD_X86_INSTRUCTION_BNDLDX, /* padding */
	NMD_X86_INSTRUCTION_BNDSTX, /* padding */
	NMD_X86_INSTRUCTION_ANDPD,
	NMD_X86_INSTRUCTION_ANDNPD,
	NMD_X86_INSTRUCTION_ORPD,
	NMD_X86_INSTRUCTION_XORPD,
	NMD_X86_INSTRUCTION_ADDPD,
	NMD_X86_INSTRUCTION_MULPD,
	NMD_X86_INSTRUCTION_CVTPD2PS,
	NMD_X86_INSTRUCTION_CVTPS2DQ,
	NMD_X86_INSTRUCTION_SUBPD,
	NMD_X86_INSTRUCTION_MINPD,
	NMD_X86_INSTRUCTION_DIVPD,
	NMD_X86_INSTRUCTION_MAXPD,

	NMD_X86_INSTRUCTION_BNDMOV,  /* padding */
	NMD_X86_INSTRUCTION_SQRTSS,
	NMD_X86_INSTRUCTION_RSQRTSS,
	NMD_X86_INSTRUCTION_RCPSS,
	NMD_X86_INSTRUCTION_CMPXCHG16B, /* padding */
	NMD_X86_INSTRUCTION_DAA, /* padding */
	NMD_X86_INSTRUCTION_CWD, /* padding */
	NMD_X86_INSTRUCTION_INSD, /* padding */
	NMD_X86_INSTRUCTION_ADDSS,
	NMD_X86_INSTRUCTION_MULSS,
	NMD_X86_INSTRUCTION_CVTSS2SD,
	NMD_X86_INSTRUCTION_CVTTPS2DQ,
	NMD_X86_INSTRUCTION_SUBSS,
	NMD_X86_INSTRUCTION_MINSS,
	NMD_X86_INSTRUCTION_DIVSS,
	NMD_X86_INSTRUCTION_MAXSS,

	NMD_X86_INSTRUCTION_BNDCL, /* padding */
	NMD_X86_INSTRUCTION_SQRTSD,
	NMD_X86_INSTRUCTION_BNDCU, /* padding */
	NMD_X86_INSTRUCTION_BNDMK, /* padding */
	NMD_X86_INSTRUCTION_CMPXCHG8B, /* padding */
	NMD_X86_INSTRUCTION_DAS, /* padding */
	NMD_X86_INSTRUCTION_CWDE, /* padding */
	NMD_X86_INSTRUCTION_INSW, /* padding */
	NMD_X86_INSTRUCTION_ADDSD,
	NMD_X86_INSTRUCTION_MULSD,
	NMD_X86_INSTRUCTION_CVTSD2SS,
	NMD_X86_INSTRUCTION_FCOMIP, /* padding */
	NMD_X86_INSTRUCTION_SUBSD,
	NMD_X86_INSTRUCTION_MINSD,
	NMD_X86_INSTRUCTION_DIVSD,
	NMD_X86_INSTRUCTION_MAXSD,

	/* Optimized for the row 0x6 of the 2 byte opcode map. */
	NMD_X86_INSTRUCTION_PUNPCKLBW,
	NMD_X86_INSTRUCTION_PUNPCKLWD,
	NMD_X86_INSTRUCTION_PUNPCKLDQ,
	NMD_X86_INSTRUCTION_PACKSSWB,
	NMD_X86_INSTRUCTION_PCMPGTB,
	NMD_X86_INSTRUCTION_PCMPGTW,
	NMD_X86_INSTRUCTION_PCMPGTD,
	NMD_X86_INSTRUCTION_PACKUSWB,
	NMD_X86_INSTRUCTION_PUNPCKHBW,
	NMD_X86_INSTRUCTION_PUNPCKHWD,
	NMD_X86_INSTRUCTION_PUNPCKHDQ,
	NMD_X86_INSTRUCTION_PACKSSDW,
	NMD_X86_INSTRUCTION_PUNPCKLQDQ,
	NMD_X86_INSTRUCTION_PUNPCKHQDQ,

	/* Optimized for AVX instructions. */
	NMD_X86_INSTRUCTION_VPSHUFB,    /* 00 */
	NMD_X86_INSTRUCTION_VPHADDW,    /* 01 */
	NMD_X86_INSTRUCTION_VPHADDD,    /* 02 */
	NMD_X86_INSTRUCTION_VPHADDSW,   /* 03 */
	NMD_X86_INSTRUCTION_VPMADDUBSW, /* 04 */
	NMD_X86_INSTRUCTION_VPHSUBW,    /* 05 */
	NMD_X86_INSTRUCTION_VPHSUBD,    /* 06 */
	NMD_X86_INSTRUCTION_VPHSUBSW,   /* 07 */
	NMD_X86_INSTRUCTION_VPSIGNB,    /* 08 */
	NMD_X86_INSTRUCTION_VPSIGNW,    /* 09 */
	NMD_X86_INSTRUCTION_VPSIGND,    /* 0A dup */
	NMD_X86_INSTRUCTION_VPMULHRSW,  /* 0B dup */

	NMD_X86_INSTRUCTION_VPHADDWQ,
	NMD_X86_INSTRUCTION_VPHADDDQ,
	NMD_X86_INSTRUCTION_BLSI,
	NMD_X86_INSTRUCTION_BLSIC,
	NMD_X86_INSTRUCTION_BLSMSK,
	NMD_X86_INSTRUCTION_BLSR,
	NMD_X86_INSTRUCTION_BSF,
	NMD_X86_INSTRUCTION_BZHI,
	NMD_X86_INSTRUCTION_CDQ,
	NMD_X86_INSTRUCTION_CDQE,
	NMD_X86_INSTRUCTION_CLFLUSHOPT,
	NMD_X86_INSTRUCTION_CMPSW,
	NMD_X86_INSTRUCTION_COMISD,
	NMD_X86_INSTRUCTION_COMISS,
	NMD_X86_INSTRUCTION_CVTDQ2PD,
	NMD_X86_INSTRUCTION_CVTPD2DQ,
	NMD_X86_INSTRUCTION_CVTSD2SI,
	NMD_X86_INSTRUCTION_CVTSI2SD,
	NMD_X86_INSTRUCTION_CVTSI2SS,
	NMD_X86_INSTRUCTION_CVTSS2SI,
	NMD_X86_INSTRUCTION_CVTTPD2DQ,
	NMD_X86_INSTRUCTION_CVTTSD2SI,
	NMD_X86_INSTRUCTION_CVTTSS2SI,
	NMD_X86_INSTRUCTION_DATA16,
	NMD_X86_INSTRUCTION_EXTRACTPS,
	NMD_X86_INSTRUCTION_EXTRQ,
	NMD_X86_INSTRUCTION_FCOMPP,
	NMD_X86_INSTRUCTION_FFREE,
	NMD_X86_INSTRUCTION_FNINIT,
	NMD_X86_INSTRUCTION_FNSTSW,
	NMD_X86_INSTRUCTION_FFREEP,
	NMD_X86_INSTRUCTION_FRSTOR,
	NMD_X86_INSTRUCTION_FNSAVE,
	NMD_X86_INSTRUCTION_FSETPM,
	NMD_X86_INSTRUCTION_FXRSTOR64,
	NMD_X86_INSTRUCTION_FXSAVE64,
	NMD_X86_INSTRUCTION_MOVAPS,
	NMD_X86_INSTRUCTION_VMOVAPD,
	NMD_X86_INSTRUCTION_VMOVAPS,
	NMD_X86_INSTRUCTION_HADDPD,
	NMD_X86_INSTRUCTION_HADDPS,
	NMD_X86_INSTRUCTION_HSUBPD,
	NMD_X86_INSTRUCTION_HSUBPS,
	NMD_X86_INSTRUCTION_IN,
	NMD_X86_INSTRUCTION_INSB,
	NMD_X86_INSTRUCTION_INSERTPS,
	NMD_X86_INSTRUCTION_INSERTQ,
	NMD_X86_INSTRUCTION_INT,
	NMD_X86_INSTRUCTION_INT3,
	NMD_X86_INSTRUCTION_INTO,
	NMD_X86_INSTRUCTION_IRET,
	NMD_X86_INSTRUCTION_IRETD,
	NMD_X86_INSTRUCTION_IRETQ,
	NMD_X86_INSTRUCTION_UCOMISD,
	NMD_X86_INSTRUCTION_UCOMISS,
	NMD_X86_INSTRUCTION_VCOMISD,
	NMD_X86_INSTRUCTION_VCOMISS,
	NMD_X86_INSTRUCTION_VCVTSD2SS,
	NMD_X86_INSTRUCTION_VCVTSI2SD,
	NMD_X86_INSTRUCTION_VCVTSI2SS,
	NMD_X86_INSTRUCTION_VCVTSS2SD,
	NMD_X86_INSTRUCTION_VCVTTSD2SI,
	NMD_X86_INSTRUCTION_VCVTTSD2USI,
	NMD_X86_INSTRUCTION_VCVTTSS2SI,
	NMD_X86_INSTRUCTION_VCVTTSS2USI,
	NMD_X86_INSTRUCTION_VCVTUSI2SD,
	NMD_X86_INSTRUCTION_VCVTUSI2SS,
	NMD_X86_INSTRUCTION_VUCOMISD,
	NMD_X86_INSTRUCTION_VUCOMISS,
	NMD_X86_INSTRUCTION_JCXZ,
	NMD_X86_INSTRUCTION_JECXZ,
	NMD_X86_INSTRUCTION_KANDB,
	NMD_X86_INSTRUCTION_KANDD,
	NMD_X86_INSTRUCTION_KANDNB,
	NMD_X86_INSTRUCTION_KANDND,
	NMD_X86_INSTRUCTION_KANDNQ,
	NMD_X86_INSTRUCTION_KANDNW,
	NMD_X86_INSTRUCTION_KANDQ,
	NMD_X86_INSTRUCTION_KANDW,
	NMD_X86_INSTRUCTION_KMOVB,
	NMD_X86_INSTRUCTION_KMOVD,
	NMD_X86_INSTRUCTION_KMOVQ,
	NMD_X86_INSTRUCTION_KMOVW,
	NMD_X86_INSTRUCTION_KNOTB,
	NMD_X86_INSTRUCTION_KNOTD,
	NMD_X86_INSTRUCTION_KNOTQ,
	NMD_X86_INSTRUCTION_KNOTW,
	NMD_X86_INSTRUCTION_KORB,
	NMD_X86_INSTRUCTION_KORD,
	NMD_X86_INSTRUCTION_KORQ,
	NMD_X86_INSTRUCTION_KORTESTB,
	NMD_X86_INSTRUCTION_KORTESTD,
	NMD_X86_INSTRUCTION_KORTESTQ,
	NMD_X86_INSTRUCTION_KORTESTW,
	NMD_X86_INSTRUCTION_KORW,
	NMD_X86_INSTRUCTION_KSHIFTLB,
	NMD_X86_INSTRUCTION_KSHIFTLD,
	NMD_X86_INSTRUCTION_KSHIFTLQ,
	NMD_X86_INSTRUCTION_KSHIFTLW,
	NMD_X86_INSTRUCTION_KSHIFTRB,
	NMD_X86_INSTRUCTION_KSHIFTRD,
	NMD_X86_INSTRUCTION_KSHIFTRQ,
	NMD_X86_INSTRUCTION_KSHIFTRW,
	NMD_X86_INSTRUCTION_KUNPCKBW,
	NMD_X86_INSTRUCTION_KXNORB,
	NMD_X86_INSTRUCTION_KXNORD,
	NMD_X86_INSTRUCTION_KXNORQ,
	NMD_X86_INSTRUCTION_KXNORW,
	NMD_X86_INSTRUCTION_KXORB,
	NMD_X86_INSTRUCTION_KXORD,
	NMD_X86_INSTRUCTION_KXORQ,
	NMD_X86_INSTRUCTION_KXORW,
	NMD_X86_INSTRUCTION_LAHF,
	NMD_X86_INSTRUCTION_LDS,
	NMD_X86_INSTRUCTION_LEA,
	NMD_X86_INSTRUCTION_LEAVE,
	NMD_X86_INSTRUCTION_LES,
	NMD_X86_INSTRUCTION_LODSB,
	NMD_X86_INSTRUCTION_LODSD,
	NMD_X86_INSTRUCTION_LODSQ,
	NMD_X86_INSTRUCTION_LODSW,
	NMD_X86_INSTRUCTION_RETF,
	NMD_X86_INSTRUCTION_XADD,
	NMD_X86_INSTRUCTION_LZCNT,
	NMD_X86_INSTRUCTION_MASKMOVDQU,
	NMD_X86_INSTRUCTION_CVTPD2PI,
	NMD_X86_INSTRUCTION_CVTPI2PD,
	NMD_X86_INSTRUCTION_CVTPI2PS,
	NMD_X86_INSTRUCTION_CVTPS2PI,
	NMD_X86_INSTRUCTION_CVTTPD2PI,
	NMD_X86_INSTRUCTION_CVTTPS2PI,
	NMD_X86_INSTRUCTION_EMMS,
	NMD_X86_INSTRUCTION_MASKMOVQ,
	NMD_X86_INSTRUCTION_MOVD,
	NMD_X86_INSTRUCTION_MOVDQ2Q,
	NMD_X86_INSTRUCTION_MOVNTQ,
	NMD_X86_INSTRUCTION_MOVQ2DQ,
	NMD_X86_INSTRUCTION_MOVQ,
	NMD_X86_INSTRUCTION_PSHUFW,
	NMD_X86_INSTRUCTION_MONTMUL,
	NMD_X86_INSTRUCTION_MOV,
	NMD_X86_INSTRUCTION_MOVABS,
	NMD_X86_INSTRUCTION_MOVBE,
	NMD_X86_INSTRUCTION_MOVDDUP,
	NMD_X86_INSTRUCTION_MOVDQA,
	NMD_X86_INSTRUCTION_MOVDQU,
	NMD_X86_INSTRUCTION_MOVHLPS,
	NMD_X86_INSTRUCTION_MOVHPD,
	NMD_X86_INSTRUCTION_MOVHPS,
	NMD_X86_INSTRUCTION_MOVLHPS,
	NMD_X86_INSTRUCTION_MOVLPD,
	NMD_X86_INSTRUCTION_MOVLPS,
	NMD_X86_INSTRUCTION_MOVNTDQ,
	NMD_X86_INSTRUCTION_MOVNTPD,
	NMD_X86_INSTRUCTION_MOVNTPS,
	NMD_X86_INSTRUCTION_MOVNTSD,
	NMD_X86_INSTRUCTION_MOVNTSS,
	NMD_X86_INSTRUCTION_MOVSB,
	NMD_X86_INSTRUCTION_MOVSD,
	NMD_X86_INSTRUCTION_MOVSHDUP,
	NMD_X86_INSTRUCTION_MOVSLDUP,
	NMD_X86_INSTRUCTION_MOVSQ,
	NMD_X86_INSTRUCTION_MOVSS,
	NMD_X86_INSTRUCTION_MOVSW,
	NMD_X86_INSTRUCTION_MOVSX,
	NMD_X86_INSTRUCTION_MOVSXD,
	NMD_X86_INSTRUCTION_MOVUPD,
	NMD_X86_INSTRUCTION_MOVUPS,
	NMD_X86_INSTRUCTION_MOVZX,
	NMD_X86_INSTRUCTION_MULX,
	NMD_X86_INSTRUCTION_NOP,
	NMD_X86_INSTRUCTION_OUT,
	NMD_X86_INSTRUCTION_OUTSB,
	NMD_X86_INSTRUCTION_OUTSD,
	NMD_X86_INSTRUCTION_OUTSW,
	NMD_X86_INSTRUCTION_PAUSE,
	NMD_X86_INSTRUCTION_PAVGUSB,
	NMD_X86_INSTRUCTION_PBLENDVB,
	NMD_X86_INSTRUCTION_PCOMMIT,
	NMD_X86_INSTRUCTION_PDEP,
	NMD_X86_INSTRUCTION_PEXT,
	NMD_X86_INSTRUCTION_PEXTRB,
	NMD_X86_INSTRUCTION_PEXTRD,
	NMD_X86_INSTRUCTION_PEXTRQ,
	NMD_X86_INSTRUCTION_PF2ID,
	NMD_X86_INSTRUCTION_PF2IW,
	NMD_X86_INSTRUCTION_PFACC,
	NMD_X86_INSTRUCTION_PFADD,
	NMD_X86_INSTRUCTION_PFCMPEQ,
	NMD_X86_INSTRUCTION_PFCMPGE,
	NMD_X86_INSTRUCTION_PFCMPGT,
	NMD_X86_INSTRUCTION_PFMAX,
	NMD_X86_INSTRUCTION_PFMIN,
	NMD_X86_INSTRUCTION_PFMUL,
	NMD_X86_INSTRUCTION_PFNACC,
	NMD_X86_INSTRUCTION_PFPNACC,
	NMD_X86_INSTRUCTION_PFRCPIT1,
	NMD_X86_INSTRUCTION_PFRCPIT2,
	NMD_X86_INSTRUCTION_PFRCP,
	NMD_X86_INSTRUCTION_PFRSQIT1,
	NMD_X86_INSTRUCTION_PFRSQRT,
	NMD_X86_INSTRUCTION_PFSUBR,
	NMD_X86_INSTRUCTION_PFSUB,
	NMD_X86_INSTRUCTION_PHMINPOSUW,
	NMD_X86_INSTRUCTION_PI2FD,
	NMD_X86_INSTRUCTION_PI2FW,
	NMD_X86_INSTRUCTION_PINSRB,
	NMD_X86_INSTRUCTION_PINSRD,
	NMD_X86_INSTRUCTION_PINSRQ,
	NMD_X86_INSTRUCTION_PMULHRW,
	NMD_X86_INSTRUCTION_PMULLD,
	NMD_X86_INSTRUCTION_POP,
	NMD_X86_INSTRUCTION_POPA,
	NMD_X86_INSTRUCTION_POPAD,
	NMD_X86_INSTRUCTION_POPCNT,
	NMD_X86_INSTRUCTION_POPF,
	NMD_X86_INSTRUCTION_POPFD,
	NMD_X86_INSTRUCTION_POPFQ,
	NMD_X86_INSTRUCTION_PREFETCH,
	NMD_X86_INSTRUCTION_PREFETCHNTA,
	NMD_X86_INSTRUCTION_PREFETCHT0,
	NMD_X86_INSTRUCTION_PREFETCHT1,
	NMD_X86_INSTRUCTION_PREFETCHT2,
	NMD_X86_INSTRUCTION_PSHUFD,
	NMD_X86_INSTRUCTION_PSHUFHW,
	NMD_X86_INSTRUCTION_PSHUFLW,
	NMD_X86_INSTRUCTION_PSLLDQ,
	NMD_X86_INSTRUCTION_PSRLDQ,
	NMD_X86_INSTRUCTION_PSWAPD,
	NMD_X86_INSTRUCTION_PTEST,
	NMD_X86_INSTRUCTION_PUSHA,
	NMD_X86_INSTRUCTION_PUSHAD,
	NMD_X86_INSTRUCTION_PUSHF,
	NMD_X86_INSTRUCTION_PUSHFD,
	NMD_X86_INSTRUCTION_PUSHFQ,
	NMD_X86_INSTRUCTION_RDRAND,
	NMD_X86_INSTRUCTION_RDPID,
	NMD_X86_INSTRUCTION_RDSEED,
	NMD_X86_INSTRUCTION_RDTSCP,
	NMD_X86_INSTRUCTION_RORX,
	NMD_X86_INSTRUCTION_RSM,
	NMD_X86_INSTRUCTION_SAHF,
	NMD_X86_INSTRUCTION_SAL,
	NMD_X86_INSTRUCTION_SALC,
	NMD_X86_INSTRUCTION_SARX,
	NMD_X86_INSTRUCTION_SCASB,
	NMD_X86_INSTRUCTION_SCASD,
	NMD_X86_INSTRUCTION_SCASQ,
	NMD_X86_INSTRUCTION_SCASW,
	NMD_X86_INSTRUCTION_SHA1RNDS4,
	NMD_X86_INSTRUCTION_SHLD,
	NMD_X86_INSTRUCTION_SHLX,
	NMD_X86_INSTRUCTION_SHRD,
	NMD_X86_INSTRUCTION_SHRX,
	NMD_X86_INSTRUCTION_SHUFPD,
	NMD_X86_INSTRUCTION_SHUFPS,
	NMD_X86_INSTRUCTION_STOSB,
	NMD_X86_INSTRUCTION_STOSD,
	NMD_X86_INSTRUCTION_STOSQ,
	NMD_X86_INSTRUCTION_STOSW,
	NMD_X86_INSTRUCTION_FSTPNCE,
	NMD_X86_INSTRUCTION_FXCH,
	NMD_X86_INSTRUCTION_SWAPGS,
	NMD_X86_INSTRUCTION_T1MSKC,
	NMD_X86_INSTRUCTION_TZCNT,
	NMD_X86_INSTRUCTION_TZMSK,
	NMD_X86_INSTRUCTION_FUCOMIP,
	NMD_X86_INSTRUCTION_FUCOMPP,
	NMD_X86_INSTRUCTION_FUCOMP,
	NMD_X86_INSTRUCTION_FUCOM,
	NMD_X86_INSTRUCTION_UD1,
	NMD_X86_INSTRUCTION_UNPCKHPD,
	NMD_X86_INSTRUCTION_UNPCKHPS,
	NMD_X86_INSTRUCTION_UNPCKLPD,
	NMD_X86_INSTRUCTION_UNPCKLPS,
	NMD_X86_INSTRUCTION_VADDPD,
	NMD_X86_INSTRUCTION_VADDPS,
	NMD_X86_INSTRUCTION_VADDSD,
	NMD_X86_INSTRUCTION_VADDSS,
	NMD_X86_INSTRUCTION_VADDSUBPD,
	NMD_X86_INSTRUCTION_VADDSUBPS,
	NMD_X86_INSTRUCTION_VAESDECLAST,
	NMD_X86_INSTRUCTION_VAESDEC,
	NMD_X86_INSTRUCTION_VAESENCLAST,
	NMD_X86_INSTRUCTION_VAESENC,
	NMD_X86_INSTRUCTION_VAESIMC,
	NMD_X86_INSTRUCTION_VAESKEYGENASSIST,
	NMD_X86_INSTRUCTION_VALIGND,
	NMD_X86_INSTRUCTION_VALIGNQ,
	NMD_X86_INSTRUCTION_VANDNPD,
	NMD_X86_INSTRUCTION_VANDNPS,
	NMD_X86_INSTRUCTION_VANDPD,
	NMD_X86_INSTRUCTION_VANDPS,
	NMD_X86_INSTRUCTION_VBLENDMPD,
	NMD_X86_INSTRUCTION_VBLENDMPS,
	NMD_X86_INSTRUCTION_VBLENDPD,
	NMD_X86_INSTRUCTION_VBLENDPS,
	NMD_X86_INSTRUCTION_VBLENDVPD,
	NMD_X86_INSTRUCTION_VBLENDVPS,
	NMD_X86_INSTRUCTION_VBROADCASTF128,
	NMD_X86_INSTRUCTION_VBROADCASTI32X4,
	NMD_X86_INSTRUCTION_VBROADCASTI64X4,
	NMD_X86_INSTRUCTION_VBROADCASTSD,
	NMD_X86_INSTRUCTION_VBROADCASTSS,
	NMD_X86_INSTRUCTION_VCOMPRESSPD,
	NMD_X86_INSTRUCTION_VCOMPRESSPS,
	NMD_X86_INSTRUCTION_VCVTDQ2PD,
	NMD_X86_INSTRUCTION_VCVTDQ2PS,
	NMD_X86_INSTRUCTION_VCVTPD2DQX,
	NMD_X86_INSTRUCTION_VCVTPD2DQ,
	NMD_X86_INSTRUCTION_VCVTPD2PSX,
	NMD_X86_INSTRUCTION_VCVTPD2PS,
	NMD_X86_INSTRUCTION_VCVTPD2UDQ,
	NMD_X86_INSTRUCTION_VCVTPH2PS,
	NMD_X86_INSTRUCTION_VCVTPS2DQ,
	NMD_X86_INSTRUCTION_VCVTPS2PD,
	NMD_X86_INSTRUCTION_VCVTPS2PH,
	NMD_X86_INSTRUCTION_VCVTPS2UDQ,
	NMD_X86_INSTRUCTION_VCVTSD2SI,
	NMD_X86_INSTRUCTION_VCVTSD2USI,
	NMD_X86_INSTRUCTION_VCVTSS2SI,
	NMD_X86_INSTRUCTION_VCVTSS2USI,
	NMD_X86_INSTRUCTION_VCVTTPD2DQX,
	NMD_X86_INSTRUCTION_VCVTTPD2DQ,
	NMD_X86_INSTRUCTION_VCVTTPD2UDQ,
	NMD_X86_INSTRUCTION_VCVTTPS2DQ,
	NMD_X86_INSTRUCTION_VCVTTPS2UDQ,
	NMD_X86_INSTRUCTION_VCVTUDQ2PD,
	NMD_X86_INSTRUCTION_VCVTUDQ2PS,
	NMD_X86_INSTRUCTION_VDIVPD,
	NMD_X86_INSTRUCTION_VDIVPS,
	NMD_X86_INSTRUCTION_VDIVSD,
	NMD_X86_INSTRUCTION_VDIVSS,
	NMD_X86_INSTRUCTION_VDPPD,
	NMD_X86_INSTRUCTION_VDPPS,
	NMD_X86_INSTRUCTION_VEXP2PD,
	NMD_X86_INSTRUCTION_VEXP2PS,
	NMD_X86_INSTRUCTION_VEXPANDPD,
	NMD_X86_INSTRUCTION_VEXPANDPS,
	NMD_X86_INSTRUCTION_VEXTRACTF128,
	NMD_X86_INSTRUCTION_VEXTRACTF32X4,
	NMD_X86_INSTRUCTION_VEXTRACTF64X4,
	NMD_X86_INSTRUCTION_VEXTRACTI128,
	NMD_X86_INSTRUCTION_VEXTRACTI32X4,
	NMD_X86_INSTRUCTION_VEXTRACTI64X4,
	NMD_X86_INSTRUCTION_VEXTRACTPS,
	NMD_X86_INSTRUCTION_VFMADD132PD,
	NMD_X86_INSTRUCTION_VFMADD132PS,
	NMD_X86_INSTRUCTION_VFMADDPD,
	NMD_X86_INSTRUCTION_VFMADD213PD,
	NMD_X86_INSTRUCTION_VFMADD231PD,
	NMD_X86_INSTRUCTION_VFMADDPS,
	NMD_X86_INSTRUCTION_VFMADD213PS,
	NMD_X86_INSTRUCTION_VFMADD231PS,
	NMD_X86_INSTRUCTION_VFMADDSD,
	NMD_X86_INSTRUCTION_VFMADD213SD,
	NMD_X86_INSTRUCTION_VFMADD132SD,
	NMD_X86_INSTRUCTION_VFMADD231SD,
	NMD_X86_INSTRUCTION_VFMADDSS,
	NMD_X86_INSTRUCTION_VFMADD213SS,
	NMD_X86_INSTRUCTION_VFMADD132SS,
	NMD_X86_INSTRUCTION_VFMADD231SS,
	NMD_X86_INSTRUCTION_VFMADDSUB132PD,
	NMD_X86_INSTRUCTION_VFMADDSUB132PS,
	NMD_X86_INSTRUCTION_VFMADDSUBPD,
	NMD_X86_INSTRUCTION_VFMADDSUB213PD,
	NMD_X86_INSTRUCTION_VFMADDSUB231PD,
	NMD_X86_INSTRUCTION_VFMADDSUBPS,
	NMD_X86_INSTRUCTION_VFMADDSUB213PS,
	NMD_X86_INSTRUCTION_VFMADDSUB231PS,
	NMD_X86_INSTRUCTION_VFMSUB132PD,
	NMD_X86_INSTRUCTION_VFMSUB132PS,
	NMD_X86_INSTRUCTION_VFMSUBADD132PD,
	NMD_X86_INSTRUCTION_VFMSUBADD132PS,
	NMD_X86_INSTRUCTION_VFMSUBADDPD,
	NMD_X86_INSTRUCTION_VFMSUBADD213PD,
	NMD_X86_INSTRUCTION_VFMSUBADD231PD,
	NMD_X86_INSTRUCTION_VFMSUBADDPS,
	NMD_X86_INSTRUCTION_VFMSUBADD213PS,
	NMD_X86_INSTRUCTION_VFMSUBADD231PS,
	NMD_X86_INSTRUCTION_VFMSUBPD,
	NMD_X86_INSTRUCTION_VFMSUB213PD,
	NMD_X86_INSTRUCTION_VFMSUB231PD,
	NMD_X86_INSTRUCTION_VFMSUBPS,
	NMD_X86_INSTRUCTION_VFMSUB213PS,
	NMD_X86_INSTRUCTION_VFMSUB231PS,
	NMD_X86_INSTRUCTION_VFMSUBSD,
	NMD_X86_INSTRUCTION_VFMSUB213SD,
	NMD_X86_INSTRUCTION_VFMSUB132SD,
	NMD_X86_INSTRUCTION_VFMSUB231SD,
	NMD_X86_INSTRUCTION_VFMSUBSS,
	NMD_X86_INSTRUCTION_VFMSUB213SS,
	NMD_X86_INSTRUCTION_VFMSUB132SS,
	NMD_X86_INSTRUCTION_VFMSUB231SS,
	NMD_X86_INSTRUCTION_VFNMADD132PD,
	NMD_X86_INSTRUCTION_VFNMADD132PS,
	NMD_X86_INSTRUCTION_VFNMADDPD,
	NMD_X86_INSTRUCTION_VFNMADD213PD,
	NMD_X86_INSTRUCTION_VFNMADD231PD,
	NMD_X86_INSTRUCTION_VFNMADDPS,
	NMD_X86_INSTRUCTION_VFNMADD213PS,
	NMD_X86_INSTRUCTION_VFNMADD231PS,
	NMD_X86_INSTRUCTION_VFNMADDSD,
	NMD_X86_INSTRUCTION_VFNMADD213SD,
	NMD_X86_INSTRUCTION_VFNMADD132SD,
	NMD_X86_INSTRUCTION_VFNMADD231SD,
	NMD_X86_INSTRUCTION_VFNMADDSS,
	NMD_X86_INSTRUCTION_VFNMADD213SS,
	NMD_X86_INSTRUCTION_VFNMADD132SS,
	NMD_X86_INSTRUCTION_VFNMADD231SS,
	NMD_X86_INSTRUCTION_VFNMSUB132PD,
	NMD_X86_INSTRUCTION_VFNMSUB132PS,
	NMD_X86_INSTRUCTION_VFNMSUBPD,
	NMD_X86_INSTRUCTION_VFNMSUB213PD,
	NMD_X86_INSTRUCTION_VFNMSUB231PD,
	NMD_X86_INSTRUCTION_VFNMSUBPS,
	NMD_X86_INSTRUCTION_VFNMSUB213PS,
	NMD_X86_INSTRUCTION_VFNMSUB231PS,
	NMD_X86_INSTRUCTION_VFNMSUBSD,
	NMD_X86_INSTRUCTION_VFNMSUB213SD,
	NMD_X86_INSTRUCTION_VFNMSUB132SD,
	NMD_X86_INSTRUCTION_VFNMSUB231SD,
	NMD_X86_INSTRUCTION_VFNMSUBSS,
	NMD_X86_INSTRUCTION_VFNMSUB213SS,
	NMD_X86_INSTRUCTION_VFNMSUB132SS,
	NMD_X86_INSTRUCTION_VFNMSUB231SS,
	NMD_X86_INSTRUCTION_VFRCZPD,
	NMD_X86_INSTRUCTION_VFRCZPS,
	NMD_X86_INSTRUCTION_VFRCZSD,
	NMD_X86_INSTRUCTION_VFRCZSS,
	NMD_X86_INSTRUCTION_VORPD,
	NMD_X86_INSTRUCTION_VORPS,
	NMD_X86_INSTRUCTION_VXORPD,
	NMD_X86_INSTRUCTION_VXORPS,
	NMD_X86_INSTRUCTION_VGATHERDPD,
	NMD_X86_INSTRUCTION_VGATHERDPS,
	NMD_X86_INSTRUCTION_VGATHERPF0DPD,
	NMD_X86_INSTRUCTION_VGATHERPF0DPS,
	NMD_X86_INSTRUCTION_VGATHERPF0QPD,
	NMD_X86_INSTRUCTION_VGATHERPF0QPS,
	NMD_X86_INSTRUCTION_VGATHERPF1DPD,
	NMD_X86_INSTRUCTION_VGATHERPF1DPS,
	NMD_X86_INSTRUCTION_VGATHERPF1QPD,
	NMD_X86_INSTRUCTION_VGATHERPF1QPS,
	NMD_X86_INSTRUCTION_VGATHERQPD,
	NMD_X86_INSTRUCTION_VGATHERQPS,
	NMD_X86_INSTRUCTION_VHADDPD,
	NMD_X86_INSTRUCTION_VHADDPS,
	NMD_X86_INSTRUCTION_VHSUBPD,
	NMD_X86_INSTRUCTION_VHSUBPS,
	NMD_X86_INSTRUCTION_VINSERTF128,
	NMD_X86_INSTRUCTION_VINSERTF32X4,
	NMD_X86_INSTRUCTION_VINSERTF32X8,
	NMD_X86_INSTRUCTION_VINSERTF64X2,
	NMD_X86_INSTRUCTION_VINSERTF64X4,
	NMD_X86_INSTRUCTION_VINSERTI128,
	NMD_X86_INSTRUCTION_VINSERTI32X4,
	NMD_X86_INSTRUCTION_VINSERTI32X8,
	NMD_X86_INSTRUCTION_VINSERTI64X2,
	NMD_X86_INSTRUCTION_VINSERTI64X4,
	NMD_X86_INSTRUCTION_VINSERTPS,
	NMD_X86_INSTRUCTION_VLDDQU,
	NMD_X86_INSTRUCTION_VLDMXCSR,
	NMD_X86_INSTRUCTION_VMASKMOVDQU,
	NMD_X86_INSTRUCTION_VMASKMOVPD,
	NMD_X86_INSTRUCTION_VMASKMOVPS,
	NMD_X86_INSTRUCTION_VMAXPD,
	NMD_X86_INSTRUCTION_VMAXPS,
	NMD_X86_INSTRUCTION_VMAXSD,
	NMD_X86_INSTRUCTION_VMAXSS,
	NMD_X86_INSTRUCTION_VMCLEAR,
	NMD_X86_INSTRUCTION_VMINPD,
	NMD_X86_INSTRUCTION_VMINPS,
	NMD_X86_INSTRUCTION_VMINSD,
	NMD_X86_INSTRUCTION_VMINSS,
	NMD_X86_INSTRUCTION_VMOVQ,
	NMD_X86_INSTRUCTION_VMOVDDUP,
	NMD_X86_INSTRUCTION_VMOVD,
	NMD_X86_INSTRUCTION_VMOVDQA32,
	NMD_X86_INSTRUCTION_VMOVDQA64,
	NMD_X86_INSTRUCTION_VMOVDQA,
	NMD_X86_INSTRUCTION_VMOVDQU16,
	NMD_X86_INSTRUCTION_VMOVDQU32,
	NMD_X86_INSTRUCTION_VMOVDQU64,
	NMD_X86_INSTRUCTION_VMOVDQU8,
	NMD_X86_INSTRUCTION_VMOVDQU,
	NMD_X86_INSTRUCTION_VMOVHLPS,
	NMD_X86_INSTRUCTION_VMOVHPD,
	NMD_X86_INSTRUCTION_VMOVHPS,
	NMD_X86_INSTRUCTION_VMOVLHPS,
	NMD_X86_INSTRUCTION_VMOVLPD,
	NMD_X86_INSTRUCTION_VMOVLPS,
	NMD_X86_INSTRUCTION_VMOVMSKPD,
	NMD_X86_INSTRUCTION_VMOVMSKPS,
	NMD_X86_INSTRUCTION_VMOVNTDQA,
	NMD_X86_INSTRUCTION_VMOVNTDQ,
	NMD_X86_INSTRUCTION_VMOVNTPD,
	NMD_X86_INSTRUCTION_VMOVNTPS,
	NMD_X86_INSTRUCTION_VMOVSD,
	NMD_X86_INSTRUCTION_VMOVSHDUP,
	NMD_X86_INSTRUCTION_VMOVSLDUP,
	NMD_X86_INSTRUCTION_VMOVSS,
	NMD_X86_INSTRUCTION_VMOVUPD,
	NMD_X86_INSTRUCTION_VMOVUPS,
	NMD_X86_INSTRUCTION_VMPSADBW,
	NMD_X86_INSTRUCTION_VMPTRLD,
	NMD_X86_INSTRUCTION_VMPTRST,
	NMD_X86_INSTRUCTION_VMREAD,
	NMD_X86_INSTRUCTION_VMULPD,
	NMD_X86_INSTRUCTION_VMULPS,
	NMD_X86_INSTRUCTION_VMULSD,
	NMD_X86_INSTRUCTION_VMULSS,
	NMD_X86_INSTRUCTION_VMWRITE,
	NMD_X86_INSTRUCTION_VMXON,
	NMD_X86_INSTRUCTION_VPABSB,
	NMD_X86_INSTRUCTION_VPABSD,
	NMD_X86_INSTRUCTION_VPABSQ,
	NMD_X86_INSTRUCTION_VPABSW,
	NMD_X86_INSTRUCTION_VPACKSSDW,
	NMD_X86_INSTRUCTION_VPACKSSWB,
	NMD_X86_INSTRUCTION_VPACKUSDW,
	NMD_X86_INSTRUCTION_VPACKUSWB,
	NMD_X86_INSTRUCTION_VPADDB,
	NMD_X86_INSTRUCTION_VPADDD,
	NMD_X86_INSTRUCTION_VPADDQ,
	NMD_X86_INSTRUCTION_VPADDSB,
	NMD_X86_INSTRUCTION_VPADDSW,
	NMD_X86_INSTRUCTION_VPADDUSB,
	NMD_X86_INSTRUCTION_VPADDUSW,
	NMD_X86_INSTRUCTION_VPADDW,
	NMD_X86_INSTRUCTION_VPALIGNR,
	NMD_X86_INSTRUCTION_VPANDD,
	NMD_X86_INSTRUCTION_VPANDND,
	NMD_X86_INSTRUCTION_VPANDNQ,
	NMD_X86_INSTRUCTION_VPANDN,
	NMD_X86_INSTRUCTION_VPANDQ,
	NMD_X86_INSTRUCTION_VPAND,
	NMD_X86_INSTRUCTION_VPAVGB,
	NMD_X86_INSTRUCTION_VPAVGW,
	NMD_X86_INSTRUCTION_VPBLENDD,
	NMD_X86_INSTRUCTION_VPBLENDMB,
	NMD_X86_INSTRUCTION_VPBLENDMD,
	NMD_X86_INSTRUCTION_VPBLENDMQ,
	NMD_X86_INSTRUCTION_VPBLENDMW,
	NMD_X86_INSTRUCTION_VPBLENDVB,
	NMD_X86_INSTRUCTION_VPBLENDW,
	NMD_X86_INSTRUCTION_VPBROADCASTB,
	NMD_X86_INSTRUCTION_VPBROADCASTD,
	NMD_X86_INSTRUCTION_VPBROADCASTMB2Q,
	NMD_X86_INSTRUCTION_VPBROADCASTMW2D,
	NMD_X86_INSTRUCTION_VPBROADCASTQ,
	NMD_X86_INSTRUCTION_VPBROADCASTW,
	NMD_X86_INSTRUCTION_VPCLMULQDQ,
	NMD_X86_INSTRUCTION_VPCMOV,
	NMD_X86_INSTRUCTION_VPCMPB,
	NMD_X86_INSTRUCTION_VPCMPD,
	NMD_X86_INSTRUCTION_VPCMPEQB,
	NMD_X86_INSTRUCTION_VPCMPEQD,
	NMD_X86_INSTRUCTION_VPCMPEQQ,
	NMD_X86_INSTRUCTION_VPCMPEQW,
	NMD_X86_INSTRUCTION_VPCMPESTRI,
	NMD_X86_INSTRUCTION_VPCMPESTRM,
	NMD_X86_INSTRUCTION_VPCMPGTB,
	NMD_X86_INSTRUCTION_VPCMPGTD,
	NMD_X86_INSTRUCTION_VPCMPGTW,
	NMD_X86_INSTRUCTION_VPCMPISTRI,
	NMD_X86_INSTRUCTION_VPCMPISTRM,
	NMD_X86_INSTRUCTION_VPCMPQ,
	NMD_X86_INSTRUCTION_VPCMPUB,
	NMD_X86_INSTRUCTION_VPCMPUD,
	NMD_X86_INSTRUCTION_VPCMPUQ,
	NMD_X86_INSTRUCTION_VPCMPUW,
	NMD_X86_INSTRUCTION_VPCMPW,
	NMD_X86_INSTRUCTION_VPCOMB,
	NMD_X86_INSTRUCTION_VPCOMD,
	NMD_X86_INSTRUCTION_VPCOMPRESSD,
	NMD_X86_INSTRUCTION_VPCOMPRESSQ,
	NMD_X86_INSTRUCTION_VPCOMQ,
	NMD_X86_INSTRUCTION_VPCOMUB,
	NMD_X86_INSTRUCTION_VPCOMUD,
	NMD_X86_INSTRUCTION_VPCOMUQ,
	NMD_X86_INSTRUCTION_VPCOMUW,
	NMD_X86_INSTRUCTION_VPCOMW,
	NMD_X86_INSTRUCTION_VPCONFLICTD,
	NMD_X86_INSTRUCTION_VPCONFLICTQ,
	NMD_X86_INSTRUCTION_VPERM2F128,
	NMD_X86_INSTRUCTION_VPERM2I128,
	NMD_X86_INSTRUCTION_VPERMD,
	NMD_X86_INSTRUCTION_VPERMI2D,
	NMD_X86_INSTRUCTION_VPERMI2PD,
	NMD_X86_INSTRUCTION_VPERMI2PS,
	NMD_X86_INSTRUCTION_VPERMI2Q,
	NMD_X86_INSTRUCTION_VPERMIL2PD,
	NMD_X86_INSTRUCTION_VPERMIL2PS,
	NMD_X86_INSTRUCTION_VPERMILPD,
	NMD_X86_INSTRUCTION_VPERMILPS,
	NMD_X86_INSTRUCTION_VPERMPD,
	NMD_X86_INSTRUCTION_VPERMPS,
	NMD_X86_INSTRUCTION_VPERMQ,
	NMD_X86_INSTRUCTION_VPERMT2D,
	NMD_X86_INSTRUCTION_VPERMT2PD,
	NMD_X86_INSTRUCTION_VPERMT2PS,
	NMD_X86_INSTRUCTION_VPERMT2Q,
	NMD_X86_INSTRUCTION_VPEXPANDD,
	NMD_X86_INSTRUCTION_VPEXPANDQ,
	NMD_X86_INSTRUCTION_VPEXTRB,
	NMD_X86_INSTRUCTION_VPEXTRD,
	NMD_X86_INSTRUCTION_VPEXTRQ,
	NMD_X86_INSTRUCTION_VPEXTRW,
	NMD_X86_INSTRUCTION_VPGATHERDD,
	NMD_X86_INSTRUCTION_VPGATHERDQ,
	NMD_X86_INSTRUCTION_VPGATHERQD,
	NMD_X86_INSTRUCTION_VPGATHERQQ,
	NMD_X86_INSTRUCTION_VPHADDBD,
	NMD_X86_INSTRUCTION_VPHADDBQ,
	NMD_X86_INSTRUCTION_VPHADDBW,
	NMD_X86_INSTRUCTION_VPHADDUBD,
	NMD_X86_INSTRUCTION_VPHADDUBQ,
	NMD_X86_INSTRUCTION_VPHADDUBW,
	NMD_X86_INSTRUCTION_VPHADDUDQ,
	NMD_X86_INSTRUCTION_VPHADDUWD,
	NMD_X86_INSTRUCTION_VPHADDUWQ,
	NMD_X86_INSTRUCTION_VPHADDWD,
	NMD_X86_INSTRUCTION_VPHMINPOSUW,
	NMD_X86_INSTRUCTION_VPHSUBBW,
	NMD_X86_INSTRUCTION_VPHSUBDQ,

	NMD_X86_INSTRUCTION_VPHSUBWD,

	NMD_X86_INSTRUCTION_VPINSRB,
	NMD_X86_INSTRUCTION_VPINSRD,
	NMD_X86_INSTRUCTION_VPINSRQ,
	NMD_X86_INSTRUCTION_VPINSRW,
	NMD_X86_INSTRUCTION_VPLZCNTD,
	NMD_X86_INSTRUCTION_VPLZCNTQ,
	NMD_X86_INSTRUCTION_VPMACSDD,
	NMD_X86_INSTRUCTION_VPMACSDQH,
	NMD_X86_INSTRUCTION_VPMACSDQL,
	NMD_X86_INSTRUCTION_VPMACSSDD,
	NMD_X86_INSTRUCTION_VPMACSSDQH,
	NMD_X86_INSTRUCTION_VPMACSSDQL,
	NMD_X86_INSTRUCTION_VPMACSSWD,
	NMD_X86_INSTRUCTION_VPMACSSWW,
	NMD_X86_INSTRUCTION_VPMACSWD,
	NMD_X86_INSTRUCTION_VPMACSWW,
	NMD_X86_INSTRUCTION_VPMADCSSWD,
	NMD_X86_INSTRUCTION_VPMADCSWD,
	NMD_X86_INSTRUCTION_VPMADDWD,
	NMD_X86_INSTRUCTION_VPMASKMOVD,
	NMD_X86_INSTRUCTION_VPMASKMOVQ,
	NMD_X86_INSTRUCTION_VPMAXSB,
	NMD_X86_INSTRUCTION_VPMAXSD,
	NMD_X86_INSTRUCTION_VPMAXSQ,
	NMD_X86_INSTRUCTION_VPMAXSW,
	NMD_X86_INSTRUCTION_VPMAXUB,
	NMD_X86_INSTRUCTION_VPMAXUD,
	NMD_X86_INSTRUCTION_VPMAXUQ,
	NMD_X86_INSTRUCTION_VPMAXUW,
	NMD_X86_INSTRUCTION_VPMINSB,
	NMD_X86_INSTRUCTION_VPMINSD,
	NMD_X86_INSTRUCTION_VPMINSQ,
	NMD_X86_INSTRUCTION_VPMINSW,
	NMD_X86_INSTRUCTION_VPMINUB,
	NMD_X86_INSTRUCTION_VPMINUD,
	NMD_X86_INSTRUCTION_VPMINUQ,
	NMD_X86_INSTRUCTION_VPMINUW,
	NMD_X86_INSTRUCTION_VPMOVDB,
	NMD_X86_INSTRUCTION_VPMOVDW,
	NMD_X86_INSTRUCTION_VPMOVM2B,
	NMD_X86_INSTRUCTION_VPMOVM2D,
	NMD_X86_INSTRUCTION_VPMOVM2Q,
	NMD_X86_INSTRUCTION_VPMOVM2W,
	NMD_X86_INSTRUCTION_VPMOVMSKB,
	NMD_X86_INSTRUCTION_VPMOVQB,
	NMD_X86_INSTRUCTION_VPMOVQD,
	NMD_X86_INSTRUCTION_VPMOVQW,
	NMD_X86_INSTRUCTION_VPMOVSDB,
	NMD_X86_INSTRUCTION_VPMOVSDW,
	NMD_X86_INSTRUCTION_VPMOVSQB,
	NMD_X86_INSTRUCTION_VPMOVSQD,
	NMD_X86_INSTRUCTION_VPMOVSQW,
	NMD_X86_INSTRUCTION_VPMOVSXBD,
	NMD_X86_INSTRUCTION_VPMOVSXBQ,
	NMD_X86_INSTRUCTION_VPMOVSXBW,
	NMD_X86_INSTRUCTION_VPMOVSXDQ,
	NMD_X86_INSTRUCTION_VPMOVSXWD,
	NMD_X86_INSTRUCTION_VPMOVSXWQ,
	NMD_X86_INSTRUCTION_VPMOVUSDB,
	NMD_X86_INSTRUCTION_VPMOVUSDW,
	NMD_X86_INSTRUCTION_VPMOVUSQB,
	NMD_X86_INSTRUCTION_VPMOVUSQD,
	NMD_X86_INSTRUCTION_VPMOVUSQW,
	NMD_X86_INSTRUCTION_VPMOVZXBD,
	NMD_X86_INSTRUCTION_VPMOVZXBQ,
	NMD_X86_INSTRUCTION_VPMOVZXBW,
	NMD_X86_INSTRUCTION_VPMOVZXDQ,
	NMD_X86_INSTRUCTION_VPMOVZXWD,
	NMD_X86_INSTRUCTION_VPMOVZXWQ,
	NMD_X86_INSTRUCTION_VPMULDQ,
	NMD_X86_INSTRUCTION_VPMULHUW,
	NMD_X86_INSTRUCTION_VPMULHW,
	NMD_X86_INSTRUCTION_VPMULLD,
	NMD_X86_INSTRUCTION_VPMULLQ,
	NMD_X86_INSTRUCTION_VPMULLW,
	NMD_X86_INSTRUCTION_VPMULUDQ,
	NMD_X86_INSTRUCTION_VPORD,
	NMD_X86_INSTRUCTION_VPORQ,
	NMD_X86_INSTRUCTION_VPOR,
	NMD_X86_INSTRUCTION_VPPERM,
	NMD_X86_INSTRUCTION_VPROTB,
	NMD_X86_INSTRUCTION_VPROTD,
	NMD_X86_INSTRUCTION_VPROTQ,
	NMD_X86_INSTRUCTION_VPROTW,
	NMD_X86_INSTRUCTION_VPSADBW,
	NMD_X86_INSTRUCTION_VPSCATTERDD,
	NMD_X86_INSTRUCTION_VPSCATTERDQ,
	NMD_X86_INSTRUCTION_VPSCATTERQD,
	NMD_X86_INSTRUCTION_VPSCATTERQQ,
	NMD_X86_INSTRUCTION_VPSHAB,
	NMD_X86_INSTRUCTION_VPSHAD,
	NMD_X86_INSTRUCTION_VPSHAQ,
	NMD_X86_INSTRUCTION_VPSHAW,
	NMD_X86_INSTRUCTION_VPSHLB,
	NMD_X86_INSTRUCTION_VPSHLD,
	NMD_X86_INSTRUCTION_VPSHLQ,
	NMD_X86_INSTRUCTION_VPSHLW,
	NMD_X86_INSTRUCTION_VPSHUFD,
	NMD_X86_INSTRUCTION_VPSHUFHW,
	NMD_X86_INSTRUCTION_VPSHUFLW,
	NMD_X86_INSTRUCTION_VPSLLDQ,
	NMD_X86_INSTRUCTION_VPSLLD,
	NMD_X86_INSTRUCTION_VPSLLQ,
	NMD_X86_INSTRUCTION_VPSLLVD,
	NMD_X86_INSTRUCTION_VPSLLVQ,
	NMD_X86_INSTRUCTION_VPSLLW,
	NMD_X86_INSTRUCTION_VPSRAD,
	NMD_X86_INSTRUCTION_VPSRAQ,
	NMD_X86_INSTRUCTION_VPSRAVD,
	NMD_X86_INSTRUCTION_VPSRAVQ,
	NMD_X86_INSTRUCTION_VPSRAW,
	NMD_X86_INSTRUCTION_VPSRLDQ,
	NMD_X86_INSTRUCTION_VPSRLD,
	NMD_X86_INSTRUCTION_VPSRLQ,
	NMD_X86_INSTRUCTION_VPSRLVD,
	NMD_X86_INSTRUCTION_VPSRLVQ,
	NMD_X86_INSTRUCTION_VPSRLW,
	NMD_X86_INSTRUCTION_VPSUBB,
	NMD_X86_INSTRUCTION_VPSUBD,
	NMD_X86_INSTRUCTION_VPSUBQ,
	NMD_X86_INSTRUCTION_VPSUBSB,
	NMD_X86_INSTRUCTION_VPSUBSW,
	NMD_X86_INSTRUCTION_VPSUBUSB,
	NMD_X86_INSTRUCTION_VPSUBUSW,
	NMD_X86_INSTRUCTION_VPSUBW,
	NMD_X86_INSTRUCTION_VPTESTMD,
	NMD_X86_INSTRUCTION_VPTESTMQ,
	NMD_X86_INSTRUCTION_VPTESTNMD,
	NMD_X86_INSTRUCTION_VPTESTNMQ,
	NMD_X86_INSTRUCTION_VPTEST,
	NMD_X86_INSTRUCTION_VPUNPCKHBW,
	NMD_X86_INSTRUCTION_VPUNPCKHDQ,
	NMD_X86_INSTRUCTION_VPUNPCKHQDQ,
	NMD_X86_INSTRUCTION_VPUNPCKHWD,
	NMD_X86_INSTRUCTION_VPUNPCKLBW,
	NMD_X86_INSTRUCTION_VPUNPCKLDQ,
	NMD_X86_INSTRUCTION_VPUNPCKLQDQ,
	NMD_X86_INSTRUCTION_VPUNPCKLWD,
	NMD_X86_INSTRUCTION_VPXORD,
	NMD_X86_INSTRUCTION_VPXORQ,
	NMD_X86_INSTRUCTION_VPXOR,
	NMD_X86_INSTRUCTION_VRCP14PD,
	NMD_X86_INSTRUCTION_VRCP14PS,
	NMD_X86_INSTRUCTION_VRCP14SD,
	NMD_X86_INSTRUCTION_VRCP14SS,
	NMD_X86_INSTRUCTION_VRCP28PD,
	NMD_X86_INSTRUCTION_VRCP28PS,
	NMD_X86_INSTRUCTION_VRCP28SD,
	NMD_X86_INSTRUCTION_VRCP28SS,
	NMD_X86_INSTRUCTION_VRCPPS,
	NMD_X86_INSTRUCTION_VRCPSS,
	NMD_X86_INSTRUCTION_VRNDSCALEPD,
	NMD_X86_INSTRUCTION_VRNDSCALEPS,
	NMD_X86_INSTRUCTION_VRNDSCALESD,
	NMD_X86_INSTRUCTION_VRNDSCALESS,
	NMD_X86_INSTRUCTION_VROUNDPD,
	NMD_X86_INSTRUCTION_VROUNDPS,
	NMD_X86_INSTRUCTION_VROUNDSD,
	NMD_X86_INSTRUCTION_VROUNDSS,
	NMD_X86_INSTRUCTION_VRSQRT14PD,
	NMD_X86_INSTRUCTION_VRSQRT14PS,
	NMD_X86_INSTRUCTION_VRSQRT14SD,
	NMD_X86_INSTRUCTION_VRSQRT14SS,
	NMD_X86_INSTRUCTION_VRSQRT28PD,
	NMD_X86_INSTRUCTION_VRSQRT28PS,
	NMD_X86_INSTRUCTION_VRSQRT28SD,
	NMD_X86_INSTRUCTION_VRSQRT28SS,
	NMD_X86_INSTRUCTION_VRSQRTPS,
	NMD_X86_INSTRUCTION_VRSQRTSS,
	NMD_X86_INSTRUCTION_VSCATTERDPD,
	NMD_X86_INSTRUCTION_VSCATTERDPS,
	NMD_X86_INSTRUCTION_VSCATTERPF0DPD,
	NMD_X86_INSTRUCTION_VSCATTERPF0DPS,
	NMD_X86_INSTRUCTION_VSCATTERPF0QPD,
	NMD_X86_INSTRUCTION_VSCATTERPF0QPS,
	NMD_X86_INSTRUCTION_VSCATTERPF1DPD,
	NMD_X86_INSTRUCTION_VSCATTERPF1DPS,
	NMD_X86_INSTRUCTION_VSCATTERPF1QPD,
	NMD_X86_INSTRUCTION_VSCATTERPF1QPS,
	NMD_X86_INSTRUCTION_VSCATTERQPD,
	NMD_X86_INSTRUCTION_VSCATTERQPS,
	NMD_X86_INSTRUCTION_VSHUFPD,
	NMD_X86_INSTRUCTION_VSHUFPS,
	NMD_X86_INSTRUCTION_VSQRTPD,
	NMD_X86_INSTRUCTION_VSQRTPS,
	NMD_X86_INSTRUCTION_VSQRTSD,
	NMD_X86_INSTRUCTION_VSQRTSS,
	NMD_X86_INSTRUCTION_VSTMXCSR,
	NMD_X86_INSTRUCTION_VSUBPD,
	NMD_X86_INSTRUCTION_VSUBPS,
	NMD_X86_INSTRUCTION_VSUBSD,
	NMD_X86_INSTRUCTION_VSUBSS,
	NMD_X86_INSTRUCTION_VTESTPD,
	NMD_X86_INSTRUCTION_VTESTPS,
	NMD_X86_INSTRUCTION_VUNPCKHPD,
	NMD_X86_INSTRUCTION_VUNPCKHPS,
	NMD_X86_INSTRUCTION_VUNPCKLPD,
	NMD_X86_INSTRUCTION_VUNPCKLPS,
	NMD_X86_INSTRUCTION_VZEROALL,
	NMD_X86_INSTRUCTION_VZEROUPPER,
	NMD_X86_INSTRUCTION_FWAIT,
	NMD_X86_INSTRUCTION_XABORT,
	NMD_X86_INSTRUCTION_XACQUIRE,
	NMD_X86_INSTRUCTION_XBEGIN,
	NMD_X86_INSTRUCTION_XCHG,
	NMD_X86_INSTRUCTION_XCRYPTCBC,
	NMD_X86_INSTRUCTION_XCRYPTCFB,
	NMD_X86_INSTRUCTION_XCRYPTCTR,
	NMD_X86_INSTRUCTION_XCRYPTECB,
	NMD_X86_INSTRUCTION_XCRYPTOFB,
	NMD_X86_INSTRUCTION_XRELEASE,
	NMD_X86_INSTRUCTION_XRSTOR64,
	NMD_X86_INSTRUCTION_XRSTORS,
	NMD_X86_INSTRUCTION_XRSTORS64,
	NMD_X86_INSTRUCTION_XSAVE64,
	NMD_X86_INSTRUCTION_XSAVEC,
	NMD_X86_INSTRUCTION_XSAVEC64,
	NMD_X86_INSTRUCTION_XSAVEOPT64,
	NMD_X86_INSTRUCTION_XSAVES,
	NMD_X86_INSTRUCTION_XSAVES64,
	NMD_X86_INSTRUCTION_XSHA1,
	NMD_X86_INSTRUCTION_XSHA256,
	NMD_X86_INSTRUCTION_XSTORE,
	NMD_X86_INSTRUCTION_FDISI8087_NOP,
	NMD_X86_INSTRUCTION_FENI8087_NOP,

	/* pseudo instructions */
	NMD_X86_INSTRUCTION_CMPSS,
	NMD_X86_INSTRUCTION_CMPEQSS,
	NMD_X86_INSTRUCTION_CMPLTSS,
	NMD_X86_INSTRUCTION_CMPLESS,
	NMD_X86_INSTRUCTION_CMPUNORDSS,
	NMD_X86_INSTRUCTION_CMPNEQSS,
	NMD_X86_INSTRUCTION_CMPNLTSS,
	NMD_X86_INSTRUCTION_CMPNLESS,
	NMD_X86_INSTRUCTION_CMPORDSS,

	NMD_X86_INSTRUCTION_CMPSD,
	NMD_X86_INSTRUCTION_CMPEQSD,
	NMD_X86_INSTRUCTION_CMPLTSD,
	NMD_X86_INSTRUCTION_CMPLESD,
	NMD_X86_INSTRUCTION_CMPUNORDSD,
	NMD_X86_INSTRUCTION_CMPNEQSD,
	NMD_X86_INSTRUCTION_CMPNLTSD,
	NMD_X86_INSTRUCTION_CMPNLESD,
	NMD_X86_INSTRUCTION_CMPORDSD,

	NMD_X86_INSTRUCTION_CMPPS,
	NMD_X86_INSTRUCTION_CMPEQPS,
	NMD_X86_INSTRUCTION_CMPLTPS,
	NMD_X86_INSTRUCTION_CMPLEPS,
	NMD_X86_INSTRUCTION_CMPUNORDPS,
	NMD_X86_INSTRUCTION_CMPNEQPS,
	NMD_X86_INSTRUCTION_CMPNLTPS,
	NMD_X86_INSTRUCTION_CMPNLEPS,
	NMD_X86_INSTRUCTION_CMPORDPS,

	NMD_X86_INSTRUCTION_CMPPD,
	NMD_X86_INSTRUCTION_CMPEQPD,
	NMD_X86_INSTRUCTION_CMPLTPD,
	NMD_X86_INSTRUCTION_CMPLEPD,
	NMD_X86_INSTRUCTION_CMPUNORDPD,
	NMD_X86_INSTRUCTION_CMPNEQPD,
	NMD_X86_INSTRUCTION_CMPNLTPD,
	NMD_X86_INSTRUCTION_CMPNLEPD,
	NMD_X86_INSTRUCTION_CMPORDPD,

	NMD_X86_INSTRUCTION_VCMPSS,
	NMD_X86_INSTRUCTION_VCMPEQSS,
	NMD_X86_INSTRUCTION_VCMPLTSS,
	NMD_X86_INSTRUCTION_VCMPLESS,
	NMD_X86_INSTRUCTION_VCMPUNORDSS,
	NMD_X86_INSTRUCTION_VCMPNEQSS,
	NMD_X86_INSTRUCTION_VCMPNLTSS,
	NMD_X86_INSTRUCTION_VCMPNLESS,
	NMD_X86_INSTRUCTION_VCMPORDSS,
	NMD_X86_INSTRUCTION_VCMPEQ_UQSS,
	NMD_X86_INSTRUCTION_VCMPNGESS,
	NMD_X86_INSTRUCTION_VCMPNGTSS,
	NMD_X86_INSTRUCTION_VCMPFALSESS,
	NMD_X86_INSTRUCTION_VCMPNEQ_OQSS,
	NMD_X86_INSTRUCTION_VCMPGESS,
	NMD_X86_INSTRUCTION_VCMPGTSS,
	NMD_X86_INSTRUCTION_VCMPTRUESS,
	NMD_X86_INSTRUCTION_VCMPEQ_OSSS,
	NMD_X86_INSTRUCTION_VCMPLT_OQSS,
	NMD_X86_INSTRUCTION_VCMPLE_OQSS,
	NMD_X86_INSTRUCTION_VCMPUNORD_SSS,
	NMD_X86_INSTRUCTION_VCMPNEQ_USSS,
	NMD_X86_INSTRUCTION_VCMPNLT_UQSS,
	NMD_X86_INSTRUCTION_VCMPNLE_UQSS,
	NMD_X86_INSTRUCTION_VCMPORD_SSS,
	NMD_X86_INSTRUCTION_VCMPEQ_USSS,
	NMD_X86_INSTRUCTION_VCMPNGE_UQSS,
	NMD_X86_INSTRUCTION_VCMPNGT_UQSS,
	NMD_X86_INSTRUCTION_VCMPFALSE_OSSS,
	NMD_X86_INSTRUCTION_VCMPNEQ_OSSS,
	NMD_X86_INSTRUCTION_VCMPGE_OQSS,
	NMD_X86_INSTRUCTION_VCMPGT_OQSS,
	NMD_X86_INSTRUCTION_VCMPTRUE_USSS,

	NMD_X86_INSTRUCTION_VCMPSD,
	NMD_X86_INSTRUCTION_VCMPEQSD,
	NMD_X86_INSTRUCTION_VCMPLTSD,
	NMD_X86_INSTRUCTION_VCMPLESD,
	NMD_X86_INSTRUCTION_VCMPUNORDSD,
	NMD_X86_INSTRUCTION_VCMPNEQSD,
	NMD_X86_INSTRUCTION_VCMPNLTSD,
	NMD_X86_INSTRUCTION_VCMPNLESD,
	NMD_X86_INSTRUCTION_VCMPORDSD,
	NMD_X86_INSTRUCTION_VCMPEQ_UQSD,
	NMD_X86_INSTRUCTION_VCMPNGESD,
	NMD_X86_INSTRUCTION_VCMPNGTSD,
	NMD_X86_INSTRUCTION_VCMPFALSESD,
	NMD_X86_INSTRUCTION_VCMPNEQ_OQSD,
	NMD_X86_INSTRUCTION_VCMPGESD,
	NMD_X86_INSTRUCTION_VCMPGTSD,
	NMD_X86_INSTRUCTION_VCMPTRUESD,
	NMD_X86_INSTRUCTION_VCMPEQ_OSSD,
	NMD_X86_INSTRUCTION_VCMPLT_OQSD,
	NMD_X86_INSTRUCTION_VCMPLE_OQSD,
	NMD_X86_INSTRUCTION_VCMPUNORD_SSD,
	NMD_X86_INSTRUCTION_VCMPNEQ_USSD,
	NMD_X86_INSTRUCTION_VCMPNLT_UQSD,
	NMD_X86_INSTRUCTION_VCMPNLE_UQSD,
	NMD_X86_INSTRUCTION_VCMPORD_SSD,
	NMD_X86_INSTRUCTION_VCMPEQ_USSD,
	NMD_X86_INSTRUCTION_VCMPNGE_UQSD,
	NMD_X86_INSTRUCTION_VCMPNGT_UQSD,
	NMD_X86_INSTRUCTION_VCMPFALSE_OSSD,
	NMD_X86_INSTRUCTION_VCMPNEQ_OSSD,
	NMD_X86_INSTRUCTION_VCMPGE_OQSD,
	NMD_X86_INSTRUCTION_VCMPGT_OQSD,
	NMD_X86_INSTRUCTION_VCMPTRUE_USSD,

	NMD_X86_INSTRUCTION_VCMPPS,
	NMD_X86_INSTRUCTION_VCMPEQPS,
	NMD_X86_INSTRUCTION_VCMPLTPS,
	NMD_X86_INSTRUCTION_VCMPLEPS,
	NMD_X86_INSTRUCTION_VCMPUNORDPS,
	NMD_X86_INSTRUCTION_VCMPNEQPS,
	NMD_X86_INSTRUCTION_VCMPNLTPS,
	NMD_X86_INSTRUCTION_VCMPNLEPS,
	NMD_X86_INSTRUCTION_VCMPORDPS,
	NMD_X86_INSTRUCTION_VCMPEQ_UQPS,
	NMD_X86_INSTRUCTION_VCMPNGEPS,
	NMD_X86_INSTRUCTION_VCMPNGTPS,
	NMD_X86_INSTRUCTION_VCMPFALSEPS,
	NMD_X86_INSTRUCTION_VCMPNEQ_OQPS,
	NMD_X86_INSTRUCTION_VCMPGEPS,
	NMD_X86_INSTRUCTION_VCMPGTPS,
	NMD_X86_INSTRUCTION_VCMPTRUEPS,
	NMD_X86_INSTRUCTION_VCMPEQ_OSPS,
	NMD_X86_INSTRUCTION_VCMPLT_OQPS,
	NMD_X86_INSTRUCTION_VCMPLE_OQPS,
	NMD_X86_INSTRUCTION_VCMPUNORD_SPS,
	NMD_X86_INSTRUCTION_VCMPNEQ_USPS,
	NMD_X86_INSTRUCTION_VCMPNLT_UQPS,
	NMD_X86_INSTRUCTION_VCMPNLE_UQPS,
	NMD_X86_INSTRUCTION_VCMPORD_SPS,
	NMD_X86_INSTRUCTION_VCMPEQ_USPS,
	NMD_X86_INSTRUCTION_VCMPNGE_UQPS,
	NMD_X86_INSTRUCTION_VCMPNGT_UQPS,
	NMD_X86_INSTRUCTION_VCMPFALSE_OSPS,
	NMD_X86_INSTRUCTION_VCMPNEQ_OSPS,
	NMD_X86_INSTRUCTION_VCMPGE_OQPS,
	NMD_X86_INSTRUCTION_VCMPGT_OQPS,
	NMD_X86_INSTRUCTION_VCMPTRUE_USPS,

	NMD_X86_INSTRUCTION_VCMPPD,
	NMD_X86_INSTRUCTION_VCMPEQPD,
	NMD_X86_INSTRUCTION_VCMPLTPD,
	NMD_X86_INSTRUCTION_VCMPLEPD,
	NMD_X86_INSTRUCTION_VCMPUNORDPD,
	NMD_X86_INSTRUCTION_VCMPNEQPD,
	NMD_X86_INSTRUCTION_VCMPNLTPD,
	NMD_X86_INSTRUCTION_VCMPNLEPD,
	NMD_X86_INSTRUCTION_VCMPORDPD,
	NMD_X86_INSTRUCTION_VCMPEQ_UQPD,
	NMD_X86_INSTRUCTION_VCMPNGEPD,
	NMD_X86_INSTRUCTION_VCMPNGTPD,
	NMD_X86_INSTRUCTION_VCMPFALSEPD,
	NMD_X86_INSTRUCTION_VCMPNEQ_OQPD,
	NMD_X86_INSTRUCTION_VCMPGEPD,
	NMD_X86_INSTRUCTION_VCMPGTPD,
	NMD_X86_INSTRUCTION_VCMPTRUEPD,
	NMD_X86_INSTRUCTION_VCMPEQ_OSPD,
	NMD_X86_INSTRUCTION_VCMPLT_OQPD,
	NMD_X86_INSTRUCTION_VCMPLE_OQPD,
	NMD_X86_INSTRUCTION_VCMPUNORD_SPD,
	NMD_X86_INSTRUCTION_VCMPNEQ_USPD,
	NMD_X86_INSTRUCTION_VCMPNLT_UQPD,
	NMD_X86_INSTRUCTION_VCMPNLE_UQPD,
	NMD_X86_INSTRUCTION_VCMPORD_SPD,
	NMD_X86_INSTRUCTION_VCMPEQ_USPD,
	NMD_X86_INSTRUCTION_VCMPNGE_UQPD,
	NMD_X86_INSTRUCTION_VCMPNGT_UQPD,
	NMD_X86_INSTRUCTION_VCMPFALSE_OSPD,
	NMD_X86_INSTRUCTION_VCMPNEQ_OSPD,
	NMD_X86_INSTRUCTION_VCMPGE_OQPD,
	NMD_X86_INSTRUCTION_VCMPGT_OQPD,
	NMD_X86_INSTRUCTION_VCMPTRUE_USPD,

	NMD_X86_INSTRUCTION_UD0,
	NMD_X86_INSTRUCTION_ENDBR32,
	NMD_X86_INSTRUCTION_ENDBR64,
};

enum NMD_X86_OPERAND_TYPE
{
	NMD_X86_OPERAND_TYPE_NONE = 0,
	NMD_X86_OPERAND_TYPE_REGISTER,
	NMD_X86_OPERAND_TYPE_MEMORY,
	NMD_X86_OPERAND_TYPE_IMMEDIATE,
};

typedef struct NMD_X86MemoryOperand
{
	uint8_t segment;     /* The segment register. A member of 'NMD_X86_REG'. */
	uint8_t base;        /* The base register. A member of 'NMD_X86_REG'. */
	uint8_t index;       /* The index register. A member of 'NMD_X86_REG'. */
	uint8_t scale;       /* Scale(1, 2, 4 or 8). */
	int64_t disp;        /* Displacement. */
} NMD_X86MemoryOperand;

enum NMD_X86_OPERAND_ACTION
{
	NMD_X86_OPERAND_ACTION_NONE = 0, /* The operand is neither read from nor written to. */

	NMD_X86_OPERAND_ACTION_READ = (1 << 0), /* The operand is read. */
	NMD_X86_OPERAND_ACTION_WRITE = (1 << 1), /* The operand is modified. */
	NMD_X86_OPERAND_ACTION_CONDITIONAL_READ = (1 << 2), /* The operand may be read depending on some condition. */
	NMD_X86_OPERAND_ACTION_CONDITIONAL_WRITE = (1 << 3), /* The operand may be modified depending on some condition. */

														 /* These are not actual actions, but rather masks of actions. */
														 NMD_X86_OPERAND_ACTION_READ_WRITE = (NMD_X86_OPERAND_ACTION_READ | NMD_X86_OPERAND_ACTION_WRITE),
														 NMD_X86_OPERAND_ACTION_ANY_READ = (NMD_X86_OPERAND_ACTION_READ | NMD_X86_OPERAND_ACTION_CONDITIONAL_READ),
														 NMD_X86_OPERAND_ACTION_ANY_WRITE = (NMD_X86_OPERAND_ACTION_WRITE | NMD_X86_OPERAND_ACTION_CONDITIONAL_WRITE)
};

typedef struct NMD_X86Operand
{
	uint8_t type;                  /* The operand's type. A member of 'NMD_X86_OPERAND_TYPE'. */
	uint8_t size;                  /* The operand's size in bytes. */
	bool isImplicit;               /* If true, the operand does not appear on the intruction's formatted form. */
	uint8_t action;                /* The action on the operand. A member of 'NMD_X86_OPERAND_ACTION'. */
	union {                        /* The operand's "raw" data. */
		uint8_t reg;               /* Register operand. A variable of type 'NMD_X86_REG' */
		int64_t imm;               /* Immediate operand. */
		NMD_X86MemoryOperand mem;  /* Memory operand. */
	} fields;
} NMD_X86Operand;

typedef union NMD_X86CpuFlags
{
	struct
	{
		uint8_t CF : 1; /* Bit  0.    Carry Flag (CF) */
		uint8_t b1 : 1; /* Bit  1.    Reserved */
		uint8_t PF : 1; /* Bit  2.    Parity Flag (PF) */
		uint8_t B3 : 1; /* Bit  3.    Reserved */
		uint8_t AF : 1; /* Bit  4.    Auxiliary Carry Flag (AF) */
		uint8_t B5 : 1; /* Bit  5.    Reserved */
		uint8_t ZF : 1; /* Bit  6.    Zero flag(ZF) */
		uint8_t SF : 1; /* Bit  7.    Sign flag(SF) */
		uint8_t TF : 1; /* Bit  8.    Trap flag(TF) */
		uint8_t IF : 1; /* Bit  9.    Interrupt Enable Flag (IF) */
		uint8_t DF : 1; /* Bit 10.    Direction Flag (DF) */
		uint8_t OF : 1; /* Bit 11.    Overflow Flag (OF) */
		uint8_t IOPL : 2; /* Bit 12,13. I/O Privilege Level (IOPL) */
		uint8_t NT : 1; /* Bit 14.    Nested Task (NT) */
		uint8_t B15 : 1; /* Bit 15.    Reserved */
		uint8_t RF : 1; /* Bit 16.    Resume Flag (RF) */
		uint8_t VM : 1; /* Bit 17.    Virtual-8086 Mode (VM) */
		uint8_t AC : 1; /* Bit 18.    Alignment Check / Access Control (AC) */
		uint8_t VIF : 1; /* Bit 19.    Virtual Interrupt Flag (VIF) */
		uint8_t VIP : 1; /* Bit 20.    Virtual Interrupt Pending (VIP) */
		uint8_t ID : 1; /* Bit 21.    ID Flag(ID) */
		uint8_t B22 : 1; /* Bit 22.    Reserved */
		uint8_t B23 : 1; /* Bit 23.    Reserved */
		uint8_t B24 : 1; /* Bit 24.    Reserved */
		uint8_t B25 : 1; /* Bit 25.    Reserved */
		uint8_t B26 : 1; /* Bit 26.    Reserved */
		uint8_t B27 : 1; /* Bit 27.    Reserved */
		uint8_t B28 : 1; /* Bit 28.    Reserved */
		uint8_t B29 : 1; /* Bit 29.    Reserved */
		uint8_t B30 : 1; /* Bit 30.    Reserved */
		uint8_t B31 : 1; /* Bit 31.    Reserved */
	} fields;
	struct
	{
		uint8_t IE : 1; /* Bit  0.    Invalid Operation (IE) */
		uint8_t DE : 1; /* Bit  1.    Denormalized Operand (DE) */
		uint8_t ZE : 1; /* Bit  2.    Zero Divide (ZE) */
		uint8_t OE : 1; /* Bit  3.    Overflow (OE) */
		uint8_t UE : 1; /* Bit  4.    Underflow (UE) */
		uint8_t PE : 1; /* Bit  5.    Precision (PE) */
		uint8_t SF : 1; /* Bit  6.    Stack Fault (SF) */
		uint8_t ES : 1; /* Bit  7.    Exception Summary Status (ES) */
		uint8_t C0 : 1; /* Bit  8.    Condition code 0 (C0) */
		uint8_t C1 : 1; /* Bit  9.    Condition code 1 (C1) */
		uint8_t C2 : 1; /* Bit 10.    Condition code 2 (C2) */
		uint8_t TOP : 3; /* Bit 11-13. Top of Stack Pointer (TOP) */
		uint8_t C3 : 1; /* Bit 14.    Condition code 3 (C3) */
		uint8_t B : 1; /* Bit 15.    FPU Busy (B) */
	} fpuFields;
	uint8_t l8;
	uint32_t eflags;
	uint16_t fpuFlags;
} NMD_X86CpuFlags;

enum NMD_X86_EFLAGS
{
	NMD_X86_EFLAGS_ID = (1 << 21),
	NMD_X86_EFLAGS_VIP = (1 << 20),
	NMD_X86_EFLAGS_VIF = (1 << 19),
	NMD_X86_EFLAGS_AC = (1 << 18),
	NMD_X86_EFLAGS_VM = (1 << 17),
	NMD_X86_EFLAGS_RF = (1 << 16),
	NMD_X86_EFLAGS_NT = (1 << 14),
	NMD_X86_EFLAGS_IOPL = (1 << 12) | (1 << 13),
	NMD_X86_EFLAGS_OF = (1 << 11),
	NMD_X86_EFLAGS_DF = (1 << 10),
	NMD_X86_EFLAGS_IF = (1 << 9),
	NMD_X86_EFLAGS_TF = (1 << 8),
	NMD_X86_EFLAGS_SF = (1 << 7),
	NMD_X86_EFLAGS_ZF = (1 << 6),
	NMD_X86_EFLAGS_AF = (1 << 4),
	NMD_X86_EFLAGS_PF = (1 << 2),
	NMD_X86_EFLAGS_CF = (1 << 0)
};

enum NMD_X86_FPU_FLAGS
{
	NMD_X86_FPU_FLAGS_C0 = (1 << 8),
	NMD_X86_FPU_FLAGS_C1 = (1 << 9),
	NMD_X86_FPU_FLAGS_C2 = (1 << 10),
	NMD_X86_FPU_FLAGS_C3 = (1 << 14)
};

typedef struct NMD_X86Instruction
{
	bool valid : 1;                                        /* If true, the instruction is valid. */
	bool hasModrm : 1;                                     /* If true, the instruction has a modrm byte. */
	bool hasSIB : 1;                                       /* If true, the instruction has an SIB byte. */
	bool hasRex : 1;                                       /* If true, the instruction has a REX prefix */
	bool operandSize64 : 1;                                /* If true, a REX.W prefix is closer to the opcode than a operand size override prefix. */
	bool repeatPrefix : 1;                                 /* If true, a 'repeat'(F3h) prefix is closer to the opcode than a 'repeat not zero'(F2h) prefix. */
	uint8_t mode;                                          /* The decoding mode. A member of 'NMD_X86_MODE'. */
	uint8_t length;                                        /* The instruction's length in bytes. */
	uint8_t opcode;                                        /* Opcode byte. */
	uint8_t opcodeSize;                                    /* The opcode's size in bytes. */
	uint16_t id;                                           /* The instruction's identifier. A member of 'NMD_X86_INSTRUCTION'. */
	uint16_t prefixes;                                     /* A mask of prefixes. See 'NMD_X86_PREFIXES'. */
	uint8_t numPrefixes;                                   /* Number of prefixes. */
	uint8_t numOperands;                                   /* The number of operands. */
	uint8_t group;                                         /* The instruction's group(e.g. jmp, prvileged...). A member of 'NMD_X86_GROUP'. */
	uint8_t buffer[NMD_X86_MAXIMUM_INSTRUCTION_LENGTH];    /* A buffer containing the full instruction. */
	NMD_X86Operand operands[NMD_X86_MAXIMUM_NUM_OPERANDS]; /* Operands. */
	NMD_Modrm modrm;                                       /* The Mod/RM byte. Check 'flags.fields.hasModrm'. */
	NMD_SIB sib;                                           /* The SIB byte. Check 'flags.fields.hasSIB'. */
	uint8_t immMask;                                       /* A mask of one or more members of 'NMD_X86_IMM'. */
	uint8_t dispMask;                                      /* A mask of one or more members of 'NMD_X86_DISP'. */
	uint64_t immediate;                                    /* Immediate. Check 'immMask'. */
	uint32_t displacement;                                 /* Displacement. Check 'dispMask'. */
	uint8_t opcodeMap;                                     /* The instruction's opcode map. A member of 'NMD_X86_OPCODE_MAP'. */
	uint8_t encoding;                                      /* The instruction's encoding. A member of 'NMD_X86_INSTRUCTION_ENCODING'. */
	NMD_X86Vex vex;                                        /* VEX prefix. */
	NMD_X86CpuFlags modifiedFlags;                         /* Cpu flags modified by the instruction. */
	NMD_X86CpuFlags testedFlags;                           /* Cpu flags tested by the instruction. */
	NMD_X86CpuFlags setFlags;                              /* Cpu flags set by the instruction. */
	NMD_X86CpuFlags clearedFlags;                          /* Cpu flags cleared by the instruction. */
	NMD_X86CpuFlags undefinedFlags;                        /* Cpu flags whose state is undefined. */
	uint8_t rex;                                           /* REX prefix. */
	uint8_t segmentOverride;                               /* The segment override prefix closest to the opcode. A member of 'NMD_X86_PREFIXES'. */
	uint16_t simdPrefix;                                   /* Either one of these prefixes that is the closest to the opcode: NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE, NMD_X86_PREFIXES_LOCK, NMD_X86_PREFIXES_REPEAT_NOT_ZERO, NMD_X86_PREFIXES_REPEAT, or NMD_X86_PREFIXES_NONE. The prefixes are specified as members of the 'NMD_X86_PREFIXES' enum. */
} NMD_X86Instruction;

typedef enum NMD_X86_EMULATOR_EXCEPTION
{
	NMD_X86_EMULATOR_EXCEPTION_NONE = 0,
	NMD_X86_EMULATOR_EXCEPTION_BREAKPOINT, /* #BP generated by int3 */
	NMD_X86_EMULATOR_EXCEPTION_DEBUG, /* #DB generated by int1 */
	NMD_X86_EMULATOR_EXCEPTION_OVERFLOW, /* #OF generated by into */
	NMD_X86_EMULATOR_EXCEPTION_GENERAL_PROTECTION,
	NMD_X86_EMULATOR_EXCEPTION_BAD_INSTRUCTION,
	NMD_X86_EMULATOR_EXCEPTION_BAD_MEMORY,
	NMD_X86_EMULATOR_EXCEPTION_STEP
} NMD_X86_EMULATOR_EXCEPTION;

typedef union NMD_X86Register
{
	int8_t  h8;
	int8_t  l8;
	int16_t l16;
	int32_t l32;
	int64_t l64;
} NMD_X86Register;

typedef union NMD_X86Register512
{
	uint64_t xmm0[2];
	uint64_t ymm0[4];
	uint64_t zmm0[8];
} NMD_X86Register512;

typedef struct NMD_X86Cpu
{
	bool running; /* If true, the emulator is running, false otherwise. */

	uint8_t mode; /* The emulator's architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'. */

	void* physicalMemory; /* A pointer to a buffer used as the emulator's memory. */
	size_t physicalMemorySize; /* The size of the buffer pointer by 'physicalMemory' in bytes. */

	uint64_t virtualAddress; /* The starting address of the emulator's virtual address space. This address can be any value. */

	void(*callback)(struct NMD_X86Cpu* cpu, const NMD_X86Instruction* instruction, NMD_X86_EMULATOR_EXCEPTION exception);

	void* userdata;

	uint64_t rip; /* The address of the next instruction to be executed(emulated). */

	NMD_X86CpuFlags flags;

	NMD_X86Register rax;
	NMD_X86Register rcx;
	NMD_X86Register rdx;
	NMD_X86Register rbx;
	NMD_X86Register rsp;
	NMD_X86Register rbp;
	NMD_X86Register rsi;
	NMD_X86Register rdi;

	NMD_X86Register r8;
	NMD_X86Register r9;
	NMD_X86Register r10;
	NMD_X86Register r11;
	NMD_X86Register r12;
	NMD_X86Register r13;
	NMD_X86Register r14;
	NMD_X86Register r15;

	NMD_X86Register mm0;
	NMD_X86Register mm1;
	NMD_X86Register mm2;
	NMD_X86Register mm3;
	NMD_X86Register mm4;
	NMD_X86Register mm5;
	NMD_X86Register mm6;
	NMD_X86Register mm7;

	NMD_X86Register512 zmm0;
	NMD_X86Register512 zmm1;
	NMD_X86Register512 zmm2;
	NMD_X86Register512 zmm3;
	NMD_X86Register512 zmm4;
	NMD_X86Register512 zmm5;
	NMD_X86Register512 zmm6;
	NMD_X86Register512 zmm7;
	NMD_X86Register512 zmm8;
	NMD_X86Register512 zmm9;
	NMD_X86Register512 zmm10;
	NMD_X86Register512 zmm11;
	NMD_X86Register512 zmm12;
	NMD_X86Register512 zmm13;
	NMD_X86Register512 zmm14;
	NMD_X86Register512 zmm15;
	NMD_X86Register512 zmm16;
	NMD_X86Register512 zmm17;
	NMD_X86Register512 zmm18;
	NMD_X86Register512 zmm19;
	NMD_X86Register512 zmm20;
	NMD_X86Register512 zmm21;
	NMD_X86Register512 zmm22;
	NMD_X86Register512 zmm23;
	NMD_X86Register512 zmm24;
	NMD_X86Register512 zmm25;
	NMD_X86Register512 zmm26;
	NMD_X86Register512 zmm27;
	NMD_X86Register512 zmm28;
	NMD_X86Register512 zmm29;
	NMD_X86Register512 zmm30;
	NMD_X86Register512 zmm31;

	NMD_X86Register dr0;
	NMD_X86Register dr1;
	NMD_X86Register dr2;
	NMD_X86Register dr3;
	NMD_X86Register dr4;
	NMD_X86Register dr5;
	NMD_X86Register dr6;
	NMD_X86Register dr7;
} NMD_X86Cpu;

/*
Assembles an instruction from a string. Returns the number of bytes written to the buffer on success, zero otherwise. Instructions can be separated using either the ';' or '\n' character.
Parameters:
- string         [in]         A pointer to a string that represents a instruction in assembly language.
- buffer         [out]        A pointer to a buffer that receives the encoded instructions.
- bufferSize     [in]         The size of the buffer in bytes.
- runtimeAddress [in]         The instruction's runtime address. You may use 'NMD_X86_INVALID_RUNTIME_ADDRESS'.
- mode           [in]         The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
- count          [in/out/opt] A pointer to a variable that on input is the maximum number of instructions that can be parsed(or zero for unlimited instructions), and on output is the number of instructions parsed. This parameter may be zero.
*/
size_t nmd_x86_assemble(const char* string, void* buffer, size_t bufferSize, uint64_t runtimeAddress, NMD_X86_MODE mode, size_t* count);

/*
Decodes an instruction. Returns true if the instruction is valid, false otherwise.
Parameters:
- buffer      [in]  A pointer to a buffer containing a encoded instruction.
- bufferSize  [in]  The buffer's size in bytes.
- instruction [out] A pointer to a variable of type 'NMD_X86Instruction' that receives information about the instruction.
- mode        [in]  The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
- flags       [in]  A mask of 'NMD_X86_DECODER_FLAGS_XXX' that specifies which features the decoder is allowed to use. If uncertain, use 'NMD_X86_DECODER_FLAGS_MINIMAL'.
*/
bool nmd_x86_decode_buffer(const void* buffer, size_t bufferSize, NMD_X86Instruction* instruction, NMD_X86_MODE mode, uint32_t flags);

/*
Formats an instruction. This function may cause a crash if you modify 'instruction' manually.
Parameters:
- instruction    [in]  A pointer to a variable of type 'NMD_X86Instruction' describing the instruction to be formatted.
- buffer         [out] A pointer to buffer that receives the string. The buffer's recommended size is 128 bytes.
- runtimeAddress [in]  The instruction's runtime address. You may use 'NMD_X86_INVALID_RUNTIME_ADDRESS'.
- formatFlags    [in]  A mask of 'NMD_X86_FORMAT_FLAGS_XXX' that specifies how the function should format the instruction. If uncertain, use 'NMD_X86_FORMAT_FLAGS_DEFAULT'.
*/
void nmd_x86_format_instruction(const NMD_X86Instruction* instruction, char* buffer, uint64_t runtimeAddress, uint32_t formatFlags);

/*
Emulates x86 code according to the cpu's state. You MUST initialize the following variables before calling this
function: 'cpu->mode', 'cpu->physicalMemory', 'cpu->physicalMemorySize', 'cpu->virtualAddress' and 'cpu->rip'.
You may optionally initialize 'cpu->rsp' if a stack is desirable. Below is a short description of each variable:
- 'cpu->mode': The emulator's operating architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
- 'cpu->physicalMemory': A pointer to a buffer used as the emulator's memory.
- 'cpu->physicalMemorySize': The size of the buffer pointer by 'physicalMemory' in bytes.
- 'cpu->virtualAddress': The starting address of the emulator's virtual address space.
- 'cpu->rip': The virtual address where emulation starts.
- 'cpu->rsp': The virtual address of the bottom of the stack.
Parameters:
- cpu      [in] A pointer to a variable of type 'NMD_X86Cpu' that holds the state of the cpu.
- maxCount [in] The maximum number of instructions that can be executed, or zero for unlimited instructions.
*/
bool nmd_x86_emulate(NMD_X86Cpu* cpu, size_t maxCount);

/*
Returns the instruction's length if it's valid, zero otherwise.
Parameters:
- buffer     [in] A pointer to a buffer containing a encoded instruction.
- bufferSize [in] The buffer's size in bytes.
- mode       [in] The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
*/
size_t nmd_x86_ldisasm(const void* buffer, size_t bufferSize, NMD_X86_MODE mode);

#endif /* NMD_ASSEMBLY_H */


#ifdef NMD_ASSEMBLY_IMPLEMENTATION

/* Four high-order bits of an opcode to index a row of the opcode table */
#define NMD_R(b) ((b) >> 4)

/* Four low-order bits to index a column of the table */
#define NMD_C(b) ((b) & 0xF)

#define NMD_NUM_ELEMENTS(arr) (sizeof(arr) / sizeof((arr)[0]))

#define NMD_IS_LOWERCASE(c) (c >= 'a' && c <= 'z')
#define NMD_IS_DECIMAL_NUMBER(c) (c >= '0' && c <= '9')

const char* const reg8[] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
const char* const reg8_x64[] = { "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil" };
const char* const reg16[] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
const char* const reg32[] = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi" };
const char* const reg64[] = { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi" };
const char* const regrx[] = { "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" };
const char* const segmentReg[] = { "es", "cs", "ss", "ds", "fs", "gs" };

const char* const conditionSuffixes[] = { "o", "no", "b", "ae", "e", "ne", "be", "a", "s", "ns", "p", "np", "l", "ge", "le", "g" };

const char* const op1OpcodeMapMnemonics[] = { "add", "adc", "and", "xor", "or", "sbb", "sub", "cmp" };
const char* const opcodeExtensionsGrp1[] = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" };
const char* const opcodeExtensionsGrp2[] = { "rol", "ror", "rcl", "rcr", "shl", "shr", "shl", "sar" };
const char* const opcodeExtensionsGrp3[] = { "test", "test", "not", "neg", "mul", "imul", "div", "idiv" };
const char* const opcodeExtensionsGrp5[] = { "inc", "dec", "call", "call far", "jmp", "jmp far", "push" };
const char* const opcodeExtensionsGrp6[] = { "sldt", "str", "lldt", "ltr", "verr", "verw" };
const char* const opcodeExtensionsGrp7[] = { "sgdt", "sidt", "lgdt", "lidt", "smsw", 0, "lmsw", "invlpg" };
const char* const opcodeExtensionsGrp7reg0[] = { "enclv", "vmcall", "vmlaunch", "vmresume", "vmxoff", "pconfig" };
const char* const opcodeExtensionsGrp7reg1[] = { "monitor", "mwait", "clac", "stac", 0, 0, 0, "encls" };
const char* const opcodeExtensionsGrp7reg2[] = { "xgetbv", "xsetbv", 0, 0, "vmfunc", "xend", "xtest", "enclu" };
const char* const opcodeExtensionsGrp7reg3[] = { "vmrun ", "vmmcall", "vmload ", "vmsave", "stgi", "clgi", "skinit eax", "invlpga " };
const char* const opcodeExtensionsGrp7reg7[] = { "swapgs", "rdtscp", "monitorx", "mwaitx", "clzero ", "rdpru" };

const char* const escapeOpcodesD8[] = { "add", "mul", "com", "comp", "sub", "subr", "div", "divr" };
const char* const escapeOpcodesD9[] = { "ld", 0, "st", "stp", "ldenv", "ldcw", "nstenv", "nstcw" };
const char* const escapeOpcodesDA_DE[] = { "iadd", "imul", "icom", "icomp", "isub", "isubr", "idiv", "idivr" };
const char* const escapeOpcodesDB[] = { "ild", "isttp", "ist", "istp", 0, "ld", 0, "stp" };
const char* const escapeOpcodesDC[] = { "add", "mul", "com", "comp", "sub", "subr", "div", "divr" };
const char* const escapeOpcodesDD[] = { "ld", "isttp", "st", "stp", "rstor", 0, "nsave", "nstsw" };
const char* const escapeOpcodesDF[] = { "ild", "isttp", "ist", "istp", "bld", "ild", "bstp", "istp" };
const char* const* escapeOpcodes[] = { escapeOpcodesD8, escapeOpcodesD9, escapeOpcodesDA_DE, escapeOpcodesDB, escapeOpcodesDC, escapeOpcodesDD, escapeOpcodesDA_DE, escapeOpcodesDF };

const uint8_t op1modrm[] = { 0xFF, 0x63, 0x69, 0x6B, 0xC0, 0xC1, 0xC6, 0xC7, 0xD0, 0xD1, 0xD2, 0xD3, 0xF6, 0xF7, 0xFE };
const uint8_t op1imm8[] = { 0x6A, 0x6B, 0x80, 0x82, 0x83, 0xA8, 0xC0, 0xC1, 0xC6, 0xCD, 0xD4, 0xD5, 0xEB };
const uint8_t op1imm32[] = { 0xE8, 0xE9, 0x68, 0x81, 0x69, 0xA9, 0xC7 };
const uint8_t invalid2op[] = { 0x04, 0x0a, 0x0c, 0x7a, 0x7b, 0x36, 0x39 };
const uint8_t twoOpcodes[] = { 0xb0, 0xb1, 0xb3, 0xbb, 0xc0, 0xc1 };
const uint8_t valid3DNowOpcodes[] = { 0x0c, 0x0d, 0x1c, 0x1d, 0x8a, 0x8e, 0x90, 0x94, 0x96, 0x97, 0x9a, 0x9e, 0xa0, 0xa4, 0xa6, 0xa7, 0xaa, 0xae, 0xb0, 0xb4, 0xb6, 0xb7, 0xbb, 0xbf };

bool nmd_findByte(const uint8_t* arr, const size_t N, const uint8_t x)
{
	size_t i = 0;
	for (; i < N; i++)
	{
		if (arr[i] == x)
			return true;
	};

	return false;
}

/* Returns a pointer to the first occurrence of 'c' in 's', or a null pointer if 'c' is not present. */
const char* nmd_strchr(const char* s, char c)
{
	for (; *s; s++)
	{
		if (*s == c)
			return s;
	}

	return 0;
}

/* Returns a pointer to the last occurrence of 'c' in 's', or a null pointer if 'c' is not present. */
const char* nmd_reverse_strchr(const char* s, char c)
{
	const char* end = s;
	while (*end)
		end++;

	for (; end > s; end--)
	{
		if (*end == c)
			return end;
	}

	return 0;
}

/* Returns a pointer to the first occurrence of 's2' in 's', or a null pointer if 's2' is not present. */
const char* nmd_strstr(const char* s, const char* s2)
{
	size_t i = 0;
	for (; *s; s++)
	{
		if (s2[i] == '\0')
			return s - i;

		if (*s != s2[i])
			i = 0;

		if (*s == s2[i])
			i++;
	}

	return 0;
}

/* Returns a pointer to the first occurrence of 's2' in 's', or a null pointer if 's2' is not present. If 's3_opt' is not null it receives the address of the next byte in 's'. */
const char* nmd_strstr_ex(const char* s, const char* s2, const char** s3_opt)
{
	size_t i = 0;
	for (; *s; s++)
	{
		if (s2[i] == '\0')
		{
			if (s3_opt)
				*s3_opt = s;
			return s - i;
		}

		if (*s != s2[i])
			i = 0;

		if (*s == s2[i])
			i++;
	}

	return 0;
}

/* Inserts 'c' at 's'. */
void nmd_insert_char(const char* s, char c)
{
	char* end = (char*)s;
	while (*end)
		end++;

	*(end + 1) = '\0';

	for (; end > s; end--)
		*end = *(end - 1);

	*end = c;
}

/* Returns true if there is only a number between 's1' and 's2', false otherwise. */
bool nmd_is_number(const char* s1, const char* s2)
{
	const char* s = s1;
	for (; s < s2; s++)
	{
		if (!(*s >= '0' && *s <= '9') && !(*s >= 'a' && *s <= 'f') && !(*s >= 'A' && *s <= 'F'))
		{
			if ((s == s1 + 1 && *s1 == '0' && (*s == 'x' || *s == 'X')) || (s == s2 - 1 && (*s == 'h' || *s == 'H')))
				continue;

			return false;
		}
	}

	return true;
}

/* Returns a pointer to the first occurence of a number between 's1' and 's2', zero otherwise. */
const char* nmd_find_number(const char* s1, const char* s2)
{
	const char* s = s1;
	for (; s < s2; s++)
	{
		if ((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F'))
			return s;
	}

	return 0;
}

/* Returns true if s1 matches s2 exactly. */
bool nmd_strcmp(const char* s1, const char* s2)
{
	for (; *s1 && *s2; s1++, s2++)
	{
		if (*s1 != *s2)
			return false;
	}

	return !*s1 && !*s2;
}

size_t nmd_getBitNumber(uint32_t mask)
{
	size_t i = 0;
	while (!(mask & (1 << i)))
		i++;

	return i;
}

typedef struct AssembleInfo
{
	const char* s; /* string */
	uint8_t* b; /* buffer */
	NMD_X86_MODE mode;
	uint64_t runtimeAddress;
} AssembleInfo;

size_t assembleReg(AssembleInfo* ai, uint8_t baseByte)
{
	uint8_t i = 0;
	if (ai->mode == NMD_X86_MODE_64)
	{
		for (i = 0; i < NMD_NUM_ELEMENTS(reg64); i++)
		{
			if (nmd_strcmp(ai->s, reg64[i]))
			{
				ai->b[0] = baseByte + i;
				return 1;
			}
		}

		for (i = 0; i < NMD_NUM_ELEMENTS(regrx); i++)
		{
			if (nmd_strcmp(ai->s, regrx[i]))
			{
				ai->b[0] = 0x41;
				ai->b[1] = baseByte + i;
				return 2;
			}
		}
	}
	else if (ai->mode == NMD_X86_MODE_32)
	{
		for (i = 0; i < NMD_NUM_ELEMENTS(reg32); i++)
		{
			if (nmd_strcmp(ai->s, reg32[i]))
			{
				ai->b[0] = baseByte + i;
				return 1;
			}
		}
	}

	for (i = 0; i < NMD_NUM_ELEMENTS(reg16); i++)
	{
		if (nmd_strcmp(ai->s, reg16[i]))
		{
			ai->b[0] = 0x66;
			ai->b[1] = baseByte + i;
			return 2;
		}
	}

	return 0;
}

enum NMD_NUMBER_BASE
{
	NMD_NUMBER_BASE_NONE = 0,
	NMD_NUMBER_BASE_DECIMAL = 10,
	NMD_NUMBER_BASE_HEXADECIMAL = 16,
	NMD_NUMBER_BASE_BINARY = 2
};

bool parseNumber(const char* string, int64_t* num, size_t* numDigits)
{
	if (*string == '\0')
		return false;

	/* Assume decimal base. */
	uint8_t base = NMD_NUMBER_BASE_DECIMAL;
	size_t i;
	const char* s = string;
	bool isNegative = false;

	if (s[0] == '-')
	{
		isNegative = true;
		s++;
	}

	if (s[0] == '0')
	{
		if (s[1] == 'x')
		{
			s += 2;
			base = NMD_NUMBER_BASE_HEXADECIMAL;
		}
		else if (s[1] == 'b')
		{
			s += 2;
			base = NMD_NUMBER_BASE_BINARY;
		}
	}

	for (i = 0; s[i]; i++)
	{
		const char c = s[i];

		if (base == NMD_NUMBER_BASE_DECIMAL)
		{
			if (c >= 'a' && c <= 'f')
			{
				base = NMD_NUMBER_BASE_HEXADECIMAL;
				continue;
			}
			else if (!(c >= '0' && c <= '9'))
				break;
		}
		else if (base == NMD_NUMBER_BASE_HEXADECIMAL)
		{
			if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
				break;
		}
		else if (c != '0' && c != '1') /* NMD_NUMBER_BASE_BINARY */
			break;
	}

	*numDigits = i;

	int64_t numTemp = 0;
	for (i = 0; i < *numDigits; i++)
	{
		const char c = s[i];
		numTemp += (c <= '9') ? (c - '0') : (10 + c - 'a');
		if (i < *numDigits - 1)
		{
			/* Return false if number is greater than 2^64-1 */
			if (*numDigits > 16 && i >= 15)
			{
				if ((base == NMD_NUMBER_BASE_DECIMAL && (uint64_t)numTemp >= (uint64_t)1844674407370955162) || /* ceiling((2^64-1) / 10) */
					(base == NMD_NUMBER_BASE_HEXADECIMAL && (uint64_t)numTemp >= (uint64_t)1152921504606846976) || /* *ceiling((2^64-1) / 16) */
					(base == NMD_NUMBER_BASE_BINARY && (uint64_t)numTemp >= (uint64_t)9223372036854775808)) /* ceiling((2^64-1) / 2) */
				{
					return false;
				}

			}
			numTemp *= base;
		}
	}

	if (s != string) /* There's either a "0x" or "0b" prefix. */
		*numDigits = (size_t)((ptrdiff_t)(s + i) - (ptrdiff_t)string);

	if (isNegative)
		numTemp *= -1;

	*num = numTemp;

	return true;
}

size_t assembleSingle(AssembleInfo* ai)
{
	size_t i = 0;

	bool lockPrefix = false, repeatPrefix = false, repeatZeroPrefix = false, repeatNotZeroPrefix = false;

	if (nmd_strstr(ai->s, "emit ") == (const char*)ai->s)
	{
		int64_t num = 0;
		size_t numDigits = 0;
		size_t offset = 5;
		while (parseNumber(ai->s + offset, &num, &numDigits))
		{
			if (num < 0 || num > 0xff)
				return 0;

			ai->b[i++] = (uint8_t)num;

			offset += numDigits;
			if (ai->s[offset] == ' ')
				offset++;
		}
		return i;
	}

	/* Parse prefixes */
	if (nmd_strstr(ai->s, "lock ") == (const char*)ai->s)
		lockPrefix = true, ai->s += 5;
	else if (nmd_strstr(ai->s, "rep ") == (const char*)ai->s)
		repeatPrefix = true, ai->s += 4;
	else if (nmd_strstr(ai->s, "repe ") == (const char*)ai->s || nmd_strstr(ai->s, "repz ") == (const char*)ai->s)
		repeatZeroPrefix = true, ai->s += 5;
	else if (nmd_strstr(ai->s, "repne ") == (const char*)ai->s || nmd_strstr(ai->s, "repnz ") == (const char*)ai->s)
		repeatNotZeroPrefix = true, ai->s += 6;

	if (nmd_strstr(ai->s, "xacquire ") == (const char*)ai->b)
	{
	}
	else if (nmd_strstr(ai->s, "xrelease ") == (const char*)ai->b)
	{
	}

	/* Parse opcode */
	if (ai->mode == NMD_X86_MODE_64) /* Only x86-64. */
	{
		if (nmd_strcmp(ai->s, "xchg r8,rax") || nmd_strcmp(ai->s, "xchg rax,r8"))
		{
			ai->b[0] = 0x49;
			ai->b[1] = 0x90;
			return 2;
		}
		else if (nmd_strcmp(ai->s, "xchg r8d,eax") || nmd_strcmp(ai->s, "xchg eax,r8d"))
		{
			ai->b[0] = 0x41;
			ai->b[1] = 0x90;
			return 2;
		}
		else if (nmd_strcmp(ai->s, "pushfq"))
		{
			ai->b[0] = 0x9c;
			return 1;
		}
		else if (nmd_strcmp(ai->s, "popfq"))
		{
			ai->b[0] = 0x9d;
			return 1;
		}
		else if (nmd_strcmp(ai->s, "iretq"))
		{
			ai->b[0] = 0x48;
			ai->b[1] = 0xcf;
			return 2;
		}
		else if (nmd_strcmp(ai->s, "cdqe"))
		{
			ai->b[0] = 0x48;
			ai->b[1] = 0x98;
			return 2;
		}
		else if (nmd_strcmp(ai->s, "cqo"))
		{
			ai->b[0] = 0x48;
			ai->b[1] = 0x99;
			return 2;
		}
	}
	else /* Only x86-16 or x86-32. */
	{
		if (nmd_strcmp(ai->s, "pushad"))
		{
			ai->b[0] = 0x60;
			return 1;
		}
		else if (nmd_strcmp(ai->s, "pusha"))
		{
			ai->b[0] = 0x66;
			ai->b[1] = 0x60;
			return 2;
		}
		else if (nmd_strcmp(ai->s, "popad"))
		{
			ai->b[0] = 0x61;
			return 1;
		}
		else if (nmd_strcmp(ai->s, "popa"))
		{
			ai->b[0] = 0x66;
			ai->b[1] = 0x62;
			return 2;
		}
		else if (nmd_strcmp(ai->s, "pushfd"))
		{
			if (ai->mode == NMD_X86_MODE_16)
			{
				ai->b[0] = 0x66;
				ai->b[1] = 0x9c;
				return 2;
			}
			else
			{
				ai->b[0] = 0x9c;
				return 1;
			}
		}
		else if (nmd_strcmp(ai->s, "popfd"))
		{
			if (ai->mode == NMD_X86_MODE_16)
			{
				ai->b[0] = 0x66;
				ai->b[1] = 0x9d;
				return 2;
			}
			else
			{
				ai->b[0] = 0x9d;
				return 1;
			}
		}
		else if (nmd_strstr(ai->s, "inc ") == ai->s || nmd_strstr(ai->s, "dec ") == ai->s)
		{
			ai->s += 4;
			for (i = 0; i < NMD_NUM_ELEMENTS(reg32); i++)
			{
				if (nmd_strcmp(ai->s, reg32[i]))
				{
					ai->b[0] = (*(ai->s - 4) == 'i' ? 0x40 : 0x48) + (uint8_t)i;
					return 1;
				}
			}

			for (i = 0; i < NMD_NUM_ELEMENTS(reg16); i++)
			{
				if (nmd_strcmp(ai->s, reg16[i]))
				{
					ai->b[0] = 0x66;
					ai->b[1] = (*(ai->s - 4) == 'i' ? 0x40 : 0x48) + (uint8_t)i;
					return 2;
				}
			}
		}
	}

	if (nmd_strcmp(ai->s, "pushf"))
	{
		if (ai->mode == NMD_X86_MODE_16)
		{
			ai->b[0] = 0x9c;
			return 1;
		}
		else
		{
			ai->b[0] = 0x66;
			ai->b[1] = 0x9c;
			return 2;
		}
	}
	else if (nmd_strcmp(ai->s, "popf"))
	{
		if (ai->mode == NMD_X86_MODE_16)
		{
			ai->b[0] = 0x9d;
			return 1;
		}
		else
		{
			ai->b[0] = 0x66;
			ai->b[1] = 0x9d;
			return 2;
		}
	}

	typedef struct NMD_StringBytePair { const char* s; uint8_t b; } NMD_StringBytePair;

	const NMD_StringBytePair op1SingleByte[] = {
		{ "int3",    0xcc },
		{ "nop",     0x90 },
		{ "ret",     0xc3 },
		{ "retf",    0xcb },
		{ "ret far", 0xcb },
		{ "leave",   0xc9 },
		{ "int1",    0xf1 },
		{ "push es", 0x06 },
		{ "push ss", 0x16 },
		{ "push ds", 0x1e },
		{ "push cs", 0x0e },
		{ "pop es",  0x07 },
		{ "pop ss",  0x17 },
		{ "pop ds",  0x1f },
		{ "daa",     0x27 },
		{ "aaa",     0x37 },
		{ "das",     0x2f },
		{ "aas",     0x3f },
		{ "xlat",    0xd7 },
		{ "fwait",   0x9b },
		{ "hlt",     0xf4 },
		{ "cmc",     0xf5 },
		{ "sahf",    0x9e },
		{ "lahf",    0x9f },
		{ "into",    0xce },
		{ "cwde",    0x98 },
		{ "cdq",     0x99 },
		{ "salc",    0xd6 },
		{ "slc",     0xf8 },
		{ "stc",     0xf9 },
		{ "cli",     0xfa },
		{ "sti",     0xfb },
		{ "cld",     0xfc },
		{ "std",     0xfd },
	};

	const NMD_StringBytePair op2SingleByte[] = {
		{ "syscall",  0x05 },
		{ "clts",     0x06 },
		{ "sysret",   0x07 },
		{ "invd",     0x08 },
		{ "wbinvd",   0x09 },
		{ "ud2",      0x0b },
		{ "femms",    0x0e },
		{ "wrmsr",    0x30 },
		{ "rdtsc",    0x31 },
		{ "rdmsr",    0x32 },
		{ "rdpmc",    0x33 },
		{ "sysenter", 0x34 },
		{ "sysexit",  0x35 },
		{ "getsec",   0x37 },
		{ "emms",     0x77 },
		{ "push fs",  0xa0 },
		{ "pop fs",   0xa1 },
		{ "cpuid",    0xa2 },
		{ "push gs",  0xa8 },
		{ "pop gs",   0xa9 },
		{ "rsm",      0xaa }
	};

	for (i = 0; i < NMD_NUM_ELEMENTS(op1SingleByte); i++)
	{
		if (nmd_strcmp(ai->s, op1SingleByte[i].s))
		{
			ai->b[0] = op1SingleByte[i].b;
			return 1;
		}
	}

	for (i = 0; i < NMD_NUM_ELEMENTS(op2SingleByte); i++)
	{
		if (nmd_strcmp(ai->s, op2SingleByte[i].s))
		{
			ai->b[0] = 0x0f;
			ai->b[1] = op2SingleByte[i].b;
			return 2;
		}
	}

	if (nmd_strcmp(ai->s, "pause"))
	{
		ai->b[0] = 0xf3;
		ai->b[1] = 0x90;
		return 2;
	}
	else if (nmd_strcmp(ai->s, "iret"))
	{
		if (ai->mode == NMD_X86_MODE_16)
		{
			ai->b[0] = 0xcf;
			return 1;
		}
		else
		{
			ai->b[0] = 0x66;
			ai->b[1] = 0xcf;
			return 2;
		}
	}
	else if (nmd_strcmp(ai->s, "iretd"))
	{
		if (ai->mode == NMD_X86_MODE_16)
		{
			ai->b[0] = 0x66;
			ai->b[1] = 0xcf;
			return 2;
		}
		else
		{
			ai->b[0] = 0xcf;
			return 1;
		}
	}
	else if (nmd_strcmp(ai->s, "cbw"))
	{
		ai->b[0] = 0x66;
		ai->b[1] = 0x98;
		return 2;
	}
	else if (nmd_strcmp(ai->s, "cwd"))
	{
		ai->b[0] = 0x66;
		ai->b[1] = 0x99;
		return 2;
	}
	else if (nmd_strcmp(ai->s, "pushf"))
	{
		ai->b[0] = 0x66;
		ai->b[1] = 0x9c;
		return 2;
	}
	else if (nmd_strcmp(ai->s, "popf"))
	{
		ai->b[0] = 0x66;
		ai->b[1] = 0x9d;
		return 2;
	}
	else if (nmd_strstr(ai->s, "push ") == ai->s)
	{
		size_t numDigits = 0;
		int64_t num = 0;
		if (parseNumber(ai->s + 5, &num, &numDigits))
		{
			if (*(ai->s + numDigits) != '\0' || !(num >= -(1 << 31) && num <= ((int64_t)1 << 31) - 1))
				return 0;

			if (num >= -(1 << 7) && num <= (1 << 7) - 1)
			{
				ai->b[0] = 0x6a;
				*(int8_t*)(ai->b + 1) = (int8_t)num;
				return 2;
			}
			else
			{
				ai->b[0] = 0x68;
				*(int32_t*)(ai->b + 1) = (int32_t)num;
				return 5;
			}
		}

		size_t n = assembleReg(ai, 0x50);
		if (n > 0)
			return n;

	}
	else if (nmd_strstr(ai->s, "pop ") == ai->s)
	{
		ai->s += 3;
		return assembleReg(ai, 0x58);
	}

	if (ai->s[0] == 'j')
	{
		const char* s = 0;
		for (i = 0; i < NMD_NUM_ELEMENTS(conditionSuffixes); i++)
		{
			if (nmd_strstr_ex(ai->s + 1, conditionSuffixes[i], &s) == ai->s + 1)
			{
				if (s[0] != ' ')
					return 0;


				int64_t num;
				size_t numDigits;
				if (!parseNumber(s + 1, &num, &numDigits))
					return 0;

				const int64_t delta = num - ai->runtimeAddress;
				if (delta >= -(1 << 7) + 2 && delta <= (1 << 7) - 1 + 2)
				{
					ai->b[0] = 0x70 + (uint8_t)i;
					*(int8_t*)(ai->b + 1) = (int8_t)(delta - 2);
					return 2;
				}
				else if (delta >= -(1 << 31) + 6 && delta <= ((size_t)(1) << 31) - 1 + 6)
				{
					ai->b[0] = 0x0f;
					ai->b[1] = 0x80 + (uint8_t)i;
					*(int32_t*)(ai->b + 2) = (int32_t)(delta - 6);
					return 6;
				}
				else
					return 0;
			}
		}
	}

	/* try to parse 00 00*/
	if (nmd_strstr("add", ai->s) == ai->s)
	{

	}

	return 0;
}

/*
Assembles an instruction from a string. Returns the number of bytes written to the buffer on success, zero otherwise. Instructions can be separated using either the ';' or '\n' character.
Parameters:
- string         [in]         A pointer to a string that represents a instruction in assembly language.
- buffer         [out]        A pointer to a buffer that receives the encoded instructions.
- bufferSize     [in]         The size of the buffer in bytes.
- runtimeAddress [in]         The instruction's runtime address. You may use 'NMD_X86_INVALID_RUNTIME_ADDRESS'.
- mode           [in]         The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
- count          [in/out/opt] A pointer to a variable that on input is the maximum number of instructions that can be parsed(or zero for unlimited instructions), and on output is the number of instructions parsed. This parameter may be 0(zero).
*/
size_t nmd_x86_assemble(const char* string, void* buffer, size_t bufferSize, uint64_t runtimeAddress, NMD_X86_MODE mode, size_t* count)
{
	char parsedString[128];
	const uint8_t* const bufferEnd = (uint8_t*)buffer + bufferSize;
	uint8_t* b = (uint8_t*)buffer;
	size_t remainingSize;

	uint8_t tempBuffer[NMD_X86_MAXIMUM_INSTRUCTION_LENGTH];

	AssembleInfo ai;
	ai.s = parsedString;
	ai.mode = mode;
	ai.runtimeAddress = runtimeAddress;
	ai.b = tempBuffer;

	size_t numInstructions = 0;
	const size_t numMaxInstructions = (count && *count != 0) ? *count : (size_t)(-1);

	while (string[0] != '\0')
	{
		if (numInstructions == numMaxInstructions)
			break;

		remainingSize = bufferEnd - b;

		/* Copy 'string' to 'buffer' converting it to lowercase and removing unwanted spaces. If the instruction separator character ';' and '\n' is found, stop. */
		size_t length = 0;
		bool allowSpace = false;
		for (; *string; string++)
		{
			const char c = *string;
			if (c == ';' || c == '\n')
				break;
			else if (c == ' ' && !allowSpace)
				continue;

			if (length >= 128)
				return 0;

			const char newChar = (c >= 'A' && c <= 'Z') ? c + 0x20 : c;
			parsedString[length++] = newChar;
			allowSpace = (NMD_IS_LOWERCASE(newChar) || NMD_IS_DECIMAL_NUMBER(newChar)) && (NMD_IS_LOWERCASE(*(string + 2)) || NMD_IS_DECIMAL_NUMBER(*(string + 2)));
		}

		if (*string != '\0')
			string++;

		/* If the last character is a ' '(space), remove it. */
		if (length > 0 && parsedString[length - 1] == ' ')
			length--;

		/* After all of the string manipulation, place the null character. */
		parsedString[length] = '\0';

		const size_t numBytes = assembleSingle(&ai);
		if (numBytes == 0 || numBytes > remainingSize)
			return 0;

		size_t i = 0;
		for (; i < numBytes; i++)
			b[i] = tempBuffer[i];

		b += numBytes;

		numInstructions++;
	}

	if (count)
		*count = numInstructions;

	return (size_t)(b - (uint8_t*)buffer);
}

void parseOperandSegmentRegister(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	if (instruction->segmentOverride)
		operand->fields.reg = (uint8_t)(NMD_X86_REG_ES + nmd_getBitNumber(instruction->segmentOverride));
	else
		operand->fields.reg = (uint8_t)(!(instruction->prefixes & NMD_X86_PREFIXES_REX_B) && (instruction->modrm.fields.rm == 0b100 || instruction->modrm.fields.rm == 0b101) ? NMD_X86_REG_SS : NMD_X86_REG_DS);
}

void parseModrmUpper32(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_MEMORY;

	if (instruction->hasSIB)
	{
		operand->size++;

		if (instruction->sib.fields.base == 0b101)
		{
			if (instruction->modrm.fields.mod != 0b00)
				operand->fields.mem.base = (uint8_t)(instruction->mode == NMD_X86_MODE_64 && !(instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? NMD_X86_REG_R13 : NMD_X86_REG_RBP) : NMD_X86_REG_EBP);
		}
		else
			operand->fields.mem.base = (uint8_t)((instruction->mode == NMD_X86_MODE_64 && !(instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? NMD_X86_REG_R8 : NMD_X86_REG_RAX) : NMD_X86_REG_EAX) + instruction->sib.fields.base);

		if (instruction->sib.fields.index != 0b100)
			operand->fields.mem.index = (uint8_t)((instruction->mode == NMD_X86_MODE_64 && !(instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (instruction->prefixes & NMD_X86_PREFIXES_REX_X ? NMD_X86_REG_R8 : NMD_X86_REG_RAX) : NMD_X86_REG_EAX) + instruction->sib.fields.index);

		if (instruction->prefixes & NMD_X86_PREFIXES_REX_X && instruction->sib.fields.index == 0b100)
		{
			operand->fields.mem.index = (uint8_t)NMD_X86_REG_R12;
			operand->fields.mem.scale = instruction->sib.fields.scale;
		}
	}
	else if (!(instruction->modrm.fields.mod == 0b00 && instruction->modrm.fields.rm == 0b101))
	{
		if ((instruction->prefixes & (NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE | NMD_X86_PREFIXES_REX_B)) == (NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE | NMD_X86_PREFIXES_REX_B) && instruction->mode == NMD_X86_MODE_64)
			operand->fields.mem.base = (uint8_t)(NMD_X86_REG_R8D + instruction->modrm.fields.rm);
		else
			operand->fields.mem.base = (uint8_t)((instruction->mode == NMD_X86_MODE_64 && !(instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? NMD_X86_REG_R8 : NMD_X86_REG_RAX) : NMD_X86_REG_EAX) + instruction->modrm.fields.rm);
	}

	parseOperandSegmentRegister(instruction, operand);

	operand->fields.mem.disp = instruction->displacement;
	operand->size += (uint8_t)(instruction->dispMask);
}

void parseMemoryOperand(const NMD_X86Instruction* instruction, NMD_X86Operand* operand, uint8_t mod11baseReg)
{
	/* At least one byte is used for ModR/M. */
	operand->size = 1;

	if (instruction->modrm.fields.mod == 0b11)
	{
		operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
		operand->fields.reg = mod11baseReg + instruction->modrm.fields.rm;
	}
	else
		parseModrmUpper32(instruction, operand);
}

void parseOperandEb(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	parseMemoryOperand(instruction, operand, NMD_X86_REG_AL);
}

void parseOperandEw(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	parseMemoryOperand(instruction, operand, NMD_X86_REG_AX);
}

void parseOperandEv(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	parseMemoryOperand(instruction, operand, (uint8_t)(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_AX : NMD_X86_REG_EAX));
}

void parseOperandEy(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	parseMemoryOperand(instruction, operand, (uint8_t)(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_AX : NMD_X86_REG_EAX));
}

void parseOperandQq(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	parseMemoryOperand(instruction, operand, NMD_X86_REG_MM0);
}

void parseOperandWdq(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	parseMemoryOperand(instruction, operand, NMD_X86_REG_XMM0);
}

void parseOperandGb(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = NMD_X86_REG_AL + instruction->modrm.fields.reg;
	operand->size = 1;
}

void parseOperandGd(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = NMD_X86_REG_EAX + instruction->modrm.fields.reg;
	operand->size = 1;
}

void parseOperandGw(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = NMD_X86_REG_AX + instruction->modrm.fields.reg;
	operand->size = 1;
}

void parseOperandGv(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	if (instruction->prefixes & NMD_X86_PREFIXES_REX_B)
		operand->fields.reg = (uint8_t)((!(instruction->prefixes & NMD_X86_PREFIXES_REX_W) ? NMD_X86_REG_R8D : NMD_X86_REG_R8) + instruction->modrm.fields.reg);
	else
		operand->fields.reg = (uint8_t)((instruction->operandSize64 ? NMD_X86_REG_RAX : (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && instruction->mode != NMD_X86_MODE_16 ? NMD_X86_REG_AX : NMD_X86_REG_EAX)) + instruction->modrm.fields.reg);
	operand->size = 1;
}

void parseOperandRv(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	if (instruction->prefixes & NMD_X86_PREFIXES_REX_R)
		operand->fields.reg = (uint8_t)((!(instruction->prefixes & NMD_X86_PREFIXES_REX_W) ? NMD_X86_REG_R8D : NMD_X86_REG_R8) + instruction->modrm.fields.rm);
	else
		operand->fields.reg = (uint8_t)((instruction->operandSize64 ? NMD_X86_REG_RAX : ((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && instruction->mode != NMD_X86_MODE_16) || (instruction->mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? NMD_X86_REG_AX : NMD_X86_REG_EAX)) + instruction->modrm.fields.rm);
	operand->size = 1;
}

void parseOperandGy(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = (uint8_t)((instruction->mode == NMD_X86_MODE_64 ? NMD_X86_REG_RAX : NMD_X86_REG_EAX) + instruction->modrm.fields.reg);
	operand->size = 1;
}

void parseOperandPq(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = NMD_X86_REG_MM0 + instruction->modrm.fields.reg;
	operand->size = 1;
}

void parseOperandNq(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = NMD_X86_REG_MM0 + instruction->modrm.fields.rm;
	operand->size = 1;
}

void parseOperandVdq(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = NMD_X86_REG_XMM0 + instruction->modrm.fields.reg;
	operand->size = 1;
}

void parseOperandUdq(const NMD_X86Instruction* instruction, NMD_X86Operand* operand)
{
	operand->type = NMD_X86_OPERAND_TYPE_REGISTER;
	operand->fields.reg = NMD_X86_REG_XMM0 + instruction->modrm.fields.rm;
	operand->size = 1;
}

void decodeConditionalFlag(NMD_X86Instruction* instruction, const uint8_t condition)
{
	switch (condition)
	{
	case 0x0: instruction->testedFlags.fields.OF = 1; break;                                                                           /* Jump if overflow (OF=1) */
	case 0x1: instruction->testedFlags.fields.OF = 1; break;                                                                           /* Jump if not overflow (OF=0) */
	case 0x2: instruction->testedFlags.fields.CF = 1; break;                                                                           /* Jump if not above or equal (CF=1) */
	case 0x3: instruction->testedFlags.fields.CF = 1; break;                                                                           /* Jump if not below (CF=0) */
	case 0x4: instruction->testedFlags.fields.ZF = 1; break;                                                                           /* Jump if equal (ZF=1) */
	case 0x5: instruction->testedFlags.fields.ZF = 1; break;                                                                           /* Jump if not equal (ZF=0) */
	case 0x6: instruction->testedFlags.fields.CF = instruction->testedFlags.fields.ZF = 1; break;                                      /* Jump if not above (CF=1 or ZF=1) */
	case 0x7: instruction->testedFlags.fields.CF = instruction->testedFlags.fields.ZF = 1; break;                                      /* Jump if not below or equal (CF=0 and ZF=0) */
	case 0x8: instruction->testedFlags.fields.SF = 1; break;                                                                           /* Jump if sign (SF=1) */
	case 0x9: instruction->testedFlags.fields.SF = 1; break;                                                                           /* Jump if not sign (SF=0) */
	case 0xa: instruction->testedFlags.fields.PF = 1; break;                                                                           /* Jump if parity/parity even (PF=1) */
	case 0xb: instruction->testedFlags.fields.PF = 1; break;                                                                           /* Jump if parity odd (PF=0) */
	case 0xc: instruction->testedFlags.fields.SF = instruction->testedFlags.fields.OF = 1; break;                                      /* Jump if not greater or equal (SF != OF) */
	case 0xd: instruction->testedFlags.fields.SF = instruction->testedFlags.fields.OF = 1; break;                                      /* Jump if not less (SF=OF) */
	case 0xe: instruction->testedFlags.fields.ZF = instruction->testedFlags.fields.SF = instruction->testedFlags.fields.OF = 1; break; /* Jump if not greater (ZF=1 or SF != OF) */
	case 0xf: instruction->testedFlags.fields.ZF = instruction->testedFlags.fields.SF = instruction->testedFlags.fields.OF = 1; break; /* Jump if not less or equal (ZF=0 and SF=OF) */
	}
}

/* 'remaningSize' in the context of this function is the number of bytes the instruction takes not counting prefixes and opcode. */
bool parseModrm(const uint8_t** b, NMD_X86Instruction* const instruction, const size_t remainingSize)
{
	if (remainingSize == 0)
		return false;

	instruction->hasModrm = true;
	instruction->modrm.modrm = *++*b;
	const bool addressPrefix = (bool)(instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE);

	if (instruction->mode == NMD_X86_MODE_16)
	{
		if (instruction->modrm.fields.mod != 0b11)
		{
			if (instruction->modrm.fields.mod == 0b00)
			{
				if (instruction->modrm.fields.rm == 0b110)
					instruction->dispMask = NMD_X86_DISP16;
			}
			else
				instruction->dispMask = (uint8_t)(instruction->modrm.fields.mod == 0b01 ? NMD_X86_DISP8 : NMD_X86_DISP16);
		}
	}
	else
	{
		if (addressPrefix && instruction->mode == NMD_X86_MODE_32)
		{
			if ((instruction->modrm.fields.mod == 0b00 && instruction->modrm.fields.rm == 0b110) || instruction->modrm.fields.mod == 0b10)
				instruction->dispMask = NMD_X86_DISP16;
			else if (instruction->modrm.fields.mod == 0b01)
				instruction->dispMask = NMD_X86_DISP8;
		}
		else /*if (!addressPrefix || (addressPrefix && **b >= 0x40) || (addressPrefix && instruction->mode == NMD_X86_MODE_64)) */
		{
			/* Check for SIB byte */
			if (instruction->modrm.modrm < 0xC0 && instruction->modrm.fields.rm == 0b100 && (!addressPrefix || (addressPrefix && instruction->mode == NMD_X86_MODE_64)))
			{
				if (remainingSize < 2)
					return false;

				instruction->hasSIB = true;
				instruction->sib.sib = *++*b;
			}

			if (instruction->modrm.fields.mod == 0b01) /* disp8 (ModR/M) */
				instruction->dispMask = NMD_X86_DISP8;
			else if ((instruction->modrm.fields.mod == 0b00 && instruction->modrm.fields.rm == 0b101) || instruction->modrm.fields.mod == 0b10) /* disp16,32 (ModR/M) */
				instruction->dispMask = (uint8_t)(addressPrefix && !(instruction->mode == NMD_X86_MODE_64 && instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? NMD_X86_DISP16 : NMD_X86_DISP32);
			else if (instruction->hasSIB && instruction->sib.fields.base == 0b101) /* disp8,32 (SIB) */
				instruction->dispMask = (uint8_t)(instruction->modrm.fields.mod == 0b01 ? NMD_X86_DISP8 : NMD_X86_DISP32);
		}
	}

	if (remainingSize - (instruction->hasSIB ? 2 : 1) < instruction->dispMask)
		return false;

	size_t i = 0;
	for (; i < (size_t)instruction->dispMask; i++, (*b)++)
		((uint8_t*)(&instruction->displacement))[i] = *(*b + 1);

	return true;
}

/*
Decodes an instruction. Returns true if the instruction is valid, false otherwise.
Parameters:
- buffer      [in]  A pointer to a buffer containing a encoded instruction.
- bufferSize  [in]  The buffer's size in bytes.
- instruction [out] A pointer to a variable of type 'NMD_X86Instruction' that receives information about the instruction.
- mode        [in]  The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
- flags       [in]  A mask of 'NMD_X86_DECODER_FLAGS_XXX' that specifies which features the decoder is allowed to use. If uncertain, use 'NMD_X86_DECODER_FLAGS_MINIMAL'.
*/
bool nmd_x86_decode_buffer(const void* buffer, size_t bufferSize, NMD_X86Instruction* instruction, NMD_X86_MODE mode, uint32_t flags)
{
	if (bufferSize == 0)
		return false;

	/* Clear 'instruction'. */
	size_t i = 0;
	for (; i < sizeof(NMD_X86Instruction); i++)
		((uint8_t*)(instruction))[i] = 0x00;

	instruction->mode = (uint8_t)mode;

	const uint8_t* b = (const uint8_t*)(buffer);

	/* Parse legacy prefixes & REX prefixes. */
	i = 0;
	for (; i < NMD_X86_MAXIMUM_INSTRUCTION_LENGTH; i++, b++)
	{
		switch (*b)
		{
		case 0xF0: instruction->prefixes = (instruction->prefixes | (instruction->simdPrefix = NMD_X86_PREFIXES_LOCK)); continue;
		case 0xF2: instruction->prefixes = (instruction->prefixes | (instruction->simdPrefix = NMD_X86_PREFIXES_REPEAT_NOT_ZERO)), instruction->repeatPrefix = false; continue;
		case 0xF3: instruction->prefixes = (instruction->prefixes | (instruction->simdPrefix = NMD_X86_PREFIXES_REPEAT)), instruction->repeatPrefix = true; continue;
		case 0x2E: instruction->prefixes = (instruction->prefixes | (instruction->segmentOverride = NMD_X86_PREFIXES_CS_SEGMENT_OVERRIDE)); continue;
		case 0x36: instruction->prefixes = (instruction->prefixes | (instruction->segmentOverride = NMD_X86_PREFIXES_SS_SEGMENT_OVERRIDE)); continue;
		case 0x3E: instruction->prefixes = (instruction->prefixes | (instruction->segmentOverride = NMD_X86_PREFIXES_DS_SEGMENT_OVERRIDE)); continue;
		case 0x26: instruction->prefixes = (instruction->prefixes | (instruction->segmentOverride = NMD_X86_PREFIXES_ES_SEGMENT_OVERRIDE)); continue;
		case 0x64: instruction->prefixes = (instruction->prefixes | (instruction->segmentOverride = NMD_X86_PREFIXES_FS_SEGMENT_OVERRIDE)); continue;
		case 0x65: instruction->prefixes = (instruction->prefixes | (instruction->segmentOverride = NMD_X86_PREFIXES_GS_SEGMENT_OVERRIDE)); continue;
		case 0x66: instruction->prefixes = (instruction->prefixes | (instruction->simdPrefix = NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)), instruction->operandSize64 = false; continue;
		case 0x67: instruction->prefixes = (instruction->prefixes | NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE); continue;
		default:
			if ((mode == NMD_X86_MODE_64) && NMD_R(*b) == 4) /* 0x40 */
			{
				instruction->hasRex = true;
				instruction->rex = *b;
				instruction->prefixes = (instruction->prefixes & ~(NMD_X86_PREFIXES_REX_B | NMD_X86_PREFIXES_REX_X | NMD_X86_PREFIXES_REX_R | NMD_X86_PREFIXES_REX_W));

				if (*b & 0b0001) /* Bit position 0. */
					instruction->prefixes = instruction->prefixes | NMD_X86_PREFIXES_REX_B;
				if (*b & 0b0010) /* Bit position 1. */
					instruction->prefixes = instruction->prefixes | NMD_X86_PREFIXES_REX_X;
				if (*b & 0b0100) /* Bit position 2. */
					instruction->prefixes = instruction->prefixes | NMD_X86_PREFIXES_REX_R;
				if (*b & 0b1000) /* Bit position 3. */
				{
					instruction->prefixes = instruction->prefixes | NMD_X86_PREFIXES_REX_W;
					instruction->operandSize64 = true;
				}

				continue;
			}
		}

		break;
	}

	instruction->numPrefixes = (uint8_t)((ptrdiff_t)(b)-(ptrdiff_t)(buffer));

	const size_t remainingValidBytes = (NMD_X86_MAXIMUM_INSTRUCTION_LENGTH - instruction->numPrefixes);
	if (remainingValidBytes == 0)
		return false;

	const size_t remainingBufferSize = bufferSize - instruction->numPrefixes;
	if (remainingBufferSize == 0)
		return false;

	const size_t remainingSize = remainingValidBytes < remainingBufferSize ? remainingValidBytes : remainingBufferSize;

	/* Assume NMD_X86_INSTRUCTION_ENCODING_LEGACY. */
	instruction->encoding = NMD_X86_ENCODING_LEGACY;

	/* Opcode byte. This variable is used because it's easier to write 'op' than 'instruction->opcode'. */
	uint8_t op = 0;

	/* Parse opcode. */
	if (*b == 0x0F) /* 2 or 3 byte opcode. */
	{
		if (remainingSize < 2)
			return false;

		b++;

		if (*b == 0x38 || *b == 0x3A) /* 3 byte opcode. */
		{
			if (remainingSize < 4)
				return false;

			instruction->opcodeMap = (uint8_t)(*b == 0x38 ? NMD_X86_OPCODE_MAP_0F38 : NMD_X86_OPCODE_MAP_0F3A);
			instruction->opcodeSize = 3;
			instruction->opcode = *++b;

			op = instruction->opcode;

			if (!parseModrm(&b, instruction, remainingSize - 3))
				return false;

			const NMD_Modrm modrm = instruction->modrm;
			if (instruction->opcodeMap == NMD_X86_OPCODE_MAP_0F38)
			{
#ifndef NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK
				if (flags & NMD_X86_DECODER_FLAGS_VALIDITY_CHECK)
				{
					/* Check if the instruction is invalid. */
					if (op == 0x36)
					{
						return false;
					}
					else if (op <= 0xb || (op >= 0x1c && op <= 0x1e))
					{
						if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT || instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
							return false;
					}
					else if (op >= 0xc8 && op <= 0xcd)
					{
						if (instruction->simdPrefix)
							return false;
					}
					else if (op == 0x10 || op == 0x14 || op == 0x15 || op == 0x17 || (op >= 0x20 && op <= 0x25) || op == 0x28 || op == 0x29 || op == 0x2b || NMD_R(op) == 3 || op == 0x40 || op == 0x41 || op == 0xcf || (op >= 0xdb && op <= 0xdf))
					{
						if (instruction->simdPrefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
							return false;
					}
					else if (op == 0x2a || (op >= 0x80 && op <= 0x82))
					{
						if (modrm.fields.mod == 0b11 || instruction->simdPrefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
							return false;
					}
					else if (op == 0xf0 || op == 0xf1)
					{
						if (modrm.fields.mod == 0b11 && (instruction->simdPrefix == NMD_X86_PREFIXES_NONE || instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE))
							return false;
						else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT)
							return false;
					}
					else if (op == 0xf5 || op == 0xf8)
					{
						if (instruction->simdPrefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || modrm.fields.mod == 0b11)
							return false;
					}
					else if (op == 0xf6)
					{
						if (instruction->simdPrefix == NMD_X86_PREFIXES_NONE && modrm.fields.mod == 0b11)
							return false;
						else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
							return false;
					}
					else if (op == 0xf9)
					{
						if (instruction->simdPrefix != NMD_X86_PREFIXES_NONE || modrm.fields.mod == 0b11)
							return false;
					}
					else
						return false;
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID
				if (flags & NMD_X86_DECODER_FLAGS_INSTRUCTION_ID)
				{
					if (NMD_R(op) == 0x00)
						instruction->id = NMD_X86_INSTRUCTION_PSHUFB + op;
					else if (op >= 0x1c && op <= 0x1e)
						instruction->id = NMD_X86_INSTRUCTION_PABSB + (op - 0x1c);
					else if (NMD_R(op) == 2)
						instruction->id = NMD_X86_INSTRUCTION_PMOVSXBW + NMD_C(op);
					else if (NMD_R(op) == 3)
						instruction->id = NMD_X86_INSTRUCTION_PMOVZXBW + NMD_C(op);
					else if (NMD_R(op) == 8)
						instruction->id = NMD_X86_INSTRUCTION_INVEPT + NMD_C(op);
					else if (NMD_R(op) == 0xc)
						instruction->id = NMD_X86_INSTRUCTION_SHA1NEXTE + (NMD_C(op) - 8);
					else if (NMD_R(op) == 0xd)
						instruction->id = NMD_X86_INSTRUCTION_AESIMC + (NMD_C(op) - 0xb);
					else
					{
						switch (op)
						{
						case 0x10: instruction->id = NMD_X86_INSTRUCTION_PBLENDVB; break;
						case 0x14: instruction->id = NMD_X86_INSTRUCTION_BLENDVPS; break;
						case 0x15: instruction->id = NMD_X86_INSTRUCTION_BLENDVPD; break;
						case 0x17: instruction->id = NMD_X86_INSTRUCTION_PTEST; break;
						case 0x40: instruction->id = NMD_X86_INSTRUCTION_PMULLD; break;
						case 0x41: instruction->id = NMD_X86_INSTRUCTION_PHMINPOSUW; break;
						case 0xf0: case 0xf1: instruction->id = (uint16_t)((instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || instruction->simdPrefix == 0x00) ? NMD_X86_INSTRUCTION_MOVBE : NMD_X86_INSTRUCTION_CRC32); break;
						case 0xf6: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_ADCX : NMD_X86_INSTRUCTION_ADOX); break;
						}
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_CPU_FLAGS
				if (flags & NMD_X86_DECODER_FLAGS_CPU_FLAGS)
				{
					if (op == 0x80 || op == 0x81) /* invept,invvpid */
					{
						instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_ZF;
						instruction->clearedFlags.eflags = NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_OF;
					}
					else if (op == 0xf6)
					{
						if (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE) /* adcx */
							instruction->modifiedFlags.eflags = instruction->testedFlags.eflags = NMD_X86_EFLAGS_CF;
						if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT) /* adox */
							instruction->modifiedFlags.eflags = instruction->testedFlags.eflags = NMD_X86_EFLAGS_OF;
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_CPU_FLAGS */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS
				if (flags & NMD_X86_DECODER_FLAGS_OPERANDS)
				{
					instruction->numOperands = 2;
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ_WRITE;
					instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;

					if (NMD_R(op) == 0 || (op >= 0x1c && op <= 0x1e))
					{
						parseOperandPq(instruction, &instruction->operands[0]);
						parseOperandQq(instruction, &instruction->operands[1]);
					}
					else if (NMD_R(op) == 8)
					{
						parseOperandGy(instruction, &instruction->operands[0]);
						parseModrmUpper32(instruction, &instruction->operands[1]);
					}
					else if (NMD_R(op) >= 1 && NMD_R(op) <= 0xe)
					{
						parseOperandVdq(instruction, &instruction->operands[0]);
						parseOperandWdq(instruction, &instruction->operands[1]);
					}
					else if (op == 0xf6)
					{
						parseOperandGy(instruction, &instruction->operands[!instruction->simdPrefix ? 1 : 0]);
						parseOperandEy(instruction, &instruction->operands[!instruction->simdPrefix ? 0 : 1]);
					}
					else if (op == 0xf0 || op == 0xf1)
					{
						if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO || (instruction->prefixes & (NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE | NMD_X86_PREFIXES_REPEAT_NOT_ZERO)) == (NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE | NMD_X86_PREFIXES_REPEAT_NOT_ZERO))
						{
							parseOperandGd(instruction, &instruction->operands[0]);
							if (op == 0xf0)
								parseOperandEb(instruction, &instruction->operands[1]);
							else if (instruction->prefixes == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
								parseOperandEy(instruction, &instruction->operands[1]);
							else
								parseOperandEw(instruction, &instruction->operands[1]);
						}
						else
						{
							if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
								parseOperandGw(instruction, &instruction->operands[op == 0xf0 ? 0 : 1]);
							else
								parseOperandGy(instruction, &instruction->operands[op == 0xf0 ? 0 : 1]);

							parseMemoryOperand(instruction, &instruction->operands[op == 0xf0 ? 1 : 0], (uint8_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_AX : (instruction->operandSize64 ? NMD_X86_REG_RAX : NMD_X86_REG_EAX)));
						}
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS */
			}
			else /* 0x3a */
			{
				if (remainingSize < 5)
					return false;

				instruction->immMask = NMD_X86_IMM8;

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK
				if (flags & NMD_X86_DECODER_FLAGS_VALIDITY_CHECK)
				{
					/* Check if the instruction is invalid. */
					if ((op >= 0x8 && op <= 0xe) || (op >= 0x14 && op <= 0x17) || (op >= 0x20 && op <= 0x22) || (op >= 0x40 && op <= 0x42) || op == 0x44 || (op >= 0x60 && op <= 0x63) || op == 0xdf || op == 0xce || op == 0xcf)
					{
						if (instruction->simdPrefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
							return false;
					}
					else if (op == 0x0f || op == 0xcc)
					{
						if (instruction->simdPrefix)
							return false;
					}
					else
						return false;
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID
				if (flags & NMD_X86_DECODER_FLAGS_INSTRUCTION_ID)
				{
					if (NMD_R(op) == 0)
						instruction->id = NMD_X86_INSTRUCTION_ROUNDPS + (op - 8);
					else if (NMD_R(op) == 4)
						instruction->id = NMD_X86_INSTRUCTION_DPPS + NMD_C(op);
					else if (NMD_R(op) == 6)
						instruction->id = NMD_X86_INSTRUCTION_PCMPESTRM + NMD_C(op);
					else
					{
						switch (op)
						{
						case 0x14: instruction->id = NMD_X86_INSTRUCTION_PEXTRB; break;
						case 0x15: instruction->id = NMD_X86_INSTRUCTION_PEXTRW; break;
						case 0x16: instruction->id = (uint16_t)(instruction->prefixes & NMD_X86_PREFIXES_REX_W ? NMD_X86_INSTRUCTION_PEXTRQ : NMD_X86_INSTRUCTION_PEXTRD); break;
						case 0x17: instruction->id = NMD_X86_INSTRUCTION_EXTRACTPS; break;
						case 0x20: instruction->id = NMD_X86_INSTRUCTION_PINSRB; break;
						case 0x21: instruction->id = NMD_X86_INSTRUCTION_INSERTPS; break;
						case 0x22: instruction->id = (uint16_t)(instruction->prefixes & NMD_X86_PREFIXES_REX_W ? NMD_X86_INSTRUCTION_PINSRQ : NMD_X86_INSTRUCTION_PINSRD); break;
						case 0xcc: instruction->id = NMD_X86_INSTRUCTION_SHA1RNDS4; break;
						case 0xdf: instruction->id = NMD_X86_INSTRUCTION_AESKEYGENASSIST; break;
						}
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS
				if (flags & NMD_X86_DECODER_FLAGS_OPERANDS)
				{
					instruction->numOperands = 3;
					instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ_WRITE;
					instruction->operands[1].action = instruction->operands[2].action = NMD_X86_OPERAND_ACTION_READ;
					instruction->operands[2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;

					if (op == 0x0f && !instruction->simdPrefix)
					{
						parseOperandPq(instruction, &instruction->operands[0]);
						parseOperandQq(instruction, &instruction->operands[1]);
					}
					else if (NMD_R(op) == 1)
					{
						parseMemoryOperand(instruction, &instruction->operands[0], NMD_X86_REG_EAX);
						parseOperandVdq(instruction, &instruction->operands[1]);
					}
					else if (NMD_R(op) == 2)
					{
						parseOperandVdq(instruction, &instruction->operands[0]);
						parseMemoryOperand(instruction, &instruction->operands[1], (uint8_t)(NMD_C(op) == 1 ? NMD_X86_REG_XMM0 : NMD_X86_REG_EAX));
					}
					else if (op == 0xcc || op == 0xdf || NMD_R(op) == 4 || NMD_R(op) == 6 || NMD_R(op) == 0)
					{
						parseOperandVdq(instruction, &instruction->operands[0]);
						parseOperandWdq(instruction, &instruction->operands[1]);
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS */
			}
		}
		else if (*b == 0x0f) /* 3DNow! opcode map*/
		{
#ifndef NMD_ASSEMBLY_DISABLE_DECODER_3DNOW
			if (flags & NMD_X86_DECODER_FLAGS_3DNOW)
			{
				if (remainingSize < 5)
					return false;

				if (!parseModrm(&b, instruction, remainingSize - 2))
					return false;

				instruction->encoding = NMD_X86_ENCODING_3DNOW;
				instruction->opcode = 0x0f;
				instruction->immMask = NMD_X86_IMM8; /* The real opcode is encoded as the immediate byte. */
				instruction->immediate = *(b + 1);

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK
				if (!nmd_findByte(valid3DNowOpcodes, sizeof(valid3DNowOpcodes), (uint8_t)instruction->immediate))
					return false;
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK */
			}
			else
				return false;
#else /* NMD_ASSEMBLY_DISABLE_DECODER_3DNOW */
			return false;
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_3DNOW */
		}
		else /* 2 byte opcode. */
		{
			instruction->opcodeSize = 2;
			instruction->opcode = *b;
			instruction->opcodeMap = NMD_X86_OPCODE_MAP_0F;

			op = instruction->opcode;

			/* Check for ModR/M, SIB and displacement. */
			if (op >= 0x20 && op <= 0x23 && remainingSize == 2)
				instruction->hasModrm = true, instruction->modrm.modrm = *++b;
			else if (op < 4 || (NMD_R(op) != 3 && NMD_R(op) > 0 && NMD_R(op) < 7) || (op >= 0xD0 && op != 0xFF) || (NMD_R(op) == 7 && NMD_C(op) != 7) || NMD_R(op) == 9 || NMD_R(op) == 0xB || (NMD_R(op) == 0xC && NMD_C(op) < 8) || (NMD_R(op) == 0xA && (op % 8) >= 3) || op == 0x0ff || op == 0x00 || op == 0x0d)
			{
				if (!parseModrm(&b, instruction, remainingSize - 2))
					return false;
			}

			const NMD_Modrm modrm = instruction->modrm;
#ifndef NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK
			if (flags & NMD_X86_DECODER_FLAGS_VALIDITY_CHECK)
			{
				/* Check if the instruction is invalid. */
				if (nmd_findByte(invalid2op, sizeof(invalid2op), op))
					return false;
				else if (op == 0xc7)
				{
					if ((!instruction->simdPrefix && (modrm.fields.mod == 0b11 ? modrm.fields.reg <= 0b101 : modrm.fields.reg == 0b000 || modrm.fields.reg == 0b010)) || (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO && (modrm.fields.mod == 0b11 || modrm.fields.reg != 0b001)) || ((instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT) && (modrm.fields.mod == 0b11 ? modrm.fields.reg <= (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? 0b110 : 0b101) : (modrm.fields.reg != 0b001 && modrm.fields.reg != 0b110))))
						return false;
				}
				else if (op == 0x00)
				{
					if (modrm.fields.reg >= 0b110)
						return false;
				}
				else if (op == 0x01)
				{
					if ((modrm.fields.mod == 0b11 ? ((instruction->prefixes & (NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE | NMD_X86_PREFIXES_REPEAT_NOT_ZERO | NMD_X86_PREFIXES_REPEAT) && ((modrm.modrm >= 0xc0 && modrm.modrm <= 0xc5) || (modrm.modrm >= 0xc8 && modrm.modrm <= 0xcb) || (modrm.modrm >= 0xcf && modrm.modrm <= 0xd1) || (modrm.modrm >= 0xd4 && modrm.modrm <= 0xd7) || modrm.modrm == 0xee || modrm.modrm == 0xef || modrm.modrm == 0xfa || modrm.modrm == 0xfb)) || (modrm.fields.reg == 0b000 && modrm.fields.rm >= 0b110) || (modrm.fields.reg == 0b001 && modrm.fields.rm >= 0b100 && modrm.fields.rm <= 0b110) || (modrm.fields.reg == 0b010 && (modrm.fields.rm == 0b010 || modrm.fields.rm == 0b011)) || (modrm.fields.reg == 0b101 && modrm.fields.rm < 0b110 && (!(instruction->prefixes & NMD_X86_PREFIXES_REPEAT) || (instruction->prefixes & NMD_X86_PREFIXES_REPEAT && (modrm.fields.rm != 0b000 && modrm.fields.rm != 0b010)))) || (modrm.fields.reg == 0b111 && (modrm.fields.rm > 0b101 || (mode != NMD_X86_MODE_64 && modrm.fields.rm == 0b000)))) : (!(instruction->prefixes & NMD_X86_PREFIXES_REPEAT) && modrm.fields.reg == 0b101)))
						return false;
				}
				else if (op == 0x1A || op == 0x1B)
				{
					if (modrm.fields.mod == 0b11)
						return false;
				}
				else if (op == 0x20 || op == 0x22)
				{
					if (modrm.fields.reg == 0b001 || modrm.fields.reg >= 0b101)
						return false;
				}
				else if (op >= 0x24 && op <= 0x27)
					return false;
				else if (op >= 0x3b && op <= 0x3f)
					return false;
				else if (NMD_R(op) == 5)
				{
					if ((op == 0x50 && modrm.fields.mod != 0b11) || (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && (op == 0x52 || op == 0x53)) || (instruction->prefixes & NMD_X86_PREFIXES_REPEAT && (op == 0x50 || (op >= 0x54 && op <= 0x57))) || (instruction->prefixes & NMD_X86_PREFIXES_REPEAT_NOT_ZERO && (op == 0x50 || (op >= 0x52 && op <= 0x57) || op == 0x5b)))
						return false;
				}
				else if (NMD_R(op) == 6)
				{
					if ((!(instruction->prefixes & (NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE | NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO)) && (op == 0x6c || op == 0x6d)) || (instruction->prefixes & NMD_X86_PREFIXES_REPEAT && op != 0x6f) || instruction->prefixes & NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
						return false;
				}
				else if (op == 0x78 || op == 0x79)
				{
					if ((((instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && op == 0x78) && !(modrm.fields.mod == 0b11 && modrm.fields.reg == 0b000)) || ((instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) && modrm.fields.mod != 0b11)) || (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT))
						return false;
				}
				else if (op == 0x7c || op == 0x7d)
				{
					if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT || !(instruction->prefixes & (NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE | NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO)))
						return false;
				}
				else if (op == 0x7e || op == 0x7f)
				{
					if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
						return false;
				}
				else if (op >= 0x71 && op <= 0x73)
				{
					if (instruction->prefixes & (NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO) || modrm.modrm <= 0xcf || (modrm.modrm >= 0xe8 && modrm.modrm <= 0xef))
						return false;
				}
				else if (op == 0x73)
				{
					if (modrm.modrm >= 0xe0 && modrm.modrm <= 0xe8)
						return false;
				}
				else if (op == 0xa6)
				{
					if (modrm.modrm != 0xc0 && modrm.modrm != 0xc8 && modrm.modrm != 0xd0)
						return false;
				}
				else if (op == 0xa7)
				{
					if (!(modrm.fields.mod == 0b11 && modrm.fields.reg <= 0b101 && modrm.fields.rm == 0b000))
						return false;
				}
				else if (op == 0xae)
				{
					if (((!instruction->simdPrefix && modrm.fields.mod == 0b11 && modrm.fields.reg <= 0b100) || (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO && !(modrm.fields.mod == 0b11 && modrm.fields.reg == 0b110)) || (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && (modrm.fields.reg < 0b110 || (modrm.fields.mod == 0b11 && modrm.fields.reg == 0b111))) || (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT && (modrm.fields.reg != 0b100 && modrm.fields.reg != 0b110) && !(modrm.fields.mod == 0b11 && modrm.fields.reg == 0b101))))
						return false;
				}
				else if (op == 0xb8)
				{
					if (!(instruction->prefixes & NMD_X86_PREFIXES_REPEAT))
						return false;
				}
				else if (op == 0xba)
				{
					if (modrm.fields.reg <= 0b011)
						return false;
				}
				else if (op == 0xd0)
				{
					if (!instruction->simdPrefix || instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT)
						return false;
				}
				else if (op == 0xe0)
				{
					if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT || instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
						return false;
				}
				else if (op == 0xf0)
				{
					if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? modrm.fields.mod == 0b11 : true)
						return false;
				}
				else if (instruction->prefixes & (NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO))
				{
					if ((op >= 0x13 && op <= 0x17 && !(op == 0x16 && instruction->prefixes & NMD_X86_PREFIXES_REPEAT)) || op == 0x28 || op == 0x29 || op == 0x2e || op == 0x2f || (op <= 0x76 && op >= 0x74))
						return false;
				}
				else if (op == 0x71 || op == 0x72 || (op == 0x73 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)))
				{
					if ((modrm.modrm >= 0xd8 && modrm.modrm <= 0xdf) || modrm.modrm >= 0xf8)
						return false;
				}
				else if (op >= 0xc3 && op <= 0xc6)
				{
					if ((op == 0xc5 && modrm.fields.mod != 0b11) || (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT || instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) || (op == 0xc3 && instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE))
						return false;
				}
				else if (NMD_R(op) >= 0xd && NMD_C(op) != 0 && op != 0xff && ((NMD_C(op) == 6 && NMD_R(op) != 0xf) ? (!instruction->simdPrefix || (NMD_R(op) == 0xD && (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT || instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) ? modrm.fields.mod != 0b11 : false)) : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT || instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO || ((NMD_C(op) == 7 && NMD_R(op) != 0xe) ? modrm.fields.mod != 0b11 : false))))
					return false;
				else if (modrm.fields.mod == 0b11)
				{
					if (op == 0xb2 || op == 0xb4 || op == 0xb5 || op == 0xc3 || op == 0xe7 || op == 0x2b || (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && (op == 0x12 || op == 0x16)) || (!(instruction->prefixes & (NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO)) && (op == 0x13 || op == 0x17)))
						return false;
				}
			}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID
			if (flags & NMD_X86_DECODER_FLAGS_INSTRUCTION_ID)
			{
				if (NMD_R(op) == 8)
					instruction->id = NMD_X86_INSTRUCTION_JO + NMD_C(op);
				else if (op >= 0xa2 && op <= 0xa5)
					instruction->id = NMD_X86_INSTRUCTION_CPUID + (op - 0xa2);
				else if (op == 0x05)
					instruction->id = NMD_X86_INSTRUCTION_SYSCALL;
				else if (NMD_R(op) == 4)
					instruction->id = NMD_X86_INSTRUCTION_CMOVO + NMD_C(op);
				else if (op == 0x00)
					instruction->id = NMD_X86_INSTRUCTION_SLDT + modrm.fields.reg;
				else if (op == 0x01)
				{
					if (modrm.fields.mod == 0b11)
					{
						switch (modrm.fields.reg)
						{
						case 0b000: instruction->id = NMD_X86_INSTRUCTION_VMCALL + modrm.fields.rm; break;
						case 0b001: instruction->id = NMD_X86_INSTRUCTION_MONITOR + modrm.fields.rm; break;
						case 0b010: instruction->id = NMD_X86_INSTRUCTION_XGETBV + modrm.fields.rm; break;
						case 0b011: instruction->id = NMD_X86_INSTRUCTION_VMRUN + modrm.fields.rm; break;
						case 0b100: instruction->id = NMD_X86_INSTRUCTION_SMSW; break;
						case 0b110: instruction->id = NMD_X86_INSTRUCTION_LMSW; break;
						case 0b111: instruction->id = (uint16_t)(modrm.fields.rm == 0b000 ? NMD_X86_INSTRUCTION_SWAPGS : NMD_X86_INSTRUCTION_RDTSCP); break;
						}
					}
					else
						instruction->id = NMD_X86_INSTRUCTION_SGDT + modrm.fields.reg;
				}
				else if (op <= 0x0b)
					instruction->id = NMD_X86_INSTRUCTION_LAR + (op - 2);
				else if (op == 0x19 || (op >= 0x1c && op <= 0x1f))
				{
					if (op == 0x1e && modrm.modrm == 0xfa)
						instruction->id = NMD_X86_INSTRUCTION_ENDBR64;
					else if (op == 0x1e && modrm.modrm == 0xfb)
						instruction->id = NMD_X86_INSTRUCTION_ENDBR32;
					else
						instruction->id = NMD_X86_INSTRUCTION_NOP;
				}
				else if (op >= 0x10 && op <= 0x17)
				{
					switch (instruction->simdPrefix)
					{
					case NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE: instruction->id = NMD_X86_INSTRUCTION_VMOVUPS + NMD_C(op); break;
					case NMD_X86_PREFIXES_REPEAT: instruction->id = NMD_X86_INSTRUCTION_VMOVUPS + NMD_C(op); break;
					case NMD_X86_PREFIXES_REPEAT_NOT_ZERO: instruction->id = NMD_X86_INSTRUCTION_VMOVUPS + NMD_C(op); break;
					default: instruction->id = NMD_X86_INSTRUCTION_VMOVUPS + NMD_C(op); break;
					}
				}
				else if (op >= 0x20 && op <= 0x23)
					instruction->id = NMD_X86_INSTRUCTION_MOV;
				else if (NMD_R(op) == 3)
					instruction->id = NMD_X86_INSTRUCTION_WRMSR + NMD_C(op);
				else if (NMD_R(op) == 5)
				{
					switch (instruction->simdPrefix)
					{
					case NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE: instruction->id = NMD_X86_INSTRUCTION_MOVMSKPD + NMD_C(op); break;
					case NMD_X86_PREFIXES_REPEAT: instruction->id = NMD_X86_INSTRUCTION_BNDMOV + NMD_C(op); break;
					case NMD_X86_PREFIXES_REPEAT_NOT_ZERO: instruction->id = NMD_X86_INSTRUCTION_BNDCL + NMD_C(op); break;
					default:   instruction->id = NMD_X86_INSTRUCTION_MOVMSKPS + NMD_C(op); break;
					}
				}
				else if (op >= 0x60 && op <= 0x6d)
					instruction->id = NMD_X86_INSTRUCTION_PUNPCKLBW + NMD_C(op);
				else if (op >= 0x74 && op <= 0x76)
					instruction->id = NMD_X86_INSTRUCTION_PCMPEQB + (op - 0x74);
				else if (op >= 0xb2 && op <= 0xb5)
					instruction->id = NMD_X86_INSTRUCTION_LSS + (op - 0xb2);
				else if (op >= 0xc3 && op <= 0xc5)
					instruction->id = NMD_X86_INSTRUCTION_MOVNTI + (op - 0xc3);
				else if (op == 0xc7)
				{
					if (modrm.fields.reg == 0b001)
						instruction->id = (uint16_t)(instruction->operandSize64 ? NMD_X86_INSTRUCTION_CMPXCHG16B : NMD_X86_INSTRUCTION_CMPXCHG8B);
					else if (modrm.fields.reg == 0b111)
						instruction->id = (uint16_t)(modrm.fields.mod == 0b11 ? (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_RDPID : NMD_X86_INSTRUCTION_RDSEED) : NMD_X86_INSTRUCTION_VMPTRST);
					else
						instruction->id = (uint16_t)(modrm.fields.mod == 0b11 ? NMD_X86_INSTRUCTION_RDRAND : (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_VMCLEAR : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_VMXON : NMD_X86_INSTRUCTION_VMPTRLD)));
				}
				else if (op >= 0xc8 && op <= 0xcf)
					instruction->id = NMD_X86_INSTRUCTION_BSWAP;
				else if (op == 0xa3)
					instruction->id = (uint16_t)((modrm.fields.mod == 0b11 ? NMD_X86_INSTRUCTION_RDFSBASE : NMD_X86_INSTRUCTION_FXSAVE) + modrm.fields.reg);
				else if (op >= 0xd1 && op <= 0xfe)
				{
					if (op == 0xd6)
						instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVQ : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_MOVQ2DQ : NMD_X86_INSTRUCTION_MOVDQ2Q));
					else if (op == 0xe6)
						instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_CVTTPD2DQ : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_CVTDQ2PD : NMD_X86_INSTRUCTION_CVTPD2DQ));
					else if (op == 0xe7)
						instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVNTDQ : NMD_X86_INSTRUCTION_MOVNTQ);
					else if (op == 0xf7)
						instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MASKMOVDQU : NMD_X86_INSTRUCTION_MASKMOVQ);
					else
						instruction->id = NMD_X86_INSTRUCTION_PSRLW + (op - 0xd1);
				}
				else
				{
					switch (op)
					{
					case 0xa0: case 0xa8: instruction->id = NMD_X86_INSTRUCTION_PUSH; break;
					case 0xa1: case 0xa9: instruction->id = NMD_X86_INSTRUCTION_POP; break;
					case 0xaf: instruction->id = NMD_X86_INSTRUCTION_IMUL; break;
					case 0xb0: case 0xb1: instruction->id = NMD_X86_INSTRUCTION_CMPXCHG; break;
					case 0x10: case 0x11: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVUPD : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_MOVSS : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_MOVSD : NMD_X86_INSTRUCTION_MOVUPD))); break;
					case 0x12: case 0x13: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVLPD : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_MOVSLDUP : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_MOVDDUP : NMD_X86_INSTRUCTION_MOVLPS))); break;
					case 0x14: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_UNPCKLPD : NMD_X86_INSTRUCTION_UNPCKLPS); break;
					case 0x15: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_UNPCKHPD : NMD_X86_INSTRUCTION_UNPCKHPS); break;
					case 0x16: case 0x17: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVHPD : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_MOVSHDUP : NMD_X86_INSTRUCTION_MOVHPS)); break;
					case 0x18: instruction->id = (uint16_t)(modrm.fields.reg >= 0b100 ? NMD_X86_INSTRUCTION_NOP : (modrm.fields.reg == 0b000 ? NMD_X86_INSTRUCTION_PREFETCHNTA : (modrm.fields.reg == 0b001 ? NMD_X86_INSTRUCTION_PREFETCHT0 : (modrm.fields.reg == 0b010 ? NMD_X86_INSTRUCTION_PREFETCHT1 : NMD_X86_INSTRUCTION_PREFETCHT2)))); break;
					case 0x1a: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_BNDMOV : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_BNDCL : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_BNDCU : NMD_X86_INSTRUCTION_BNDLDX))); break;
					case 0x1b: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_BNDMOV : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_BNDMK : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_BNDCN : NMD_X86_INSTRUCTION_BNDSTX))); break;
					case 0x28: case 0x29: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVAPD : NMD_X86_INSTRUCTION_MOVAPS); break;
					case 0x2a: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_CVTPI2PD : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_CVTSI2SS : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_CVTSI2SD : NMD_X86_INSTRUCTION_CVTPI2PS))); break;
					case 0x2b: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVNTPD : NMD_X86_INSTRUCTION_MOVNTPS); break;
					case 0x2c: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_CVTTPD2PI : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_CVTTSS2SI : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_CVTTSS2SI : NMD_X86_INSTRUCTION_CVTTPS2PI))); break;
					case 0x2d: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_CVTPD2PI : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_CVTSS2SI : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_CVTSS2SI : NMD_X86_INSTRUCTION_CVTPS2PI))); break;
					case 0x2e: case 0x2f: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_UCOMISD : NMD_X86_INSTRUCTION_UCOMISS); break;
					case 0x6e: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && !instruction->operandSize64 && (instruction->prefixes & NMD_X86_PREFIXES_REX_W) ? NMD_X86_INSTRUCTION_MOVQ : NMD_X86_INSTRUCTION_MOVD); break;
					case 0x6f: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVDQA : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_MOVDQU : NMD_X86_INSTRUCTION_MOVQ)); break;
					case 0x70: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_PSHUFD : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_PSHUFHW : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_PSHUFLW : NMD_X86_INSTRUCTION_PSHUFW))); break;
					case 0x71: instruction->id = (uint16_t)(modrm.fields.reg == 0b101 ? NMD_X86_INSTRUCTION_PSRLQ : (modrm.fields.reg == 0b100 ? NMD_X86_INSTRUCTION_PSRAW : NMD_X86_INSTRUCTION_PSLLW)); break;
					case 0x72: instruction->id = (uint16_t)(modrm.fields.reg == 0b101 ? NMD_X86_INSTRUCTION_PSRLD : (modrm.fields.reg == 0b100 ? NMD_X86_INSTRUCTION_PSRAD : NMD_X86_INSTRUCTION_PSLLD)); break;
					case 0x73: instruction->id = (uint16_t)(modrm.fields.reg == 0b010 ? NMD_X86_INSTRUCTION_PSRLQ : (modrm.fields.reg == 0b011 ? NMD_X86_INSTRUCTION_PSRLDQ : (modrm.fields.reg == 0b110 ? NMD_X86_INSTRUCTION_PSLLQ : NMD_X86_INSTRUCTION_PSLLDQ))); break;
					case 0x77: instruction->id = NMD_X86_INSTRUCTION_EMMS; break;
					case 0x78: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_EXTRQ : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_INSERTQ : NMD_X86_INSTRUCTION_VMREAD)); break;
					case 0x79: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_EXTRQ : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_INSERTQ : NMD_X86_INSTRUCTION_VMWRITE)); break;
					case 0x7c: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_HADDPD : NMD_X86_INSTRUCTION_HADDPS); break;
					case 0x7d: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_HSUBPD : NMD_X86_INSTRUCTION_HSUBPS); break;
					case 0x7e: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT || (instruction->operandSize64 && instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE) ? NMD_X86_INSTRUCTION_MOVQ : NMD_X86_INSTRUCTION_MOVD); break;
					case 0x7f: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_MOVDQA : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_MOVDQU : NMD_X86_INSTRUCTION_MOVQ)); break;
					case 0xa3: instruction->id = NMD_X86_INSTRUCTION_BT; break;
					case 0xa4: case 0xa5: instruction->id = NMD_X86_INSTRUCTION_SHLD; break;
					case 0xaa: instruction->id = NMD_X86_INSTRUCTION_RSM; break;
					case 0xab: instruction->id = NMD_X86_INSTRUCTION_BTS; break;
					case 0xac: case 0xad: instruction->id = NMD_X86_INSTRUCTION_SHRD; break;
					case 0xb6: case 0xb7: instruction->id = NMD_X86_INSTRUCTION_MOVZX; break;
					case 0xb8: instruction->id = NMD_X86_INSTRUCTION_POPCNT; break;
					case 0xb9: instruction->id = NMD_X86_INSTRUCTION_UD1; break;
					case 0xba: instruction->id = (uint16_t)(modrm.fields.reg == 0b100 ? NMD_X86_INSTRUCTION_BT : (modrm.fields.reg == 0b101 ? NMD_X86_INSTRUCTION_BTS : (modrm.fields.reg == 0b110 ? NMD_X86_INSTRUCTION_BTR : NMD_X86_INSTRUCTION_BTC))); break;
					case 0xbb: instruction->id = NMD_X86_INSTRUCTION_BTC; break;
					case 0xbc: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_BSF : NMD_X86_INSTRUCTION_TZCNT); break;
					case 0xbd: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_BSR : NMD_X86_INSTRUCTION_LZCNT); break;
					case 0xbe: case 0xbf: instruction->id = NMD_X86_INSTRUCTION_MOVSX; break;
					case 0xc0: case 0xc1: instruction->id = NMD_X86_INSTRUCTION_XADD; break;
					case 0xc2: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_CMPPD : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? NMD_X86_INSTRUCTION_CMPSS : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? NMD_X86_INSTRUCTION_CMPSD : NMD_X86_INSTRUCTION_CMPPS))); break;
					case 0xd0: instruction->id = (uint16_t)(instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_INSTRUCTION_ADDSUBPD : NMD_X86_INSTRUCTION_ADDSUBPS); break;
					case 0xff: instruction->id = NMD_X86_INSTRUCTION_UD0; break;
					}
				}
			}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_CPU_FLAGS
			if (flags & NMD_X86_DECODER_FLAGS_CPU_FLAGS)
			{
				if (NMD_R(op) == 4 || NMD_R(op) == 8 || NMD_R(op) == 9) /* Conditional Move (CMOVcc),Conditional jump(Jcc),Byte set on condition(SETcc) */
					decodeConditionalFlag(instruction, NMD_C(op));
				else if (op == 0x05 || op == 0x07) /* syscall,sysret */
					instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_TF | NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_DF | NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_AC | NMD_X86_EFLAGS_VIF | NMD_X86_EFLAGS_VIP | NMD_X86_EFLAGS_ID;
				else if (op == 0xaf) /* mul */
				{
					instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_OF;
					instruction->undefinedFlags.eflags = NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF;
				}
				else if (op == 0xb0 || op == 0xb1) /* cmpxchg */
					instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_OF;
				else if (op == 0xc0 || op == 0xc1) /* xadd */
					instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_OF;
				else if (op == 0x00 && (modrm.fields.reg == 0b100 || modrm.fields.reg == 0b101)) /* verr,verw*/
					instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_OF;
				else if (op == 0x01 && modrm.fields.mod == 0b11)
				{
					if (modrm.fields.reg == 0b000)
					{
						if (modrm.fields.rm == 0b001 || modrm.fields.rm == 0b010 || modrm.fields.rm == 0b011) /* vmcall,vmlaunch,vmresume */
						{
							instruction->testedFlags.eflags = NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_VM;
							instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_TF | NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_DF | NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_NT | NMD_X86_EFLAGS_RF | NMD_X86_EFLAGS_VM | NMD_X86_EFLAGS_AC | NMD_X86_EFLAGS_VIF | NMD_X86_EFLAGS_VIP | NMD_X86_EFLAGS_ID;
						}
					}
				}
				else if (op == 0x34)
					instruction->clearedFlags.eflags = NMD_X86_EFLAGS_VM | NMD_X86_EFLAGS_IF;
				else if (op == 0x78 || op == 0x79) /* vmread,vmwrite */
				{
					instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_ZF;
					instruction->clearedFlags.eflags = NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_OF;
				}
				else if (op == 0x02 || op == 0x03) /* lar,lsl */
					instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_ZF;
				else if (op == 0xa3 || op == 0xab || op == 0xb3 || op == 0xba || op == 0xbb) /* bt,bts,btc */
				{
					instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF;
					instruction->undefinedFlags.eflags = NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_PF;
				}
				else if (op == 0xa4 || op == 0xa5 || op == 0xac || op == 0xad || op == 0xbc) /* shld,shrd */
				{
					instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF;
					instruction->undefinedFlags.eflags = NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_OF;
				}
				else if (op == 0xaa) /* rsm */
					instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_TF | NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_DF | NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_IOPL | NMD_X86_EFLAGS_NT | NMD_X86_EFLAGS_RF | NMD_X86_EFLAGS_VM | NMD_X86_EFLAGS_AC | NMD_X86_EFLAGS_VIF | NMD_X86_EFLAGS_VIP | NMD_X86_EFLAGS_ID;
				else if ((op == 0xbc || op == 0xbd) && instruction->prefixes & NMD_X86_PREFIXES_REPEAT) /* tzcnt */
				{
					instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_ZF;
					instruction->undefinedFlags.eflags = NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_OF;
				}
				else if (op == 0xbc || op == 0xbd) /* bsf */
				{
					instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_ZF;
					instruction->undefinedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_OF;
				}
			}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_CPU_FLAGS */

			if (NMD_R(op) == 8) /* imm32 */
				instruction->immMask = (uint8_t)(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_IMM16 : NMD_X86_IMM32);
			else if ((NMD_R(op) == 7 && NMD_C(op) < 4) || op == 0xA4 || op == 0xC2 || (op > 0xC3 && op <= 0xC6) || op == 0xBA || op == 0xAC) /* imm8 */
				instruction->immMask = NMD_X86_IMM8;
			else if (op == 0x78 && (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO || instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) /* imm8 + imm8 = "imm16" */
				instruction->immMask = NMD_X86_IMM16;

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_GROUP
			/* Parse the instruction's group. */
			if (flags & NMD_X86_DECODER_FLAGS_GROUP)
			{
				if (NMD_R(op) == 8)
					instruction->group = NMD_GROUP_JUMP | NMD_GROUP_CONDITIONAL_BRANCH | NMD_GROUP_RELATIVE_ADDRESSING;
				else if ((op == 0x01 && modrm.fields.rm == 0b111 && (modrm.fields.mod == 0b00 || modrm.modrm == 0xf8)) || op == 0x06 || op == 0x08 || op == 0x09)
					instruction->group = NMD_GROUP_PRIVILEGE;
				else if (op == 0x05)
					instruction->group = NMD_GROUP_INT;
			}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_GROUP */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS
			if (flags & NMD_X86_DECODER_FLAGS_OPERANDS)
			{
				if (op == 0x2 || op == 0x3 || (op >= 0x10 && op <= 0x17) || NMD_R(op) == 2 || (NMD_R(op) >= 4 && NMD_R(op) <= 7) || op == 0xa3 || op == 0xab || op == 0xaf || (NMD_R(op) >= 0xc && op != 0xc7 && op != 0xff))
					instruction->numOperands = 2;
				else if (NMD_R(op) == 8 || NMD_R(op) == 9 || (NMD_R(op) == 0xa && op % 8 < 2) || op == 0xc7)
					instruction->numOperands = 1;
				else if (op == 0xa4 || op == 0xa5 || op == 0xc2 || (op >= 0xc4 && op <= 0xc6))
					instruction->numOperands = 3;

				if (instruction->numOperands > 0)
				{
					if (op == 0x00)
					{
						if (instruction->modrm.fields.reg >= 0b010)
							parseOperandEw(instruction, &instruction->operands[0]);
						else
							parseOperandEv(instruction, &instruction->operands[0]);

						instruction->operands[0].action = (uint8_t)(instruction->modrm.fields.reg >= 0b010 ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_WRITE);
					}
					else if (op == 0x01)
					{
						if (instruction->modrm.fields.mod != 0b11)
						{
							parseModrmUpper32(instruction, &instruction->operands[0]);
							instruction->operands[0].action = (uint8_t)(instruction->modrm.fields.reg >= 0b010 ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_WRITE);
						}
						else if (instruction->modrm.fields.reg == 0b100)
							parseOperandRv(instruction, &instruction->operands[0]);
						else if (instruction->modrm.fields.reg == 0b110)
						{
							parseOperandEw(instruction, &instruction->operands[0]);
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ;
						}

						if (instruction->modrm.fields.reg == 0b100)
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					}
					else if (op == 0x02 || op == 0x03)
					{
						parseOperandGv(instruction, &instruction->operands[0]);
						parseOperandEw(instruction, &instruction->operands[1]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op == 0x0d)
					{
						parseOperandEv(instruction, &instruction->operands[0]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (NMD_R(op) == 0x8)
					{
						instruction->operands[0].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
					}
					else if (NMD_R(op) == 9)
					{
						parseOperandEb(instruction, &instruction->operands[0]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
					}
					else if (op == 0x17)
					{
						parseModrmUpper32(instruction, &instruction->operands[0]);
						parseOperandVdq(instruction, &instruction->operands[1]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op >= 0x20 && op <= 0x23)
					{
						instruction->operands[0].type = instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
						instruction->operands[op < 0x22 ? 0 : 1].fields.reg = NMD_X86_REG_EAX + instruction->modrm.fields.rm;
						instruction->operands[op < 0x22 ? 1 : 0].fields.reg = (uint8_t)((op % 2 == 0 ? NMD_X86_REG_CR0 : NMD_X86_REG_DR0) + instruction->modrm.fields.reg);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op == 0x29 || op == 0x2b || (op == 0x7f && instruction->simdPrefix))
					{
						parseOperandWdq(instruction, &instruction->operands[0]);
						parseOperandVdq(instruction, &instruction->operands[1]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op == 0x2a || op == 0x2c || op == 0x2d)
					{
						if (op == 0x2a)
							parseOperandVdq(instruction, &instruction->operands[0]);
						else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT || instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
							parseOperandGy(instruction, &instruction->operands[0]);
						else if (op == 0x2d && instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
							parseOperandQq(instruction, &instruction->operands[0]);
						else
							parseOperandPq(instruction, &instruction->operands[0]);

						if (op == 0x2a)
						{
							if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT || instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
								parseOperandEy(instruction, &instruction->operands[1]);
							else
								parseOperandQq(instruction, &instruction->operands[1]);
						}
						else
							parseOperandWdq(instruction, &instruction->operands[1]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op == 0x50)
					{
						parseOperandGy(instruction, &instruction->operands[0]);
						parseOperandUdq(instruction, &instruction->operands[1]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (NMD_R(op) == 5 || (op >= 0x10 && op <= 0x16) || op == 0x28 || op == 0x2e || op == 0x2f || (op == 0x7e && instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT))
					{
						parseOperandVdq(instruction, &instruction->operands[op == 0x11 || op == 0x13 ? 1 : 0]);
						parseOperandWdq(instruction, &instruction->operands[op == 0x11 || op == 0x13 ? 0 : 1]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op == 0x7e)
					{
						parseOperandEy(instruction, &instruction->operands[0]);
						instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
						instruction->operands[1].size = 1;
						instruction->operands[1].fields.reg = (uint8_t)((instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_XMM0 : NMD_X86_REG_MM0) + instruction->modrm.fields.reg);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (NMD_R(op) == 6 || op == 0x70 || (op >= 0x74 && op <= 0x76) || (op >= 0x7c && op <= 0x7f))
					{
						if (!instruction->simdPrefix)
						{
							parseOperandPq(instruction, &instruction->operands[op == 0x7f ? 1 : 0]);

							if (op == 0x6e)
								parseOperandEy(instruction, &instruction->operands[1]);
							else
								parseOperandQq(instruction, &instruction->operands[op == 0x7f ? 0 : 1]);
						}
						else
						{
							parseOperandVdq(instruction, &instruction->operands[0]);

							if (op == 0x6e)
								parseOperandEy(instruction, &instruction->operands[1]);
							else
								parseOperandWdq(instruction, &instruction->operands[1]);
						}

						if (op == 0x70)
							instruction->operands[2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;

						instruction->operands[0].action = (uint8_t)(((op >= 0x60 && op <= 0x6d) || (op >= 0x74 && op <= 0x76)) ? NMD_X86_OPERAND_ACTION_READ_WRITE : NMD_X86_OPERAND_ACTION_WRITE);
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op >= 0x71 && op <= 0x73)
					{
						if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
							parseOperandUdq(instruction, &instruction->operands[0]);
						else
							parseOperandQq(instruction, &instruction->operands[0]);
						instruction->operands[1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op == 0x78 || op == 0x79)
					{
						if (instruction->simdPrefix)
						{
							if (op == 0x78)
							{
								i = 0;
								if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
									parseOperandVdq(instruction, &instruction->operands[i++]);
								parseOperandUdq(instruction, &instruction->operands[i + 0]);
								instruction->operands[i + 1].type = instruction->operands[i + 2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
								instruction->operands[i + 1].size = instruction->operands[i + 2].size = 1;
								instruction->operands[i + 1].fields.imm = b[1];
								instruction->operands[i + 2].fields.imm = b[2];
							}
							else
							{
								parseOperandVdq(instruction, &instruction->operands[0]);
								parseOperandWdq(instruction, &instruction->operands[1]);
							}
						}
						else
						{
							parseOperandEy(instruction, &instruction->operands[op == 0x78 ? 0 : 1]);
							parseOperandGy(instruction, &instruction->operands[op == 0x78 ? 1 : 0]);
						}
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (NMD_R(op) == 0xa && (op % 8) < 2)
					{
						instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
						instruction->operands[0].fields.reg = (uint8_t)(op > 0xa8 ? NMD_X86_REG_GS : NMD_X86_REG_FS);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if ((NMD_R(op) == 0xa && ((op % 8) >= 3 && (op % 8) <= 5)) || op == 0xb3 || op == 0xbb)
					{
						parseOperandEv(instruction, &instruction->operands[0]);
						parseOperandGv(instruction, &instruction->operands[1]);

						if (NMD_R(op) == 0xa)
						{
							if ((op % 8) == 4)
								instruction->operands[2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
							else if ((op % 8) == 5)
							{
								instruction->operands[2].type = NMD_X86_OPERAND_TYPE_REGISTER;
								instruction->operands[2].fields.reg = NMD_X86_REG_CL;
							}
						}

						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op == 0xaf || op == 0xb8)
					{
						parseOperandGv(instruction, &instruction->operands[0]);
						parseOperandEv(instruction, &instruction->operands[1]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op == 0xba)
					{
						parseOperandEv(instruction, &instruction->operands[0]);
						instruction->operands[0].action = (uint8_t)(instruction->modrm.fields.reg <= 0b101 ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_READ_WRITE);
						instruction->operands[1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (NMD_R(op) == 0xb && (op % 8) >= 6)
					{
						parseOperandGv(instruction, &instruction->operands[0]);
						if ((op % 8) == 6)
							parseOperandEb(instruction, &instruction->operands[1]);
						else
							parseOperandEw(instruction, &instruction->operands[1]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (NMD_R(op) == 0x4 || (NMD_R(op) == 0xb && ((op % 8) == 0x4 || (op % 8) == 0x5)))
					{
						parseOperandGv(instruction, &instruction->operands[0]);
						parseOperandEv(instruction, &instruction->operands[1]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if ((NMD_R(op) == 0xb || NMD_R(op) == 0xc) && NMD_C(op) < 2)
					{
						if (NMD_C(op) == 0)
						{
							parseOperandEb(instruction, &instruction->operands[0]);
							parseOperandGb(instruction, &instruction->operands[1]);
						}
						else
						{
							parseOperandEv(instruction, &instruction->operands[0]);
							parseOperandGv(instruction, &instruction->operands[1]);
						}

						if (NMD_R(op) == 0xb)
						{
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ | NMD_X86_OPERAND_ACTION_CONDITIONAL_WRITE;
							instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
						}
						else
							instruction->operands[0].action = instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ_WRITE;
					}
					else if (op == 0xb2)
					{
						parseOperandGv(instruction, &instruction->operands[0]);
						parseModrmUpper32(instruction, &instruction->operands[1]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op == 0xc3)
					{
						parseModrmUpper32(instruction, &instruction->operands[0]);
						parseOperandGy(instruction, &instruction->operands[1]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
					else if (op == 0xc2 || op == 0xc6)
					{
						parseOperandVdq(instruction, &instruction->operands[0]);
						parseOperandWdq(instruction, &instruction->operands[1]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
						instruction->operands[2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
					}
					else if (op == 0xc4)
					{
						if (instruction->prefixes == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
							parseOperandVdq(instruction, &instruction->operands[0]);
						else
							parseOperandPq(instruction, &instruction->operands[0]);
						parseOperandEy(instruction, &instruction->operands[1]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
						instruction->operands[2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
					}
					else if (op == 0xc5)
					{
						parseOperandGd(instruction, &instruction->operands[0]);
						if (instruction->prefixes == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
							parseOperandUdq(instruction, &instruction->operands[1]);
						else
							parseOperandNq(instruction, &instruction->operands[1]);
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
						instruction->operands[2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
					}
					else if (op == 0xc7)
					{
						if (instruction->modrm.fields.mod == 0b11)
							parseOperandEv(instruction, &instruction->operands[0]);
						else
							parseModrmUpper32(instruction, &instruction->operands[0]);
						instruction->operands[0].action = (uint8_t)(instruction->modrm.fields.reg == 0b001 ? (NMD_X86_OPERAND_ACTION_READ | NMD_X86_OPERAND_ACTION_CONDITIONAL_WRITE) : (instruction->modrm.fields.mod == 0b11 || !instruction->simdPrefix ? NMD_X86_OPERAND_ACTION_WRITE : NMD_X86_OPERAND_ACTION_READ));
					}
					else if (op >= 0xc8 && op <= 0xcf)
					{
						instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
						instruction->operands[0].fields.reg = (uint8_t)((instruction->prefixes & (NMD_X86_PREFIXES_REX_W | NMD_X86_PREFIXES_REX_B)) == (NMD_X86_PREFIXES_REX_W | NMD_X86_PREFIXES_REX_B) ? NMD_X86_REG_R8 : (instruction->prefixes & NMD_X86_PREFIXES_REX_W ? NMD_X86_REG_RAX : (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? NMD_X86_REG_R8D : NMD_X86_REG_EAX)) + (op % 8));
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ_WRITE;
					}
					else if (NMD_R(op) >= 0xd)
					{
						if (op == 0xff)
						{
							parseOperandGd(instruction, &instruction->operands[0]);
							parseMemoryOperand(instruction, &instruction->operands[1], NMD_X86_REG_EAX);
						}
						else if (op == 0xd6 && instruction->simdPrefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
						{
							if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT)
							{
								parseOperandVdq(instruction, &instruction->operands[0]);
								parseOperandQq(instruction, &instruction->operands[1]);
							}
							else
							{
								parseOperandPq(instruction, &instruction->operands[0]);
								parseOperandWdq(instruction, &instruction->operands[1]);
							}
						}
						else
						{
							const size_t opIndex1 = op == 0xe7 || op == 0xd6 ? 1 : 0;
							const size_t opIndex2 = op == 0xe7 || op == 0xd6 ? 0 : 1;

							if (!instruction->simdPrefix)
							{
								if (op == 0xd7)
									parseOperandGd(instruction, &instruction->operands[0]);
								else
									parseOperandPq(instruction, &instruction->operands[opIndex1]);
								parseOperandQq(instruction, &instruction->operands[opIndex2]);
							}
							else
							{
								if (op == 0xd7)
									parseOperandGd(instruction, &instruction->operands[0]);
								else
									parseOperandVdq(instruction, &instruction->operands[opIndex1]);
								parseOperandWdq(instruction, &instruction->operands[opIndex2]);
							}
						}
						instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
					}
				}
			}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS */
		}
	}
	else /* 1 byte opcode */
	{
		instruction->opcodeSize = 1;
		instruction->opcode = *b;
		instruction->opcodeMap = NMD_X86_OPCODE_MAP_DEFAULT;

		op = instruction->opcode;

		/* Check for ModR/M, SIB and displacement. */
		if (NMD_R(op) == 8 || nmd_findByte(op1modrm, sizeof(op1modrm), op) || (NMD_R(op) < 4 && (NMD_C(op) < 4 || (NMD_C(op) >= 8 && NMD_C(op) < 0xC))) || (NMD_R(op) == 0xD && NMD_C(op) >= 8) || (remainingSize > 1 && ((NMD_Modrm*)(b + 1))->fields.mod != 0b11 && (op == 0xc4 || op == 0xc5 || op == 0x62)))
		{
			if (!parseModrm(&b, instruction, remainingSize - 1))
				return false;
		}

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_EVEX
		/* Check if instruction is EVEX. */
		if (flags & NMD_X86_DECODER_FLAGS_EVEX && op == 0x62 && !instruction->hasModrm)
		{
			instruction->encoding = NMD_X86_ENCODING_EVEX;
		}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_EVEX */
#if !defined(NMD_ASSEMBLY_DISABLE_DECODER_EVEX) && !defined(NMD_ASSEMBLY_DISABLE_DECODER_VEX)
		else
#endif
#ifndef NMD_ASSEMBLY_DISABLE_DECODER_VEX
			/* Check if instruction is VEX. */
			if (flags & NMD_X86_DECODER_FLAGS_VEX && (op == 0xc4 || op == 0xc5) && !instruction->hasModrm)
			{
				instruction->encoding = NMD_X86_ENCODING_VEX;

				instruction->vex.vex[0] = op;
				if (remainingSize < 4)
					return false;

				const uint8_t byte1 = *++b;

				instruction->vex.R = byte1 & 0b10000000;
				if (instruction->vex.vex[0] == 0xc4)
				{
					instruction->vex.X = (byte1 & 0b01000000) == 0b01000000;
					instruction->vex.B = (byte1 & 0b00100000) == 0b00100000;
					instruction->vex.m_mmmm = (uint8_t)(byte1 & 0b00011111);

					const uint8_t byte2 = *++b;
					instruction->vex.W = (byte2 & 0b10000000) == 0b10000000;
					instruction->vex.vvvv = (uint8_t)((byte2 & 0b01111000) >> 3);
					instruction->vex.L = (byte2 & 0b00000100) == 0b00000100;
					instruction->vex.pp = (uint8_t)(byte2 & 0b00000011);

					instruction->opcode = *++b;
					op = instruction->opcode;

					if (op == 0x0c || op == 0x0d || op == 0x40 || op == 0x41 || op == 0x17 || op == 0x21 || op == 0x42)
						instruction->immMask = NMD_X86_IMM8;

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK
					/* Check if the instruction is invalid. */
					if (op == 0x0c && instruction->vex.m_mmmm != 3)
						return false;
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK */


#ifndef NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID
					/*if(op == 0x0c)
					instruction->id = NMD_X86_INSTR
					*/
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID */
				}
				else /* 0xc5 */
				{
					instruction->vex.vvvv = (uint8_t)(byte1 & 0b01111000);
					instruction->vex.L = byte1 & 0b00000100;
					instruction->vex.pp = (uint8_t)(byte1 & 0b00000011);

					b++;
					instruction->opcode = *b;
					op = instruction->opcode;
				}

				if (!parseModrm(&b, instruction, remainingSize - (instruction->vex.vex[0] == 0xc4 ? 4 : 3)))
					return false;
			}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_VEX */
#if !(defined(NMD_ASSEMBLY_DISABLE_DECODER_EVEX) && defined(NMD_ASSEMBLY_DISABLE_DECODER_VEX))
			else
#endif
			{
				const NMD_Modrm modrm = instruction->modrm;
#ifndef NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK
				/* Check if the instruction is invalid. */
				if (flags & NMD_X86_DECODER_FLAGS_VALIDITY_CHECK)
				{
					if (op == 0xC6 || op == 0xC7)
					{
						if ((modrm.fields.reg != 0b000 && modrm.fields.reg != 0b111) || (modrm.fields.reg == 0b111 && (modrm.fields.mod != 0b11 || modrm.fields.rm != 0b000)))
							return false;
					}
					else if (op == 0x8f)
					{
						if (modrm.fields.reg != 0b000)
							return false;
					}
					else if (op == 0xfe)
					{
						if (modrm.fields.reg >= 0b010)
							return false;
					}
					else if (op == 0xff)
					{
						if (modrm.fields.reg == 0b111 || (modrm.fields.mod == 0b11 && (modrm.fields.reg == 0b011 || modrm.fields.reg == 0b101)))
							return false;
					}
					else if (op == 0x8c)
					{
						if (modrm.fields.reg >= 0b110)
							return false;
					}
					else if (op == 0x8e)
					{
						if (modrm.fields.reg == 0b001 || modrm.fields.reg >= 0b110)
							return false;
					}
					else if (op == 0x62)
					{
						if (mode == NMD_X86_MODE_64)
							return false;
					}
					else if (op == 0x8d)
					{
						if (modrm.fields.mod == 0b11)
							return false;
					}
					else if (op == 0xc4 || op == 0xc5)
					{
						if (mode == NMD_X86_MODE_64 && instruction->hasModrm && modrm.fields.mod != 0b11)
							return false;
					}
					else if (op >= 0xd8 && op <= 0xdf)
					{
						switch (op)
						{
						case 0xd9:
							if ((modrm.fields.reg == 0b001 && modrm.fields.mod != 0b11) || (modrm.modrm > 0xd0 && modrm.modrm < 0xd8) || modrm.modrm == 0xe2 || modrm.modrm == 0xe3 || modrm.modrm == 0xe6 || modrm.modrm == 0xe7 || modrm.modrm == 0xef)
								return false;
							break;
						case 0xda:
							if (modrm.modrm >= 0xe0 && modrm.modrm != 0xe9)
								return false;
							break;
						case 0xdb:
							if (((modrm.fields.reg == 0b100 || modrm.fields.reg == 0b110) && modrm.fields.mod != 0b11) || (modrm.modrm >= 0xe5 && modrm.modrm <= 0xe7) || modrm.modrm >= 0xf8)
								return false;
							break;
						case 0xdd:
							if ((modrm.fields.reg == 0b101 && modrm.fields.mod != 0b11) || NMD_R(modrm.modrm) == 0xf)
								return false;
							break;
						case 0xde:
							if (modrm.modrm == 0xd8 || (modrm.modrm >= 0xda && modrm.modrm <= 0xdf))
								return false;
							break;
						case 0xdf:
							if ((modrm.modrm >= 0xe1 && modrm.modrm <= 0xe7) || modrm.modrm >= 0xf8)
								return false;
							break;
						}
					}
					else if (mode == NMD_X86_MODE_64)
					{
						if (op == 0x6 || op == 0x7 || op == 0xe || op == 0x16 || op == 0x17 || op == 0x1e || op == 0x1f || op == 0x27 || op == 0x2f || op == 0x37 || op == 0x3f || (op >= 0x60 && op <= 0x62) || op == 0x82 || op == 0xce || (op >= 0xd4 && op <= 0xd6))
							return false;
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_VALIDITY_CHECK */

				/* Check for immediate */
				if (nmd_findByte(op1imm32, sizeof(op1imm32), op) || (NMD_R(op) < 4 && (NMD_C(op) == 5 || NMD_C(op) == 0xD)) || (NMD_R(op) == 0xB && NMD_C(op) >= 8) || (op == 0xF7 && modrm.fields.reg == 0b000)) /* imm32,16 */
				{
					if (NMD_R(op) == 0xB && NMD_C(op) >= 8)
						instruction->immMask = (uint8_t)(instruction->prefixes & NMD_X86_PREFIXES_REX_W ? NMD_X86_IMM64 : (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || (mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? NMD_X86_IMM16 : NMD_X86_IMM32));
					else
					{
						if (mode == NMD_X86_MODE_16 && instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || mode != NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE))
							instruction->immMask = NMD_X86_IMM32;
						else
							instruction->immMask = NMD_X86_IMM16;
					}
				}
				else if (NMD_R(op) == 7 || (NMD_R(op) == 0xE && NMD_C(op) < 8) || (NMD_R(op) == 0xB && NMD_C(op) < 8) || (NMD_R(op) < 4 && (NMD_C(op) == 4 || NMD_C(op) == 0xC)) || (op == 0xF6 && modrm.fields.reg <= 0b001) || nmd_findByte(op1imm8, sizeof(op1imm8), op)) /* imm8 */
					instruction->immMask = NMD_X86_IMM8;
				else if (NMD_R(op) == 0xA && NMD_C(op) < 4)
					instruction->immMask = (uint8_t)(mode == NMD_X86_MODE_64 ? (instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE ? NMD_X86_IMM32 : NMD_X86_IMM64) : (instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE ? NMD_X86_IMM16 : NMD_X86_IMM32));
				else if (op == 0xEA || op == 0x9A) /* imm32,48 */
				{
					if (mode == NMD_X86_MODE_64)
						return false;
					instruction->immMask = (uint8_t)(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_IMM32 : NMD_X86_IMM48);
				}
				else if (op == 0xC2 || op == 0xCA) /* imm16 */
					instruction->immMask = NMD_X86_IMM16;
				else if (op == 0xC8) /* imm16 + imm8 */
					instruction->immMask = NMD_X86_IMM16 | NMD_X86_IMM8;


#ifndef NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID
				if (flags & NMD_X86_DECODER_FLAGS_INSTRUCTION_ID)
				{
					const bool operandSize = instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE;
					if ((op >= 0x88 && op <= 0x8c) || (op >= 0xa0 && op <= 0xa3) || NMD_R(op) == 0xb || op == 0x8e)
						instruction->id = NMD_X86_INSTRUCTION_MOV;
					else if (NMD_R(op) == 5)
						instruction->id = (uint16_t)((NMD_C(op) < 8) ? NMD_X86_INSTRUCTION_PUSH : NMD_X86_INSTRUCTION_POP);
					else if (NMD_R(op) < 4 && (op % 8 < 6))
						instruction->id = (NMD_X86_INSTRUCTION_ADD + (NMD_R(op) << 1) + (NMD_C(op) >= 8 ? 1 : 0));
					else if (op >= 0x80 && op <= 0x84)
						instruction->id = NMD_X86_INSTRUCTION_ADD + modrm.fields.reg;
					else if (op == 0xe8)
						instruction->id = NMD_X86_INSTRUCTION_CALL;
					else if (op == 0xcc)
						instruction->id = NMD_X86_INSTRUCTION_INT3;
					else if (op == 0x8d)
						instruction->id = NMD_X86_INSTRUCTION_LEA;
					else if (NMD_R(op) == 4)
						instruction->id = (uint16_t)((NMD_C(op) < 8) ? NMD_X86_INSTRUCTION_INC : NMD_X86_INSTRUCTION_DEC);
					else if (NMD_R(op) == 7)
						instruction->id = NMD_X86_INSTRUCTION_JO + NMD_C(op);
					else if (op == 0xff)
						instruction->id = NMD_X86_INSTRUCTION_INC + modrm.fields.reg;
					else if (op == 0xeb || op == 0xe9)
						instruction->id = NMD_X86_INSTRUCTION_JMP;
					else if (op == 0x90)
					{
						if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT)
							instruction->id = NMD_X86_INSTRUCTION_PAUSE;
						else if (instruction->prefixes & NMD_X86_PREFIXES_REX_B)
							instruction->id = NMD_X86_INSTRUCTION_XCHG;
						else
							instruction->id = NMD_X86_INSTRUCTION_NOP;
					}
					else if (op == 0xc3 || op == 0xc2)
						instruction->id = NMD_X86_INSTRUCTION_RET;
					else if ((op >= 0x91 && op <= 0x97) || op == 0x86 || op == 0x87)
						instruction->id = NMD_X86_INSTRUCTION_XCHG;
					else if (op == 0xc0 || op == 0xc1 || (op >= 0xd0 && op <= 0xd3))
						instruction->id = NMD_X86_INSTRUCTION_ROL + modrm.fields.reg;
					else if (NMD_R(op) == 0x0f && (op % 8 < 6))
						instruction->id = NMD_X86_INSTRUCTION_INT1 + (op - 0xf1);
					else if (op >= 0xd4 && op <= 0xd7)
						instruction->id = NMD_X86_INSTRUCTION_AAM + (op - 0xd4);
					else if (op >= 0xe0 && op <= 0xe3)
						instruction->id = NMD_X86_INSTRUCTION_LOOPNE + (op - 0xe0);
					else /* case 0x: instruction->id = NMD_X86_INSTRUCTION_; break; */
					{
						switch (op)
						{
						case 0x8f: instruction->id = NMD_X86_INSTRUCTION_POP; break;
						case 0xfe: instruction->id = (uint16_t)(modrm.fields.reg == 0b000 ? NMD_X86_INSTRUCTION_INC : NMD_X86_INSTRUCTION_DEC); break;
						case 0x84: case 0x85: case 0xa8: case 0xa9: instruction->id = NMD_X86_INSTRUCTION_TEST; break;
						case 0xf6: case 0xf7: instruction->id = NMD_X86_INSTRUCTION_TEST + modrm.fields.reg; break;
						case 0x69: case 0x6b: instruction->id = NMD_X86_INSTRUCTION_IMUL; break;
						case 0x9a: instruction->id = NMD_X86_INSTRUCTION_CALL; break;
						case 0x62: instruction->id = NMD_X86_INSTRUCTION_BOUND; break;
						case 0x63: instruction->id = (uint16_t)(mode == NMD_X86_MODE_64 ? NMD_X86_INSTRUCTION_MOVSXD : NMD_X86_INSTRUCTION_ARPL); break;
						case 0x68: case 0x6a: case 0x06: case 0x16: case 0x1e: case 0x0e: instruction->id = NMD_X86_INSTRUCTION_PUSH; break;
						case 0x6c: instruction->id = NMD_X86_INSTRUCTION_INSB; break;
						case 0x6d: instruction->id = (uint16_t)(operandSize ? NMD_X86_INSTRUCTION_INSW : NMD_X86_INSTRUCTION_INSD); break;
						case 0x6e: instruction->id = NMD_X86_INSTRUCTION_OUTSB; break;
						case 0x6f: instruction->id = (uint16_t)(operandSize ? NMD_X86_INSTRUCTION_OUTSW : NMD_X86_INSTRUCTION_OUTSD); break;
						case 0xc2: case 0xc3:; break;
						case 0xc4: instruction->id = NMD_X86_INSTRUCTION_LES; break;
						case 0xc5: instruction->id = NMD_X86_INSTRUCTION_LDS; break;
						case 0xc6: case 0xc7: instruction->id = (uint16_t)(modrm.fields.reg == 0b000 ? NMD_X86_INSTRUCTION_MOV : (instruction->opcode == 0xc6 ? NMD_X86_INSTRUCTION_XABORT : NMD_X86_INSTRUCTION_XBEGIN)); break;
						case 0xc8: instruction->id = NMD_X86_INSTRUCTION_ENTER; break;
						case 0xc9: instruction->id = NMD_X86_INSTRUCTION_LEAVE; break;
						case 0xca: case 0xcb: instruction->id = NMD_X86_INSTRUCTION_RETF; break;
						case 0xcd: instruction->id = NMD_X86_INSTRUCTION_INT; break;
						case 0xce: instruction->id = NMD_X86_INSTRUCTION_INTO; break;
						case 0xcf:
							if (instruction->operandSize64)
								instruction->id = NMD_X86_INSTRUCTION_IRETQ;
							else if (mode == NMD_X86_MODE_16)
								instruction->id = operandSize ? NMD_X86_INSTRUCTION_IRETD : NMD_X86_INSTRUCTION_IRET;
							else
								instruction->id = operandSize ? NMD_X86_INSTRUCTION_IRET : NMD_X86_INSTRUCTION_IRETD;
							break;
						case 0xe4: case 0xe5: case 0xec: case 0xed: instruction->id = NMD_X86_INSTRUCTION_IN; break;
						case 0xe6: case 0xe7: case 0xee: case 0xef: instruction->id = NMD_X86_INSTRUCTION_OUT; break;
						case 0xea: instruction->id = NMD_X86_INSTRUCTION_LJMP; break;
						case 0x9c:
							if (operandSize)
								instruction->id = (uint16_t)(mode == NMD_X86_MODE_16 ? NMD_X86_INSTRUCTION_PUSHFD : NMD_X86_INSTRUCTION_PUSHF);
							else
								instruction->id = (uint16_t)(mode == NMD_X86_MODE_16 ? NMD_X86_INSTRUCTION_PUSHF : (mode == NMD_X86_MODE_32 ? NMD_X86_INSTRUCTION_PUSHFD : NMD_X86_INSTRUCTION_PUSHFQ));
							break;
						case 0x9d:
							if (operandSize)
								instruction->id = (uint16_t)(mode == NMD_X86_MODE_16 ? NMD_X86_INSTRUCTION_POPFD : NMD_X86_INSTRUCTION_POPF);
							else
								instruction->id = (uint16_t)(mode == NMD_X86_MODE_16 ? NMD_X86_INSTRUCTION_POPF : (mode == NMD_X86_MODE_32 ? NMD_X86_INSTRUCTION_POPFD : NMD_X86_INSTRUCTION_POPFQ));
							break;
						case 0x60:
						case 0x61:
							instruction->id = (uint16_t)(operandSize ? (instruction->opcode == 0x60 ? NMD_X86_INSTRUCTION_PUSHA : NMD_X86_INSTRUCTION_POPA) : (instruction->opcode == 0x60 ? NMD_X86_INSTRUCTION_PUSHAD : NMD_X86_INSTRUCTION_POPAD));
							break;
						case 0x07: case 0x17: case 0x1f: instruction->id = NMD_X86_INSTRUCTION_POP; break;
						case 0x27: instruction->id = NMD_X86_INSTRUCTION_DAA; break;
						case 0x37: instruction->id = NMD_X86_INSTRUCTION_AAA; break;
						case 0x2f: instruction->id = NMD_X86_INSTRUCTION_DAS; break;
						case 0x3f: instruction->id = NMD_X86_INSTRUCTION_AAS; break;
						case 0x9b: instruction->id = NMD_X86_INSTRUCTION_FWAIT; break;
						case 0x9e: instruction->id = NMD_X86_INSTRUCTION_SAHF; break;
						case 0x9f: instruction->id = NMD_X86_INSTRUCTION_LAHF; break;
						case 0xA4: instruction->id = NMD_X86_INSTRUCTION_MOVSB; break;
						case 0xA5: instruction->id = (uint16_t)(instruction->operandSize64 ? NMD_X86_INSTRUCTION_MOVSQ : (operandSize ? NMD_X86_INSTRUCTION_MOVSW : NMD_X86_INSTRUCTION_MOVSD)); break;
						case 0xA6: instruction->id = NMD_X86_INSTRUCTION_CMPSB; break;
						case 0xA7: instruction->id = (uint16_t)(instruction->operandSize64 ? NMD_X86_INSTRUCTION_CMPSQ : (operandSize ? NMD_X86_INSTRUCTION_CMPSW : NMD_X86_INSTRUCTION_CMPSD)); break;
						case 0xAA: instruction->id = NMD_X86_INSTRUCTION_STOSB; break;
						case 0xAB: instruction->id = (uint16_t)(instruction->operandSize64 ? NMD_X86_INSTRUCTION_STOSQ : (operandSize ? NMD_X86_INSTRUCTION_STOSW : NMD_X86_INSTRUCTION_STOSD)); break;
						case 0xAC: instruction->id = NMD_X86_INSTRUCTION_LODSB; break;
						case 0xAD: instruction->id = (uint16_t)(instruction->operandSize64 ? NMD_X86_INSTRUCTION_LODSQ : (operandSize ? NMD_X86_INSTRUCTION_LODSW : NMD_X86_INSTRUCTION_LODSD)); break;
						case 0xAE: instruction->id = NMD_X86_INSTRUCTION_SCASB; break;
						case 0xAF: instruction->id = (uint16_t)(instruction->operandSize64 ? NMD_X86_INSTRUCTION_SCASQ : (operandSize ? NMD_X86_INSTRUCTION_SCASW : NMD_X86_INSTRUCTION_SCASD)); break;
						case 0x98: instruction->id = (uint16_t)(instruction->prefixes & NMD_X86_PREFIXES_REX_W ? NMD_X86_INSTRUCTION_CDQE : (operandSize ? NMD_X86_INSTRUCTION_CBW : NMD_X86_INSTRUCTION_CWDE)); break;
						case 0x99: instruction->id = (uint16_t)(instruction->prefixes & NMD_X86_PREFIXES_REX_W ? NMD_X86_INSTRUCTION_CQO : (operandSize ? NMD_X86_INSTRUCTION_CWD : NMD_X86_INSTRUCTION_CDQ)); break;
						case 0xd6: instruction->id = NMD_X86_INSTRUCTION_SALC; break;

							/* Floating-point opcodes. */
#define NMD_F_OP_GET_OFFSET() ((NMD_R(modrm.modrm) - 0xc) << 1) + (NMD_C(op) >= 8 ? 1 : 0)
						case 0xd8: instruction->id = (NMD_X86_INSTRUCTION_FADD + (modrm.fields.mod == 0b11 ? NMD_F_OP_GET_OFFSET() : modrm.fields.reg)); break;
						case 0xd9:
							if (modrm.fields.mod == 0b11)
							{
								if (modrm.modrm <= 0xcf)
									instruction->id = (uint16_t)(modrm.modrm <= 0xc7 ? NMD_X86_INSTRUCTION_FLD : NMD_X86_INSTRUCTION_FXCH);
								else if (modrm.modrm >= 0xd8 && modrm.modrm <= 0xdf)
									instruction->id = NMD_X86_INSTRUCTION_FSTPNCE;
								else if (modrm.modrm == 0xd0)
									instruction->id = NMD_X86_INSTRUCTION_FNOP;
								else
									instruction->id = NMD_X86_INSTRUCTION_FCHS + (modrm.modrm - 0xe0);
							}
							else
								instruction->id = NMD_X86_INSTRUCTION_FLD + modrm.fields.reg;
							break;
						case 0xda:
							if (modrm.fields.mod == 0b11)
								instruction->id = ((modrm.modrm == 0xe9) ? NMD_X86_INSTRUCTION_FUCOMPP : NMD_X86_INSTRUCTION_FCMOVB + NMD_F_OP_GET_OFFSET());
							else
								instruction->id = NMD_X86_INSTRUCTION_FIADD + modrm.fields.reg;
							break;
						case 0xdb:
							if (modrm.fields.mod == 0b11)
								instruction->id = (modrm.modrm == 0xe2 ? NMD_X86_INSTRUCTION_FNCLEX : (modrm.modrm == 0xe2 ? NMD_X86_INSTRUCTION_FNINIT : NMD_X86_INSTRUCTION_FCMOVNB + NMD_F_OP_GET_OFFSET()));
							else
								instruction->id = (modrm.fields.reg == 0b101 ? NMD_X86_INSTRUCTION_FLD : (modrm.fields.reg == 0b111 ? NMD_X86_INSTRUCTION_FSTP : NMD_X86_INSTRUCTION_FILD + modrm.fields.reg));
							break;
						case 0xdc:
							if (modrm.fields.mod == 0b11)
								instruction->id = (NMD_X86_INSTRUCTION_FADD + ((NMD_R(modrm.modrm) - 0xc) << 1) + ((NMD_C(modrm.modrm) >= 8 && NMD_R(modrm.modrm) <= 0xd) ? 1 : 0));
							else
								instruction->id = NMD_X86_INSTRUCTION_FADD + modrm.fields.reg;
							break;
						case 0xdd:
							if (modrm.fields.mod == 0b11)
							{
								switch ((modrm.modrm - 0xc0) >> 3)
								{
								case 0b000: instruction->id = NMD_X86_INSTRUCTION_FFREE; break;
								case 0b001: instruction->id = NMD_X86_INSTRUCTION_FXCH; break;
								case 0b010: instruction->id = NMD_X86_INSTRUCTION_FST; break;
								case 0b011: instruction->id = NMD_X86_INSTRUCTION_FSTP; break;
								case 0b100: instruction->id = NMD_X86_INSTRUCTION_FUCOM; break;
								case 0b101: instruction->id = NMD_X86_INSTRUCTION_FUCOMP; break;
								}
							}
							else
							{
								switch (modrm.fields.reg)
								{
								case 0b000: instruction->id = NMD_X86_INSTRUCTION_FLD; break;
								case 0b001: instruction->id = NMD_X86_INSTRUCTION_FISTTP; break;
								case 0b010: instruction->id = NMD_X86_INSTRUCTION_FST; break;
								case 0b011: instruction->id = NMD_X86_INSTRUCTION_FSTP; break;
								case 0b100: instruction->id = NMD_X86_INSTRUCTION_FRSTOR; break;
								case 0b110: instruction->id = NMD_X86_INSTRUCTION_FNSAVE; break;
								case 0b111: instruction->id = NMD_X86_INSTRUCTION_FNSTSW; break;
								}
							}
							break;
						case 0xde:
							if (modrm.fields.mod == 0b11)
								instruction->id = (modrm.modrm == 0xd9 ? NMD_X86_INSTRUCTION_FCOMPP : ((modrm.modrm >= 0xd0 && modrm.modrm <= 0xd7) ? NMD_X86_INSTRUCTION_FCOMP : NMD_X86_INSTRUCTION_FADDP + NMD_F_OP_GET_OFFSET()));
							else
								instruction->id = NMD_X86_INSTRUCTION_FIADD + modrm.fields.reg;
							break;
						case 0xdf:
							if (modrm.fields.mod == 0b11)
							{
								if (modrm.fields.reg == 0b000)
									instruction->id = NMD_X86_INSTRUCTION_FFREEP;
								else if (modrm.fields.reg == 0b001)
									instruction->id = NMD_X86_INSTRUCTION_FXCH;
								else if (modrm.fields.reg <= 3)
									instruction->id = NMD_X86_INSTRUCTION_FSTP;
								else if (modrm.modrm == 0xe0)
									instruction->id = NMD_X86_INSTRUCTION_FNSTSW;
								else if (modrm.fields.reg == 0b110)
									instruction->id = NMD_X86_INSTRUCTION_FCOMIP;
								else
									instruction->id = NMD_X86_INSTRUCTION_FUCOMIP;
							}
							else
								instruction->id = (modrm.fields.reg == 0b101 ? NMD_X86_INSTRUCTION_FILD : (modrm.fields.reg == 0b111 ? NMD_X86_INSTRUCTION_FISTP : (NMD_X86_INSTRUCTION_FILD + modrm.fields.reg)));
							break;
						}
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_INSTRUCTION_ID */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_CPU_FLAGS
				if (flags & NMD_X86_DECODER_FLAGS_CPU_FLAGS)
				{
					if (op == 0xcc || op == 0xcd || op == 0xce) /* int3,int,into */
					{
						instruction->clearedFlags.eflags = NMD_X86_EFLAGS_TF | NMD_X86_EFLAGS_RF;
						instruction->testedFlags.eflags = NMD_X86_EFLAGS_NT | NMD_X86_EFLAGS_VM;
						instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_NT | NMD_X86_EFLAGS_VM | NMD_X86_EFLAGS_AC | NMD_X86_EFLAGS_VIF;
					}
					else if (NMD_R(op) == 7) /* conditional jump */
						decodeConditionalFlag(instruction, NMD_C(op));
					else if (NMD_R(op) == 4 || ((op == 0xfe || op == 0xff) && modrm.fields.reg <= 0b001)) /* inc,dec */
						instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_PF;
					else if (op <= 0x05 || (op >= 0x10 && op <= 0x15) || ((NMD_R(op) == 1 || NMD_R(op) == 2 || NMD_R(op) == 3) && (NMD_C(op) >= 0x8 && NMD_C(op) <= 0x0d)) || ((op >= 0x80 && op <= 0x83) && (modrm.fields.reg == 0b000 || modrm.fields.reg == 0b010 || modrm.fields.reg == 0b011 || modrm.fields.reg == 0b010 || modrm.fields.reg == 0b101 || modrm.fields.reg == 0b111)) || (op == 0xa6 || op == 0xa7) || (op == 0xae || op == 0xaf)) /* add,adc,sbb,sub,cmp, cmps,cmpsb,cmpsw,cmpsd,cmpsq, scas,scasb,scasw,scasd */
						instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF;
					else if ((op >= 0x08 && op <= 0x0d) || ((NMD_R(op) == 2 || NMD_R(op) == 3) && NMD_C(op) <= 5) || ((op >= 0x80 && op <= 0x83) && (modrm.fields.reg == 0b001 || modrm.fields.reg == 0b100 || modrm.fields.reg == 0b110)) || (op == 0x84 || op == 0x85 || op == 0xa8 || op == 0xa9) || ((op == 0xf6 || op == 0xf7) && modrm.fields.reg == 0b000)) /* or,and,xor, test */
					{
						instruction->clearedFlags.eflags = NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_CF;
						instruction->undefinedFlags.eflags = NMD_X86_EFLAGS_AF;
						instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_PF;
					}
					else if (op == 0x69 || op == 0x6b || ((op == 0xf6 || op == 0xf7) && (modrm.fields.reg == 0b100 || modrm.fields.reg == 0b101))) /* mul,imul */
					{
						instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_OF;
						instruction->undefinedFlags.eflags = NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_PF;
					}
					else if (op == 0xf6 || op == 0xf7) /* Group 3 */
					{
						if (modrm.fields.reg == 0b011) /* neg */
							instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_OF;
						else if (modrm.fields.reg >= 0b110) /* div,idiv */
							instruction->undefinedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_PF;
					}
					else if (op == 0xc0 || op == 0xc1 || (op >= 0xd0 && op <= 0xd3))
					{
						if (modrm.fields.reg <= 0b011) /* rol,ror,rcl,rcr */
						{
							instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF;
							instruction->undefinedFlags.eflags = NMD_X86_EFLAGS_OF;
						}
						else /* shl,shr,sar */
						{
							instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_OF;
							instruction->undefinedFlags.eflags = NMD_X86_EFLAGS_AF;
						}
					}
					else if (op == 0x27 || op == 0x2f) /* daa,das */
					{
						instruction->testedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_AF;
						instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_PF;
						instruction->undefinedFlags.eflags = NMD_X86_EFLAGS_OF;
					}
					else if (op == 0x37 || op == 0x3f) /* aaa,aas */
					{
						instruction->testedFlags.eflags = NMD_X86_EFLAGS_AF;
						instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_CF;
						instruction->undefinedFlags.eflags = NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_PF;
					}
					else if (op == 0x63 && mode != NMD_X86_MODE_64) /* arpl */
						instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_ZF;
					else if (op == 0x9b) /* fwait,wait */
						instruction->undefinedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C1 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
					else if (op == 0x9e) /* sahf */
						instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_PF | NMD_X86_EFLAGS_CF;
					else if (op == 0xd4 || op == 0xd5) /* aam,aad */
					{
						instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_SF | NMD_X86_EFLAGS_ZF | NMD_X86_EFLAGS_PF;
						instruction->undefinedFlags.eflags = NMD_X86_EFLAGS_OF | NMD_X86_EFLAGS_AF | NMD_X86_EFLAGS_CF;
					}
					else if (op >= 0xd8 && op <= 0xdf) /* escape opcodes */
					{
						if (op == 0xd8 || op == 0xdc)
						{
							if (modrm.fields.reg == 0b000 || modrm.fields.reg == 0b001 || modrm.fields.reg == 0b100 || modrm.fields.reg == 0b101 || modrm.fields.reg == 0b110 || modrm.fields.reg == 0b111) /* fadd,fmul,fsub,fsubr,fdiv,fdivr */
							{
								instruction->modifiedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C1;
								instruction->undefinedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
							}
							else if (modrm.fields.reg == 0b010 || modrm.fields.reg == 0b011) /* fcom,fcomp */
							{
								instruction->modifiedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								instruction->clearedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C1;
							}
						}
						else if (op == 0xd9)
						{
							if (modrm.fields.mod != 0b11)
							{
								if (modrm.fields.reg == 0b000 || modrm.fields.reg == 0b010 || modrm.fields.reg == 0b011) /* fld,fst,fstp */
								{
									instruction->modifiedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefinedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
								else if (modrm.fields.reg == 0b100) /* fldenv */
									instruction->modifiedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C1 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								else if (modrm.fields.reg == 0b101 || modrm.fields.reg == 0b110 || modrm.fields.reg == 0b111) /* fldcw,fstenv,fstcw */
									instruction->undefinedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C1 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
							}
							else
							{
								if (modrm.modrm < 0xc8) /* fld */
								{
									instruction->modifiedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefinedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
								else /*if (modrm.modrm <= 0xcf)*/ /* fxch */
								{
									instruction->clearedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefinedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
							}
						}
						else if (op == 0xda || op == 0xde)
						{
							if (modrm.fields.mod != 0b11)
							{
								if (modrm.fields.reg == 0b000 || modrm.fields.reg == 0b001 || modrm.fields.reg == 0b100 || modrm.fields.reg == 0b101 || modrm.fields.reg == 0b110 || modrm.fields.reg == 0b111) /* fiadd,fimul,fisub,fisubr,fidiv,fidivr */
								{
									instruction->modifiedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefinedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
								else /*if (modrm.fields.reg == 0b010 || modrm.fields.reg == 0b011)*/ /* ficom,ficomp */
								{
									instruction->clearedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C1;
									instruction->modifiedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
							}
							else
							{

								if ((op == 0xda && modrm.modrm == 0xe9) || (op == 0xde && modrm.modrm == 0xd9))
									instruction->modifiedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C1 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								else
								{
									instruction->modifiedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefinedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
							}
						}
						else if (op == 0xdb || op == 0xdd || op == 0xdf)
						{
							if (modrm.fields.mod != 0b11)
							{
								if (modrm.fields.reg == 0b000 || modrm.fields.reg == 0b010 || modrm.fields.reg == 0b011 || modrm.fields.reg == 0b101 || modrm.fields.reg == 0b111) /* fild,fist,fistp,fld,fstp */
								{
									instruction->modifiedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefinedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
								else if (modrm.fields.reg == 0b001) /* fisttp */
								{
									instruction->clearedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefinedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
							}
							else
							{
								if (modrm.fields.reg <= 0b011) /* fcmovnb,fcmovne,fcmovnbe,fcmovnu */
								{
									instruction->modifiedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefinedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
								else if (modrm.modrm == 0xe0 || modrm.modrm == 0xe2) /* fstsw,fclex */
									instruction->undefinedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C1 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								else if (modrm.modrm == 0xe3) /* finit */
									instruction->clearedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C1 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								else /* fucomi,fcomi */
								{
									instruction->clearedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C1;
									instruction->undefinedFlags.fpuFlags = NMD_X86_FPU_FLAGS_C0 | NMD_X86_FPU_FLAGS_C2 | NMD_X86_FPU_FLAGS_C3;
								}
							}
						}
					}
					else if (op == 0xf5) /* cmc */
						instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_CF;
					else if (op == 0xf8) /* clc */
						instruction->clearedFlags.eflags = NMD_X86_EFLAGS_CF;
					else if (op == 0xf9) /* stc */
						instruction->setFlags.eflags = NMD_X86_EFLAGS_CF;
					else if (op == 0xfa || op == 0xfb) /* cli,sti */
					{
						instruction->modifiedFlags.eflags = NMD_X86_EFLAGS_IF | NMD_X86_EFLAGS_VIF;
						instruction->testedFlags.eflags = NMD_X86_EFLAGS_IOPL;
					}
					else if (op == 0xfc) /* cld */
						instruction->clearedFlags.eflags = NMD_X86_EFLAGS_DF;
					else if (op == 0xfd) /* std */
						instruction->setFlags.eflags = NMD_X86_EFLAGS_DF;
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_CPU_FLAGS */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_GROUP
				/* Parse the instruction's group. */
				if (flags & NMD_X86_DECODER_FLAGS_GROUP)
				{
					if (NMD_R(op) == 7 || op == 0xe3)
						instruction->group = NMD_GROUP_JUMP | NMD_GROUP_CONDITIONAL_BRANCH | NMD_GROUP_RELATIVE_ADDRESSING;
					else if (op == 0xe9 || op == 0xea || op == 0xeb || (op == 0xff && (modrm.fields.reg == 0b100 || modrm.fields.reg == 0b101)))
						instruction->group = NMD_GROUP_JUMP | NMD_GROUP_UNCONDITIONAL_BRANCH | (op == 0xe9 || op == 0xeb ? NMD_GROUP_RELATIVE_ADDRESSING : 0);
					else if (op == 0x9a || op == 0xe8 || (op == 0xff && (modrm.fields.reg == 0b010 || modrm.fields.reg == 0b011)))
						instruction->group = NMD_GROUP_CALL | NMD_GROUP_UNCONDITIONAL_BRANCH | (op == 0xe8 ? NMD_GROUP_RELATIVE_ADDRESSING : 0);
					else if (op == 0xc2 || op == 0xc3 || op == 0xca || op == 0xcb)
						instruction->group = NMD_GROUP_RET;
					else if ((op >= 0xcc && op <= 0xce) || op == 0xf1)
						instruction->group = NMD_GROUP_INT;
					else if (op == 0xf4)
						instruction->group = NMD_GROUP_PRIVILEGE;
					else if (op == 0xc7 && modrm.modrm == 0xf8)
						instruction->group = NMD_GROUP_UNCONDITIONAL_BRANCH | NMD_GROUP_RELATIVE_ADDRESSING;
					else if (op >= 0xe0 && op <= 0xe2)
						instruction->group = NMD_GROUP_CONDITIONAL_BRANCH | NMD_GROUP_RELATIVE_ADDRESSING;
					else if (op == 0x8d && mode == NMD_X86_MODE_64)
						instruction->group = NMD_GROUP_RELATIVE_ADDRESSING;
					else if (op == 0xcf)
						instruction->group = NMD_GROUP_RET | NMD_GROUP_INT;
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_GROUP */

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS
				if (flags & NMD_X86_DECODER_FLAGS_OPERANDS)
				{
					if (op >= 0xd8 && op <= 0xdf)
					{
						if (modrm.fields.mod == 0b11)
						{
							if ((op == 0xd9 && (NMD_R(modrm.modrm) == 0xc || (op >= 0xc8 && op <= 0xcf))) ||
								(op == 0xda && NMD_R(modrm.modrm) <= 0xd) ||
								(op == 0xdb && (NMD_R(modrm.modrm) <= 0xd || modrm.modrm >= 0xe8)) ||
								(op == 0xde && modrm.modrm != 0xd9) ||
								(op == 0xdf && modrm.modrm != 0xe0))
								instruction->numOperands = 2;
						}
						else
							instruction->numOperands = 1;
					}
					else if ((NMD_R(op) < 4 && op % 8 < 6) || (NMD_R(op) >= 8 && NMD_R(op) <= 0xb && op != 0x8f && op != 0x90 && !(op >= 0x98 && op <= 0x9f)) || op == 0x62 || op == 0x63 || (op >= 0x6c && op <= 0x6f) || op == 0xc0 || op == 0xc1 || (op >= 0xc4 && op <= 0xc8) || (op >= 0xd0 && op <= 0xd3) || (NMD_R(op) == 0xe && op % 8 >= 4))
						instruction->numOperands = 2;
					else if (NMD_R(op) == 4 || NMD_R(op) == 5 || NMD_R(op) == 7 || (op == 0x68 || op == 0x6a) || op == 0x8f || op == 0x9a || op == 0xc2 || op == 0xca || op == 0xcd || op == 0xd4 || op == 0xd5 || (NMD_R(op) == 0xe && op % 8 <= 3) || (NMD_R(op) == 0xf && op % 8 >= 6))
						instruction->numOperands = 1;
					else if (op == 0x69 || op == 0x6b)
						instruction->numOperands = 3;

					if (instruction->numOperands > 0)
					{
						if (op >= 0x84 && op <= 0x8b)
						{
							if (op % 2 == 0)
							{
								parseOperandEb(instruction, &instruction->operands[op == 0x8a ? 1 : 0]);
								parseOperandGb(instruction, &instruction->operands[op == 0x8a ? 0 : 1]);
							}
							else
							{
								parseOperandEv(instruction, &instruction->operands[op == 0x8b ? 1 : 0]);
								parseOperandGv(instruction, &instruction->operands[op == 0x8b ? 0 : 1]);
							}

							if (op >= 0x88)
							{
								instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
								instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
							}
							else if (op >= 0x86)
								instruction->operands[0].action = instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ_WRITE;
						}
						else if (op >= 0x80 && op <= 0x83)
						{
							if (op % 2 == 0)
								parseOperandEb(instruction, &instruction->operands[0]);
							else
								parseOperandEv(instruction, &instruction->operands[0]);
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ_WRITE;
							instruction->operands[1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
						}
						else if (op == 0x68 || NMD_R(op) == 7 || op == 0x6a || op == 0x9a || op == 0xc2 || op == 0xca || op == 0xcd || op == 0xd4 || op == 0xd5)
							instruction->operands[0].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
						else if (op == 0x90 && instruction->prefixes & NMD_X86_PREFIXES_REX_B)
						{
							instruction->operands[0].type = instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
							instruction->operands[0].fields.reg = (uint8_t)(instruction->prefixes & NMD_X86_PREFIXES_REX_W ? NMD_X86_REG_R8 : NMD_X86_REG_R8D);
							instruction->operands[1].fields.reg = (uint8_t)(instruction->prefixes & NMD_X86_PREFIXES_REX_W ? NMD_X86_REG_RAX : NMD_X86_REG_EAX);
						}
						else if (NMD_R(op) < 4)
						{
							const size_t opMod8 = (size_t)(op % 8);
							if (opMod8 == 0 || opMod8 == 2)
							{
								parseOperandEb(instruction, &instruction->operands[opMod8 == 0 ? 0 : 1]);
								parseOperandGb(instruction, &instruction->operands[opMod8 == 0 ? 1 : 0]);
							}
							else if (opMod8 == 1 || opMod8 == 3)
							{
								parseOperandEv(instruction, &instruction->operands[opMod8 == 1 ? 0 : 1]);
								parseOperandGv(instruction, &instruction->operands[opMod8 == 1 ? 1 : 0]);
							}
							else if (opMod8 == 4 || opMod8 == 5)
							{
								instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
								if (opMod8 == 4)
									instruction->operands[0].fields.reg = NMD_X86_REG_AL;
								else
									instruction->operands[0].fields.reg = (uint8_t)(instruction->operandSize64 ? NMD_X86_REG_RAX : (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_AX : NMD_X86_REG_EAX));

								instruction->operands[1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
							}

							instruction->operands[0].action = instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
							if (!(NMD_R(op) == 3 && NMD_C(op) >= 8))
								instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ_WRITE;
						}
						else if (NMD_R(op) == 4)
						{
							instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
							instruction->operands[0].fields.reg = (uint8_t)((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_AX : NMD_X86_REG_EAX) + (op % 8));
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ_WRITE;
						}
						else if (NMD_R(op) == 5)
						{
							instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
							instruction->operands[0].fields.reg = (uint8_t)((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_AX : (mode == NMD_X86_MODE_64 ? NMD_X86_REG_RAX : NMD_X86_REG_EAX)) + (op % 8));
							instruction->operands[0].action = (uint8_t)(NMD_C(op) < 8 ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_WRITE);
						}
						else if (op == 0x62)
						{
							parseOperandGv(instruction, &instruction->operands[0]);
							parseModrmUpper32(instruction, &instruction->operands[1]);
							instruction->operands[0].action = instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
						}
						else if (op == 0x63)
						{
							if (mode == NMD_X86_MODE_64)
							{
								parseOperandGv(instruction, &instruction->operands[0]);
								parseOperandEv(instruction, &instruction->operands[1]);
								instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
								instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
							}
							else
							{
								if (instruction->modrm.fields.mod == 0b11)
								{
									instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
									instruction->operands[0].fields.reg = NMD_X86_REG_AX + instruction->modrm.fields.rm;
								}
								else
									parseModrmUpper32(instruction, &instruction->operands[0]);

								instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
								instruction->operands[1].fields.reg = NMD_X86_REG_AX + instruction->modrm.fields.reg;
								instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ_WRITE;
								instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
							}
						}
						else if (op == 0x69 || op == 0x6b)
						{
							parseOperandGv(instruction, &instruction->operands[0]);
							parseOperandEv(instruction, &instruction->operands[1]);
							instruction->operands[2].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
							instruction->operands[2].fields.imm = (int64_t)(instruction->immediate);
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
							instruction->operands[1].action = instruction->operands[2].action = NMD_X86_OPERAND_ACTION_READ;
						}
						else if (op == 0x8c)
						{
							parseOperandEv(instruction, &instruction->operands[0]);
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
							instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
							instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
							instruction->operands[1].fields.reg = NMD_X86_REG_ES + instruction->modrm.fields.reg;
						}
						else if (op == 0x8d)
						{
							parseOperandGv(instruction, &instruction->operands[0]);
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
							instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
							parseModrmUpper32(instruction, &instruction->operands[1]);
						}
						else if (op == 0x8e)
						{
							instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
							instruction->operands[0].fields.reg = NMD_X86_REG_ES + instruction->modrm.fields.reg;
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
							instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
							parseOperandEw(instruction, &instruction->operands[1]);
						}
						else if (op == 0x8f)
						{
							parseOperandEv(instruction, &instruction->operands[0]);
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						}
						else if (op >= 0x91 && op <= 0x97)
						{
							parseOperandGv(instruction, &instruction->operands[0]);
							instruction->operands[0].fields.reg = instruction->operands[0].fields.reg + NMD_C(op);
							instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
							instruction->operands[1].fields.reg = (uint8_t)(instruction->operandSize64 ? NMD_X86_REG_RAX : (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && mode != NMD_X86_MODE_16 ? NMD_X86_REG_AX : NMD_X86_REG_EAX));
							instruction->operands[0].action = instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ_WRITE;
						}
						else if (op >= 0xa0 && op <= 0xa3)
						{
							instruction->operands[op < 0xa2 ? 0 : 1].type = NMD_X86_OPERAND_TYPE_REGISTER;
							instruction->operands[op < 0xa2 ? 0 : 1].fields.reg = (uint8_t)(op % 2 == 0 ? NMD_X86_REG_AL : (instruction->operandSize64 ? NMD_X86_REG_RAX : ((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && mode != NMD_X86_MODE_16) || (mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? NMD_X86_REG_AX : NMD_X86_REG_EAX)));
							instruction->operands[op < 0xa2 ? 1 : 0].type = NMD_X86_OPERAND_TYPE_MEMORY;
							instruction->operands[op < 0xa2 ? 1 : 0].fields.mem.disp = (mode == NMD_X86_MODE_64) ? *(uint64_t*)(b + 1) : *(uint32_t*)(b + 1);
							parseOperandSegmentRegister(instruction, &instruction->operands[op < 0xa2 ? 1 : 0]);
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
							instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
						}
						else if (op == 0xa8 || op == 0xa9)
						{
							instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
							instruction->operands[0].fields.reg = (uint8_t)(op == 0xa8 ? NMD_X86_REG_AL : (instruction->operandSize64 ? NMD_X86_REG_RAX : ((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && mode != NMD_X86_MODE_16) || (mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? NMD_X86_REG_AX : NMD_X86_REG_EAX)));
							instruction->operands[1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
						}
						else if (NMD_R(op) == 0xb)
						{
							instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
							instruction->operands[0].fields.reg = (uint8_t)((op < 0xb8 ? (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? NMD_X86_REG_R8B : NMD_X86_REG_AL) : (instruction->prefixes & NMD_X86_PREFIXES_REX_W ? (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? NMD_X86_REG_R8 : NMD_X86_REG_RAX) : (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? NMD_X86_REG_R8D : NMD_X86_REG_EAX))) + op % 8);
							instruction->operands[1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
						}
						else if (op == 0xc0 || op == 0xc1 || op == 0xc6 || op == 0xc7)
						{
							if (!(op >= 0xc6 && instruction->modrm.fields.reg))
							{
								if (op % 2 == 0)
									parseOperandEb(instruction, &instruction->operands[0]);
								else
									parseOperandEv(instruction, &instruction->operands[0]);
							}
							instruction->operands[op >= 0xc6 && instruction->modrm.fields.reg ? 0 : 1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
							instruction->operands[0].action = (uint8_t)(op <= 0xc1 ? NMD_X86_OPERAND_ACTION_READ_WRITE : NMD_X86_OPERAND_ACTION_WRITE);
						}
						else if (op == 0xc4 || op == 0xc5)
						{
							instruction->operands[0].type = NMD_X86_OPERAND_TYPE_REGISTER;
							instruction->operands[0].fields.reg = (uint8_t)((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_AX : NMD_X86_REG_EAX) + instruction->modrm.fields.reg);
							parseModrmUpper32(instruction, &instruction->operands[1]);
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_WRITE;
							instruction->operands[1].action = NMD_X86_OPERAND_ACTION_READ;
						}
						else if (op == 0xc8)
						{
							instruction->operands[0].type = instruction->operands[1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
							instruction->operands[0].size = 2;
							instruction->operands[0].fields.imm = *(uint16_t*)(b + 1);
							instruction->operands[1].size = 1;
							instruction->operands[1].fields.imm = b[3];
						}
						else if (op >= 0xd0 && op <= 0xd3)
						{
							if (op % 2 == 0)
								parseOperandEb(instruction, &instruction->operands[0]);
							else
								parseOperandEv(instruction, &instruction->operands[0]);

							if (op < 0xd2)
							{
								instruction->operands[1].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
								instruction->operands[1].fields.imm = 1;
							}
							else
							{
								instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
								instruction->operands[1].fields.reg = NMD_X86_REG_CL;
							}
							instruction->operands[0].action = NMD_X86_OPERAND_ACTION_READ_WRITE;
						}
						else if (op >= 0xd8 && op <= 0xdf)
						{
							if (instruction->modrm.fields.mod != 0b11 ||
								op == 0xd8 ||
								(op == 0xd9 && NMD_C(instruction->modrm.modrm) == 0xc) ||
								(op == 0xda && NMD_C(instruction->modrm.modrm) <= 0xd) ||
								(op == 0xdb && (NMD_C(instruction->modrm.modrm) <= 0xd || instruction->modrm.modrm >= 0xe8)) ||
								op == 0xdc ||
								op == 0xdd ||
								(op == 0xde && instruction->modrm.modrm != 0xd9) ||
								(op == 0xdf && instruction->modrm.modrm != 0xe0))
							{
								instruction->operands[0].type = instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
								instruction->operands[0].isImplicit = true;
								instruction->operands[0].fields.reg = NMD_X86_REG_ST0;
								instruction->operands[1].fields.reg = NMD_X86_REG_ST0 + instruction->modrm.fields.reg;
							}
						}
						else if (NMD_R(op) == 0xe)
						{
							if (op % 8 < 4)
							{
								instruction->operands[0].type = NMD_X86_OPERAND_TYPE_IMMEDIATE;
								instruction->operands[0].fields.imm = (int64_t)(instruction->immediate);
							}
							else
							{
								if (op < 0xe8)
								{
									instruction->operands[0].type = (uint8_t)(NMD_C(op) < 6 ? NMD_X86_OPERAND_TYPE_REGISTER : NMD_X86_OPERAND_TYPE_IMMEDIATE);
									instruction->operands[1].type = (uint8_t)(NMD_C(op) < 6 ? NMD_X86_OPERAND_TYPE_IMMEDIATE : NMD_X86_OPERAND_TYPE_REGISTER);
									instruction->operands[0].fields.imm = instruction->operands[1].fields.imm = (int64_t)(instruction->immediate);
								}
								else
								{
									instruction->operands[0].type = instruction->operands[1].type = NMD_X86_OPERAND_TYPE_REGISTER;
									instruction->operands[0].fields.reg = instruction->operands[1].fields.reg = NMD_X86_REG_DX;
								}

								if (op % 2 == 0)
									instruction->operands[op % 8 == 4 ? 0 : 1].fields.reg = NMD_X86_REG_AL;
								else
									instruction->operands[op % 8 == 5 ? 0 : 1].fields.reg = (uint8_t)((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? NMD_X86_REG_AX : NMD_X86_REG_EAX) + instruction->modrm.fields.reg);

								instruction->operands[op % 8 <= 5 ? 0 : 1].action = NMD_X86_OPERAND_ACTION_WRITE;
								instruction->operands[op % 8 <= 5 ? 1 : 0].action = NMD_X86_OPERAND_ACTION_READ;
							}
						}
						else if (op == 0xf6 || op == 0xfe)
						{
							parseOperandEb(instruction, &instruction->operands[0]);
							instruction->operands[0].action = (uint8_t)(op == 0xfe && instruction->modrm.fields.reg >= 0b010 ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_READ_WRITE);
						}
						else if (op == 0xf7 || op == 0xff)
						{
							parseOperandEv(instruction, &instruction->operands[0]);
							instruction->operands[0].action = (uint8_t)(op == 0xff && instruction->modrm.fields.reg >= 0b010 ? NMD_X86_OPERAND_ACTION_READ : NMD_X86_OPERAND_ACTION_READ_WRITE);
						}
					}
				}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS */
			}
	}

	if (instruction->prefixes & NMD_X86_PREFIXES_LOCK)
	{
		if (!(instruction->hasModrm && instruction->modrm.fields.mod != 0b11 &&
			((instruction->opcodeSize == 1 && (op == 0x86 || op == 0x87 || (NMD_R(op) < 4 && (op % 8) < 2 && op < 0x38) || ((op >= 0x80 && op <= 0x83) && instruction->modrm.fields.reg != 0b111) || (op >= 0xfe && instruction->modrm.fields.reg < 2) || ((op == 0xf6 || op == 0xf7) && (instruction->modrm.fields.reg == 0b010 || instruction->modrm.fields.reg == 0b011)))) ||
			(instruction->opcodeSize == 2 && (nmd_findByte(twoOpcodes, sizeof(twoOpcodes), op) || op == 0xab || (op == 0xba && instruction->modrm.fields.reg != 0b100) || (op == 0xc7 && instruction->modrm.fields.reg == 0b001))))))
			return false;
	}

	instruction->length = (uint8_t)((ptrdiff_t)(++b + (size_t)instruction->immMask) - (ptrdiff_t)(buffer));
	for (i = 0; i < instruction->length; i++)
		instruction->buffer[i] = ((const uint8_t*)(buffer))[i];

	for (i = 0; i < (size_t)instruction->immMask; i++)
		((uint8_t*)(&instruction->immediate))[i] = b[i];

#ifndef NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS
	for (i = 0; i < instruction->numOperands; i++)
	{
		if (instruction->operands[i].type == NMD_X86_OPERAND_TYPE_IMMEDIATE)
		{
			if (instruction->operands[i].action == NMD_X86_OPERAND_ACTION_NONE)
				instruction->operands[i].action = NMD_X86_OPERAND_ACTION_READ;

			if (instruction->operands[i].size == 0)
			{
				instruction->operands[i].size = (uint8_t)instruction->immMask;
				instruction->operands[i].fields.imm = instruction->immediate;
			}
		}
	}
#endif /* NMD_ASSEMBLY_DISABLE_DECODER_OPERANDS */

	instruction->valid = true;

	return true;
}

bool ldisasm_parseModrm(const uint8_t** b, bool addressPrefix, NMD_X86_MODE mode, NMD_Modrm* const pModrm, size_t remainingSize)
{
	if (remainingSize == 0)
		return false;

	const NMD_Modrm modrm = *(NMD_Modrm*)(++*b);
	*pModrm = modrm;
	bool hasSIB = false;
	size_t dispSize = 0;

	if (mode == NMD_X86_MODE_16)
	{
		if (modrm.fields.mod != 0b11)
		{
			if (modrm.fields.mod == 0b00)
			{
				if (modrm.fields.rm == 0b110)
					dispSize = 2;
			}
			else
				dispSize = modrm.fields.mod == 0b01 ? 1 : 2;
		}
	}
	else
	{
		if (addressPrefix && mode == NMD_X86_MODE_32)
		{
			if ((modrm.fields.mod == 0b00 && modrm.fields.rm == 0b110) || modrm.fields.mod == 0b10)
				dispSize = 2;
			else if (modrm.fields.mod == 0b01)
				dispSize = 1;
		}
		else
		{
			/* Check for SIB byte */
			uint8_t sib = 0;
			if (modrm.modrm < 0xC0 && modrm.fields.rm == 0b100 && (!addressPrefix || (addressPrefix && mode == NMD_X86_MODE_64)))
			{
				if (remainingSize < 2)
					return false;

				hasSIB = true;
				sib = *++*b;
			}

			if (modrm.fields.mod == 0b01) /* disp8 (ModR/M) */
				dispSize = 1;
			else if ((modrm.fields.mod == 0b00 && modrm.fields.rm == 0b101) || modrm.fields.mod == 0b10) /* disp16,32 (ModR/M) */
				dispSize = (addressPrefix && !(mode == NMD_X86_MODE_64 && addressPrefix) ? 2 : 4);
			else if (hasSIB && (sib & 0b111) == 0b101) /* disp8,32 (SIB) */
				dispSize = (modrm.fields.mod == 0b01 ? 1 : 4);
		}
	}

	*b += dispSize;

	return remainingSize - (hasSIB ? 2 : 1) >= dispSize;
}

/*
Returns the instruction's length if it's valid, zero otherwise.
Parameters:
- buffer     [in] A pointer to a buffer containing a encoded instruction.
- bufferSize [in] The buffer's size in bytes.
- mode       [in] The architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
*/
size_t nmd_x86_ldisasm(const void* buffer, size_t bufferSize, NMD_X86_MODE mode)
{
	const uint8_t* b = (const uint8_t*)(buffer);

	bool operandPrefix = false;
	bool addressPrefix = false;
	bool repeatPrefix = false;
	bool repeatNotZeroPrefix = false;
	bool rexW = false;
	bool lockPrefix = false;
	uint16_t simdPrefix = NMD_X86_PREFIXES_NONE;
	uint8_t op = 0;
	uint8_t opcodeSize = 0;

	bool hasModrm = false;
	NMD_Modrm modrm = { 0,0,0 };

	size_t offset = 0;

	/* Parse legacy prefixes & REX prefixes. */
	size_t i = 0;
	for (; i < NMD_X86_MAXIMUM_INSTRUCTION_LENGTH; i++, b++)
	{
		switch (*b)
		{
		case 0xF0: lockPrefix = true; continue;
		case 0xF2: repeatNotZeroPrefix = true, simdPrefix = NMD_X86_PREFIXES_REPEAT_NOT_ZERO; continue;
		case 0xF3: repeatPrefix = true, simdPrefix = NMD_X86_PREFIXES_REPEAT; continue;
		case 0x2E: continue;
		case 0x36: continue;
		case 0x3E: continue;
		case 0x26: continue;
		case 0x64: continue;
		case 0x65: continue;
		case 0x66: operandPrefix = true, simdPrefix = NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE; continue;
		case 0x67: addressPrefix = true; continue;
		default:
			if (mode == NMD_X86_MODE_64 && NMD_R(*b) == 4) /* [40,4f[ */
			{
				if (NMD_C(*b) & 0b1000)
					rexW = true;
				continue;
			}
		}

		break;
	}

	const size_t numPrefixes = (uint8_t)((ptrdiff_t)(b)-(ptrdiff_t)(buffer));

	const size_t remainingValidBytes = (NMD_X86_MAXIMUM_INSTRUCTION_LENGTH - numPrefixes);
	if (remainingValidBytes == 0)
		return 0;

	const size_t remainingBufferSize = bufferSize - numPrefixes;
	if (remainingBufferSize == 0)
		return 0;

	const size_t remainingSize = remainingValidBytes < remainingBufferSize ? remainingValidBytes : remainingBufferSize;

	/* Parse opcode. */
	if (*b == 0x0F) /* 2 or 3 byte opcode. */
	{
		if (remainingSize == 1)
			return false;

		b++;

		if (*b == 0x38 || *b == 0x3A) /* 3 byte opcode. */
		{

			if (remainingSize < 4)
				return false;

			const bool isOpcodeMap38 = *b == 0x38;
			op = *++b;
			modrm = *(NMD_Modrm*)(b + 1);
			opcodeSize = 3;
			hasModrm = true;
			if (!ldisasm_parseModrm(&b, addressPrefix, mode, &modrm, remainingSize - 3))
				return 0;

			if (isOpcodeMap38)
			{
#ifndef NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK
				/* Check if the instruction is invalid. */
				if (op == 0x36)
				{
					return 0;
				}
				else if (op <= 0xb || (op >= 0x1c && op <= 0x1e))
				{
					if (simdPrefix == NMD_X86_PREFIXES_REPEAT || simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
						return 0;
				}
				else if (op >= 0xc8 && op <= 0xcd)
				{
					if (simdPrefix)
						return 0;
				}
				else if (op == 0x10 || op == 0x14 || op == 0x15 || op == 0x17 || (op >= 0x20 && op <= 0x25) || op == 0x28 || op == 0x29 || op == 0x2b || NMD_R(op) == 3 || op == 0x40 || op == 0x41 || op == 0xcf || (op >= 0xdb && op <= 0xdf))
				{
					if (simdPrefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
						return 0;
				}
				else if (op == 0x2a || (op >= 0x80 && op <= 0x82))
				{
					if (modrm.fields.mod == 0b11 || simdPrefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
						return 0;
				}
				else if (op == 0xf0 || op == 0xf1)
				{
					if (modrm.fields.mod == 0b11 && (simdPrefix == NMD_X86_PREFIXES_NONE || simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE))
						return 0;
					else if (simdPrefix == NMD_X86_PREFIXES_REPEAT)
						return 0;
				}
				else if (op == 0xf5 || op == 0xf8)
				{
					if (simdPrefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || modrm.fields.mod == 0b11)
						return 0;
				}
				else if (op == 0xf6)
				{
					if (simdPrefix == NMD_X86_PREFIXES_NONE && modrm.fields.mod == 0b11)
						return 0;
					else if (simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
						return 0;
				}
				else if (op == 0xf9)
				{
					if (simdPrefix != NMD_X86_PREFIXES_NONE || modrm.fields.mod == 0b11)
						return 0;
				}
				else
					return 0;
#endif /* NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK */
			}
			else /* 0x3a */
			{
				if (remainingSize < 5)
					return false;

				offset++;

#ifndef NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK
				/* Check if the instruction is invalid. */
				if ((op >= 0x8 && op <= 0xe) || (op >= 0x14 && op <= 0x17) || (op >= 0x20 && op <= 0x22) || (op >= 0x40 && op <= 0x42) || op == 0x44 || (op >= 0x60 && op <= 0x63) || op == 0xdf || op == 0xce || op == 0xcf)
				{
					if (simdPrefix != NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
						return 0;
				}
				else if (op == 0x0f || op == 0xcc)
				{
					if (simdPrefix)
						return 0;
				}
				else
					return 0;
#endif /* NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK */
			}
		}
		else if (*b == 0x0f) /* 3DNow! opcode map*/
		{
#ifndef NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_3DNOW
			if (remainingSize < 5)
				return false;

			/*
			if (!parseModrm(&b, instruction, remainingSize - 2))
			return false;

			instruction->encoding = NMD_X86_ENCODING_3DNOW;
			instruction->opcode = 0x0f;
			instruction->immMask = NMD_X86_IMM8;
			instruction->immediate = *(b + 1);
			*/

#ifndef NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK
			/*if (!nmd_findByte(valid3DNowOpcodes, sizeof(valid3DNowOpcodes), (uint8_t)instruction->immediate))
			return false;*/
#endif /* NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK */
#else /* NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_3DNOW */
			return false;
#endif /* NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_3DNOW */
		}
		else /* 2 byte opcode. */
		{
			op = *b;
			opcodeSize = 2;

			/* Check for ModR/M, SIB and displacement. */
			if (op >= 0x20 && op <= 0x23 && remainingSize == 2)
				hasModrm = true, modrm.modrm = *++b;
			else if (op < 4 || (NMD_R(op) != 3 && NMD_R(op) > 0 && NMD_R(op) < 7) || (op >= 0xD0 && op != 0xFF) || (NMD_R(op) == 7 && NMD_C(op) != 7) || NMD_R(op) == 9 || NMD_R(op) == 0xB || (NMD_R(op) == 0xC && NMD_C(op) < 8) || (NMD_R(op) == 0xA && (op % 8) >= 3) || op == 0x0ff || op == 0x00 || op == 0x0d)
			{
				if (!ldisasm_parseModrm(&b, addressPrefix, mode, &modrm, remainingSize - 2))
					return 0;
				hasModrm = true;
			}

#ifndef NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK
			/* Check if the instruction is invalid. */
			if (nmd_findByte(invalid2op, sizeof(invalid2op), op))
				return 0;
			else if (op == 0xc7)
			{
				if ((!simdPrefix && (modrm.fields.mod == 0b11 ? modrm.fields.reg <= 0b101 : modrm.fields.reg == 0b000 || modrm.fields.reg == 0b010)) || (simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO && (modrm.fields.mod == 0b11 || modrm.fields.reg != 0b001)) || ((simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || simdPrefix == NMD_X86_PREFIXES_REPEAT) && (modrm.fields.mod == 0b11 ? modrm.fields.reg <= (simdPrefix == NMD_X86_PREFIXES_REPEAT ? 0b110 : 0b101) : (modrm.fields.reg != 0b001 && modrm.fields.reg != 0b110))))
					return 0;
			}
			else if (op == 0x00)
			{
				if (modrm.fields.reg >= 0b110)
					return 0;
			}
			else if (op == 0x01)
			{
				if ((modrm.fields.mod == 0b11 ? (((simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO || simdPrefix == NMD_X86_PREFIXES_REPEAT) && ((modrm.modrm >= 0xc0 && modrm.modrm <= 0xc5) || (modrm.modrm >= 0xc8 && modrm.modrm <= 0xcb) || (modrm.modrm >= 0xcf && modrm.modrm <= 0xd1) || (modrm.modrm >= 0xd4 && modrm.modrm <= 0xd7) || modrm.modrm == 0xee || modrm.modrm == 0xef || modrm.modrm == 0xfa || modrm.modrm == 0xfb)) || (modrm.fields.reg == 0b000 && modrm.fields.rm >= 0b110) || (modrm.fields.reg == 0b001 && modrm.fields.rm >= 0b100 && modrm.fields.rm <= 0b110) || (modrm.fields.reg == 0b010 && (modrm.fields.rm == 0b010 || modrm.fields.rm == 0b011)) || (modrm.fields.reg == 0b101 && modrm.fields.rm < 0b110 && (!repeatPrefix || (simdPrefix == NMD_X86_PREFIXES_REPEAT && (modrm.fields.rm != 0b000 && modrm.fields.rm != 0b010)))) || (modrm.fields.reg == 0b111 && (modrm.fields.rm > 0b101 || (mode != NMD_X86_MODE_64 && modrm.fields.rm == 0b000)))) : (!repeatPrefix && modrm.fields.reg == 0b101)))
					return 0;
			}
			else if (op == 0x1A || op == 0x1B)
			{
				if (modrm.fields.mod == 0b11)
					return 0;
			}
			else if (op == 0x20 || op == 0x22)
			{
				if (modrm.fields.reg == 0b001 || modrm.fields.reg >= 0b101)
					return 0;
			}
			else if (op >= 0x24 && op <= 0x27)
				return 0;
			else if (op >= 0x3b && op <= 0x3f)
				return 0;
			else if (NMD_R(op) == 5)
			{
				if ((op == 0x50 && modrm.fields.mod != 0b11) || (simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && (op == 0x52 || op == 0x53)) || (simdPrefix == NMD_X86_PREFIXES_REPEAT && (op == 0x50 || (op >= 0x54 && op <= 0x57))) || (repeatNotZeroPrefix && (op == 0x50 || (op >= 0x52 && op <= 0x57) || op == 0x5b)))
					return 0;
			}
			else if (NMD_R(op) == 6)
			{
				if ((!(simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || simdPrefix == NMD_X86_PREFIXES_REPEAT || simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) && (op == 0x6c || op == 0x6d)) || (simdPrefix == NMD_X86_PREFIXES_REPEAT && op != 0x6f) || repeatNotZeroPrefix)
					return 0;
			}
			else if (op == 0x78 || op == 0x79)
			{
				if ((((simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && op == 0x78) && !(modrm.fields.mod == 0b11 && modrm.fields.reg == 0b000)) || ((simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) && modrm.fields.mod != 0b11)) || (simdPrefix == NMD_X86_PREFIXES_REPEAT))
					return 0;
			}
			else if (op == 0x7c || op == 0x7d)
			{
				if (simdPrefix == NMD_X86_PREFIXES_REPEAT || !(simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || simdPrefix == NMD_X86_PREFIXES_REPEAT || simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO))
					return 0;
			}
			else if (op == 0x7e || op == 0x7f)
			{
				if (repeatNotZeroPrefix)
					return 0;
			}
			else if (op >= 0x71 && op <= 0x73)
			{
				if ((simdPrefix == NMD_X86_PREFIXES_REPEAT || simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) || modrm.modrm <= 0xcf || (modrm.modrm >= 0xe8 && modrm.modrm <= 0xef))
					return 0;
			}
			else if (op == 0x73)
			{
				if (modrm.modrm >= 0xe0 && modrm.modrm <= 0xe8)
					return 0;
			}
			else if (op == 0xa6)
			{
				if (modrm.modrm != 0xc0 && modrm.modrm != 0xc8 && modrm.modrm != 0xd0)
					return 0;
			}
			else if (op == 0xa7)
			{
				if (!(modrm.fields.mod == 0b11 && modrm.fields.reg <= 0b101 && modrm.fields.rm == 0b000))
					return 0;
			}
			else if (op == 0xae)
			{
				if (((!simdPrefix && modrm.fields.mod == 0b11 && modrm.fields.reg <= 0b100) || (simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO && !(modrm.fields.mod == 0b11 && modrm.fields.reg == 0b110)) || (simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && (modrm.fields.reg < 0b110 || (modrm.fields.mod == 0b11 && modrm.fields.reg == 0b111))) || (simdPrefix == NMD_X86_PREFIXES_REPEAT && (modrm.fields.reg != 0b100 && modrm.fields.reg != 0b110) && !(modrm.fields.mod == 0b11 && modrm.fields.reg == 0b101))))
					return 0;
			}
			else if (op == 0xb8)
			{
				if (!repeatPrefix)
					return 0;
			}
			else if (op == 0xba)
			{
				if (modrm.fields.reg <= 0b011)
					return 0;
			}
			else if (op == 0xd0)
			{
				if (!simdPrefix || simdPrefix == NMD_X86_PREFIXES_REPEAT)
					return 0;
			}
			else if (op == 0xe0)
			{
				if (simdPrefix == NMD_X86_PREFIXES_REPEAT || simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
					return 0;
			}
			else if (op == 0xf0)
			{
				if (simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? modrm.fields.mod == 0b11 : true)
					return 0;
			}
			else if (simdPrefix == NMD_X86_PREFIXES_REPEAT || simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
			{
				if ((op >= 0x13 && op <= 0x17 && !(op == 0x16 && simdPrefix == NMD_X86_PREFIXES_REPEAT)) || op == 0x28 || op == 0x29 || op == 0x2e || op == 0x2f || (op <= 0x76 && op >= 0x74))
					return 0;
			}
			else if (op == 0x71 || op == 0x72 || (op == 0x73 && !(simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)))
			{
				if ((modrm.modrm >= 0xd8 && modrm.modrm <= 0xdf) || modrm.modrm >= 0xf8)
					return 0;
			}
			else if (op >= 0xc3 && op <= 0xc6)
			{
				if ((op == 0xc5 && modrm.fields.mod != 0b11) || (simdPrefix == NMD_X86_PREFIXES_REPEAT || simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) || (op == 0xc3 && simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE))
					return 0;
			}
			else if (NMD_R(op) >= 0xd && NMD_C(op) != 0 && op != 0xff && ((NMD_C(op) == 6 && NMD_R(op) != 0xf) ? (!simdPrefix || (NMD_R(op) == 0xD && (simdPrefix == NMD_X86_PREFIXES_REPEAT || simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) ? modrm.fields.mod != 0b11 : false)) : (simdPrefix == NMD_X86_PREFIXES_REPEAT || simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO || ((NMD_C(op) == 7 && NMD_R(op) != 0xe) ? modrm.fields.mod != 0b11 : false))))
				return 0;
			else if (hasModrm && modrm.fields.mod == 0b11)
			{
				if (op == 0xb2 || op == 0xb4 || op == 0xb5 || op == 0xc3 || op == 0xe7 || op == 0x2b || (simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && (op == 0x12 || op == 0x16)) || (!(simdPrefix == NMD_X86_PREFIXES_REPEAT || simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO) && (op == 0x13 || op == 0x17)))
					return 0;
			}
#endif /* NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK */

			if (NMD_R(op) == 8) /* imm32 */
				offset += (operandPrefix ? 2 : 4);
			else if ((NMD_R(op) == 7 && NMD_C(op) < 4) || op == 0xA4 || op == 0xC2 || (op > 0xC3 && op <= 0xC6) || op == 0xBA || op == 0xAC) /* imm8 */
				offset++;
			else if (op == 0x78 && (repeatNotZeroPrefix || operandPrefix)) /* imm8 + imm8 = "imm16" */
				offset += 2;
		}
	}
	else /* 1 byte opcode */
	{
		op = *b;
		opcodeSize = 1;

		/* Check for ModR/M, SIB and displacement. */
		if (NMD_R(op) == 8 || nmd_findByte(op1modrm, sizeof(op1modrm), op) || (NMD_R(op) < 4 && (NMD_C(op) < 4 || (NMD_C(op) >= 8 && NMD_C(op) < 0xC))) || (NMD_R(op) == 0xD && NMD_C(op) >= 8) || ((op == 0xc4 || op == 0xc5) && remainingSize > 1 && ((NMD_Modrm*)(b + 1))->fields.mod != 0b11))
		{
			if (!ldisasm_parseModrm(&b, addressPrefix, mode, &modrm, remainingSize - 1))
				return 0;
			hasModrm = true;
		}

#ifndef NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK
		if (op == 0xC6 || op == 0xC7)
		{
			if ((modrm.fields.reg != 0b000 && modrm.fields.reg != 0b111) || (modrm.fields.reg == 0b111 && (modrm.fields.mod != 0b11 || modrm.fields.rm != 0b000)))
				return 0;
		}
		else if (op == 0x8f)
		{
			if (modrm.fields.reg != 0b000)
				return 0;
		}
		else if (op == 0xfe)
		{
			if (modrm.fields.reg >= 0b010)
				return 0;
		}
		else if (op == 0xff)
		{
			if (modrm.fields.reg == 0b111 || (modrm.fields.mod == 0b11 && (modrm.fields.reg == 0b011 || modrm.fields.reg == 0b101)))
				return 0;
		}
		else if (op == 0x8c)
		{
			if (modrm.fields.reg >= 0b110)
				return 0;
		}
		else if (op == 0x8e)
		{
			if (modrm.fields.reg == 0b001 || modrm.fields.reg >= 0b110)
				return 0;
		}
		else if (op == 0x62)
		{
			if (mode == NMD_X86_MODE_64)
				return 0;
		}
		else if (op == 0x8d)
		{
			if (modrm.fields.mod == 0b11)
				return 0;
		}
		else if (op == 0xc4 || op == 0xc5)
		{
			if (mode == NMD_X86_MODE_64 && hasModrm && modrm.fields.mod != 0b11)
				return 0;
		}
		else if (op >= 0xd8 && op <= 0xdf)
		{
			switch (op)
			{
			case 0xd9:
				if ((modrm.fields.reg == 0b001 && modrm.fields.mod != 0b11) || (modrm.modrm > 0xd0 && modrm.modrm < 0xd8) || modrm.modrm == 0xe2 || modrm.modrm == 0xe3 || modrm.modrm == 0xe6 || modrm.modrm == 0xe7 || modrm.modrm == 0xef)
					return 0;
				break;
			case 0xda:
				if (modrm.modrm >= 0xe0 && modrm.modrm != 0xe9)
					return 0;
				break;
			case 0xdb:
				if (((modrm.fields.reg == 0b100 || modrm.fields.reg == 0b110) && modrm.fields.mod != 0b11) || (modrm.modrm >= 0xe5 && modrm.modrm <= 0xe7) || modrm.modrm >= 0xf8)
					return 0;
				break;
			case 0xdd:
				if ((modrm.fields.reg == 0b101 && modrm.fields.mod != 0b11) || NMD_R(modrm.modrm) == 0xf)
					return 0;
				break;
			case 0xde:
				if (modrm.modrm == 0xd8 || (modrm.modrm >= 0xda && modrm.modrm <= 0xdf))
					return 0;
				break;
			case 0xdf:
				if ((modrm.modrm >= 0xe1 && modrm.modrm <= 0xe7) || modrm.modrm >= 0xf8)
					return 0;
				break;
			}
		}
		else if (mode == NMD_X86_MODE_64)
		{
			if (op == 0x6 || op == 0x7 || op == 0xe || op == 0x16 || op == 0x17 || op == 0x1e || op == 0x1f || op == 0x27 || op == 0x2f || op == 0x37 || op == 0x3f || (op >= 0x60 && op <= 0x62) || op == 0x82 || op == 0xce || (op >= 0xd4 && op <= 0xd6))
				return 0;
		}
#endif /* NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VALIDITY_CHECK */

#ifndef NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VEX
		/* Check if instruction is VEX. */
		if ((op == 0xc4 || op == 0xc5) && !hasModrm)
		{
			const uint8_t byte0 = op;
			if (remainingSize < 4)
				return 0;

			if (byte0 == 0xc4)
			{
				b += 3;
				op = *b;

				if (op == 0x0c || op == 0x0d || op == 0x40 || op == 0x41 || op == 0x17 || op == 0x21 || op == 0x42)
					offset++;
			}
			else /* 0xc5 */
			{
				b += 2;
				op = *b;
			}

			if (!ldisasm_parseModrm(&b, addressPrefix, mode, &modrm, remainingSize - (byte0 == 0xc4 ? 4 : 3)))
				return false;
			hasModrm = true;
		}
		else
#endif /* NMD_ASSEMBLY_DISABLE_LENGTH_DISASSEMBLER_VEX */

		{
			/* Check for immediate */
			if (nmd_findByte(op1imm32, sizeof(op1imm32), op) || (NMD_R(op) < 4 && (NMD_C(op) == 5 || NMD_C(op) == 0xD)) || (NMD_R(op) == 0xB && NMD_C(op) >= 8) || (op == 0xF7 && modrm.fields.reg == 0b000)) /* imm32,16 */
			{
				if (NMD_R(op) == 0xB && NMD_C(op) >= 8)
					offset += rexW ? 8 : (operandPrefix || (mode == NMD_X86_MODE_16 && !operandPrefix) ? 2 : 4);
				else
				{
					if ((mode == NMD_X86_MODE_16 && operandPrefix) || (mode != NMD_X86_MODE_16 && !operandPrefix))
						offset += NMD_X86_IMM32;
					else
						offset += NMD_X86_IMM16;
				}
			}
			else if (NMD_R(op) == 7 || (NMD_R(op) == 0xE && NMD_C(op) < 8) || (NMD_R(op) == 0xB && NMD_C(op) < 8) || (NMD_R(op) < 4 && (NMD_C(op) == 4 || NMD_C(op) == 0xC)) || (op == 0xF6 && modrm.fields.reg <= 0b001) || nmd_findByte(op1imm8, sizeof(op1imm8), op)) /* imm8 */
				offset++;
			else if (NMD_R(op) == 0xA && NMD_C(op) < 4)
				offset += (mode == NMD_X86_MODE_64) ? (addressPrefix ? 4 : 8) : (addressPrefix ? 2 : 4);
			else if (op == 0xEA || op == 0x9A) /* imm32,48 */
			{
				if (mode == NMD_X86_MODE_64)
					return 0;
				offset += (operandPrefix ? 4 : 6);
			}
			else if (op == 0xC2 || op == 0xCA) /* imm16 */
				offset += 2;
			else if (op == 0xC8) /* imm16 + imm8 */
				offset += 3;
		}
	}

	if (lockPrefix)
	{
		if (!(hasModrm && modrm.fields.mod != 0b11 &&
			((opcodeSize == 1 && (op == 0x86 || op == 0x87 || (NMD_R(op) < 4 && (op % 8) < 2 && op < 0x38) || ((op >= 0x80 && op <= 0x83) && modrm.fields.reg != 0b111) || (op >= 0xfe && modrm.fields.reg < 2) || ((op == 0xf6 || op == 0xf7) && (modrm.fields.reg == 0b010 || modrm.fields.reg == 0b011)))) ||
			(opcodeSize == 2 && (nmd_findByte(twoOpcodes, sizeof(twoOpcodes), op) || op == 0xab || (op == 0xba && modrm.fields.reg != 0b100) || (op == 0xc7 && modrm.fields.reg == 0b001))))))
			return 0;
	}

	return (size_t)((ptrdiff_t)(++b + offset) - (ptrdiff_t)(buffer));
}

typedef struct StringInfo
{
	char* buffer;
	const NMD_X86Instruction* instruction;
	uint64_t runtimeAddress;
	uint32_t formatFlags;
} StringInfo;

void appendString(StringInfo* const si, const char* source)
{
	while (*source)
		*si->buffer++ = *source++;
}

size_t getNumDigits(uint64_t n, bool hex)
{
	size_t numDigits = 0;
	while ((n /= (hex ? 16 : 10)) > 0)
		numDigits++;

	return numDigits;
}

void appendNumber(StringInfo* const si, uint64_t n)
{
	size_t numDigits = getNumDigits(n, si->formatFlags & NMD_X86_FORMAT_FLAGS_HEX);
	size_t bufferOffset = numDigits + 1;

	if (si->formatFlags & NMD_X86_FORMAT_FLAGS_HEX)
	{
		const bool condition = n > 9 || si->formatFlags & NMD_X86_FORMAT_FLAGS_ENFORCE_HEX_ID;
		if (si->formatFlags & NMD_X86_FORMAT_FLAGS_0X_PREFIX && condition)
			*si->buffer++ = '0', *si->buffer++ = 'x';

		const uint8_t baseChar = (uint8_t)(si->formatFlags & NMD_X86_FORMAT_FLAGS_HEX_LOWERCASE ? 0x57 : 0x37);
		do {
			size_t num = n % 16;
			*(si->buffer + numDigits--) = (char)((num > 9 ? baseChar : '0') + num);
		} while ((n /= 16) > 0);

		if (si->formatFlags & NMD_X86_FORMAT_FLAGS_H_SUFFIX && condition)
			*(si->buffer + bufferOffset++) = 'h';
	}
	else
	{
		do {
			*(si->buffer + numDigits--) = (char)('0' + n % 10);
		} while ((n /= 10) > 0);
	}

	si->buffer += bufferOffset;
}

void appendSignedNumber(StringInfo* const si, int64_t n, bool showPositiveSign)
{
	if (n >= 0)
	{
		if (showPositiveSign)
			*si->buffer++ = '+';

		appendNumber(si, (uint64_t)n);
	}
	else
	{
		*si->buffer++ = '-';
		appendNumber(si, (uint64_t)(~n + 1));
	}
}

void appendSignedNumberMemoryView(StringInfo* const si)
{
	appendNumber(si, (si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? 0xFF00 : (si->instruction->mode == NMD_X86_MODE_64 ? 0xFFFFFFFFFFFFFF00 : 0xFFFFFF00)) | si->instruction->immediate);
	if (si->formatFlags & NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_HEX)
	{
		*si->buffer++ = '(';
		appendSignedNumber(si, (int8_t)(si->instruction->immediate), false);
		*si->buffer++ = ')';
	}
	else if (si->formatFlags & NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_HINT_DEC)
	{
		*si->buffer++ = '(';
		const uint32_t previousMask = si->formatFlags;
		si->formatFlags &= ~NMD_X86_FORMAT_FLAGS_HEX;
		appendSignedNumber(si, (int8_t)(si->instruction->immediate), false);
		si->formatFlags = previousMask;
		*si->buffer++ = ')';
	}
}

void appendRelativeAddress8(StringInfo* const si)
{
	if (si->runtimeAddress == (uint64_t)NMD_X86_INVALID_RUNTIME_ADDRESS)
	{
		/* *si->buffer++ = '$'; */
		appendSignedNumber(si, (int64_t)((int8_t)(si->instruction->immediate) + (int8_t)(si->instruction->length)), true);
	}
	else
	{
		uint64_t n;
		if (si->instruction->mode == NMD_X86_MODE_64)
			n = (uint64_t)((int64_t)(si->runtimeAddress + si->instruction->length) + (int8_t)(si->instruction->immediate));
		else if (si->instruction->mode == NMD_X86_MODE_16)
			n = (uint16_t)((int16_t)(si->runtimeAddress + si->instruction->length) + (int8_t)(si->instruction->immediate));
		else
			n = (uint32_t)((int32_t)(si->runtimeAddress + si->instruction->length) + (int8_t)(si->instruction->immediate));
		appendNumber(si, n);
	}
}

void appendRelativeAddress16_32(StringInfo* const si)
{
	if (si->runtimeAddress == (uint64_t)NMD_X86_INVALID_RUNTIME_ADDRESS)
	{
		/* *si->buffer++ = '$'; */
		appendSignedNumber(si, (int64_t)((int32_t)(si->instruction->immediate) + (int32_t)(si->instruction->length)), true);
	}
	else
		appendNumber(si, ((si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && si->instruction->mode == NMD_X86_MODE_32) || (si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? 0xFFFF : 0xFFFFFFFFFFFFFFFF) & (si->instruction->mode == NMD_X86_MODE_64 ?
		(uint64_t)((uint64_t)((int64_t)(si->runtimeAddress + si->instruction->length) + (int32_t)(si->instruction->immediate))) :
			(uint64_t)((uint32_t)((int32_t)(si->runtimeAddress + si->instruction->length) + (int32_t)(si->instruction->immediate)))
			));
}

void appendModRmMemoryPrefix(StringInfo* const si, const char* addrSpecifierReg)
{
#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_POINTER_SIZE
	if (si->formatFlags & NMD_X86_FORMAT_FLAGS_POINTER_SIZE)
	{
		appendString(si, addrSpecifierReg);
		appendString(si, " ptr ");
	}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_POINTER_SIZE */

	if (!(si->formatFlags & NMD_X86_FORMAT_FLAGS_ONLY_SEGMENT_OVERRIDE && !si->instruction->segmentOverride))
	{
		size_t i = 0;
		if (si->instruction->segmentOverride)
			i = nmd_getBitNumber(si->instruction->segmentOverride);

		appendString(si, si->instruction->segmentOverride ? segmentReg[i] : (!(si->instruction->prefixes & NMD_X86_PREFIXES_REX_B) && (si->instruction->modrm.fields.rm == 0b100 || si->instruction->modrm.fields.rm == 0b101) ? "ss" : "ds"));
		*si->buffer++ = ':';
	}
}

void appendModRm16Upper(StringInfo* const si)
{
	*si->buffer++ = '[';

	if (!(si->instruction->modrm.fields.mod == 0b00 && si->instruction->modrm.fields.rm == 0b110))
	{
		const char* addresses[] = { "bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx" };
		appendString(si, addresses[si->instruction->modrm.fields.rm]);
	}

	if (si->instruction->dispMask != NMD_X86_DISP_NONE && (si->instruction->displacement != 0 || *(si->buffer - 1) == '['))
	{
		if (si->instruction->modrm.fields.mod == 0b00 && si->instruction->modrm.fields.rm == 0b110)
			appendNumber(si, si->instruction->displacement);
		else
		{
			const bool isNegative = si->instruction->displacement & (1 << (si->instruction->dispMask * 8 - 1));
			if (*(si->buffer - 1) != '[')
				*si->buffer++ = isNegative ? '-' : '+';

			if (isNegative)
			{
				const uint16_t mask = (uint16_t)(si->instruction->dispMask == 2 ? 0xFFFF : 0xFF);
				appendNumber(si, (uint64_t)(~si->instruction->displacement & mask) + 1);
			}
			else
				appendNumber(si, si->instruction->displacement);
		}
	}

	*si->buffer++ = ']';
}

void appendModRm32Upper(StringInfo* const si)
{
	*si->buffer++ = '[';

	if (si->instruction->hasSIB)
	{
		if (si->instruction->sib.fields.base == 0b101)
		{
			if (si->instruction->modrm.fields.mod != 0b00)
				appendString(si, si->instruction->mode == NMD_X86_MODE_64 && !(si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (si->instruction->prefixes & NMD_X86_PREFIXES_REX_B ? "r13" : "rbp") : "ebp");
		}
		else
			appendString(si, (si->instruction->mode == NMD_X86_MODE_64 && !(si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (si->instruction->prefixes & NMD_X86_PREFIXES_REX_B ? regrx : reg64) : reg32)[si->instruction->sib.fields.base]);

		if (si->instruction->sib.fields.index != 0b100)
		{
			if (!(si->instruction->sib.fields.base == 0b101 && si->instruction->modrm.fields.mod == 0b00))
				*si->buffer++ = '+';
			appendString(si, (si->instruction->mode == NMD_X86_MODE_64 && !(si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (si->instruction->prefixes & NMD_X86_PREFIXES_REX_X ? regrx : reg64) : reg32)[si->instruction->sib.fields.index]);
			if (!(si->instruction->sib.fields.scale == 0b00 && !(si->formatFlags & NMD_X86_FORMAT_FLAGS_SCALE_ONE)))
				*si->buffer++ = '*', *si->buffer++ = (char)('0' + (1 << si->instruction->sib.fields.scale));
		}

		if (si->instruction->prefixes & NMD_X86_PREFIXES_REX_X && si->instruction->sib.fields.index == 0b100)
		{
			if (*(si->buffer - 1) != '[')
				*si->buffer++ = '+';
			appendString(si, "r12");
			if (!(si->instruction->sib.fields.scale == 0b00 && !(si->formatFlags & NMD_X86_FORMAT_FLAGS_SCALE_ONE)))
				*si->buffer++ = '*', *si->buffer++ = (char)('0' + (1 << si->instruction->sib.fields.scale));
		}
	}
	else if (!(si->instruction->modrm.fields.mod == 0b00 && si->instruction->modrm.fields.rm == 0b101))
	{
		if ((si->instruction->prefixes & (NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE | NMD_X86_PREFIXES_REX_B)) == (NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE | NMD_X86_PREFIXES_REX_B) && si->instruction->mode == NMD_X86_MODE_64)
			appendString(si, regrx[si->instruction->modrm.fields.rm]), *si->buffer++ = 'd';
		else
			appendString(si, (si->instruction->mode == NMD_X86_MODE_64 && !(si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE) ? (si->instruction->prefixes & NMD_X86_PREFIXES_REX_B ? regrx : reg64) : reg32)[si->instruction->modrm.fields.rm]);
	}

	/* Handle displacement. */
	if (si->instruction->dispMask != NMD_X86_DISP_NONE && (si->instruction->displacement != 0 || *(si->buffer - 1) == '['))
	{
		/* Relative address. */
		if (si->instruction->modrm.fields.rm == 0b101 && si->instruction->mode == NMD_X86_MODE_64 && si->instruction->modrm.fields.mod == 0b00 && si->runtimeAddress != NMD_X86_INVALID_RUNTIME_ADDRESS)
		{
			if (si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE)
				appendNumber(si, (uint32_t)((int32_t)(si->runtimeAddress + si->instruction->length) + (int32_t)si->instruction->displacement));
			else
				appendNumber(si, (uint64_t)((int64_t)(si->runtimeAddress + si->instruction->length) + (int64_t)((int32_t)si->instruction->displacement)));
		}
		else if (si->instruction->modrm.fields.mod == 0b00 && ((si->instruction->sib.fields.base == 0b101 && si->instruction->sib.fields.index == 0b100) || si->instruction->modrm.fields.rm == 0b101) && *(si->buffer - 1) == '[')
			appendNumber(si, si->instruction->mode == NMD_X86_MODE_64 ? 0xFFFFFFFF00000000 | si->instruction->displacement : si->instruction->displacement);
		else
		{
			if (si->instruction->modrm.fields.rm == 0b101 && si->instruction->mode == NMD_X86_MODE_64 && si->instruction->modrm.fields.mod == 0b00)
				appendString(si, si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE ? "eip" : "rip");

			const bool isNegative = si->instruction->displacement & (1 << (si->instruction->dispMask * 8 - 1));
			if (*(si->buffer - 1) != '[')
				*si->buffer++ = isNegative ? '-' : '+';

			if (isNegative)
			{
				const uint32_t mask = (uint32_t)(si->instruction->dispMask == 4 ? -1 : (1 << (si->instruction->dispMask * 8)) - 1);
				appendNumber(si, (uint64_t)(~si->instruction->displacement & mask) + 1);
			}
			else
				appendNumber(si, si->instruction->displacement);
		}
	}

	*si->buffer++ = ']';
}

void appendModRmUpper(StringInfo* const si, const char* addrSpecifierReg)
{
	appendModRmMemoryPrefix(si, addrSpecifierReg);

	if ((si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE)) || (si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE && si->instruction->mode == NMD_X86_MODE_32))
		appendModRm16Upper(si);
	else
		appendModRm32Upper(si);
}

void appendModRmUpperWithoutAddressSpecifier(StringInfo* const si)
{
	if ((si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE)) || (si->instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE && si->instruction->mode == NMD_X86_MODE_32))
		appendModRm16Upper(si);
	else
		appendModRm32Upper(si);
}

void appendNq(StringInfo* const si)
{
	*si->buffer++ = 'm', *si->buffer++ = 'm';
	*si->buffer++ = (char)('0' + si->instruction->modrm.fields.rm);
}

void appendPq(StringInfo* const si)
{
	*si->buffer++ = 'm', *si->buffer++ = 'm';
	*si->buffer++ = (char)('0' + si->instruction->modrm.fields.reg);
}

void appendAvxRegisterReg(StringInfo* const si)
{
	*si->buffer++ = si->instruction->vex.L ? 'y' : 'x';
	appendPq(si);
}

void appendAvxVvvvRegister(StringInfo* const si)
{
	*si->buffer++ = si->instruction->vex.L ? 'y' : 'x';
	*si->buffer++ = 'm', *si->buffer++ = 'm';
	if ((15 - si->instruction->vex.vvvv) > 9)
		*si->buffer++ = '1', *si->buffer++ = (char)(0x26 + (15 - si->instruction->vex.vvvv));
	else
		*si->buffer++ = (char)('0' + (15 - si->instruction->vex.vvvv));
}

void appendVdq(StringInfo* const si)
{
	*si->buffer++ = 'x';
	appendPq(si);
}

void appendVqq(StringInfo* const si)
{
	*si->buffer++ = 'y';
	appendPq(si);
}

void appendVx(StringInfo* const si)
{
	if (si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
		appendVdq(si);
	else
		appendVqq(si);
}

void appendUdq(StringInfo* const si)
{
	*si->buffer++ = 'x';
	appendNq(si);
}

void appendUqq(StringInfo* const si)
{
	*si->buffer++ = 'y';
	appendNq(si);
}

void appendUx(StringInfo* const si)
{
	if (si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
		appendUdq(si);
	else
		appendUqq(si);
}

void appendQq(StringInfo* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
		appendNq(si);
	else
		appendModRmUpper(si, "qword");
}

void appendEv(StringInfo* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
	{
		if (si->instruction->prefixes & NMD_X86_PREFIXES_REX_B)
		{
			appendString(si, regrx[si->instruction->modrm.fields.rm]);
			if (!(si->instruction->prefixes & NMD_X86_PREFIXES_REX_W))
				*si->buffer++ = 'd';
		}
		else
			appendString(si, ((si->instruction->operandSize64 ? reg64 : (si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && si->instruction->mode != NMD_X86_MODE_16) || (si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? reg16 : reg32))[si->instruction->modrm.fields.rm]);
	}
	else
		appendModRmUpper(si, (si->instruction->operandSize64) ? "qword" : ((si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && si->instruction->mode != NMD_X86_MODE_16) || (si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? "word" : "dword"));
}

void appendEy(StringInfo* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
		appendString(si, (si->instruction->operandSize64 ? reg64 : reg32)[si->instruction->modrm.fields.rm]);
	else
		appendModRmUpper(si, si->instruction->operandSize64 ? "qword" : "dword");
}

void appendEb(StringInfo* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
	{
		if (si->instruction->prefixes & NMD_X86_PREFIXES_REX_B)
			appendString(si, regrx[si->instruction->modrm.fields.rm]), *si->buffer++ = 'b';
		else
			appendString(si, (si->instruction->hasRex ? reg8_x64 : reg8)[si->instruction->modrm.fields.rm]);
	}
	else
		appendModRmUpper(si, "byte");
}

void appendEw(StringInfo* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
		appendString(si, reg16[si->instruction->modrm.fields.rm]);
	else
		appendModRmUpper(si, "word");
}

void appendEd(StringInfo* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
		appendString(si, reg32[si->instruction->modrm.fields.rm]);
	else
		appendModRmUpper(si, "dword");
}

void appendEq(StringInfo* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
		appendString(si, reg64[si->instruction->modrm.fields.rm]);
	else
		appendModRmUpper(si, "qword");
}

void appendRv(StringInfo* const si)
{
	appendString(si, (si->instruction->operandSize64 ? reg64 : (si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? reg16 : reg32))[si->instruction->modrm.fields.rm]);
}

void appendGv(StringInfo* const si)
{
	if (si->instruction->prefixes & NMD_X86_PREFIXES_REX_R)
	{
		appendString(si, regrx[si->instruction->modrm.fields.reg]);
		if (!(si->instruction->prefixes & NMD_X86_PREFIXES_REX_W))
			*si->buffer++ = 'd';
	}
	else
		appendString(si, ((si->instruction->operandSize64) ? reg64 : ((si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && si->instruction->mode != NMD_X86_MODE_16) || (si->instruction->mode == NMD_X86_MODE_16 && !(si->instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? reg16 : reg32))[si->instruction->modrm.fields.reg]);
}

void appendGy(StringInfo* const si)
{
	appendString(si, (si->instruction->operandSize64 ? reg64 : reg32)[si->instruction->modrm.fields.reg]);
}

void appendGb(StringInfo* const si)
{
	if (si->instruction->prefixes & NMD_X86_PREFIXES_REX_R)
		appendString(si, regrx[si->instruction->modrm.fields.reg]), *si->buffer++ = 'b';
	else
		appendString(si, (si->instruction->hasRex ? reg8_x64 : reg8)[si->instruction->modrm.fields.reg]);
}

void appendGw(StringInfo* const si)
{
	appendString(si, reg16[si->instruction->modrm.fields.reg]);
}

void appendW(StringInfo* const si)
{
	if (si->instruction->modrm.fields.mod == 0b11)
		appendString(si, "xmm"), *si->buffer++ = (char)('0' + si->instruction->modrm.fields.rm);
	else
		appendModRmUpper(si, "xmmword");
}

#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_ATT_SYNTAX
char* formatOperandToAtt(char* operand, StringInfo* si)
{
	char* nextOperand = (char*)nmd_strchr(operand, ',');
	const char* operandEnd = nextOperand ? nextOperand : si->buffer;

	/* Memory operand. */
	const char* memoryOperand = nmd_strchr(operand, '[');
	if (memoryOperand && memoryOperand < operandEnd)
	{
		memoryOperand++;
		const char* segReg = nmd_strchr(operand, ':');
		if (segReg)
		{
			if (segReg == operand + 2)
				nmd_insert_char(operand, '%'), si->buffer++, operand += 4;
			else
			{
				*operand++ = '%';
				*operand++ = *(segReg - 2);
				*operand++ = 's';
				*operand++ = ':';
			}
		}

		/* Handle displacement. */
		char* displacement = operand;
		do
		{
			displacement++;
			displacement = (char*)nmd_find_number(displacement, operandEnd);
		} while (displacement && ((*(displacement - 1) != '+' && *(displacement - 1) != '-' && *(displacement - 1) != '[') || !nmd_is_number(displacement, operandEnd - 2)));

		bool isThereBaseOrIndex = true;
		char memoryOperandBuffer[96];

		if (displacement)
		{
			if (*(displacement - 1) != '[')
				displacement--;
			else
				isThereBaseOrIndex = false;

			char* i = (char*)memoryOperand;
			char* j = memoryOperandBuffer;
			for (; i < displacement; i++, j++)
				*j = *i;
			*j = '\0';

			if (*displacement == '+')
				displacement++;

			for (; *displacement != ']'; displacement++, operand++)
				*operand = *displacement;
		}

		/* Handle base, index and scale. */
		if (isThereBaseOrIndex)
		{
			*operand++ = '(';

			char* baseOrIndex = operand;
			if (displacement)
			{
				char* s = memoryOperandBuffer;
				for (; *s; s++, operand++)
					*operand = *s;
			}
			else
			{
				for (; *memoryOperand != ']'; operand++, memoryOperand++)
					*operand = *memoryOperand;
			}

			nmd_insert_char(baseOrIndex, '%');
			operand++;
			*operand++ = ')';

			for (; *baseOrIndex != ')'; baseOrIndex++)
			{
				if (*baseOrIndex == '+' || *baseOrIndex == '*')
				{
					if (*baseOrIndex == '+')
						nmd_insert_char(baseOrIndex + 1, '%'), operand++;
					*baseOrIndex = ',';
				}
			}

			operand = baseOrIndex;
			operand++;
		}

		if (nextOperand)
		{
			/* Move second operand to the left until the comma. */
			operandEnd = nmd_strchr(operand, ',');
			for (; *operandEnd != '\0'; operand++, operandEnd++)
				*operand = *operandEnd;

			*operand = '\0';

			operandEnd = operand;
			while (*operandEnd != ',')
				operandEnd--;
		}
		else
			*operand = '\0', operandEnd = operand;

		si->buffer = operand;

		return (char*)operandEnd;
	}
	else /* Immediate or register operand. */
	{
		nmd_insert_char(operand, nmd_is_number(operand, operandEnd) ? '$' : '%');
		si->buffer++;
		return (char*)operandEnd + 1;
	}
}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_ATT_SYNTAX */

/*
Formats an instruction. This function may cause a crash if you modify 'instruction' manually.
Parameters:
- instruction    [in]  A pointer to a variable of type 'NMD_X86Instruction' describing the instruction to be formatted.
- buffer         [out] A pointer to buffer that receives the string. The buffer's recommended size is 128 bytes.
- runtimeAddress [in]  The instruction's runtime address. You may use 'NMD_X86_INVALID_RUNTIME_ADDRESS'.
- formatFlags    [in]  A mask of 'NMD_X86_FORMAT_FLAGS_XXX' that specifies how the function should format the instruction. If uncertain, use 'NMD_X86_FORMAT_FLAGS_DEFAULT'.
*/
void nmd_x86_format_instruction(const NMD_X86Instruction* instruction, char* buffer, uint64_t runtimeAddress, uint32_t formatFlags)
{
	if (!instruction->valid)
		return;

	StringInfo si;
	si.buffer = buffer;
	si.instruction = instruction;
	si.runtimeAddress = runtimeAddress;
	si.formatFlags = formatFlags;

#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_BYTES
	if (formatFlags & NMD_X86_FORMAT_FLAGS_BYTES)
	{
		size_t i = 0;
		for (; i < instruction->length; i++)
		{
			uint8_t num = instruction->buffer[i] >> 4;
			*si.buffer++ = (char)((num > 9 ? 0x37 : '0') + num);
			num = instruction->buffer[i] & 0xf;
			*si.buffer++ = (char)((num > 9 ? 0x37 : '0') + num);
			*si.buffer++ = ' ';
		}

		const size_t numPaddingBytes = instruction->length < NMD_X86_FORMATTER_NUM_PADDING_BYTES ? (NMD_X86_FORMATTER_NUM_PADDING_BYTES - instruction->length) : 0;
		for (i = 0; i < numPaddingBytes * 3; i++)
			*si.buffer++ = ' ';
	}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_BYTES */

	const uint8_t op = instruction->opcode;

	if (instruction->prefixes & (NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO) && (instruction->prefixes & NMD_X86_PREFIXES_LOCK || ((op == 0x86 || op == 0x87) && instruction->modrm.fields.mod != 0b11)))
		appendString(&si, instruction->repeatPrefix ? "xrelease " : "xacquire ");
	else if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT_NOT_ZERO && (instruction->opcodeSize == 1 && (op == 0xc2 || op == 0xc3 || op == 0xe8 || op == 0xe9 || NMD_R(op) == 7 || (op == 0xff && (instruction->modrm.fields.reg == 0b010 || instruction->modrm.fields.reg == 0b100)))))
		appendString(&si, "bnd ");

	if (instruction->prefixes & NMD_X86_PREFIXES_LOCK)
		appendString(&si, "lock ");

	const bool operandSize = instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE;

	if (instruction->opcodeMap == NMD_X86_OPCODE_MAP_DEFAULT)
	{
#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_EVEX
		if (instruction->encoding == NMD_X86_ENCODING_EVEX)
		{

		}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_EVEX */

#if !defined(NMD_ASSEMBLY_DISABLE_FORMATTER_EVEX) && !defined(NMD_ASSEMBLY_DISABLE_FORMATTER_VEX)
		else
#endif
#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_VEX
			if (instruction->encoding == NMD_X86_ENCODING_VEX)
			{
				if (instruction->vex.vex[0] == 0xc4)
				{
					if (instruction->opcode == 0x0c || instruction->opcode == 0x0d || instruction->opcode == 0x4a || instruction->opcode == 0x4b)
					{
						appendString(&si, instruction->opcode == 0x0c ? "vblendps" : (instruction->opcode == 0x0c ? "vblendpd" : (instruction->opcode == 0x4a ? "vblendvps" : "vblendvpd")));
						*si.buffer++ = ' ';

						appendAvxRegisterReg(&si);
						*si.buffer++ = ',';

						appendAvxVvvvRegister(&si);
						*si.buffer++ = ',';

						appendW(&si);
						*si.buffer++ = ',';

						if (instruction->opcode <= 0x0d)
							appendNumber(&si, instruction->immediate);
						else
						{
							appendString(&si, "xmm");
							*si.buffer++ = (char)('0' + ((instruction->immediate & 0xf0) >> 4) % 8);
						}
					}
					else if (instruction->opcode == 0x40 || instruction->opcode == 0x41)
					{
						appendString(&si, instruction->opcode == 0x40 ? "vdpps" : "vdppd");
						*si.buffer++ = ' ';

						appendAvxRegisterReg(&si);
						*si.buffer++ = ',';

						appendAvxVvvvRegister(&si);
						*si.buffer++ = ',';

						appendW(&si);
						*si.buffer++ = ',';

						appendNumber(&si, instruction->immediate);
					}
					else if (instruction->opcode == 0x17)
					{
						appendString(&si, "vextractps ");

						appendEv(&si);
						*si.buffer++ = ',';

						appendVdq(&si);
						*si.buffer++ = ',';

						appendNumber(&si, instruction->immediate);
					}
					else if (instruction->opcode == 0x21)
					{
						appendString(&si, "vinsertps ");

						appendVdq(&si);
						*si.buffer++ = ',';

						appendAvxVvvvRegister(&si);
						*si.buffer++ = ',';

						appendW(&si);
						*si.buffer++ = ',';

						appendNumber(&si, instruction->immediate);
					}
					else if (instruction->opcode == 0x2a)
					{
						appendString(&si, "vmovntdqa ");

						appendVdq(&si);
						*si.buffer++ = ',';

						appendModRmUpperWithoutAddressSpecifier(&si);
					}
					else if (instruction->opcode == 0x42)
					{
						appendString(&si, "vmpsadbw ");

						appendVdq(&si);
						*si.buffer++ = ',';

						appendAvxVvvvRegister(&si);
						*si.buffer++ = ',';

						if (si.instruction->modrm.fields.mod == 0b11)
							appendString(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
						else
							appendModRmUpperWithoutAddressSpecifier(&si);
						*si.buffer++ = ',';

						appendNumber(&si, instruction->immediate);
					}
				}
			}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_VEX */

#if (!defined(NMD_ASSEMBLY_DISABLE_FORMATTER_EVEX) || !defined(NMD_ASSEMBLY_DISABLE_FORMATTER_VEX)) && !defined(NMD_ASSEMBLY_DISABLE_FORMATTER_3DNOW)
			else
#endif
#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_3DNOW
				if (instruction->encoding == NMD_X86_ENCODING_3DNOW)
				{
					const char* mnemonic = 0;
					switch (instruction->opcode)
					{
					case 0x0c: mnemonic = "pi2fw"; break;
					case 0x0d: mnemonic = "pi2fd"; break;
					case 0x1c: mnemonic = "pf2iw"; break;
					case 0x1d: mnemonic = "pf2id"; break;
					case 0x8a: mnemonic = "pfnacc"; break;
					case 0x8e: mnemonic = "pfpnacc"; break;
					case 0x90: mnemonic = "pfcmpge"; break;
					case 0x94: mnemonic = "pfmin"; break;
					case 0x96: mnemonic = "pfrcp"; break;
					case 0x97: mnemonic = "pfrsqrt"; break;
					case 0x9a: mnemonic = "pfsub"; break;
					case 0x9e: mnemonic = "pfadd"; break;
					case 0xa0: mnemonic = "pfcmpgt"; break;
					case 0xa4: mnemonic = "pfmax"; break;
					case 0xa6: mnemonic = "pfrcpit1"; break;
					case 0xa7: mnemonic = "pfrsqit1"; break;
					case 0xaa: mnemonic = "pfsubr"; break;
					case 0xae: mnemonic = "pfacc"; break;
					case 0xb0: mnemonic = "pfcmpeq"; break;
					case 0xb4: mnemonic = "pfmul"; break;
					case 0xb6: mnemonic = "pfrcpit2"; break;
					case 0xb7: mnemonic = "pmulhrw"; break;
					case 0xbb: mnemonic = "pswapd"; break;
					case 0xbf: mnemonic = "pavgusb"; break;
					}

					appendString(&si, mnemonic);
					*si.buffer++ = ' ';

					appendPq(&si);
					*si.buffer++ = ',';
					appendQq(&si);
				}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_3DNOW */

#if !defined(NMD_ASSEMBLY_DISABLE_FORMATTER_EVEX) || !defined(NMD_ASSEMBLY_DISABLE_FORMATTER_VEX) || !defined(NMD_ASSEMBLY_DISABLE_FORMATTER_3DNOW)
				else /*if (instruction->encoding == INSTRUCTION_ENCODING_LEGACY) */
#endif
				{
					if (op >= 0x88 && op <= 0x8c) /* mov [88,8c] */
					{
						appendString(&si, "mov ");
						if (op == 0x8b)
						{
							appendGv(&si);
							*si.buffer++ = ',';
							appendEv(&si);
						}
						else if (op == 0x89)
						{
							appendEv(&si);
							*si.buffer++ = ',';
							appendGv(&si);
						}
						else if (op == 0x88)
						{
							appendEb(&si);
							*si.buffer++ = ',';
							appendGb(&si);
						}
						else if (op == 0x8a)
						{
							appendGb(&si);
							*si.buffer++ = ',';
							appendEb(&si);
						}
						else if (op == 0x8c)
						{
							if (si.instruction->modrm.fields.mod == 0b11)
								appendString(&si, (si.instruction->operandSize64 ? reg64 : (si.instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || instruction->mode == NMD_X86_MODE_16 ? reg16 : reg32))[si.instruction->modrm.fields.rm]);
							else
								appendModRmUpper(&si, "word");

							*si.buffer++ = ',';
							appendString(&si, segmentReg[instruction->modrm.fields.reg]);
						}
					}
					else if (op == 0x68 || op == 0x6A) /* push */
					{
						appendString(&si, "push ");
						if (op == 0x6a)
						{
							if (formatFlags & NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_MEMORY_VIEW && instruction->immediate >= 0x80)
								appendSignedNumberMemoryView(&si);
							else
								appendSignedNumber(&si, (int8_t)instruction->immediate, false);
						}
						else
							appendNumber(&si, instruction->immediate);
					}
					else if (op == 0xff) /* Opcode extensions Group 5 */
					{
						appendString(&si, opcodeExtensionsGrp5[instruction->modrm.fields.reg]);
						*si.buffer++ = ' ';
						if (instruction->modrm.fields.mod == 0b11)
							appendString(&si, (si.instruction->operandSize64 ? reg64 : (operandSize ? reg16 : reg32))[si.instruction->modrm.fields.rm]);
						else
							appendModRmUpper(&si, (instruction->modrm.fields.reg == 0b011 || instruction->modrm.fields.reg == 0b101) ? "fword" : (instruction->mode == NMD_X86_MODE_64 && ((instruction->modrm.fields.reg >= 0b010 && instruction->modrm.fields.reg <= 0b110) || (instruction->prefixes & NMD_X86_PREFIXES_REX_W && instruction->modrm.fields.reg <= 0b010)) ? "qword" : (operandSize ? "word" : "dword")));
					}
					else if (NMD_R(op) < 4 && (NMD_C(op) < 6 || (NMD_C(op) >= 8 && NMD_C(op) < 0xE))) /* add,adc,and,xor,or,sbb,sub,cmp */
					{
						appendString(&si, op1OpcodeMapMnemonics[NMD_R((NMD_C(op) > 6 ? op + 0x40 : op))]);
						*si.buffer++ = ' ';

						switch (op % 8)
						{
						case 0:
							appendEb(&si);
							*si.buffer++ = ',';
							appendGb(&si);
							break;
						case 1:
							appendEv(&si);
							*si.buffer++ = ',';
							appendGv(&si);
							break;
						case 2:
							appendGb(&si);
							*si.buffer++ = ',';
							appendEb(&si);
							break;
						case 3:
							appendGv(&si);
							*si.buffer++ = ',';
							appendEv(&si);
							break;
						case 4:
							appendString(&si, "al,");
							appendNumber(&si, instruction->immediate);
							break;
						case 5:
							appendString(&si, instruction->operandSize64 ? "rax" : (operandSize ? "ax" : "eax"));
							*si.buffer++ = ',';
							appendNumber(&si, instruction->immediate);
							break;
						}
					}
					else if (NMD_R(op) == 4 || NMD_R(op) == 5) /* inc,dec,push,pop [0x40, 0x5f] */
					{
						appendString(&si, NMD_C(op) < 8 ? (NMD_R(op) == 4 ? "inc " : "push ") : (NMD_R(op) == 4 ? "dec " : "pop "));
						appendString(&si, (instruction->prefixes & NMD_X86_PREFIXES_REX_B ? regrx : (instruction->mode == NMD_X86_MODE_64 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE) ? reg64 : (operandSize ? reg16 : reg32)))[op % 8]);
					}
					else if (op >= 0x80 && op < 0x84) /* add,adc,and,xor,or,sbb,sub,cmp [80,83] */
					{
						appendString(&si, opcodeExtensionsGrp1[instruction->modrm.fields.reg]);
						*si.buffer++ = ' ';
						if (op == 0x80 || op == 0x82)
							appendEb(&si);
						else
							appendEv(&si);
						*si.buffer++ = ',';
						if (op == 0x83)
						{
							if ((instruction->modrm.fields.reg == 0b001 || instruction->modrm.fields.reg == 0b100 || instruction->modrm.fields.reg == 0b110) && instruction->immediate >= 0x80)
								appendNumber(&si, (instruction->prefixes & NMD_X86_PREFIXES_REX_W ? 0xFFFFFFFFFFFFFF00 : (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE || instruction->mode == NMD_X86_MODE_16 ? 0xFF00 : 0xFFFFFF00)) | instruction->immediate);
							else
								appendSignedNumber(&si, (int8_t)(instruction->immediate), false);
						}
						else
							appendNumber(&si, instruction->immediate);
					}
					else if (op == 0xe8 || op == 0xe9 || op == 0xeb) /* call,jmp */
					{
						appendString(&si, op == 0xe8 ? "call " : "jmp ");
						if (op == 0xeb)
							appendRelativeAddress8(&si);
						else
							appendRelativeAddress16_32(&si);
					}
					else if (op >= 0xA0 && op < 0xA4) /* mov [a0, a4] */
					{
						appendString(&si, "mov ");
						if (op == 0xa0)
						{
							appendString(&si, "al,");
							appendModRmMemoryPrefix(&si, "byte");
							*si.buffer++ = '[';
							appendNumber(&si, (instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE || instruction->mode == NMD_X86_MODE_16 ? 0xFFFF : 0xFFFFFFFFFFFFFFFF) & instruction->immediate);
							*si.buffer++ = ']';
						}
						else if (op == 0xa1)
						{
							appendString(&si, instruction->operandSize64 ? "rax," : (operandSize ? "ax," : "eax,"));
							appendModRmMemoryPrefix(&si, instruction->operandSize64 ? "qword" : (operandSize ? "word" : "dword"));
							*si.buffer++ = '[';
							appendNumber(&si, (instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE || instruction->mode == NMD_X86_MODE_16 ? 0xFFFF : 0xFFFFFFFFFFFFFFFF) & instruction->immediate);
							*si.buffer++ = ']';
						}
						else if (op == 0xa2)
						{
							appendModRmMemoryPrefix(&si, "byte");
							*si.buffer++ = '[';
							appendNumber(&si, (instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE || instruction->mode == NMD_X86_MODE_16 ? 0xFFFF : 0xFFFFFFFFFFFFFFFF) & instruction->immediate);
							appendString(&si, "],al");
						}
						else if (op == 0xa3)
						{
							appendModRmMemoryPrefix(&si, instruction->operandSize64 ? "qword" : (operandSize ? "word" : "dword"));
							*si.buffer++ = '[';
							appendNumber(&si, (instruction->prefixes & NMD_X86_PREFIXES_ADDRESS_SIZE_OVERRIDE || instruction->mode == NMD_X86_MODE_16 ? 0xFFFF : 0xFFFFFFFFFFFFFFFF) & instruction->immediate);
							appendString(&si, "],");
							appendString(&si, instruction->operandSize64 ? "rax" : (operandSize ? "ax" : "eax"));
						}
					}
					else if (op == 0xcc) /* int3 */
						appendString(&si, "int3");
					else if (op == 0x8d) /* lea */
					{
						appendString(&si, "lea ");
						appendGv(&si);
						*si.buffer++ = ',';
						appendModRmUpperWithoutAddressSpecifier(&si);
					}
					else if (op == 0x8f) /* pop */
					{
						appendString(&si, "pop ");
						if (instruction->modrm.fields.mod == 0b11)
							appendString(&si, (operandSize ? reg16 : reg32)[instruction->modrm.fields.rm]);
						else
							appendModRmUpper(&si, instruction->mode == NMD_X86_MODE_64 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE) ? "qword" : (operandSize ? "word" : "dword"));
					}
					else if (NMD_R(op) == 7) /* conditional jump [70,7f]*/
					{
						*si.buffer++ = 'j';
						appendString(&si, conditionSuffixes[NMD_C(op)]);
						*si.buffer++ = ' ';
						appendRelativeAddress8(&si);
					}
					else if (op == 0xa8) /* test */
					{
						appendString(&si, "test al,");
						appendNumber(&si, instruction->immediate);
					}
					else if (op == 0xa9) /* test */
					{
						appendString(&si, instruction->operandSize64 ? "test rax" : (operandSize ? "test ax" : "test eax"));
						*si.buffer++ = ',';
						appendNumber(&si, instruction->immediate);
					}
					else if (op == 0x90)
					{
						if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT)
							appendString(&si, "pause");
						else if (instruction->prefixes & NMD_X86_PREFIXES_REX_B)
							appendString(&si, instruction->prefixes & NMD_X86_PREFIXES_REX_W ? "xchg r8,rax" : "xchg r8d,eax");
						else
							appendString(&si, "nop");
					}
					else if (op == 0xc3)
						appendString(&si, "ret");
					else if (NMD_R(op) == 0xb) /* mov [b0, bf] */
					{
						appendString(&si, "mov ");
						if (instruction->prefixes & NMD_X86_PREFIXES_REX_B)
							appendString(&si, regrx[op % 8]), *si.buffer++ = NMD_C(op) < 8 ? 'b' : 'd';
						else
							appendString(&si, (NMD_C(op) < 8 ? (instruction->hasRex ? reg8_x64 : reg8) : (instruction->operandSize64 ? reg64 : (operandSize ? reg16 : reg32)))[op % 8]);
						*si.buffer++ = ',';
						appendNumber(&si, instruction->immediate);
					}
					else if (op == 0xfe) /* inc,dec */
					{
						appendString(&si, instruction->modrm.fields.reg == 0b000 ? "inc " : "dec ");
						appendEb(&si);
					}
					else if (op == 0xf6 || op == 0xf7) /* test,test,not,neg,mul,imul,div,idiv */
					{
						appendString(&si, opcodeExtensionsGrp3[instruction->modrm.fields.reg]);
						*si.buffer++ = ' ';
						if (op == 0xf6)
							appendEb(&si);
						else
							appendEv(&si);

						if (instruction->modrm.fields.reg <= 0b001)
						{
							*si.buffer++ = ',';
							appendNumber(&si, instruction->immediate);
						}
					}
					else if (op == 0x69 || op == 0x6B)
					{
						appendString(&si, "imul ");
						appendGv(&si);
						*si.buffer++ = ',';
						appendEv(&si);
						*si.buffer++ = ',';
						if (op == 0x6b)
						{
							if (si.formatFlags & NMD_X86_FORMAT_FLAGS_SIGNED_NUMBER_MEMORY_VIEW && instruction->immediate >= 0x80)
								appendSignedNumberMemoryView(&si);
							else
								appendSignedNumber(&si, (int8_t)instruction->immediate, false);
						}
						else
							appendNumber(&si, instruction->immediate);
					}
					else if (op >= 0x84 && op <= 0x87)
					{
						appendString(&si, op > 0x85 ? "xchg " : "test ");
						if (op % 2 == 0)
						{
							appendEb(&si);
							*si.buffer++ = ',';
							appendGb(&si);
						}
						else
						{
							appendEv(&si);
							*si.buffer++ = ',';
							appendGv(&si);
						}
					}
					else if (op == 0x8e)
					{
						appendString(&si, "mov ");
						appendString(&si, segmentReg[instruction->modrm.fields.reg]);
						*si.buffer++ = ',';
						appendEw(&si);
					}
					else if (op >= 0x91 && op <= 0x97)
					{
						appendString(&si, "xchg ");
						if (instruction->prefixes & NMD_X86_PREFIXES_REX_B)
						{
							appendString(&si, regrx[NMD_C(op)]);
							if (!(instruction->prefixes & NMD_X86_PREFIXES_REX_W))
								*si.buffer++ = 'd';
						}
						else
							appendString(&si, (instruction->prefixes & NMD_X86_PREFIXES_REX_W ? reg64 : (operandSize ? reg16 : reg32))[NMD_C(op)]);
						appendString(&si, (instruction->prefixes & NMD_X86_PREFIXES_REX_W ? ",rax" : (operandSize ? ",ax" : ",eax")));
					}
					else if (op == 0x9A)
					{
						appendString(&si, "call far ");
						appendNumber(&si, (uint64_t)(*(uint16_t*)((char*)(&instruction->immediate) + (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? 2 : 4))));
						*si.buffer++ = ':';
						appendNumber(&si, (uint64_t)(operandSize ? *((uint16_t*)(&instruction->immediate)) : *((uint32_t*)(&instruction->immediate))));
					}
					else if ((op >= 0x6c && op <= 0x6f) || (op >= 0xa4 && op <= 0xa7) || (op >= 0xaa && op <= 0xaf))
					{
						if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT)
							appendString(&si, "rep ");
						else if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
							appendString(&si, "repne ");

						const char* str = 0;
						switch (op)
						{
						case 0x6c: case 0x6d: str = "ins"; break;
						case 0x6e: case 0x6f: str = "outs"; break;
						case 0xa4: case 0xa5: str = "movs"; break;
						case 0xa6: case 0xa7: str = "cmps"; break;
						case 0xaa: case 0xab: str = "stos"; break;
						case 0xac: case 0xad: str = "lods"; break;
						case 0xae: case 0xaf: str = "scas"; break;
						}
						appendString(&si, str);
						*si.buffer++ = (op % 2 == 0) ? 'b' : (operandSize ? 'w' : 'd');
					}
					else if (op == 0xC0 || op == 0xC1 || (NMD_R(op) == 0xd && NMD_C(op) < 4))
					{
						appendString(&si, opcodeExtensionsGrp2[instruction->modrm.fields.reg]);
						*si.buffer++ = ' ';
						if (op % 2 == 0)
							appendEb(&si);
						else
							appendEv(&si);
						*si.buffer++ = ',';
						if (NMD_R(op) == 0xc)
							appendNumber(&si, instruction->immediate);
						else if (NMD_C(op) < 2)
							appendNumber(&si, 1);
						else
							appendString(&si, "cl");
					}
					else if (op == 0xc2)
					{
						appendString(&si, "ret ");
						appendNumber(&si, instruction->immediate);
					}
					else if (op >= 0xe0 && op <= 0xe3)
					{
						const char* mnemonics[] = { "loopne", "loope", "loop" };
						appendString(&si, op == 0xe3 ? (instruction->mode == NMD_X86_MODE_64 ? "jrcxz" : "jecxz") : mnemonics[NMD_C(op)]);
						*si.buffer++ = ' ';
						appendRelativeAddress8(&si);
					}
					else if (op == 0xea)
					{
						appendString(&si, "jmp far ");
						appendNumber(&si, (uint64_t)(*(uint16_t*)(((uint8_t*)(&instruction->immediate) + 4))));
						*si.buffer++ = ':';
						appendNumber(&si, (uint64_t)(*(uint32_t*)(&instruction->immediate)));
					}
					else if (op == 0xca)
					{
						appendString(&si, "ret far");
						appendNumber(&si, instruction->immediate);
					}
					else if (op == 0xcd)
					{
						appendString(&si, "int ");
						appendNumber(&si, instruction->immediate);
					}
					else if (op == 0x63)
					{
						if (instruction->mode == NMD_X86_MODE_64)
						{
							appendString(&si, "movsxd ");
							appendString(&si, (instruction->mode == NMD_X86_MODE_64 ? (instruction->prefixes & NMD_X86_PREFIXES_REX_R ? regrx : reg64) : (operandSize ? reg16 : reg32))[instruction->modrm.fields.reg]);
							*si.buffer++ = ',';
							if (instruction->modrm.fields.mod == 0b11)
							{
								if (instruction->prefixes & NMD_X86_PREFIXES_REX_B)
									appendString(&si, regrx[instruction->modrm.fields.rm]), *si.buffer++ = 'd';
								else
									appendString(&si, ((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && instruction->mode == NMD_X86_MODE_32) || (instruction->mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? reg16 : reg32)[instruction->modrm.fields.rm]);
							}
							else
								appendModRmUpper(&si, (instruction->operandSize64 && !(instruction->prefixes & NMD_X86_PREFIXES_REX_W)) ? "qword" : ((instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE && instruction->mode == NMD_X86_MODE_32) || (instruction->mode == NMD_X86_MODE_16 && !(instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)) ? "word" : "dword"));
						}
						else
						{
							appendString(&si, "arpl ");
							appendEw(&si);
							*si.buffer++ = ',';
							appendGw(&si);
						}
					}
					else if (op == 0xc4 || op == 0xc5)
					{
						appendString(&si, op == 0xc4 ? "les" : "lds");
						*si.buffer++ = ' ';
						appendGv(&si);
						*si.buffer++ = ',';
						if (si.instruction->modrm.fields.mod == 0b11)
							appendString(&si, (si.instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? reg16 : reg32)[si.instruction->modrm.fields.rm]);
						else
							appendModRmUpper(&si, si.instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "dword" : "fword");
					}
					else if (op == 0xc6 || op == 0xc7)
					{
						appendString(&si, instruction->modrm.fields.reg == 0b000 ? "mov " : (op == 0xc6 ? "xabort " : "xbegin "));
						if (instruction->modrm.fields.reg == 0b111)
						{
							if (op == 0xc6)
								appendNumber(&si, instruction->immediate);
							else
								appendRelativeAddress16_32(&si);
						}
						else
						{
							if (op == 0xc6)
								appendEb(&si);
							else
								appendEv(&si);
							*si.buffer++ = ',';
							appendNumber(&si, instruction->immediate);
						}
					}
					else if (op == 0xc8)
					{
						appendString(&si, "enter ");
						appendNumber(&si, (uint64_t)(*(uint16_t*)(&instruction->immediate)));
						*si.buffer++ = ',';
						appendNumber(&si, (uint64_t)(*((uint8_t*)(&instruction->immediate) + 2)));
					}
					else if (op == 0xd4)
					{
						appendString(&si, "aam ");
						appendNumber(&si, instruction->immediate);
					}
					else if (op == 0xd5)
					{
						appendString(&si, "aad ");
						appendNumber(&si, instruction->immediate);
					}
					else if (op >= 0xd8 && op <= 0xdf)
					{
						*si.buffer++ = 'f';

						if (instruction->modrm.modrm < 0xc0)
						{
							appendString(&si, escapeOpcodes[NMD_C(op) - 8][instruction->modrm.fields.reg]);
							*si.buffer++ = ' ';
							switch (op)
							{
							case 0xd8: case 0xda: appendModRmUpper(&si, "dword"); break;
							case 0xd9: appendModRmUpper(&si, instruction->modrm.fields.reg & 0b100 ? (instruction->modrm.fields.reg & 0b001 ? "word" : (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "m14" : "m28")) : "dword"); break;
							case 0xdb: appendModRmUpper(&si, instruction->modrm.fields.reg & 0b100 ? "tbyte" : "dword"); break;
							case 0xdc: appendModRmUpper(&si, "qword"); break;
							case 0xdd: appendModRmUpper(&si, instruction->modrm.fields.reg & 0b100 ? ((instruction->modrm.fields.reg & 0b111) == 0b111 ? "word" : "byte") : "qword"); break;
							case 0xde: appendModRmUpper(&si, "word"); break;
							case 0xdf: appendModRmUpper(&si, instruction->modrm.fields.reg & 0b100 ? (instruction->modrm.fields.reg & 0b001 ? "qword" : "tbyte") : "word"); break;
							}
						}
						else
						{
							switch (op)
							{
							case 0xd8:
								appendString(&si, escapeOpcodesD8[(NMD_R(instruction->modrm.modrm) - 0xc) * 2 + (NMD_C(instruction->modrm.modrm) > 7 ? 1 : 0)]);
								appendString(&si, " st(0),st(");
								*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8), *si.buffer++ = ')';
								break;
							case 0xd9:
								if (NMD_R(instruction->modrm.modrm) == 0xc)
								{
									appendString(&si, NMD_C(instruction->modrm.modrm) < 8 ? "ld" : "xch");
									appendString(&si, " st(0),st(");
									*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8), *si.buffer++ = ')';
								}
								else if (instruction->modrm.modrm >= 0xd8 && instruction->modrm.modrm <= 0xdf)
								{
									appendString(&si, "stpnce st(");
									*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
									appendString(&si, "),st(0)");
								}
								else
								{
									const char* str = 0;
									switch (instruction->modrm.modrm)
									{
									case 0xd0: str = "nop"; break;
									case 0xe0: str = "chs"; break;
									case 0xe1: str = "abs"; break;
									case 0xe4: str = "tst"; break;
									case 0xe5: str = "xam"; break;
									case 0xe8: str = "ld1"; break;
									case 0xe9: str = "ldl2t"; break;
									case 0xea: str = "ldl2e"; break;
									case 0xeb: str = "ldpi"; break;
									case 0xec: str = "ldlg2"; break;
									case 0xed: str = "ldln2"; break;
									case 0xee: str = "ldz"; break;
									case 0xf0: str = "2xm1"; break;
									case 0xf1: str = "yl2x"; break;
									case 0xf2: str = "ptan"; break;
									case 0xf3: str = "patan"; break;
									case 0xf4: str = "xtract"; break;
									case 0xf5: str = "prem1"; break;
									case 0xf6: str = "decstp"; break;
									case 0xf7: str = "incstp"; break;
									case 0xf8: str = "prem"; break;
									case 0xf9: str = "yl2xp1"; break;
									case 0xfa: str = "sqrt"; break;
									case 0xfb: str = "sincos"; break;
									case 0xfc: str = "rndint"; break;
									case 0xfd: str = "scale"; break;
									case 0xfe: str = "sin"; break;
									case 0xff: str = "cos"; break;
									}
									appendString(&si, str);
								}
								break;
							case 0xda:
								if (instruction->modrm.modrm == 0xe9)
									appendString(&si, "ucompp");
								else
								{
									const char* mnemonics[4] = { "cmovb", "cmovbe", "cmove", "cmovu" };
									appendString(&si, mnemonics[(NMD_R(instruction->modrm.modrm) - 0xc) + (NMD_C(instruction->modrm.modrm) > 7 ? 2 : 0)]);
									appendString(&si, " st(0),st(");
									*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
									*si.buffer++ = ')';
								}
								break;
							case 0xdb:
								if (NMD_R(instruction->modrm.modrm) == 0xe && NMD_C(instruction->modrm.modrm) < 8)
								{
									const char* mnemonics[] = { "eni8087_nop", "disi8087_nop", "nclex", "ninit", "setpm287_nop" };
									appendString(&si, mnemonics[NMD_C(instruction->modrm.modrm)]);
								}
								else
								{
									if (instruction->modrm.modrm >= 0xe0)
										appendString(&si, instruction->modrm.modrm < 0xf0 ? "ucomi" : "comi");
									else
									{
										appendString(&si, "cmovn");
										if (instruction->modrm.modrm < 0xc8)
											*si.buffer++ = 'b';
										else if (instruction->modrm.modrm < 0xd0)
											*si.buffer++ = 'e';
										else if (instruction->modrm.modrm >= 0xd8)
											*si.buffer++ = 'u';
										else
											appendString(&si, "be");
									}
									appendString(&si, " st(0),st(");
									*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
									*si.buffer++ = ')';
								}
								break;
							case 0xdc:
								if (NMD_R(instruction->modrm.modrm) == 0xc)
									appendString(&si, NMD_C(instruction->modrm.modrm) > 7 ? "mul" : "add");
								else
								{
									appendString(&si, NMD_R(instruction->modrm.modrm) == 0xd ? "com" : (NMD_R(instruction->modrm.modrm) == 0xe ? "subr" : "div"));
									if (NMD_R(instruction->modrm.modrm) == 0xd && NMD_C(instruction->modrm.modrm) >= 8)
									{
										if (NMD_R(instruction->modrm.modrm) >= 8)
											*si.buffer++ = 'p';
									}
									else
									{
										if (NMD_R(instruction->modrm.modrm) < 8)
											*si.buffer++ = 'r';
									}
								}

								if (NMD_R(instruction->modrm.modrm) == 0xd)
								{
									appendString(&si, " st(0),st(");
									*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
									*si.buffer++ = ')';
								}
								else
								{
									appendString(&si, " st(");
									*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
									appendString(&si, "),st(0)");
								}
								break;
							case 0xdd:
								if (NMD_R(instruction->modrm.modrm) == 0xc)
									appendString(&si, NMD_C(instruction->modrm.modrm) < 8 ? "free" : "xch");
								else
								{
									appendString(&si, instruction->modrm.modrm < 0xe0 ? "st" : "ucom");
									if (NMD_C(instruction->modrm.modrm) >= 8)
										*si.buffer++ = 'p';
								}

								appendString(&si, " st(");
								*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
								*si.buffer++ = ')';

								break;
							case 0xde:
								if (instruction->modrm.modrm == 0xd9)
									appendString(&si, "compp");
								else
								{
									if (instruction->modrm.modrm >= 0xd0 && instruction->modrm.modrm <= 0xd7)
									{
										appendString(&si, "comp st(0),st(");
										*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
										*si.buffer++ = ')';
									}
									else
									{
										if (NMD_R(instruction->modrm.modrm) == 0xc)
											appendString(&si, NMD_C(instruction->modrm.modrm) < 8 ? "add" : "mul");
										else
										{
											appendString(&si, instruction->modrm.modrm < 0xf0 ? "sub" : "div");
											if (NMD_R(instruction->modrm.modrm) < 8 || (NMD_R(instruction->modrm.modrm) >= 0xe && NMD_C(instruction->modrm.modrm) < 8))
												*si.buffer++ = 'r';
										}
										appendString(&si, "p st(");
										*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
										appendString(&si, "),st(0)");
									}
								}
								break;
							case 0xdf:
								if (instruction->modrm.modrm == 0xe0)
									appendString(&si, "nstsw ax");
								else
								{
									if (instruction->modrm.modrm >= 0xe8)
									{
										if (instruction->modrm.modrm < 0xf0)
											*si.buffer++ = 'u';
										appendString(&si, "comip");
										appendString(&si, " st(0),st(");
										*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
										*si.buffer++ = ')';
									}
									else
									{
										appendString(&si, instruction->modrm.modrm < 0xc8 ? "freep" : (instruction->modrm.modrm >= 0xd0 ? "stp" : "xch"));
										appendString(&si, " st(");
										*si.buffer++ = (char)('0' + instruction->modrm.modrm % 8);
										*si.buffer++ = ')';
									}
								}

								break;
							}
						}
					}
					else if (op == 0xe4 || op == 0xe5)
					{
						appendString(&si, "in ");
						appendString(&si, op == 0xe4 ? "al" : (operandSize ? "ax" : "eax"));
						*si.buffer++ = ',';
						appendNumber(&si, instruction->immediate);
					}
					else if (op == 0xe6 || op == 0xe7)
					{
						appendString(&si, "out ");
						appendNumber(&si, instruction->immediate);
						*si.buffer++ = ',';
						appendString(&si, op == 0xe6 ? "al" : (operandSize ? "ax" : "eax"));
					}
					else if (op == 0xec || op == 0xed)
					{
						appendString(&si, "in ");
						appendString(&si, op == 0xec ? "al" : (operandSize ? "ax" : "eax"));
						appendString(&si, ",dx");
					}
					else if (op == 0xee || op == 0xef)
					{
						appendString(&si, "out dx,");
						appendString(&si, op == 0xee ? "al" : (operandSize ? "ax" : "eax"));
					}
					else if (op == 0x62)
					{
						appendString(&si, "bound ");
						appendGv(&si);
						*si.buffer++ = ',';
						appendModRmUpper(&si, operandSize ? "dword" : "qword");
					}
					else /* Try to parse all opcodes not parsed by the checks above. */
					{
						const char* str = 0;
						switch (instruction->opcode)
						{
						case 0x9c:
						{
							if (operandSize)
								str = (instruction->mode == NMD_X86_MODE_16) ? "pushfd" : "pushf";
							else
								str = (instruction->mode == NMD_X86_MODE_16) ? "pushf" : ((instruction->mode == NMD_X86_MODE_32) ? "pushfd" : "pushfq");
							break;
						}
						case 0x9d:
						{
							if (operandSize)
								str = (instruction->mode == NMD_X86_MODE_16) ? "popfd" : "popf";
							else
								str = (instruction->mode == NMD_X86_MODE_16) ? "popf" : ((instruction->mode == NMD_X86_MODE_32) ? "popfd" : "popfq");
							break;
						}
						case 0x60:
						case 0x61:
							str = operandSize ? (instruction->opcode == 0x60 ? "pusha" : "popa") : (instruction->opcode == 0x60 ? "pushad" : "popad");
							break;
						case 0xcb: str = "retf"; break;
						case 0xc9: str = "leave"; break;
						case 0xf1: str = "int1"; break;
						case 0x06: str = "push es"; break;
						case 0x16: str = "push ss"; break;
						case 0x1e: str = "push ds"; break;
						case 0x0e: str = "push cs"; break;
						case 0x07: str = "pop es"; break;
						case 0x17: str = "pop ss"; break;
						case 0x1f: str = "pop ds"; break;
						case 0x27: str = "daa"; break;
						case 0x37: str = "aaa"; break;
						case 0x2f: str = "das"; break;
						case 0x3f: str = "aas"; break;
						case 0xd7: str = "xlat"; break;
						case 0x9b: str = "fwait"; break;
						case 0xf4: str = "hlt"; break;
						case 0xf5: str = "cmc"; break;
						case 0x9e: str = "sahf"; break;
						case 0x9f: str = "lahf"; break;
						case 0xce: str = "into"; break;
						case 0xcf:
							if (instruction->operandSize64)
								str = "iretq";
							else if (instruction->mode == NMD_X86_MODE_16)
								str = operandSize ? "iretd" : "iret";
							else
								str = operandSize ? "iret" : "iretd";
							break;
						case 0x98: str = (instruction->prefixes & NMD_X86_PREFIXES_REX_W ? "cdqe" : (operandSize ? "cbw" : "cwde")); break;
						case 0x99: str = (instruction->prefixes & NMD_X86_PREFIXES_REX_W ? "cqo" : (operandSize ? "cwd" : "cdq")); break;
						case 0xd6: str = "salc"; break;
						case 0xf8: str = "clc"; break;
						case 0xf9: str = "stc"; break;
						case 0xfa: str = "cli"; break;
						case 0xfb: str = "sti"; break;
						case 0xfc: str = "cld"; break;
						case 0xfd: str = "std"; break;
						default: return;
						}
						appendString(&si, str);
					}
				}
	}
	else if (instruction->opcodeMap == NMD_X86_OPCODE_MAP_0F)
	{
		if (NMD_R(op) == 8)
		{
			*si.buffer++ = 'j';
			appendString(&si, conditionSuffixes[NMD_C(op)]);
			*si.buffer++ = ' ';
			appendRelativeAddress16_32(&si);
		}
		else if (op == 0x05)
			appendString(&si, "syscall");
		else if (op == 0xa2)
			appendString(&si, "cpuid");
		else if (NMD_R(op) == 4)
		{
			appendString(&si, "cmov");
			appendString(&si, conditionSuffixes[NMD_C(op)]);
			*si.buffer++ = ' ';
			appendGv(&si);
			*si.buffer++ = ',';
			appendEv(&si);
		}
		else if (op >= 0x10 && op <= 0x17)
		{
			if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
			{
				const char* prefix66Mnemonics[] = { "movupd", "movupd", "movlpd", "movlpd", "unpcklpd", "unpckhpd", "movhpd", "movhpd" };

				appendString(&si, prefix66Mnemonics[NMD_C(op)]);
				*si.buffer++ = ' ';

				switch (NMD_C(op))
				{
				case 0:
					appendVx(&si);
					*si.buffer++ = ',';
					appendW(&si);
					break;
				case 1:
					appendW(&si);
					*si.buffer++ = ',';
					appendVx(&si);
					break;
				case 2:
				case 6:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						appendString(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						appendModRmUpper(&si, "qword");
					break;
				default:
					break;
				}
			}
			else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT)
			{
				const char* prefixF3Mnemonics[] = { "movss", "movss", "movsldup", 0, 0, 0, "movshdup" };

				appendString(&si, prefixF3Mnemonics[NMD_C(op)]);
				*si.buffer++ = ' ';

				switch (NMD_C(op))
				{
				case 0:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						appendString(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						appendModRmUpper(&si, "dword");
					break;
				case 1:
					if (si.instruction->modrm.fields.mod == 0b11)
						appendString(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						appendModRmUpper(&si, "dword");
					*si.buffer++ = ',';
					appendVdq(&si);
					break;
				case 2:
				case 6:
					appendVdq(&si);
					*si.buffer++ = ',';
					appendW(&si);
					break;
				}
			}
			else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
			{
				const char* prefixF2Mnemonics[] = { "movsd", "movsd", "movddup" };

				appendString(&si, prefixF2Mnemonics[NMD_C(op)]);
				*si.buffer++ = ' ';

				switch (NMD_C(op))
				{
				case 0:
				case 2:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						appendString(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						appendModRmUpper(&si, "qword");
					break;
				case 1:
					if (si.instruction->modrm.fields.mod == 0b11)
						appendString(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						appendModRmUpper(&si, "qword");
					*si.buffer++ = ',';
					appendVdq(&si);
					break;
				}
			}
			else
			{
				const char* noPrefixMnemonics[] = { "movups", "movups", "movlps", "movlps", "unpcklps", "unpckhps", "movhps", "movhps" };

				if (op == 0x12 && instruction->modrm.fields.mod == 0b11)
					appendString(&si, "movhlps");
				else if (op == 0x16 && instruction->modrm.fields.mod == 0b11)
					appendString(&si, "movlhps");
				else
					appendString(&si, noPrefixMnemonics[NMD_C(op)]);
				*si.buffer++ = ' ';

				switch (NMD_C(op))
				{
				case 0:
					appendVdq(&si);
					*si.buffer++ = ',';
					appendW(&si);
					break;
				case 1:
					appendW(&si);
					*si.buffer++ = ',';
					appendVdq(&si);
					break;
				case 2:
				case 6:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						appendString(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						appendModRmUpper(&si, "qword");
					break;
				default:
					break;
				};

			}

			switch (NMD_C(op))
			{
			case 3:
			case 7:
				if (si.instruction->modrm.fields.mod == 0b11)
					appendString(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
				else
					appendModRmUpper(&si, "qword");
				*si.buffer++ = ',';
				appendVdq(&si);
				break;
			case 4:
			case 5:
				appendVdq(&si);
				*si.buffer++ = ',';
				appendW(&si);
				break;
			};
		}
		else if (NMD_R(op) == 6 || (op >= 0x74 && op <= 0x76))
		{
			if (op == 0x6e)
			{
				appendString(&si, "movd ");
				if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
					*si.buffer++ = 'x';
				appendPq(&si);
				*si.buffer++ = ',';
				if (si.instruction->modrm.fields.mod == 0b11)
					appendString(&si, reg32[si.instruction->modrm.fields.rm]);
				else
					appendModRmUpper(&si, "dword");
			}
			else
			{
				if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
				{
					const char* prefix66Mnemonics[] = { "punpcklbw", "punpcklwd", "punpckldq", "packsswb", "pcmpgtb", "pcmpgtw", "pcmpgtd", "packuswb", "punpckhbw", "punpckhwd", "punpckhdq", "packssdw", "punpcklqdq", "punpckhqdq", "movd", "movdqa" };

					appendString(&si, op == 0x74 ? "pcmpeqb" : (op == 0x75 ? "pcmpeqw" : (op == 0x76 ? "pcmpeqd" : prefix66Mnemonics[op % 0x10])));
					*si.buffer++ = ' ';
					appendVdq(&si);
					*si.buffer++ = ',';
					appendW(&si);
				}
				else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT)
				{
					appendString(&si, "movdqu ");
					appendVdq(&si);
					*si.buffer++ = ',';
					appendW(&si);
				}
				else
				{
					const char* noPrefixMnemonics[] = { "punpcklbw", "punpcklwd", "punpckldq", "packsswb", "pcmpgtb", "pcmpgtw", "pcmpgtd", "packuswb", "punpckhbw", "punpckhwd", "punpckhdq", "packssdw", 0, 0, "movd", "movq" };

					appendString(&si, op == 0x74 ? "pcmpeqb" : (op == 0x75 ? "pcmpeqw" : (op == 0x76 ? "pcmpeqd" : noPrefixMnemonics[op % 0x10])));
					*si.buffer++ = ' ';
					appendPq(&si);
					*si.buffer++ = ',';
					appendQq(&si);
				}
			}
		}
		else if (op == 0x00)
		{
			appendString(&si, opcodeExtensionsGrp6[instruction->modrm.fields.reg]);
			*si.buffer++ = ' ';
			if (NMD_R(instruction->modrm.modrm) == 0xc)
				appendEv(&si);
			else
				appendEw(&si);
		}
		else if (op == 0x01)
		{
			if (instruction->modrm.fields.mod == 0b11)
			{
				if (instruction->modrm.fields.reg == 0b000)
					appendString(&si, opcodeExtensionsGrp7reg0[instruction->modrm.fields.rm]);
				else if (instruction->modrm.fields.reg == 0b001)
					appendString(&si, opcodeExtensionsGrp7reg1[instruction->modrm.fields.rm]);
				else if (instruction->modrm.fields.reg == 0b010)
					appendString(&si, opcodeExtensionsGrp7reg2[instruction->modrm.fields.rm]);
				else if (instruction->modrm.fields.reg == 0b011)
				{
					appendString(&si, opcodeExtensionsGrp7reg3[instruction->modrm.fields.rm]);
					if (instruction->modrm.fields.rm == 0b000 || instruction->modrm.fields.rm == 0b010 || instruction->modrm.fields.rm == 0b111)
						appendString(&si, instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "ax" : "eax");

					if (instruction->modrm.fields.rm == 0b111)
						appendString(&si, ",ecx");
				}
				else if (instruction->modrm.fields.reg == 0b100)
					appendString(&si, "smsw "), appendString(&si, (instruction->operandSize64 ? reg64 : (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? reg16 : reg32))[instruction->modrm.fields.rm]);
				else if (instruction->modrm.fields.reg == 0b101)
				{
					if (instruction->prefixes & NMD_X86_PREFIXES_REPEAT)
						appendString(&si, instruction->modrm.fields.rm == 0b000 ? "setssbsy" : "saveprevssp");
					else
						appendString(&si, instruction->modrm.fields.rm == 0b111 ? "wrpkru" : "rdpkru");
				}
				else if (instruction->modrm.fields.reg == 0b110)
					appendString(&si, "lmsw "), appendString(&si, reg16[instruction->modrm.fields.rm]);
				else if (instruction->modrm.fields.reg == 0b111)
				{
					appendString(&si, opcodeExtensionsGrp7reg7[instruction->modrm.fields.rm]);
					if (instruction->modrm.fields.rm == 0b100)
						appendString(&si, instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "ax" : "eax");
				}
			}
			else
			{
				if (instruction->modrm.fields.reg == 0b101)
				{
					appendString(&si, "rstorssp ");
					appendModRmUpper(&si, "qword");
				}
				else
				{
					appendString(&si, opcodeExtensionsGrp7[instruction->modrm.fields.reg]);
					*si.buffer++ = ' ';
					if (si.instruction->modrm.fields.reg == 0b110)
						appendEw(&si);
					else
						appendModRmUpper(&si, si.instruction->modrm.fields.reg == 0b111 ? "byte" : si.instruction->modrm.fields.reg == 0b100 ? "word" : "fword");
				}
			}
		}
		else if (op == 0x02 || op == 0x03)
		{
			appendString(&si, op == 0x02 ? "lar" : "lsl");
			*si.buffer++ = ' ';
			appendGv(&si);
			*si.buffer++ = ',';
			if (si.instruction->modrm.fields.mod == 0b11)
				appendString(&si, (operandSize ? reg16 : reg32)[si.instruction->modrm.fields.rm]);
			else
				appendModRmUpper(&si, "word");
		}
		else if (op == 0x0d)
		{
			if (instruction->modrm.fields.mod == 0b11)
			{
				appendString(&si, "nop ");
				appendString(&si, (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? reg16 : reg32)[instruction->modrm.fields.rm]);
				*si.buffer++ = ',';
				appendString(&si, (instruction->prefixes & NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? reg16 : reg32)[instruction->modrm.fields.reg]);
			}
			else
			{
				appendString(&si, "prefetch");
				if (instruction->modrm.fields.reg == 0b001)
					*si.buffer++ = 'w';
				else if (instruction->modrm.fields.reg == 0b010)
					appendString(&si, "wt1");

				*si.buffer++ = ' ';

				appendModRmUpper(&si, "byte");
			}
		}
		else if (op == 0x18)
		{
			if (instruction->modrm.fields.mod == 0b11 || instruction->modrm.fields.reg >= 0b100)
			{
				appendString(&si, "nop ");
				appendEv(&si);
			}
			else
			{
				if (instruction->modrm.fields.reg == 0b000)
					appendString(&si, "prefetchnta");
				else
				{
					appendString(&si, "prefetcht");
					*si.buffer++ = (char)('0' + (instruction->modrm.fields.reg - 1));
				}
				*si.buffer++ = ' ';

				appendEb(&si);
			}
		}
		else if (op == 0x19)
		{
			appendString(&si, "nop ");
			appendEv(&si);
			*si.buffer++ = ',';
			appendGv(&si);
		}
		else if (op == 0x1A)
		{
			if (instruction->modrm.fields.mod == 0b11)
			{
				appendString(&si, "nop ");
				appendEv(&si);
				*si.buffer++ = ',';
				appendGv(&si);
			}
			else
			{
				if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
					appendString(&si, "bndmov");
				else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT)
					appendString(&si, "bndcl");
				else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
					appendString(&si, "bndcu");
				else
					appendString(&si, "bndldx");

				appendString(&si, " bnd");
				*si.buffer++ = (char)('0' + instruction->modrm.fields.reg);
				*si.buffer++ = ',';
				if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
					*si.buffer++ = 'q';
				appendEv(&si);
			}
		}
		else if (op == 0x1B)
		{
			if (instruction->modrm.fields.mod == 0b11)
			{
				appendString(&si, "nop ");
				appendEv(&si);
				*si.buffer++ = ',';
				appendGv(&si);
			}
			else
			{
				if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
					appendString(&si, "bndmov");
				else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT)
					appendString(&si, "bndmk");
				else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
					appendString(&si, "bndcn");
				else
					appendString(&si, "bndstx");

				*si.buffer++ = ' ';
				appendEv(&si);
				*si.buffer++ = ',';
				appendString(&si, "bnd");
				*si.buffer++ = (char)('0' + instruction->modrm.fields.reg);
			}
		}
		else if (op >= 0x1c && op <= 0x1f)
		{
			if (op == 0x1e && instruction->modrm.modrm == 0xfa)
				appendString(&si, "endbr64");
			else if (op == 0x1e && instruction->modrm.modrm == 0xfb)
				appendString(&si, "endbr32");
			else
			{
				appendString(&si, "nop ");
				appendEv(&si);
				*si.buffer++ = ',';
				appendGv(&si);
			}
		}
		else if (op >= 0x20 && op <= 0x23)
		{
			appendString(&si, "mov ");
			if (op < 0x22)
			{
				appendString(&si, (instruction->mode == NMD_X86_MODE_64 ? reg64 : reg32)[instruction->modrm.fields.rm]);
				appendString(&si, op == 0x20 ? ",cr" : ",dr");
				*si.buffer++ = (char)('0' + instruction->modrm.fields.reg);
			}
			else
			{
				appendString(&si, op == 0x22 ? "cr" : "dr");
				*si.buffer++ = (char)('0' + instruction->modrm.fields.reg);
				*si.buffer++ = ',';
				appendString(&si, (instruction->mode == NMD_X86_MODE_64 ? reg64 : reg32)[instruction->modrm.fields.rm]);
			}
		}
		else if (op >= 0x28 && op <= 0x2f)
		{
			if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
			{
				const char* prefix66Mnemonics[] = { "movapd", "movapd", "cvtpi2pd", "movntpd", "cvttpd2pi", "cvtpd2pi", "ucomisd", "comisd" };

				appendString(&si, prefix66Mnemonics[op % 8]);
				*si.buffer++ = ' ';

				switch (op % 8)
				{
				case 0:
					appendVx(&si);
					*si.buffer++ = ',';
					appendW(&si);
					break;
				case 1:
					appendW(&si);
					*si.buffer++ = ',';
					appendVx(&si);
					break;
				case 2:
					appendVdq(&si);
					*si.buffer++ = ',';
					appendQq(&si);
					break;
				case 4:
				case 5:
					appendPq(&si);
					*si.buffer++ = ',';
					appendW(&si);
					break;
				case 6:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (instruction->modrm.fields.mod == 0b11)
						appendString(&si, "xmm"), *si.buffer++ = (char)('0' + instruction->modrm.fields.rm);
					else
						appendModRmUpper(&si, "qword");
					break;
				case 7:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (instruction->modrm.fields.mod == 0b11)
						appendString(&si, "xmm"), *si.buffer++ = (char)('0' + instruction->modrm.fields.rm);
					else
						appendModRmUpper(&si, "qword");
				default:
					break;
				}
			}
			else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT)
			{
				const char* prefixF3Mnemonics[] = { 0, 0, "cvtsi2ss", "movntss", "cvttss2si", "cvtss2si", 0, 0 };

				appendString(&si, prefixF3Mnemonics[op % 8]);
				*si.buffer++ = ' ';

				switch (op % 8)
				{
				case 3:
					appendModRmUpper(&si, "dword");
					*si.buffer++ = ',';
					appendVdq(&si);
					break;
				case 4:
				case 5:
					appendGv(&si);
					*si.buffer++ = ',';
					if (instruction->modrm.fields.mod == 0b11)
						appendUdq(&si);
					else
						appendModRmUpper(&si, "dword");
					break;
				case 2:
				case 6:
					appendVdq(&si);
					*si.buffer++ = ',';
					appendEv(&si);
					break;
				}
			}
			else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
			{
				const char* prefixF2Mnemonics[] = { 0, 0, "cvtsi2sd", "movntsd", "cvttsd2si", "cvtsd2si", 0, 0 };

				appendString(&si, prefixF2Mnemonics[op % 8]);
				*si.buffer++ = ' ';

				switch (op % 8)
				{
				case 2:
					appendVdq(&si);
					*si.buffer++ = ',';
					appendEv(&si);
					break;
				case 3:
					appendModRmUpper(&si, "qword");
					*si.buffer++ = ',';
					appendVdq(&si);
					break;
				case 4:
				case 5:
					appendGv(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						appendString(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						appendModRmUpper(&si, "qword");
					break;
				}
			}
			else
			{
				const char* noPrefixMnemonics[] = { "movaps", "movaps", "cvtpi2ps", "movntps", "cvttps2pi", "cvtps2pi", "ucomiss", "comiss" };

				appendString(&si, noPrefixMnemonics[op % 8]);
				*si.buffer++ = ' ';

				switch (op % 8)
				{
				case 0:
					appendVdq(&si);
					*si.buffer++ = ',';
					appendW(&si);
					break;
				case 1:
					appendW(&si);
					*si.buffer++ = ',';
					appendVdq(&si);
					break;
				case 4:
				case 5:
					appendPq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						appendString(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						appendModRmUpper(&si, "qword");
					break;
				case 2:
					appendVdq(&si);
					*si.buffer++ = ',';
					appendQq(&si);
					break;
				case 6:
				case 7:
					appendVdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						appendString(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						appendModRmUpper(&si, "dword");
					break;
				default:
					break;
				};

			}

			if (!(instruction->prefixes & (NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO)) && (op % 8) == 3)
			{
				appendModRmUpper(&si, "xmmword");
				*si.buffer++ = ',';
				appendVdq(&si);
			}
		}
		else if (NMD_R(op) == 5)
		{
			if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
			{
				const char* prefix66Mnemonics[] = { "movmskpd", "sqrtpd", 0, 0, "andpd", "andnpd", "orpd", "xorpd", "addpd", "mulpd", "cvtpd2ps",  "cvtps2dq", "subpd", "minpd", "divpd", "maxpd" };

				appendString(&si, prefix66Mnemonics[op % 0x10]);
				*si.buffer++ = ' ';
				if (op == 0x50)
					appendString(&si, reg32[instruction->modrm.fields.reg]);
				else
					appendVdq(&si);
				*si.buffer++ = ',';
				appendW(&si);
			}
			else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT)
			{
				const char* prefixF3Mnemonics[] = { 0, "sqrtss", "rsqrtss", "rcpss", 0, 0, 0, 0, "addss", "mulss", "cvtss2sd", "cvttps2dq", "subss", "minss", "divss", "maxss" };

				appendString(&si, prefixF3Mnemonics[op % 0x10]);
				*si.buffer++ = ' ';
				appendVdq(&si);
				*si.buffer++ = ',';
				if (si.instruction->modrm.fields.mod == 0b11)
					appendString(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
				else
					appendModRmUpper(&si, op == 0x5b ? "xmmword" : "dword");
			}
			else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
			{
				const char* prefixF2Mnemonics[] = { 0, "sqrtsd", 0, 0, 0, 0, 0, 0, "addsd", "mulsd", "cvtsd2ss", 0, "subsd", "minsd", "divsd", "maxsd" };

				appendString(&si, prefixF2Mnemonics[op % 0x10]);
				*si.buffer++ = ' ';
				appendVdq(&si);
				*si.buffer++ = ',';
				if (si.instruction->modrm.fields.mod == 0b11)
					appendString(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
				else
					appendModRmUpper(&si, "qword");
			}
			else
			{
				const char* noPrefixMnemonics[] = { "movmskps", "sqrtps", "rsqrtps", "rcpps", "andps", "andnps", "orps", "xorps", "addps", "mulps", "cvtps2pd",  "cvtdq2ps", "subps", "minps", "divps", "maxps" };

				appendString(&si, noPrefixMnemonics[op % 0x10]);
				*si.buffer++ = ' ';
				if (op == 0x50)
				{
					appendString(&si, reg32[instruction->modrm.fields.reg]);
					*si.buffer++ = ',';
					appendUdq(&si);
				}
				else
				{
					appendVdq(&si);
					*si.buffer++ = ',';
					if (si.instruction->modrm.fields.mod == 0b11)
						appendString(&si, "xmm"), *si.buffer++ = (char)('0' + si.instruction->modrm.fields.rm);
					else
						appendModRmUpper(&si, op == 0x5a ? "qword" : "xmmword");
				}
			}
		}
		else if (op == 0x70)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "pshufd" : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? "pshufhw" : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? "pshuflw" : "pshufw")));
			*si.buffer++ = ' ';
			if (!(instruction->prefixes & (NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE | NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO)))
			{
				appendPq(&si);
				*si.buffer++ = ',';
				appendQq(&si);
			}
			else
			{
				appendVdq(&si);
				*si.buffer++ = ',';
				appendW(&si);
			}

			*si.buffer++ = ',';
			appendNumber(&si, instruction->immediate);
		}
		else if (op >= 0x71 && op <= 0x73)
		{
			if (instruction->modrm.fields.reg % 2 == 1)
				appendString(&si, instruction->modrm.fields.reg == 0b111 ? "pslldq" : "psrldq");
			else
			{
				const char* mnemonics[] = { "psrl", "psra", "psll" };
				appendString(&si, mnemonics[(instruction->modrm.fields.reg >> 1) - 1]);
				*si.buffer++ = op == 0x71 ? 'w' : (op == 0x72 ? 'd' : 'q');
			}

			*si.buffer++ = ' ';
			if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
				appendUdq(&si);
			else
				appendNq(&si);
			*si.buffer++ = ',';
			appendNumber(&si, instruction->immediate);
		}
		else if (op == 0x78)
		{
			if (!instruction->simdPrefix)
			{
				appendString(&si, "vmread ");
				appendEy(&si);
				*si.buffer++ = ',';
				appendGy(&si);
			}
			else
			{
				if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
					appendString(&si, "extrq ");
				else
				{
					appendString(&si, "insertq ");
					appendVdq(&si);
					*si.buffer++ = ',';
				}
				appendUdq(&si);
				*si.buffer++ = ',';
				appendNumber(&si, instruction->immediate & 0x00FF);
				*si.buffer++ = ',';
				appendNumber(&si, (instruction->immediate & 0xFF00) >> 8);
			}
		}
		else if (op == 0x79)
		{
			if (!instruction->simdPrefix)
			{
				appendString(&si, "vmwrite ");
				appendGy(&si);
				*si.buffer++ = ',';
				appendEy(&si);
			}
			else
			{
				appendString(&si, instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "extrq " : "insertq ");
				appendVdq(&si);
				*si.buffer++ = ',';
				appendUdq(&si);
			}

		}
		else if (op == 0x7c || op == 0x7d)
		{
			appendString(&si, op == 0x7c ? "haddp" : "hsubp");
			*si.buffer++ = instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? 'd' : 's';
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			appendW(&si);
		}
		else if (op == 0x7e)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? "movq " : "movd ");
			if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT)
			{
				appendVdq(&si);
				*si.buffer++ = ',';
				if (si.instruction->modrm.fields.mod == 0b11)
					appendUdq(&si);
				else
					appendModRmUpper(&si, "qword");
			}
			else
			{
				if (si.instruction->modrm.fields.mod == 0b11)
					appendString(&si, reg32[instruction->modrm.fields.rm]);
				else
					appendModRmUpper(&si, "dword");
				*si.buffer++ = ',';
				if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
					appendVdq(&si);
				else
					appendPq(&si);
			}
		}
		else if (op == 0x7f)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? "movdqu" : (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "movdqa" : "movq"));
			*si.buffer++ = ' ';
			if (instruction->prefixes & (NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE))
			{
				appendW(&si);
				*si.buffer++ = ',';
				appendVdq(&si);
			}
			else
			{
				if (si.instruction->modrm.fields.mod == 0b11)
					appendNq(&si);
				else
					appendModRmUpper(&si, "qword");
				*si.buffer++ = ',';
				appendPq(&si);
			}
		}
		else if (NMD_R(op) == 9)
		{
			appendString(&si, "set");
			appendString(&si, conditionSuffixes[NMD_C(op)]);
			*si.buffer++ = ' ';
			appendEb(&si);
		}
		else if ((NMD_R(op) == 0xA || NMD_R(op) == 0xB) && op % 8 == 3)
		{
			appendString(&si, op == 0xa3 ? "bt" : (op == 0xb3 ? "btr" : (op == 0xab ? "bts" : "btc")));
			*si.buffer++ = ' ';
			appendEv(&si);
			*si.buffer++ = ',';
			appendGv(&si);
		}
		else if (NMD_R(op) == 0xA && (op % 8 == 4 || op % 8 == 5))
		{
			appendString(&si, op > 0xA8 ? "shrd" : "shld");
			*si.buffer++ = ' ';
			appendEv(&si);
			*si.buffer++ = ',';
			appendGv(&si);
			*si.buffer++ = ',';
			if (op % 8 == 4)
				appendNumber(&si, instruction->immediate);
			else
				appendString(&si, "cl");
		}
		else if (op == 0xb4 || op == 0xb5)
		{
			appendString(&si, op == 0xb4 ? "lfs " : "lgs ");
			appendGv(&si);
			*si.buffer++ = ',';
			appendModRmUpper(&si, "fword");
		}
		else if (op == 0xbc || op == 0xbd)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? (op == 0xbc ? "tzcnt" : "lzcnt") : (op == 0xbc ? "bsf" : "bsr"));
			*si.buffer++ = ' ';
			appendGv(&si);
			*si.buffer++ = ',';
			appendEv(&si);
		}
		else if (op == 0xa6)
		{
			const char* mnemonics[] = { "montmul", "xsha1", "xsha256" };
			appendString(&si, mnemonics[instruction->modrm.fields.reg]);
		}
		else if (op == 0xa7)
		{
			const char* mnemonics[] = { "xstorerng", "xcryptecb", "xcryptcbc", "xcryptctr", "xcryptcfb", "xcryptofb" };
			appendString(&si, mnemonics[instruction->modrm.fields.reg]);
		}
		else if (op == 0xae)
		{
			if (instruction->modrm.fields.mod == 0b11)
			{
				if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
					appendString(&si, "pcommit");
				else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT)
				{
					appendString(&si, "incsspd ");
					appendString(&si, reg32[instruction->modrm.fields.rm]);
				}
				else
				{
					const char* mnemonics[] = { "rdfsbase", "rdgsbase", "wrfsbase", "wrgsbase", 0, "lfence", "mfence", "sfence" };
					appendString(&si, mnemonics[instruction->modrm.fields.reg]);
				}
			}
			else
			{
				if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
				{
					appendString(&si, instruction->modrm.fields.reg == 0b110 ? "clwb " : "clflushopt ");
					appendModRmUpper(&si, "byte");
				}
				else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT)
				{
					appendString(&si, instruction->modrm.fields.reg == 0b100 ? "ptwrite " : "clrssbsy ");
					appendModRmUpper(&si, instruction->modrm.fields.reg == 0b100 ? "dword" : "qword");
				}
				else
				{
					const char* mnemonics[] = { "fxsave", "fxrstor", "ldmxcsr", "stmxcsr", "xsave", "xrstor", "xsaveopt", "clflush" };
					appendString(&si, mnemonics[instruction->modrm.fields.reg]);
					*si.buffer++ = ' ';
					appendModRmUpper(&si, "dword");
				}
			}
		}
		else if (op == 0xaf)
		{
			appendString(&si, "imul ");
			appendGv(&si);
			*si.buffer++ = ',';
			appendEv(&si);
		}
		else if (op == 0xb0 || op == 0xb1)
		{
			appendString(&si, "cmpxchg ");
			if (op == 0xb0)
			{
				appendEb(&si);
				*si.buffer++ = ',';
				appendGb(&si);
			}
			else
			{
				appendEv(&si);
				*si.buffer++ = ',';
				appendGv(&si);
			}
		}
		else if (op == 0xb2)
		{
			appendString(&si, "lss ");
			appendGv(&si);
			*si.buffer++ = ',';
			appendModRmUpper(&si, "fword");
		}
		else if (NMD_R(op) == 0xb && (op % 8) >= 6)
		{
			appendString(&si, op > 0xb8 ? "movsx " : "movzx ");
			appendGv(&si);
			*si.buffer++ = ',';
			if ((op % 8) == 6)
				appendEb(&si);
			else
				appendEw(&si);
		}
		else if (op == 0xb8)
		{
			appendString(&si, "popcnt ");
			appendGv(&si);
			*si.buffer++ = ',';
			appendEv(&si);
		}
		else if (op == 0xba)
		{
			const char* mnemonics[] = { "bt","bts","btr","btc" };
			appendString(&si, mnemonics[instruction->modrm.fields.reg - 4]);
			*si.buffer++ = ' ';
			appendEv(&si);
			*si.buffer++ = ',';
			appendNumber(&si, instruction->immediate);
		}
		else if (op == 0xc0 || op == 0xc1)
		{
			appendString(&si, "xadd ");
			if (op == 0xc0)
			{
				appendEb(&si);
				*si.buffer++ = ',';
				appendGb(&si);
			}
			else
			{
				appendEv(&si);
				*si.buffer++ = ',';
				appendGv(&si);
			}
		}
		else if (op == 0xc2)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "cmppd" : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? "cmpss" : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? "cmpsd" : "cmpps")));
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			if (si.instruction->modrm.fields.mod == 0b11)
				appendUdq(&si);
			else
				appendModRmUpper(&si, instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? "dword" : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? "qword" : "xmmword"));
			*si.buffer++ = ',';
			appendNumber(&si, instruction->immediate);
		}
		else if (op == 0xc3)
		{
			appendString(&si, "movnti ");
			appendModRmUpper(&si, "dword");
			*si.buffer++ = ',';
			appendString(&si, reg32[instruction->modrm.fields.reg]);
		}
		else if (op == 0xc4)
		{
			appendString(&si, "pinsrw ");
			if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
				appendVdq(&si);
			else
				appendPq(&si);
			*si.buffer++ = ',';
			if (si.instruction->modrm.fields.mod == 0b11)
				appendString(&si, reg32[si.instruction->modrm.fields.rm]);
			else
				appendModRmUpper(&si, "word");
			*si.buffer++ = ',';
			appendNumber(&si, instruction->immediate);
		}
		else if (op == 0xc5)
		{
			appendString(&si, "pextrw ");
			appendString(&si, reg32[si.instruction->modrm.fields.reg]);
			*si.buffer++ = ',';
			if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
				appendUdq(&si);
			else
				appendNq(&si);
			*si.buffer++ = ',';
			appendNumber(&si, instruction->immediate);
		}
		else if (op == 0xc6)
		{
			appendString(&si, "shufp");
			*si.buffer++ = instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? 'd' : 's';
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			appendW(&si);
			*si.buffer++ = ',';
			appendNumber(&si, instruction->immediate);
		}
		else if (op == 0xC7)
		{
			if (instruction->modrm.fields.reg == 0b001)
			{
				appendString(&si, "cmpxchg8b ");
				appendModRmUpper(&si, "qword");
			}
			else if (instruction->modrm.fields.reg <= 0b101)
			{
				const char* mnemonics[] = { "xrstors", "xsavec", "xsaves" };
				appendString(&si, mnemonics[instruction->modrm.fields.reg - 3]);
				*si.buffer++ = ' ';
				appendEb(&si);
			}
			else if (instruction->modrm.fields.reg == 0b110)
			{
				if (instruction->modrm.fields.mod == 0b11)
				{
					appendString(&si, "rdrand ");
					appendRv(&si);
				}
				else
				{
					appendString(&si, instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "vmclear" : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? "vmxon" : "vmptrld"));
					*si.buffer++ = ' ';
					appendModRmUpper(&si, "qword");
				}
			}
			else /* reg == 0b111 */
			{
				if (instruction->modrm.fields.mod == 0b11)
				{
					appendString(&si, "rdseed ");
					appendRv(&si);
				}
				else
				{
					appendString(&si, "vmptrst ");
					appendModRmUpper(&si, "qword");
				}
			}
		}
		else if (op >= 0xc8 && op <= 0xcf)
		{
			appendString(&si, "bswap ");
			appendString(&si, (operandSize ? reg16 : reg32)[op % 8]);
		}
		else if (op == 0xd0)
		{
			appendString(&si, "addsubp");
			*si.buffer++ = instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? 'd' : 's';
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			appendW(&si);
		}
		else if (op == 0xd6)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "movq" : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? "movq2dq" : "movdq2q"));
			*si.buffer++ = ' ';
			if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT)
			{
				appendVdq(&si);
				*si.buffer++ = ',';
				appendNq(&si);
			}
			else if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
			{
				appendPq(&si);
				*si.buffer++ = ',';
				appendUdq(&si);
			}
			else
			{
				if (si.instruction->modrm.fields.mod == 0b11)
					appendUdq(&si);
				else
					appendModRmUpper(&si, "qword");
				*si.buffer++ = ',';
				appendVdq(&si);
			}
		}
		else if (op == 0xd7)
		{
			appendString(&si, "pmovmskb ");
			appendString(&si, reg32[instruction->modrm.fields.reg]);
			*si.buffer++ = ',';
			if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
				appendUdq(&si);
			else
				appendNq(&si);
		}
		else if (op == 0xe6)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "cvttpd2dq" : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? "cvtdq2pd" : "cvtpd2dq"));
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			if (si.instruction->modrm.fields.mod == 0b11)
				appendUdq(&si);
			else
				appendModRmUpper(&si, instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? "qword" : "xmmword");
		}
		else if (op == 0xe7)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "movntdq" : "movntq");
			*si.buffer++ = ' ';
			appendModRmUpper(&si, instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "xmmword" : "qword");
			*si.buffer++ = ',';
			if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
				appendVdq(&si);
			else
				appendPq(&si);
		}
		else if (op == 0xf0)
		{
			appendString(&si, "lddqu ");
			appendVdq(&si);
			*si.buffer++ = ',';
			appendModRmUpper(&si, "xmmword");
		}
		else if (op == 0xf7)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "maskmovdqu " : "maskmovq ");
			if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
			{
				appendVdq(&si);
				*si.buffer++ = ',';
				appendUdq(&si);
			}
			else
			{
				appendPq(&si);
				*si.buffer++ = ',';
				appendNq(&si);
			}
		}
		else if (op >= 0xd1 && op <= 0xfe)
		{
			const char* mnemonics[] = { "srlw", "srld", "srlq", "addq", "mullw", 0, 0, "subusb", "subusw", "minub", "and", "addusb", "addusw", "maxub", "andn", "avgb", "sraw", "srad", "avgw", "mulhuw", "mulhw", 0, 0, "subsb", "subsw", "minsw", "or", "addsb", "addsw", "maxsw", "xor", 0, "sllw", "slld", "sllq", "muludq", "maddwd", "sadbw", 0, "subb", "subw", "subd", "subq", "addb", "addw", "addd" };
			*si.buffer++ = 'p';
			appendString(&si, mnemonics[op - 0xd1]);
			*si.buffer++ = ' ';
			if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
			{
				appendVdq(&si);
				*si.buffer++ = ',';
				appendW(&si);
			}
			else
			{
				appendPq(&si);
				*si.buffer++ = ',';
				appendQq(&si);
			}
		}
		else if (op == 0xb9 || op == 0xff)
		{
			appendString(&si, op == 0xb9 ? "ud1 " : "ud0 ");
			appendString(&si, reg32[instruction->modrm.fields.reg]);
			*si.buffer++ = ',';
			if (instruction->modrm.fields.mod == 0b11)
				appendString(&si, (instruction->mode == NMD_X86_MODE_64 ? reg64 : reg32)[instruction->modrm.fields.rm]);
			else
				appendModRmUpper(&si, "dword");
		}
		else
		{
			const char* str = 0;
			switch (op)
			{
			case 0x31: str = "rdtsc"; break;
			case 0x07: str = "sysret"; break;
			case 0x06: str = "clts"; break;
			case 0x08: str = "invd"; break;
			case 0x09: str = "wbinvd"; break;
			case 0x0b: str = "ud2"; break;
			case 0x0e: str = "femms"; break;
			case 0x30: str = "wrmsr"; break;
			case 0x32: str = "rdmsr"; break;
			case 0x33: str = "rdpmc"; break;
			case 0x34: str = "sysenter"; break;
			case 0x35: str = "sysexit"; break;
			case 0x37: str = "getsec"; break;
			case 0x77: str = "emms"; break;
			case 0xa0: str = "push fs"; break;
			case 0xa1: str = "pop fs"; break;
			case 0xa8: str = "push gs"; break;
			case 0xa9: str = "pop gs"; break;
			case 0xaa: str = "rsm"; break;
			default: return;
			}
			appendString(&si, str);
		}
	}
	else if (instruction->opcodeMap == NMD_X86_OPCODE_MAP_0F38)
	{
		if ((NMD_R(op) == 2 || NMD_R(op) == 3) && NMD_C(op) <= 5)
		{
			const char* mnemonics[] = { "pmovsxbw", "pmovsxbd", "pmovsxbq", "pmovsxwd", "pmovsxwq", "pmovsxdq" };
			appendString(&si, mnemonics[NMD_C(op)]);
			if (NMD_R(op) == 3)
				*(si.buffer - 4) = 'z';
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			if (instruction->modrm.fields.mod == 0b11)
				appendUdq(&si);
			else
				appendModRmUpper(&si, NMD_C(op) == 5 ? "qword" : (NMD_C(op) % 3 == 0 ? "qword" : (NMD_C(op) % 3 == 1 ? "dword" : "word")));
		}
		else if (op >= 0x80 && op <= 0x83)
		{
			appendString(&si, op == 0x80 ? "invept" : (op == 0x81 ? "invvpid" : "invpcid"));
			*si.buffer++ = ' ';
			appendGy(&si);
			*si.buffer++ = ',';
			appendModRmUpper(&si, "xmmword");
		}
		else if (op >= 0xc8 && op <= 0xcd)
		{
			const char* mnemonics[] = { "sha1nexte", "sha1msg1", "sha1msg2", "sha256rnds2", "sha256msg1", "sha256msg2" };
			appendString(&si, mnemonics[op - 0xc8]);
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			appendW(&si);
		}
		else if (op == 0xcf)
		{
			appendString(&si, "gf2p8mulb ");
			appendVdq(&si);
			*si.buffer++ = ',';
			appendW(&si);
		}
		else if (op == 0xf0 || op == 0xf1)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO ? "crc32" : "movbe");
			*si.buffer++ = ' ';
			if (op == 0xf0)
			{
				if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
				{
					appendString(&si, reg32[instruction->modrm.fields.reg]);
					*si.buffer++ = ',';
					appendEb(&si);
				}
				else
				{
					appendGv(&si);
					*si.buffer++ = ',';
					appendEv(&si);
				}
			}
			else
			{
				if (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT_NOT_ZERO)
				{
					appendString(&si, reg32[instruction->modrm.fields.reg]);
					*si.buffer++ = ',';
					if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
						appendEw(&si);
					else
						appendEy(&si);
				}
				else
				{
					appendEv(&si);
					*si.buffer++ = ',';
					appendGv(&si);
				}
			}
		}
		else if (op == 0xf6)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "adcx" : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? "adox" : (instruction->operandSize64 ? "wrssq" : "wrssd")));
			*si.buffer++ = ' ';
			if (!instruction->simdPrefix)
			{
				appendEy(&si);
				*si.buffer++ = ',';
				appendGy(&si);
			}
			else
			{
				appendGy(&si);
				*si.buffer++ = ',';
				appendEy(&si);
			}
		}
		else if (op == 0xf5)
		{
			appendString(&si, instruction->operandSize64 ? "wrussq " : "wrussd ");
			appendModRmUpper(&si, instruction->operandSize64 ? "qword" : "dword");
			*si.buffer++ = ',';
			appendString(&si, (instruction->operandSize64 ? reg64 : reg32)[instruction->modrm.fields.reg]);
		}
		else if (op == 0xf8)
		{
			appendString(&si, instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE ? "movdir64b" : (instruction->simdPrefix == NMD_X86_PREFIXES_REPEAT ? "enqcmd" : "enqcmds"));
			*si.buffer++ = ' ';
			appendString(&si, (instruction->mode == NMD_X86_MODE_64 ? reg64 : (instruction->mode == NMD_X86_MODE_16 ? reg16 : reg32))[instruction->modrm.fields.rm]);
			*si.buffer++ = ',';
			appendModRmUpper(&si, "zmmword");
		}
		else if (op == 0xf9)
		{
			appendString(&si, "movdiri ");
			appendModRmUpperWithoutAddressSpecifier(&si);
			*si.buffer++ = ',';
			appendString(&si, reg32[instruction->modrm.fields.rm]);
		}
		else
		{
			if (op == 0x40)
				appendString(&si, "pmulld");
			else if (op == 0x41)
				appendString(&si, "phminposuw");
			else if (op >= 0xdb && op <= 0xdf)
			{
				const char* mnemonics[] = { "aesimc", "aesenc", "aesenclast", "aesdec", "aesdeclast" };
				appendString(&si, mnemonics[op - 0xdb]);
			}
			else if (op == 0x37)
				appendString(&si, "pcmpgtq");
			else if (NMD_R(op) == 2)
			{
				const char* mnemonics[] = { "pmuldq", "pcmpeqq", "movntdqa", "packusdw" };
				appendString(&si, mnemonics[NMD_C(op) - 8]);
			}
			else if (NMD_R(op) == 3)
			{
				const char* mnemonics[] = { "pminsb", "pminsd", "pminuw", "pminud", "pmaxsb", "pmaxsd", "pmaxuw", "pmaxud" };
				appendString(&si, mnemonics[NMD_C(op) - 8]);
			}
			else if (op < 0x10)
			{
				const char* mnemonics[] = { "pshufb", "phaddw", "phaddd", "phaddsw", "pmaddubsw", "phsubw", "phsubd", "phsubsw", "psignb", "psignw", "psignd", "pmulhrsw", "permilpsv", "permilpdv", "testpsv", "testpdv" };
				appendString(&si, mnemonics[op]);
			}
			else if (op < 0x18)
				appendString(&si, op == 0x10 ? "pblendvb" : (op == 0x14 ? "blendvps" : (op == 0x15 ? "blendvpd" : "ptest")));
			else
			{
				appendString(&si, "pabs");
				*si.buffer++ = op == 0x1c ? 'b' : (op == 0x1d ? 'w' : 'd');
			}
			*si.buffer++ = ' ';
			if (instruction->simdPrefix == NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE)
			{
				appendVdq(&si);
				*si.buffer++ = ',';
				appendW(&si);
			}
			else
			{
				appendPq(&si);
				*si.buffer++ = ',';
				appendQq(&si);
			}
		}

	}
	else if (instruction->opcodeMap == NMD_X86_OPCODE_MAP_0F3A)
	{
		if (NMD_R(op) == 1)
		{
			const char* mnemonics[] = { "pextrb", "pextrw", "pextrd", "extractps" };
			appendString(&si, mnemonics[op - 0x14]);
			*si.buffer++ = ' ';
			if (instruction->modrm.fields.mod == 0b11)
				appendString(&si, (si.instruction->operandSize64 ? reg64 : reg32)[instruction->modrm.fields.rm]);
			else
			{
				if (op == 0x14)
					appendModRmUpper(&si, "byte");
				else if (op == 0x15)
					appendModRmUpper(&si, "word");
				else if (op == 0x16)
					appendEy(&si);
				else
					appendModRmUpper(&si, "dword");
			}
			*si.buffer++ = ',';
			appendVdq(&si);
		}
		else if (NMD_R(op) == 2)
		{
			appendString(&si, op == 0x20 ? "pinsrb" : (op == 0x21 ? "insertps" : "pinsrd"));
			*si.buffer++ = ' ';
			appendVdq(&si);
			*si.buffer++ = ',';
			if (op == 0x20)
			{
				if (instruction->modrm.fields.mod == 0b11)
					appendString(&si, reg32[instruction->modrm.fields.rm]);
				else
					appendModRmUpper(&si, "byte");
			}
			else if (op == 0x21)
			{
				if (instruction->modrm.fields.mod == 0b11)
					appendUdq(&si);
				else
					appendModRmUpper(&si, "dword");
			}
			else
				appendEy(&si);
		}
		else
		{
			if (op < 0x10)
			{
				const char* mnemonics[] = { "roundps", "roundpd", "roundss", "roundsd", "blendps", "blendpd", "pblendw", "palignr" };
				appendString(&si, mnemonics[op - 8]);
			}
			else if (NMD_R(op) == 4)
			{
				const char* mnemonics[] = { "dpps", "dppd", "mpsadbw", 0, "pclmulqdq" };
				appendString(&si, mnemonics[NMD_C(op)]);
			}
			else if (NMD_R(op) == 6)
			{
				const char* mnemonics[] = { "pcmpestrm", "pcmpestri", "pcmpistrm", "pcmpistri" };
				appendString(&si, mnemonics[NMD_C(op)]);
			}
			else if (op > 0x80)
				appendString(&si, op == 0xcc ? "sha1rnds4" : (op == 0xce ? "gf2p8affineqb" : (op == 0xcf ? "gf2p8affineinvqb" : "aeskeygenassist")));
			*si.buffer++ = ' ';
			if (op == 0xf && !(instruction->prefixes & (NMD_X86_PREFIXES_OPERAND_SIZE_OVERRIDE | NMD_X86_PREFIXES_REPEAT | NMD_X86_PREFIXES_REPEAT_NOT_ZERO)))
			{
				appendPq(&si);
				*si.buffer++ = ',';
				appendQq(&si);
			}
			else
			{
				appendVdq(&si);
				*si.buffer++ = ',';
				if (instruction->modrm.fields.mod == 0b11)
					appendString(&si, "xmm"), *si.buffer++ = (char)('0' + instruction->modrm.fields.rm);
				else
					appendModRmUpper(&si, op == 0xa ? "dword" : (op == 0xb ? "qword" : "xmmword"));
			}
		}
		*si.buffer++ = ',';
		appendNumber(&si, instruction->immediate);
	}

#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_ATT_SYNTAX
	if (formatFlags & NMD_X86_FORMAT_FLAGS_ATT_SYNTAX)
	{
		*si.buffer = '\0';
		char* operand = (char*)nmd_reverse_strchr(buffer, ' ');
		if (operand && *(operand - 1) != ' ') /* If the instruction has a ' '(space character) and the left character of 'operand' is not ' '(space) the instruction has operands. */
		{
			/* If there is a memory operand. */
			const char* memoryOperand = nmd_strchr(buffer, '[');
			if (memoryOperand)
			{
				/* If the memory operand has pointer size. */
				char* tmp2 = (char*)memoryOperand - (*(memoryOperand - 1) == ':' ? 7 : 4);
				if (nmd_strstr(tmp2, "ptr") == tmp2)
				{
					/* Find the ' '(space) that is after two ' '(spaces). */
					tmp2 -= 2;
					while (*tmp2 != ' ')
						tmp2--;
					operand = tmp2;
				}
			}

			const char* const firstOperandConst = operand;
			char* firstOperand = operand + 1;
			char* secondOperand = 0;
			/* Convert each operand to AT&T syntax. */
			do
			{
				operand++;
				operand = formatOperandToAtt(operand, &si);
				if (*operand == ',')
					secondOperand = operand;
			} while (*operand);

			/* Swap operands. */
			if (secondOperand) /* At least two operands. */
			{
				/* Copy first operand to 'tmpBuffer'. */
				char tmpBuffer[64];
				char* i = tmpBuffer;
				char* j = firstOperand;
				for (; j < secondOperand; i++, j++)
					*i = *j;

				*i = '\0';

				/* Copy second operand to first operand. */
				for (i = secondOperand + 1; *i; firstOperand++, i++)
					*firstOperand = *i;

				*firstOperand++ = ',';

				/* 'firstOperand' is now the second operand. */
				/* Copy 'tmpBuffer' to second operand. */
				for (i = tmpBuffer; *firstOperand; i++, firstOperand++)
					*firstOperand = *i;
			}

			/* Memory operands change the mnemonic string(e.g. 'mov eax, dword ptr [ebx]' -> 'movl (%ebx), %eax'). */
			if (memoryOperand && !nmd_strstr(firstOperandConst - 4, "lea"))
			{
				const char* r_char = nmd_strchr(firstOperandConst, 'r');
				const char* e_char = nmd_strchr(firstOperandConst, 'e');
				const char* call_str = nmd_strstr(firstOperandConst - 5, "call");
				const char* jmp_str = nmd_strstr(firstOperandConst - 4, "jmp");
				nmd_insert_char(firstOperandConst, (instruction->mode == NMD_X86_MODE_64 && ((r_char && *(r_char - 1) == '%') || call_str || jmp_str)) ? 'q' : (instruction->mode == NMD_X86_MODE_32 && ((e_char && *(e_char - 1) == '%') || call_str || jmp_str) ? 'l' : 'b'));
				si.buffer++;
			}
		}
	}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_ATT_SYNTAX */

	size_t stringLength = si.buffer - buffer;
#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_UPPERCASE
	if (formatFlags & NMD_X86_FORMAT_FLAGS_UPPERCASE)
	{
		size_t i = 0;
		for (; i < stringLength; i++)
		{
			if (NMD_IS_LOWERCASE(buffer[i]))
				buffer[i] -= 0x20; /* Capitalize letter. */
		}
	}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_UPPERCASE */

#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_COMMA_SPACES
	if (formatFlags & NMD_X86_FORMAT_FLAGS_COMMA_SPACES)
	{
		size_t i = 0;
		for (; i < stringLength; i++)
		{
			if (buffer[i] == ',')
			{
				/* Move all characters after the comma one position to the right. */
				size_t j = stringLength;
				for (; j > i; j--)
					buffer[j] = buffer[j - 1];

				buffer[i + 1] = ' ';
				si.buffer++, stringLength++;
			}
		}
	}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_COMMA_SPACES */

#ifndef NMD_ASSEMBLY_DISABLE_FORMATTER_OPERATOR_SPACES
	if (formatFlags & NMD_X86_FORMAT_FLAGS_OPERATOR_SPACES)
	{
		size_t i = 0;
		for (; i < stringLength; i++)
		{
			if (buffer[i] == '+' || (buffer[i] == '-' && buffer[i - 1] != ' ' && buffer[i - 1] != '('))
			{
				/* Move all characters after the operator two positions to the right. */
				size_t j = stringLength + 1;
				for (; j > i; j--)
					buffer[j] = buffer[j - 2];

				buffer[i + 1] = buffer[i];
				buffer[i] = ' ';
				buffer[i + 2] = ' ';
				si.buffer += 2, stringLength += 2;
				i++;
			}
		}
	}
#endif /* NMD_ASSEMBLY_DISABLE_FORMATTER_OPERATOR_SPACES */

	*si.buffer = '\0';
}

bool check_jump_condition(NMD_X86Cpu* const cpu, uint8_t opcodeCondition)
{
	switch (opcodeCondition)
	{
	case 0x0: return cpu->flags.fields.OF == 1;                                                           /* Jump if overflow (OF=1) */
	case 0x1: return cpu->flags.fields.OF == 0;                                                           /* Jump if not overflow (OF=0) */
	case 0x2: return cpu->flags.fields.CF == 1;                                                           /* Jump if not above or equal (CF=1) */
	case 0x3: return cpu->flags.fields.CF == 0;                                                           /* Jump if not below (CF=0) */
	case 0x4: return cpu->flags.fields.ZF == 1;                                                           /* Jump if equal (ZF=1) */
	case 0x5: return cpu->flags.fields.ZF == 0;                                                           /* Jump if not equal (ZF=0) */
	case 0x6: return cpu->flags.fields.CF == 1 || cpu->flags.fields.ZF == 1;                              /* Jump if not above (CF=1 or ZF=1) */
	case 0x7: return cpu->flags.fields.CF == 0 && cpu->flags.fields.ZF == 0;                              /* Jump if not below or equal (CF=0 and ZF=0) */
	case 0x8: return cpu->flags.fields.SF == 1;                                                           /* Jump if sign (SF=1) */
	case 0x9: return cpu->flags.fields.SF == 0;                                                           /* Jump if not sign (SF=0) */
	case 0xa: return cpu->flags.fields.PF == 1;                                                           /* Jump if parity/parity even (PF=1) */
	case 0xb: return cpu->flags.fields.PF == 0;                                                           /* Jump if parity odd (PF=0) */
	case 0xc: return cpu->flags.fields.SF != cpu->flags.fields.OF;                                        /* Jump if not greater or equal (SF != OF) */
	case 0xd: return cpu->flags.fields.SF == cpu->flags.fields.OF;                                        /* Jump if not less (SF=OF) */
	case 0xe: return cpu->flags.fields.ZF == 1 || cpu->flags.fields.SF != cpu->flags.fields.OF;           /* Jump if not greater (ZF=1 or SF != OF) */
	case 0xf: return cpu->flags.fields.ZF == 0 && cpu->flags.fields.SF == cpu->flags.fields.OF;           /* Jump if not less or equal (ZF=0 and SF=OF) */
	default: return false;
	}
}

/*
Checks if the number of set bits in an 8-bit number is even.
Credits: https://stackoverflow.com/questions/21617970/how-to-check-if-value-has-even-parity-of-bits-or-odd
*/
bool isParityEven8(uint8_t x)
{
	x ^= x >> 4;
	x ^= x >> 2;
	x ^= x >> 1;
	return !(x & 1);
}

void copyByMode(void* dst, void* src, NMD_X86_MODE mode)
{
	if (mode == NMD_X86_MODE_32)
		*(int32_t*)(dst) = *(int32_t*)(src);
	else if (mode == NMD_X86_MODE_64)
		*(int64_t*)(dst) = *(int64_t*)(src);
	else /* (mode == NMD_X86_MODE_16) */
		*(int16_t*)(dst) = *(int16_t*)(src);
}

#define NMD_GET_GREG(index) (&cpu->rax + (index)) /* general register */
#define NMD_GET_RREG(index) (&cpu->r8 + (index)) /* r8,r9...r15 */
#define NMD_GET_PHYSICAL_ADDRESS(address) (uint8_t*)((uint64_t)(cpu->physicalMemory)+((address)-cpu->virtualAddress))
#define NMD_IN_BOUNDARIES(address) (address >= cpu->physicalMemory && address < endPhysicalMemory)
/* #define NMD_TEST(value, bit) ((value&(1<<bit))==(1<<bit)) */

/*
Emulates x86 code according to the cpu's state. You MUST initialize the following variables before calling this
function: 'cpu->mode', 'cpu->physicalMemory', 'cpu->physicalMemorySize', 'cpu->virtualAddress' and 'cpu->rip'.
You may optionally initialize 'cpu->rsp' if a stack is desirable. Below is a short description of each variable:
- 'cpu->mode': The emulator's operating architecture mode. 'NMD_X86_MODE_32', 'NMD_X86_MODE_64' or 'NMD_X86_MODE_16'.
- 'cpu->physicalMemory': A pointer to a buffer used as the emulator's memory.
- 'cpu->physicalMemorySize': The size of the buffer pointer by 'physicalMemory' in bytes.
- 'cpu->virtualAddress': The starting address of the emulator's virtual address space.
- 'cpu->rip': The virtual address where emulation starts.
- 'cpu->rsp': The virtual address of the bottom of the stack.
Parameters:
- cpu      [in] A pointer to a variable of type 'NMD_X86Cpu' that holds the state of the cpu.
- maxCount [in] The maximum number of instructions that can be executed, or zero for unlimited instructions.
*/
bool nmd_x86_emulate(NMD_X86Cpu* cpu, size_t maxCount)
{
	const uint64_t endVirtualAddress = cpu->virtualAddress + cpu->physicalMemorySize;
	const void* endPhysicalMemory = (uint8_t*)cpu->physicalMemory + cpu->physicalMemorySize;
	size_t count = 0;

	cpu->running = true;

	while (cpu->running)
	{
		NMD_X86Instruction instruction;
		const void* buffer = NMD_GET_PHYSICAL_ADDRESS(cpu->rip);
		const bool validBuffer = NMD_IN_BOUNDARIES(buffer);
		if (!validBuffer || !nmd_x86_decode_buffer(buffer, (size_t)(endVirtualAddress - cpu->rip), &instruction, (NMD_X86_MODE)cpu->mode, NMD_X86_DECODER_FLAGS_MINIMAL))
		{
			if (cpu->callback)
				cpu->callback(cpu, &instruction, validBuffer ? NMD_X86_EMULATOR_EXCEPTION_BAD_INSTRUCTION : NMD_X86_EMULATOR_EXCEPTION_BAD_MEMORY);
			return false;
		}

		if (instruction.opcodeMap == NMD_X86_OPCODE_MAP_DEFAULT)
		{
			if (instruction.opcode >= 0x88 && instruction.opcode <= 0x8b) /* mov [88,8b] */
			{
				NMD_X86Register* r0 = NMD_GET_GREG(instruction.modrm.fields.reg);
				void* addr;
				if (instruction.modrm.fields.mod == 0b11)
					addr = NMD_GET_GREG(instruction.modrm.fields.rm);
				else
				{
					int64_t x = NMD_GET_GREG(instruction.modrm.fields.rm)->l64 + instruction.displacement;
					addr = NMD_GET_PHYSICAL_ADDRESS(x);
					if (!NMD_IN_BOUNDARIES(addr))
					{
						if (cpu->callback)
							cpu->callback(cpu, &instruction, NMD_X86_EMULATOR_EXCEPTION_BAD_MEMORY);
						return false;
					}
				}

				if (instruction.opcode == 0x88)
					*(int8_t*)(addr) = r0->l8;
				else if (instruction.opcode == 0x89)
				{
					copyByMode(addr, r0, (NMD_X86_MODE)cpu->mode);
				}
				else if (instruction.opcode == 0x8a)
					r0->l8 = *(int8_t*)(addr);
				else /* if (instruction.opcode == 0x8b) */
				{
					copyByMode(r0, addr, (NMD_X86_MODE)cpu->mode);
				}
			}
			else if (NMD_R(instruction.opcode) == 5) /* push,pop [50,5f] */
			{
				NMD_X86Register* r0 = NMD_GET_GREG(instruction.opcode % 8);
				void* dst, *src;

				if (instruction.opcode < 0x58) /* push */
				{
					cpu->rsp.l64 -= (int8_t)cpu->mode;
					dst = NMD_GET_PHYSICAL_ADDRESS(cpu->rsp.l64);
					src = r0;
				}
				else /* pop */
				{
					src = NMD_GET_PHYSICAL_ADDRESS(cpu->rsp.l64);
					cpu->rsp.l64 += (int8_t)cpu->mode;
					dst = r0;
				}

				copyByMode(dst, src, (NMD_X86_MODE)cpu->mode);
			}
			else if (instruction.opcode == 0xe8) /* call */
			{
				/* push the instruction pointer onto the stack. */
				cpu->rsp.l64 -= (int8_t)cpu->mode;
				copyByMode(NMD_GET_PHYSICAL_ADDRESS(cpu->rsp.l64), &cpu->rip, (NMD_X86_MODE)cpu->mode);

				/* jump */
				cpu->rip += (int32_t)instruction.immediate;
			}
			else if (instruction.opcode == 0xc3) /* ret */
			{
				/* pop rip */
				copyByMode(&cpu->rip, NMD_GET_PHYSICAL_ADDRESS(cpu->rsp.l64), (NMD_X86_MODE)cpu->mode);
				cpu->rsp.l64 += (int8_t)cpu->mode;
			}
			else if (instruction.opcode == 0xc2) /* ret imm8 */
			{
				/* pop rip */
				copyByMode(&cpu->rip, NMD_GET_PHYSICAL_ADDRESS(cpu->rsp.l64), (NMD_X86_MODE)cpu->mode);
				cpu->rsp.l64 += (int8_t)(cpu->mode + instruction.immediate);
			}
			else if (instruction.opcode == 0x8d) /* lea */
			{
				NMD_X86Register* r0 = NMD_GET_GREG(instruction.modrm.fields.reg);
				r0->l64 = 0;
				/* compute... */
			}
			else if (instruction.opcode == 0xe9) /* jmp r32 */
				cpu->rip += (int32_t)instruction.immediate;
			else if (instruction.opcode == 0xeb) /* jmp r8 */
				cpu->rip += (int8_t)instruction.immediate;
			else if (instruction.opcode == 0x03) /* add reg, mem/reg */
				NMD_GET_GREG(instruction.modrm.fields.reg)->l32 += NMD_GET_GREG(instruction.modrm.fields.rm)->l32;
			else if (instruction.opcode == 0x04) /* add al, imm8 */
				cpu->rax.l8 += (int8_t)instruction.immediate;
			else if (instruction.opcode == 0x05) /* add ax/eax/rax, imm16/imm32/imm32*/
				cpu->rax.l32 += (int32_t)instruction.immediate;
			else if (NMD_R(instruction.opcode) == 4) /* inc/dec [40,4f] */
			{
				NMD_X86Register* r0 = NMD_GET_GREG(instruction.opcode % 8);
				instruction.opcode < 0x48 ? r0->l64++ : r0->l64--;
			}
			else if (NMD_R(instruction.opcode) == 7 && check_jump_condition(cpu, NMD_C(instruction.opcode))) /* conditional jump r8 */
				cpu->rip += (int8_t)(instruction.immediate);
			else if (instruction.opcode >= 0x91 && instruction.opcode <= 0x97) /* xchg rax, ... */
			{
				const NMD_X86Register tmp = cpu->rax;
				NMD_X86Register* r0 = NMD_GET_GREG(instruction.opcode - 0x91);
				cpu->rax = *r0;
				*r0 = tmp;
			}
			else if (instruction.opcode == 0x60) /* pusha,pushad */
			{
				void* stack = NMD_GET_PHYSICAL_ADDRESS(cpu->rsp.l32);
				cpu->rsp.l32 -= cpu->mode * 8;
				if (instruction.mode == NMD_X86_MODE_32) /* pushad */
				{
					((uint32_t*)(stack))[0] = cpu->rax.l32;
					((uint32_t*)(stack))[1] = cpu->rcx.l32;
					((uint32_t*)(stack))[2] = cpu->rdx.l32;
					((uint32_t*)(stack))[3] = cpu->rbx.l32;
					((uint32_t*)(stack))[4] = cpu->rsp.l32;
					((uint32_t*)(stack))[5] = cpu->rbp.l32;
					((uint32_t*)(stack))[6] = cpu->rsi.l32;
					((uint32_t*)(stack))[7] = cpu->rdi.l32;
				}
				else /* if (instruction.mode == NMD_X86_MODE_16) pusha */
				{
					((uint16_t*)(stack))[0] = cpu->rax.l16;
					((uint16_t*)(stack))[1] = cpu->rcx.l16;
					((uint16_t*)(stack))[2] = cpu->rdx.l16;
					((uint16_t*)(stack))[3] = cpu->rbx.l16;
					((uint16_t*)(stack))[4] = cpu->rsp.l16;
					((uint16_t*)(stack))[5] = cpu->rbp.l16;
					((uint16_t*)(stack))[6] = cpu->rsi.l16;
					((uint16_t*)(stack))[7] = cpu->rdi.l16;
				}
			}
			else if (instruction.opcode == 0x61) /* popa,popad */
			{
				void* stack = NMD_GET_PHYSICAL_ADDRESS(cpu->rsp.l32);
				if (instruction.mode == NMD_X86_MODE_32) /* popad */
				{
					cpu->rax.l32 = ((uint32_t*)(stack))[0];
					cpu->rcx.l32 = ((uint32_t*)(stack))[1];
					cpu->rdx.l32 = ((uint32_t*)(stack))[2];
					cpu->rbx.l32 = ((uint32_t*)(stack))[3];
					cpu->rsp.l32 = ((uint32_t*)(stack))[4];
					cpu->rbp.l32 = ((uint32_t*)(stack))[5];
					cpu->rsi.l32 = ((uint32_t*)(stack))[6];
					cpu->rdi.l32 = ((uint32_t*)(stack))[7];
				}
				else /* if (instruction.mode == NMD_X86_MODE_16) popa */
				{
					cpu->rax.l16 = ((uint16_t*)(stack))[0];
					cpu->rcx.l16 = ((uint16_t*)(stack))[1];
					cpu->rdx.l16 = ((uint16_t*)(stack))[2];
					cpu->rbx.l16 = ((uint16_t*)(stack))[3];
					cpu->rsp.l16 = ((uint16_t*)(stack))[4];
					cpu->rbp.l16 = ((uint16_t*)(stack))[5];
					cpu->rsi.l16 = ((uint16_t*)(stack))[6];
					cpu->rdi.l16 = ((uint16_t*)(stack))[7];
				}
				cpu->rsp.l32 += cpu->mode * 8;
			}
			else if (NMD_R(instruction.opcode) == 0xb) /* mov reg, imm */
			{
				const uint8_t width = (instruction.prefixes & NMD_X86_PREFIXES_REX_W && instruction.opcode >= 0xb8) ? 8 : instruction.mode;
				NMD_X86Register* r0 = instruction.prefixes & NMD_X86_PREFIXES_REX_B ? NMD_GET_RREG(NMD_C(instruction.opcode)) : NMD_GET_GREG(NMD_C(instruction.opcode));
				copyByMode(r0, &instruction.immediate, (NMD_X86_MODE)width);
			}
			else if (instruction.opcode == 0x90)
			{
				if (instruction.simdPrefix == NMD_X86_PREFIXES_REPEAT) /* pause */
				{
					/* spin-wait loop ahead? */
				}
			}
			else if (instruction.opcode == 0x9e) /* sahf */
				cpu->flags.l8 = cpu->rax.l8;
			else if (instruction.opcode == 0x9f) /* lahf */
				cpu->rax.l8 = cpu->flags.l8;
			else if (instruction.opcode == 0xcc) /* int3 */
			{
				if (cpu->callback)
					cpu->callback(cpu, &instruction, NMD_X86_EMULATOR_EXCEPTION_BREAKPOINT);
			}
			else if (instruction.opcode == 0xf1) /* int1 */
			{
				if (cpu->callback)
					cpu->callback(cpu, &instruction, NMD_X86_EMULATOR_EXCEPTION_DEBUG);
			}
			else if (instruction.opcode == 0xce) /* into */
			{
				if (cpu->callback)
					cpu->callback(cpu, &instruction, NMD_X86_EMULATOR_EXCEPTION_OVERFLOW);
			}
			else if (instruction.opcode == 0xcd) /* int n */
			{
				if (cpu->callback)
					cpu->callback(cpu, &instruction, NMD_X86_EMULATOR_EXCEPTION_GENERAL_PROTECTION);
			}
			else if (instruction.opcode == 0xf4) /* hlt */
				cpu->running = false;
			else if (instruction.opcode == 0xf5) /* cmc */
				cpu->flags.fields.CF = ~cpu->flags.fields.CF;
			else if (instruction.opcode == 0xf8) /* clc */
				cpu->flags.fields.CF = 0;
			else if (instruction.opcode == 0xf9) /* stc */
				cpu->flags.fields.CF = 1;
			else if (instruction.opcode == 0xfa) /* cli */
				cpu->flags.fields.IF = 0;
			else if (instruction.opcode == 0xfb) /* sti */
				cpu->flags.fields.IF = 1;
			else if (instruction.opcode == 0xfc) /* cld */
				cpu->flags.fields.DF = 0;
			else if (instruction.opcode == 0xfd) /* std */
				cpu->flags.fields.DF = 1;
		}
		else if (instruction.opcodeMap == NMD_X86_OPCODE_MAP_0F)
		{
			if (NMD_R(instruction.opcode) == 8 && check_jump_condition(cpu, NMD_C(instruction.opcode))) /* conditional jump r32 */
				cpu->rip += (int32_t)(instruction.immediate);
		}
		else if (instruction.opcodeMap == NMD_X86_OPCODE_MAP_0F38)
		{

		}
		else /* if (instruction.opcodeMap == NMD_X86_OPCODE_MAP_0F_38) */
		{

		}

		/*
		if (r0)
		{
		cpu->flags.fields.ZF = (r0->l64 == 0);
		cpu->flags.fields.PF = isParityEven8(r0->l8);

		}
		*//* OF,SF,CF*/


		if (cpu->flags.fields.TF && cpu->callback)
			cpu->callback(cpu, &instruction, NMD_X86_EMULATOR_EXCEPTION_STEP);

		cpu->rip += instruction.length;

		if (maxCount > 0 && ++count >= maxCount)
			return true;
	}

	cpu->running = false;

	return true;
}

#endif /* NMD_ASSEMBLY_IMPLEMENTATION */
