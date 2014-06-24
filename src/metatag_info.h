/* 
 *
 *   File: metatag_info.h
 *
 *   Copyright (C) 2009-2013 Darran Kartaschew
 *
 *   This file is part of the gMTP package.
 *
 *   gMTP is free software; you can redistribute it and/or modify
 *   it under the terms of the BSD License as included within the
 *   file 'COPYING' located in the root directory
 *
 */

#ifndef _METATAG_INFO_H
#define	_METATAG_INFO_H

#ifdef	__cplusplus
extern "C" {
#endif

    typedef struct {
        uint32_t bitrate;
        uint32_t duration; //  in milliseconds
        uint16_t VBR; // 0 = unused, 1 = constant, 2 = VBR, 3 = free
        uint16_t channels; // 0 = Unknown, 1 = mono, 2 = stereo.
    } MP3_Info;

    typedef struct {
        uint16_t header_sync;
        uint16_t version;
        uint16_t layer;
        uint16_t crc;
        uint16_t bitrate;
        uint16_t samplerate;
        uint16_t padding;
        uint16_t private_bit;
        uint16_t channel_mode;
        uint16_t mode_extension;
        uint16_t copyright;
        uint16_t original;
        uint16_t emphasis;
    } MP3_header;

    typedef struct {
        uint32_t f1;
        uint16_t f2;
        uint16_t f3;
        uint16_t f4;
        uint8_t f5_1;
        uint8_t f5_2;
        uint8_t f5_3;
        uint8_t f5_4;
        uint8_t f5_5;
        uint8_t f5_6;
    } GUID;

    // We only include our ASF header objects we want, not all of them

    static const GUID ASF_header = {
        0x75B22630, 0x668E, 0x11CF, 0xA6D9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C
    };

    static const GUID ASF_comment_header = {
        0x75B22633, 0x668E, 0x11CF, 0xD9A6, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C
    };

    static const GUID ASF_extended_content_header = {
        0xD2D0A440, 0xE307, 0x11D2, 0xF097, 0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50
    };

    static const GUID ASF_File_Properties_header = {
        0x8CABDCA1, 0xA947, 0x11CF, 0xE48E, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65
    };

    static const GUID ASF_Stream_header = {
        0xB7DC0791, 0xA9B7, 0x11CF, 0xE68E, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65
    };

    static const GUID ASF_Audio_Media_header = {
        0xF8699E40, 0x5B4D, 0x11CF, 0xFDA8, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B
    };

    //gchar * ID3_getFrameText(struct id3_tag *tag, char *frame_name);
    //gchar * FLAC_getFieldText(const FLAC__StreamMetadata *tags, const char *name);
    //gchar * OGG_getFieldText(const vorbis_comment *comments, const char *name);

    void get_mp3_info(gchar *filename, MP3_Info *mp3_struct);

    void get_id3_tags(gchar *filename, LIBMTP_track_t *trackinformation);
    void get_ogg_tags(gchar *filename, LIBMTP_track_t *trackinformation);
    void get_flac_tags(gchar *filename, LIBMTP_track_t *trackinformation);
    void get_asf_tags(gchar *filename, LIBMTP_track_t *trackinformation);

#ifdef	__cplusplus
}
#endif

#endif	/* _METATAG_INFO_H */

