#include "PngBridge.h"

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(PNG_READ_eXIf_SUPPORTED) || !defined(PNG_WRITE_eXIf_SUPPORTED) \
    || !defined(PNG_READ_iCCP_SUPPORTED) || !defined(PNG_WRITE_iCCP_SUPPORTED) \
    || !defined(PNG_READ_SWAP_SUPPORTED) || !defined(PNG_WRITE_SWAP_SUPPORTED) \
    || !defined(PNG_READ_USER_CHUNKS_SUPPORTED)
#error "imp_io requires libpng EXIF, ICC, byte swapping, and user chunk support"
#endif

enum { byteDepth = 8, wordDepth = 16, rgbChannels = 3, rgbaChannels = 4 };

typedef struct {
    png_structp png;
    png_infop info;
    png_infop end;
    ImpPngError* error;
    const unsigned char* input;
    size_t input_size;
    size_t position;
    ImpPngBytes* output;
    size_t capacity;
} PngState;

static void fail(png_structp png, ImpPngStatus code, const char* message) {
    PngState* state = png_get_error_ptr(png);
    state->error->code = code;
    png_error(png, message);
}

static void pngError(png_structp png, png_const_charp message) {
    PngState* state = png_get_error_ptr(png);
    snprintf(state->error->message, sizeof(state->error->message), "%s", message);
    png_longjmp(png, 1);
}

static void pngWarning(png_structp png, png_const_charp message) {
    /* Fail rather than silently lose malformed profiles or ancillary metadata. */
    pngError(png, message);
}

static void readBytes(png_structp png, png_bytep destination, png_size_t size) {
    PngState* state = png_get_io_ptr(png);
    if (size > state->input_size - state->position) {
        fail(png, IMP_PNG_INVALID, "Truncated PNG data");
    }

    memcpy(destination, state->input + state->position, size);
    state->position += size;
}

static void writeBytes(png_structp png, png_bytep source, png_size_t size) {
    PngState* state = png_get_io_ptr(png);
    size_t needed;
    if (size > IMP_PNG_DATA_LIMIT - state->output->size) {
        fail(png, IMP_PNG_RESOURCE, "Encoded PNG exceeds the byte limit");
    }

    needed = state->output->size + size;
    if (needed > state->capacity) {
        const size_t initialCapacity = 4096;
        size_t capacity = state->capacity ? state->capacity : initialCapacity;
        unsigned char* grown;
        while (capacity < needed) {
            capacity = capacity > IMP_PNG_DATA_LIMIT / 2 ? IMP_PNG_DATA_LIMIT : capacity * 2;
        }
        grown = realloc(state->output->bytes, capacity);
        if (!grown) {
            fail(png, IMP_PNG_RESOURCE, "Cannot allocate encoded PNG data");
        }
        state->output->bytes = grown;
        state->capacity = capacity;
    }

    memcpy(state->output->bytes + state->output->size, source, size);
    state->output->size += size;
}

static int rejectChunk(png_structp png, png_unknown_chunkp chunk) {
    const size_t chunkNameSize = 4;
    const unsigned char ancillaryBit = 0x20;
    if (memcmp(chunk->name, "acTL", chunkNameSize) == 0
        || memcmp(chunk->name, "cICP", chunkNameSize) == 0
        || memcmp(chunk->name, "mDCV", chunkNameSize) == 0
        || memcmp(chunk->name, "cLLI", chunkNameSize) == 0) {
        fail(png, IMP_PNG_UNSUPPORTED, "Animation and extended HDR color chunks are unsupported");
    }
    /* Ignore other ancillary chunks; let libpng reject unknown critical ones. */
    return (chunk->name[0] & ancillaryBit) ? 1 : 0;
}

static int littleEndian(void) {
    const uint16_t value = 1;
    return *(const unsigned char*)&value == 1;
}

static const unsigned char* copyMetadata(png_structp png, const unsigned char* source, size_t size) {
    unsigned char* copy;
    if (size > IMP_PNG_METADATA_LIMIT) {
        fail(png, IMP_PNG_RESOURCE, "PNG metadata exceeds the byte limit");
    }
    copy = malloc(size);
    if (!copy) {
        fail(png, IMP_PNG_RESOURCE, "Cannot allocate PNG metadata");
    }
    memcpy(copy, source, size);
    return copy;
}

