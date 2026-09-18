#include "JpegBridge.h"

#include <stdio.h>

#include <jpeglib.h>
#include <jerror.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

enum {
    byteDepth = 8,
    rgbChannels = 3,
    markerLimit = 0xffff,
    markerPrefix = 0xff,
    startOfImage = 0xd8,
    startOfScan = 0xda,
    endOfImage = 0xd9,
    temporaryMarker = 0x01,
    firstRestartMarker = 0xd0,
    lastRestartMarker = 0xd7,
    baselineSof = 0xc0,
    extendedSequentialSof = 0xc1,
    progressiveSof = 0xc2,
    losslessSof = 0xc3,
    differentialSequentialSof = 0xc5,
    differentialProgressiveSof = 0xc6,
    differentialLosslessSof = 0xc7,
    arithmeticSequentialSof = 0xc9,
    arithmeticProgressiveSof = 0xca,
    arithmeticLosslessSof = 0xcb,
    arithmeticDifferentialSequentialSof = 0xcd,
    arithmeticDifferentialProgressiveSof = 0xce,
    arithmeticDifferentialLosslessSof = 0xcf
};

static const unsigned char iccSignature[] = "ICC_PROFILE\0";
static const unsigned char exifSignature[] = {'E', 'x', 'i', 'f', 0, 0};

typedef struct {
    struct jpeg_error_mgr base;
    jmp_buf jump;
    ImpJpegError* output;
    ImpJpegStatus fallback;
} JpegError;

typedef struct {
    struct jpeg_decompress_struct jpeg;
    JpegError error;
} JpegReader;

typedef struct {
    struct jpeg_compress_struct jpeg;
    JpegError error;
    unsigned char* destination;
    unsigned long destination_size;
    unsigned char* marker;
} JpegWriter;

static void errorExit(j_common_ptr jpeg) {
    JpegError* error = (JpegError*)jpeg->err;
    error->output->code = error->fallback;
    if (jpeg->err->msg_code == JERR_OUT_OF_MEMORY) {
        error->output->code = IMP_JPEG_RESOURCE;
    }
    if (jpeg->err->msg_code == JERR_BAD_PRECISION) {
        error->output->code = IMP_JPEG_UNSUPPORTED_PRECISION;
    }
    (*jpeg->err->format_message)(jpeg, error->output->message);
    longjmp(error->jump, 1);
}

static void emitMessage(j_common_ptr jpeg, int level) {
    if (level < 0) {
        errorExit(jpeg);
    }
}

static void initError(JpegError* error, ImpJpegError* output, ImpJpegStatus fallback) {
    jpeg_std_error(&error->base);
    error->base.error_exit = errorExit;
    error->base.emit_message = emitMessage;
    error->output = output;
    error->fallback = fallback;
    output->code = fallback;
    output->message[0] = '\0';
}

static void fail(j_common_ptr jpeg, ImpJpegStatus status, const char* message) {
    JpegError* error = (JpegError*)jpeg->err;
    error->output->code = status;
    snprintf(error->output->message, sizeof(error->output->message), "%s", message);
    longjmp(error->jump, 1);
}

static const unsigned char* copyMetadata(j_common_ptr jpeg, const unsigned char* source,
                                         size_t size) {
    unsigned char* copy;
    if (!size) {
        fail(jpeg, IMP_JPEG_INVALID, "JPEG metadata payload is empty");
    }
    if (size > IMP_JPEG_METADATA_LIMIT) {
        fail(jpeg, IMP_JPEG_RESOURCE, "JPEG metadata exceeds the byte limit");
    }
    copy = malloc(size);
    if (!copy) {
        fail(jpeg, IMP_JPEG_RESOURCE, "Cannot allocate JPEG metadata");
    }
    memcpy(copy, source, size);
    return copy;
}

static int hasPrefix(jpeg_saved_marker_ptr marker, const unsigned char* prefix, size_t size) {
    return marker->data_length >= size && memcmp(marker->data, prefix, size) == 0;
}

