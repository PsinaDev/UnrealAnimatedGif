// Copyright PsinaDev. All Rights Reserved.

// The single translation unit that compiles the vendored stb_image. Only the GIF
// path is enabled — the other formats are owned by UE's ImageWrapper, so we strip
// them to avoid duplicate codecs and cut build time. Allocations route through UE.

#include "CoreTypes.h"
#include "HAL/UnrealMemory.h"
#include "Misc/AssertionMacros.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_JPEG
#define STBI_NO_PNG
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_LINEAR
#define STBI_MALLOC(Size)          FMemory::Malloc(Size)
#define STBI_REALLOC(Ptr, NewSize) FMemory::Realloc(Ptr, NewSize)
#define STBI_FREE(Ptr)             FMemory::Free(Ptr)
#define STBI_ASSERT(Expr)          check(Expr)

THIRD_PARTY_INCLUDES_START
#include "stb_image.h"
THIRD_PARTY_INCLUDES_END
