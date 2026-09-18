#ifndef IMP_IO_JPEG_BRIDGE_H
#define IMP_IO_JPEG_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IMP_JPEG_DATA_LIMIT ((size_t)512 * 1024 * 1024)
#define IMP_JPEG_ICC_CHUNK_SIZE 65519U
#define IMP_JPEG_METADATA_LIMIT ((size_t)IMP_JPEG_ICC_CHUNK_SIZE * 255)
#define IMP_JPEG_DIMENSION_LIMIT 65500U
#define IMP_JPEG_MESSAGE_SIZE 256

typedef enum {
    IMP_JPEG_OK,
    IMP_JPEG_INVALID,
    IMP_JPEG_METADATA,
    IMP_JPEG_UNSUPPORTED_PRECISION,
    IMP_JPEG_UNSUPPORTED_LAYOUT,
    IMP_JPEG_UNSUPPORTED_FEATURE,
    IMP_JPEG_RESOURCE,
    IMP_JPEG_ENCODE
} ImpJpegStatus;

typedef struct {
    const unsigned char* icc;
    size_t icc_size;
    const unsigned char* exif;
    size_t exif_size;
} ImpJpegMetadata;

typedef struct {
    uint32_t width;
    uint32_t height;
    int channels;
    int depth;
    const unsigned char* samples;
    size_t size;
    ImpJpegMetadata metadata;
} ImpJpegRaster;

typedef struct {
    unsigned char* bytes;
    size_t size;
} ImpJpegBytes;

typedef struct {
    ImpJpegStatus code;
    char message[IMP_JPEG_MESSAGE_SIZE];
} ImpJpegError;

ImpJpegStatus imp_jpeg_decode(const unsigned char* bytes, size_t size,
                              ImpJpegRaster* output, ImpJpegError* error);
ImpJpegStatus imp_jpeg_encode(const ImpJpegRaster* input, int quality,
                              ImpJpegBytes* output, ImpJpegError* error);
void imp_jpeg_free_raster(ImpJpegRaster* raster);
void imp_jpeg_free_bytes(ImpJpegBytes* bytes);

#ifdef __cplusplus
}
#endif

#endif