static int isSof(unsigned char marker) {
    switch (marker) {
        case baselineSof:
        case extendedSequentialSof:
        case progressiveSof:
        case losslessSof:
        case differentialSequentialSof:
        case differentialProgressiveSof:
        case differentialLosslessSof:
        case arithmeticSequentialSof:
        case arithmeticProgressiveSof:
        case arithmeticLosslessSof:
        case arithmeticDifferentialSequentialSof:
        case arithmeticDifferentialProgressiveSof:
        case arithmeticDifferentialLosslessSof:
            return 1;
        default:
            return 0;
    }
}

static void checkProcess(j_common_ptr jpeg, const unsigned char* bytes, size_t size) {
    size_t offset = 2;

    // Accept only the two scoped SOF processes before libjpeg starts decoding.
    if (size < offset || bytes[0] != markerPrefix || bytes[1] != startOfImage) {
        fail(jpeg, IMP_JPEG_INVALID, "Malformed JPEG signature");
    }

    while (offset < size) {
        unsigned char marker;
        size_t length;
        if (bytes[offset] != markerPrefix) {
            fail(jpeg, IMP_JPEG_INVALID, "Malformed JPEG marker stream");
        }
        while (offset < size && bytes[offset] == markerPrefix) {
            ++offset;
        }
        if (offset >= size) {
            break;
        }

        marker = bytes[offset++];
        if (isSof(marker)) {
            if (marker != baselineSof && marker != progressiveSof) {
                fail(jpeg, IMP_JPEG_UNSUPPORTED_FEATURE,
                     "JPEG process must be baseline or progressive Huffman DCT");
            }
            return;
        }
        if (marker == startOfScan || marker == endOfImage) {
            break;
        }
        if (marker == temporaryMarker
            || (marker >= firstRestartMarker && marker <= lastRestartMarker)) {
            continue;
        }
        if (offset > size - 2) {
            break;
        }

        length = ((size_t)bytes[offset] << 8) | bytes[offset + 1];
        if (length < 2 || length > size - offset) {
            break;
        }
        offset += length;
    }

    fail(jpeg, IMP_JPEG_INVALID, "JPEG frame header is missing or malformed");
}

static void readExif(j_decompress_ptr jpeg, ImpJpegMetadata* metadata) {
    jpeg_saved_marker_ptr marker;
    for (marker = jpeg->marker_list; marker; marker = marker->next) {
        if (marker->marker != JPEG_APP0 + 1
            || !hasPrefix(marker, exifSignature, sizeof(exifSignature))) {
            continue;
        }
        if (metadata->exif) {
            fail((j_common_ptr)jpeg, IMP_JPEG_INVALID, "Duplicate JPEG EXIF metadata");
        }
        metadata->exif_size = marker->data_length - sizeof(exifSignature);
        metadata->exif = copyMetadata((j_common_ptr)jpeg,
                                      marker->data + sizeof(exifSignature),
                                      metadata->exif_size);
    }
}

