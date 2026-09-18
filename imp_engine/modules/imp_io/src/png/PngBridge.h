#ifndef IMP_IO_PNG_BRIDGE_H
#define IMP_IO_PNG_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Bound encoded files, expanded samples, and individual metadata allocations. */
#define IMP_PNG_DATA_LIMIT ((size_t)512 * 1024 * 1024)
#define IMP_PNG_METADATA_LIMIT ((size_t)16 * 1024 * 1024)
#define IMP_PNG_DIMENSION_LIMIT 100000U
#define IMP_PNG_MESSAGE_SIZE 256
#define IMP_PNG_XY_COUNT 8

typedef enum {
    IMP_PNG_OK,
    IMP_PNG_INVALID,
    IMP_PNG_METADATA,
    IMP_PNG_UNSUPPORTED,
    IMP_PNG_RESOURCE,
    IMP_PNG_ENCODE
} ImpPngStatus;

typedef enum {
    IMP_PNG_GAMMA = 1,
    IMP_PNG_CHROMATICITIES = 2,
    IMP_PNG_SRGB = 4
} ImpPngColorField;

/* This private C boundary owns no C++ objects across libpng's longjmp. */
typedef struct {
    unsigned fields;
    double gamma;
    double xy[IMP_PNG_XY_COUNT];
    int intent;
    const unsigned char* icc;
    size_t icc_size;
    const unsigned char* exif;
    size_t exif_size;
} ImpPngMetadata;

typedef struct {
    uint32_t width;
    uint32_t height;
    int channels;
    int depth;
    const unsigned char* samples;
    size_t size;
    ImpPngMetadata metadata;
} ImpPngRaster;

typedef struct {
    unsigned char* bytes;
    size_t size;
} ImpPngBytes;

typedef struct {
    ImpPngStatus code;
    char message[IMP_PNG_MESSAGE_SIZE];
} ImpPngError;

ImpPngStatus imp_png_decode(const unsigned char* bytes, size_t size,
                            ImpPngRaster* output, ImpPngError* error);
ImpPngStatus imp_png_encode(const ImpPngRaster* input, ImpPngBytes* output,
                            ImpPngError* error);
void imp_png_free_raster(ImpPngRaster* raster);
void imp_png_free_bytes(ImpPngBytes* bytes);

#ifdef __cplusplus
}
#endif

#endif