static void readMetadata(PngState* state, png_infop info, ImpPngMetadata* metadata) {
    png_charp name;
    int compression;
    png_bytep bytes;
    png_uint_32 size;
    if (png_get_valid(state->png, info, PNG_INFO_gAMA)) {
        png_get_gAMA(state->png, info, &metadata->gamma);
        metadata->fields |= IMP_PNG_GAMMA;
    }
    if (png_get_valid(state->png, info, PNG_INFO_cHRM)) {
        double* xy = metadata->xy;
        png_get_cHRM(state->png, info, &xy[0], &xy[1], &xy[2], &xy[3],
                     &xy[4], &xy[5], &xy[6], &xy[7]);
        metadata->fields |= IMP_PNG_CHROMATICITIES;
    }
    if (png_get_sRGB(state->png, info, &metadata->intent)) {
        metadata->fields |= IMP_PNG_SRGB;
    }
    if (png_get_iCCP(state->png, info, &name, &compression, &bytes, &size)) {
        if (metadata->icc) {
            fail(state->png, IMP_PNG_INVALID, "Duplicate PNG ICC profile");
        }
        metadata->icc = copyMetadata(state->png, bytes, size);
        metadata->icc_size = size;
    }
    if (png_get_eXIf_1(state->png, info, &size, &bytes)) {
        if (metadata->exif) {
            fail(state->png, IMP_PNG_INVALID, "Duplicate PNG EXIF data");
        }
        metadata->exif = copyMetadata(state->png, bytes, size);
        metadata->exif_size = size;
    }
}