static void readIcc(j_decompress_ptr jpeg, ImpJpegMetadata* metadata) {
    jpeg_saved_marker_ptr chunks[256] = {NULL};
    jpeg_saved_marker_ptr marker;
    unsigned int expected = 0;
    unsigned int index;
    size_t size = 0;
    unsigned char* destination;
    for (marker = jpeg->marker_list; marker; marker = marker->next) {
        unsigned int sequence;
        unsigned int count;
        if (marker->marker != JPEG_APP0 + 2
            || !hasPrefix(marker, iccSignature, sizeof(iccSignature) - 1)) {
            continue;
        }
        if (marker->data_length < sizeof(iccSignature) + 1) {
            fail((j_common_ptr)jpeg, IMP_JPEG_INVALID, "Malformed JPEG ICC chunk");
        }
        sequence = marker->data[sizeof(iccSignature) - 1];
        count = marker->data[sizeof(iccSignature)];
        if (!sequence || !count || sequence > count || (expected && count != expected)
            || chunks[sequence]) {
            fail((j_common_ptr)jpeg, IMP_JPEG_INVALID, "Malformed JPEG ICC sequence");
        }
        expected = count;
        chunks[sequence] = marker;
    }
    if (!expected) {
        return;
    }
    for (index = 1; index <= expected; ++index) {
        if (!chunks[index]) {
            fail((j_common_ptr)jpeg, IMP_JPEG_INVALID, "Incomplete JPEG ICC sequence");
        }
        size += chunks[index]->data_length - (sizeof(iccSignature) + 1);
        if (size > IMP_JPEG_METADATA_LIMIT) {
            fail((j_common_ptr)jpeg, IMP_JPEG_RESOURCE, "JPEG ICC profile exceeds the byte limit");
        }
    }
    if (!size) {
        fail((j_common_ptr)jpeg, IMP_JPEG_INVALID, "JPEG ICC profile is empty");
    }
    destination = malloc(size);
    if (!destination) {
        fail((j_common_ptr)jpeg, IMP_JPEG_RESOURCE, "Cannot allocate JPEG ICC profile");
    }
    metadata->icc = destination;
    metadata->icc_size = size;
    size = 0;
    for (index = 1; index <= expected; ++index) {
        const size_t header = sizeof(iccSignature) + 1;
        const size_t payload = chunks[index]->data_length - header;
        memcpy(destination + size, chunks[index]->data + header, payload);
        size += payload;
    }
}

void imp_jpeg_free_raster(ImpJpegRaster* raster) {
    free((void*)raster->samples);
    free((void*)raster->metadata.icc);
    free((void*)raster->metadata.exif);
    memset(raster, 0, sizeof(*raster));
}

void imp_jpeg_free_bytes(ImpJpegBytes* bytes) {
    free(bytes->bytes);
    memset(bytes, 0, sizeof(*bytes));
}

