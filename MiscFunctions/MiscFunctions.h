/*HEADER******************************************************************************************
BSD 3-Clause License

Copyright (c) 2020, Carlos Neri
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**END********************************************************************************************/
#ifndef MISCFUNCTIONS_H_
#define MISCFUNCTIONS_H_


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////
//! String match status
#define STRING_OK		(1)
//! String mismatch status
#define STRING_ERROR	(0)

#define SET_FLAG(Register,Flag)			(Register |= (1<<Flag))

#define CLEAR_FLAG(Register,Flag)		(Register &= ~(1<<Flag))

#define CHECK_FLAG(Register,Flag)		(Register & (1<<Flag))

#define SIZE_OF_ARRAY(Array)			(sizeof(Array)/sizeof(Array[0]))

#define COUNT_TO_NSEC(Count,ClockReferenceMHz)             (uint64_t)(((uint64_t)(Count)*1000u)/ClockReferenceMHz)
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                Function-like Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Extern Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Extern Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

uint8_t MiscFunction_StringCompare(const uint8_t * StringBase, const uint8_t * StringToCompare, uint16_t AmountOfCharacters);

void MiscFunctions_MemCopy(const void * Source, void * Destination, uint16_t DataSize);

uint8_t MiscFunctions_SearchInString(const uint8_t * Source, uint16_t SourceSize, const uint8_t * StringToSearch, uint16_t StringToSearchSize);

void MiscFunctions_MemClear(void * Source, uint16_t DataSize);

uint16_t MiscFunctions_IntegerToAscii(uint32_t Data, uint8_t *AsciiBuffer);

void MiscFunctions_StringReverse(uint8_t * StringToReverse);

uint32_t MiscFunctions_AsciiToUnsignedInteger(uint8_t * AsciiString);

uint8_t * MiscFunctions_FindTokenInString(uint8_t * StringToSearch, uint8_t Token);

uint8_t MiscFunctions_StringCopyUntilToken(const uint8_t * Source, uint8_t * Destination, uint8_t Token);

uint8_t ReverseBitsInByte(uint8_t DataToReverse);

void MiscFunctions_BlockingDelay(uint32_t TargetDelay);

#if defined(__cplusplus)
}
#endif // __cplusplus


#endif /* MISCFUNCTIONS_H_ */
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////