static void readRaster(PngState* state, ImpPngRaster* output) {
    int type;
    int passes;
    int pass;
    png_uint_32 row;
    size_t rowSize;
    /* Explicitly route these chunks to rejection, even if libpng knows them. */
    static const png_byte unsupportedChunks[] = "acTL\0cICP\0mDCV\0cLLI\0";
    const int unsupportedCount = 4;
    png_set_read_fn(state->png, state, readBytes);
    png_set_crc_action(state->png, PNG_CRC_ERROR_QUIT, PNG_CRC_ERROR_QUIT);
    png_set_user_limits(state->png, PNG_UINT_31_MAX, PNG_UINT_31_MAX);
    png_set_chunk_malloc_max(state->png, IMP_PNG_METADATA_LIMIT);
    png_set_keep_unknown_chunks(state->png, PNG_HANDLE_CHUNK_ALWAYS, unsupportedChunks, unsupportedCount);
    png_set_read_user_chunk_fn(state->png, state, rejectChunk);
    png_read_info(state->png, state->info);
    output->width = png_get_image_width(state->png, state->info);
    output->height = png_get_image_height(state->png, state->info);
    if (output->width > IMP_PNG_DIMENSION_LIMIT || output->height > IMP_PNG_DIMENSION_LIMIT) {
        fail(state->png, IMP_PNG_RESOURCE, "PNG dimensions exceed the configured limit");
    }
    type = png_get_color_type(state->png, state->info);
    output->depth = png_get_bit_depth(state->png, state->info);

    /* Expand representation only; no gamma, scaling, or premultiplication. */
    if (type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(state->png);
    }
    if (type == PNG_COLOR_TYPE_GRAY && output->depth < byteDepth) {
        png_set_expand_gray_1_2_4_to_8(state->png);
    }
    if (png_get_valid(state->png, state->info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(state->png);
    }
    if (type == PNG_COLOR_TYPE_GRAY || type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(state->png);
    }
    if (output->depth == wordDepth && littleEndian()) {
        png_set_swap(state->png);
    }
    passes = png_set_interlace_handling(state->png);
    png_read_update_info(state->png, state->info);
    output->depth = png_get_bit_depth(state->png, state->info);
    output->channels = png_get_channels(state->png, state->info);
    rowSize = png_get_rowbytes(state->png, state->info);
    if ((output->depth != byteDepth && output->depth != wordDepth)
        || (output->channels != rgbChannels && output->channels != rgbaChannels)) {
        fail(state->png, IMP_PNG_UNSUPPORTED, "Unsupported expanded PNG layout");
    }
    if (rowSize != (size_t)output->width * output->channels * (output->depth / byteDepth)
        || rowSize > IMP_PNG_DATA_LIMIT / output->height) {
        fail(state->png, IMP_PNG_RESOURCE, "Decoded PNG exceeds the sample byte limit");
    }
    output->size = rowSize * output->height;
    output->samples = calloc(1, output->size);
    if (!output->samples) {
        fail(state->png, IMP_PNG_RESOURCE, "Cannot allocate PNG samples");
    }
    readMetadata(state, state->info, &output->metadata);
    for (pass = 0; pass < passes; ++pass) {
        for (row = 0; row < output->height; ++row) {
            png_read_row(state->png, (png_bytep)output->samples + row * rowSize, NULL);
        }
    }
    png_read_end(state->png, state->end);
    readMetadata(state, state->end, &output->metadata);
}

void imp_png_free_raster(ImpPngRaster* raster) {
    free((void*)raster->samples);
    free((void*)raster->metadata.icc);
    free((void*)raster->metadata.exif);
    memset(raster, 0, sizeof(*raster));
}

void imp_png_free_bytes(ImpPngBytes* bytes) {
    free(bytes->bytes);
    memset(bytes, 0, sizeof(*bytes));
}

ImpPngStatus imp_png_decode(const unsigned char* bytes, size_t size,
                            ImpPngRaster* output, ImpPngError* error) {
    PngState* state = calloc(1, sizeof(*state));
    memset(output, 0, sizeof(*output));
    error->code = IMP_PNG_RESOURCE;
    snprintf(error->message, sizeof(error->message), "Cannot allocate PNG reader");
    if (!state) {
        return error->code;
    }
    state->error = error;
    state->input = bytes;
    state->input_size = size;
    state->png = png_create_read_struct(PNG_LIBPNG_VER_STRING, state, pngError, pngWarning);
    if (!state->png) {
        free(state);
        return error->code;
    }
    /* All mutated resources live on the heap, so longjmp leaves cleanup valid. */
    if (setjmp(png_jmpbuf(state->png))) {
        png_destroy_read_struct(&state->png, &state->info, &state->end);
        imp_png_free_raster(output);
        free(state);
        return error->code;
    }
    state->info = png_create_info_struct(state->png);
    state->end = png_create_info_struct(state->png);
    if (!state->info || !state->end) {
        fail(state->png, IMP_PNG_RESOURCE, "Cannot allocate PNG information");
    }
    error->code = IMP_PNG_INVALID;
    readRaster(state, output);
    png_destroy_read_struct(&state->png, &state->info, &state->end);
    free(state);
    error->code = IMP_PNG_OK;
    error->message[0] = '\0';
    return IMP_PNG_OK;
}

static void writeMetadata(PngState* state, const ImpPngMetadata* metadata) {
    if (metadata->fields & IMP_PNG_GAMMA) {
        png_set_gAMA(state->png, state->info, metadata->gamma);
    }
    if (metadata->fields & IMP_PNG_CHROMATICITIES) {
        const double* xy = metadata->xy;
        png_set_cHRM(state->png, state->info, xy[0], xy[1], xy[2], xy[3], xy[4], xy[5], xy[6], xy[7]);
    }
    if (metadata->fields & IMP_PNG_SRGB) {
        png_set_sRGB(state->png, state->info, metadata->intent);
    }
    if (metadata->icc_size) {
        png_set_iCCP(state->png, state->info, "ICC", PNG_COMPRESSION_TYPE_BASE,
                     metadata->icc, (png_uint_32)metadata->icc_size);
    }
    if (metadata->exif_size) {
        png_set_eXIf_1(state->png, state->info, (png_uint_32)metadata->exif_size,
                       (png_bytep)metadata->exif);
    }
}

ImpPngStatus imp_png_encode(const ImpPngRaster* input, ImpPngBytes* output, ImpPngError* error) {
    PngState* state = calloc(1, sizeof(*state));
    png_uint_32 row;
    const size_t rowSize = input->size / input->height;
    memset(output, 0, sizeof(*output));
    error->code = IMP_PNG_RESOURCE;
    snprintf(error->message, sizeof(error->message), "Cannot allocate PNG writer");
    if (!state) {
        return error->code;
    }
    state->error = error;
    state->output = output;
    state->png = png_create_write_struct(PNG_LIBPNG_VER_STRING, state, pngError, pngWarning);
    if (!state->png) {
        free(state);
        return error->code;
    }
    if (setjmp(png_jmpbuf(state->png))) {
        png_destroy_write_struct(&state->png, &state->info);
        imp_png_free_bytes(output);
        free(state);
        return error->code;
    }
    state->info = png_create_info_struct(state->png);
    if (!state->info) {
        fail(state->png, IMP_PNG_RESOURCE, "Cannot allocate PNG information");
    }
    png_set_write_fn(state->png, state, writeBytes, NULL);
    error->code = IMP_PNG_METADATA;
    png_set_IHDR(state->png, state->info, input->width, input->height, input->depth,
                  input->channels == rgbChannels ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGB_ALPHA,
                  PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    writeMetadata(state, &input->metadata);
    png_write_info(state->png, state->info);
    error->code = IMP_PNG_ENCODE;
    if (input->depth == wordDepth && littleEndian()) {
        png_set_swap(state->png);
    }
    for (row = 0; row < input->height; ++row) {
        png_write_row(state->png, input->samples + row * rowSize);
    }
    png_write_end(state->png, state->info);
    png_destroy_write_struct(&state->png, &state->info);
    free(state);
    error->code = IMP_PNG_OK;
    error->message[0] = '\0';
    return IMP_PNG_OK;
}