ImpJpegStatus imp_jpeg_decode(const unsigned char* bytes, size_t size,
                              ImpJpegRaster* output, ImpJpegError* error) {
    JpegReader* reader = calloc(1, sizeof(*reader));
    size_t row_size;
    memset(output, 0, sizeof(*output));
    if (!reader) {
        error->code = IMP_JPEG_RESOURCE;
        snprintf(error->message, sizeof(error->message), "Cannot allocate JPEG reader");
        return error->code;
    }
    initError(&reader->error, error, IMP_JPEG_INVALID);
    reader->jpeg.err = &reader->error.base;
    if (setjmp(reader->error.jump)) {
        jpeg_destroy_decompress(&reader->jpeg);
        imp_jpeg_free_raster(output);
        free(reader);
        return error->code;
    }
    jpeg_create_decompress(&reader->jpeg);
    jpeg_save_markers(&reader->jpeg, JPEG_APP0 + 1, markerLimit);
    jpeg_save_markers(&reader->jpeg, JPEG_APP0 + 2, markerLimit);
    jpeg_mem_src(&reader->jpeg, bytes, (unsigned long)size);
    checkProcess((j_common_ptr)&reader->jpeg, bytes, size);
    jpeg_read_header(&reader->jpeg, TRUE);
    if (reader->jpeg.data_precision != byteDepth) {
        fail((j_common_ptr)&reader->jpeg, IMP_JPEG_UNSUPPORTED_PRECISION,
             "JPEG precision must be 8 bits");
    }
    if (reader->jpeg.jpeg_color_space == JCS_CMYK
        || reader->jpeg.jpeg_color_space == JCS_YCCK
        || (reader->jpeg.num_components != 1
            && reader->jpeg.num_components != rgbChannels)) {
        fail((j_common_ptr)&reader->jpeg, IMP_JPEG_UNSUPPORTED_LAYOUT,
             "CMYK, YCCK, and non-RGB JPEG layouts are unsupported");
    }
    if (reader->jpeg.image_width > IMP_JPEG_DIMENSION_LIMIT
        || reader->jpeg.image_height > IMP_JPEG_DIMENSION_LIMIT) {
        fail((j_common_ptr)&reader->jpeg, IMP_JPEG_RESOURCE,
             "JPEG dimensions exceed the configured limit");
    }
    readExif(&reader->jpeg, &output->metadata);
    readIcc(&reader->jpeg, &output->metadata);
    reader->jpeg.out_color_space = JCS_RGB;
    reader->jpeg.dct_method = JDCT_ISLOW;
    jpeg_start_decompress(&reader->jpeg);
    output->width = reader->jpeg.output_width;
    output->height = reader->jpeg.output_height;
    output->channels = reader->jpeg.output_components;
    output->depth = byteDepth;
    row_size = (size_t)output->width * output->channels;
    if (output->channels != rgbChannels || row_size > IMP_JPEG_DATA_LIMIT / output->height) {
        fail((j_common_ptr)&reader->jpeg, IMP_JPEG_RESOURCE,
             "Decoded JPEG exceeds the sample byte limit");
    }
    output->size = row_size * output->height;
    output->samples = malloc(output->size);
    if (!output->samples) {
        fail((j_common_ptr)&reader->jpeg, IMP_JPEG_RESOURCE, "Cannot allocate JPEG samples");
    }
    while (reader->jpeg.output_scanline < reader->jpeg.output_height) {
        JSAMPROW row = (JSAMPROW)output->samples
            + reader->jpeg.output_scanline * row_size;
        if (jpeg_read_scanlines(&reader->jpeg, &row, 1) != 1) {
            fail((j_common_ptr)&reader->jpeg, IMP_JPEG_INVALID,
                 "Cannot decode JPEG scanline");
        }
    }
    jpeg_finish_decompress(&reader->jpeg);
    jpeg_destroy_decompress(&reader->jpeg);
    free(reader);
    error->code = IMP_JPEG_OK;
    return IMP_JPEG_OK;
}

static void writeExif(JpegWriter* writer, const ImpJpegMetadata* metadata) {
    const size_t size = sizeof(exifSignature) + metadata->exif_size;
    if (!metadata->exif_size) {
        return;
    }
    if (size > markerLimit - 2) {
        fail((j_common_ptr)&writer->jpeg, IMP_JPEG_METADATA,
             "JPEG EXIF metadata exceeds one APP1 marker");
    }
    writer->marker = malloc(size);
    if (!writer->marker) {
        fail((j_common_ptr)&writer->jpeg, IMP_JPEG_RESOURCE,
             "Cannot allocate JPEG EXIF marker");
    }
    memcpy(writer->marker, exifSignature, sizeof(exifSignature));
    memcpy(writer->marker + sizeof(exifSignature), metadata->exif, metadata->exif_size);
    jpeg_write_marker(&writer->jpeg, JPEG_APP0 + 1, writer->marker, (unsigned int)size);
    free(writer->marker);
    writer->marker = NULL;
}

static void writeIcc(JpegWriter* writer, const ImpJpegMetadata* metadata) {
    const size_t header = sizeof(iccSignature) + 1;
    const size_t count = (metadata->icc_size + IMP_JPEG_ICC_CHUNK_SIZE - 1)
        / IMP_JPEG_ICC_CHUNK_SIZE;
    size_t offset = 0;
    size_t sequence;
    if (!metadata->icc_size) {
        return;
    }
    if (metadata->icc_size > IMP_JPEG_METADATA_LIMIT || count > 255) {
        fail((j_common_ptr)&writer->jpeg, IMP_JPEG_METADATA,
             "JPEG ICC profile exceeds APP2 sequence capacity");
    }
    writer->marker = malloc(header + IMP_JPEG_ICC_CHUNK_SIZE);
    if (!writer->marker) {
        fail((j_common_ptr)&writer->jpeg, IMP_JPEG_RESOURCE,
             "Cannot allocate JPEG ICC marker");
    }
    memcpy(writer->marker, iccSignature, sizeof(iccSignature) - 1);
    for (sequence = 1; sequence <= count; ++sequence) {
        const size_t payload = metadata->icc_size - offset < IMP_JPEG_ICC_CHUNK_SIZE
            ? metadata->icc_size - offset : IMP_JPEG_ICC_CHUNK_SIZE;
        writer->marker[sizeof(iccSignature) - 1] = (unsigned char)sequence;
        writer->marker[sizeof(iccSignature)] = (unsigned char)count;
        memcpy(writer->marker + header, metadata->icc + offset, payload);
        jpeg_write_marker(&writer->jpeg, JPEG_APP0 + 2, writer->marker,
                          (unsigned int)(header + payload));
        offset += payload;
    }
    free(writer->marker);
    writer->marker = NULL;
}

ImpJpegStatus imp_jpeg_encode(const ImpJpegRaster* input, int quality,
                              ImpJpegBytes* output, ImpJpegError* error) {
    JpegWriter* writer = calloc(1, sizeof(*writer));
    size_t row_size;
    memset(output, 0, sizeof(*output));
    if (!writer) {
        error->code = IMP_JPEG_RESOURCE;
        snprintf(error->message, sizeof(error->message), "Cannot allocate JPEG writer");
        return error->code;
    }
    initError(&writer->error, error, IMP_JPEG_ENCODE);
    writer->jpeg.err = &writer->error.base;
    if (setjmp(writer->error.jump)) {
        jpeg_destroy_compress(&writer->jpeg);
        free(writer->destination);
        free(writer->marker);
        free(writer);
        return error->code;
    }
    jpeg_create_compress(&writer->jpeg);
    jpeg_mem_dest(&writer->jpeg, &writer->destination, &writer->destination_size);
    writer->jpeg.image_width = input->width;
    writer->jpeg.image_height = input->height;
    writer->jpeg.input_components = rgbChannels;
    writer->jpeg.in_color_space = JCS_RGB;
    jpeg_set_defaults(&writer->jpeg);
    jpeg_set_quality(&writer->jpeg, quality, TRUE);
    writer->jpeg.comp_info[0].h_samp_factor = 1;
    writer->jpeg.comp_info[0].v_samp_factor = 1;
    writer->jpeg.comp_info[1].h_samp_factor = 1;
    writer->jpeg.comp_info[1].v_samp_factor = 1;
    writer->jpeg.comp_info[2].h_samp_factor = 1;
    writer->jpeg.comp_info[2].v_samp_factor = 1;
    jpeg_start_compress(&writer->jpeg, TRUE);
    writer->error.fallback = IMP_JPEG_METADATA;
    writeExif(writer, &input->metadata);
    writeIcc(writer, &input->metadata);
    writer->error.fallback = IMP_JPEG_ENCODE;
    row_size = input->size / input->height;
    while (writer->jpeg.next_scanline < writer->jpeg.image_height) {
        JSAMPROW row = (JSAMPROW)input->samples + writer->jpeg.next_scanline * row_size;
        jpeg_write_scanlines(&writer->jpeg, &row, 1);
    }
    jpeg_finish_compress(&writer->jpeg);
    if (writer->destination_size > IMP_JPEG_DATA_LIMIT) {
        fail((j_common_ptr)&writer->jpeg, IMP_JPEG_RESOURCE,
             "Encoded JPEG exceeds the byte limit");
    }
    output->bytes = writer->destination;
    output->size = writer->destination_size;
    writer->destination = NULL;
    jpeg_destroy_compress(&writer->jpeg);
    free(writer);
    error->code = IMP_JPEG_OK;
    return IMP_JPEG_OK;
}
