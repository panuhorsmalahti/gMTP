/* 
 *
 *   File: metatag_info.c
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

#include "config.h"

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#if HAVE_GTK3 == 0
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#else
#include <gio/gio.h>
#endif
#include <libmtp.h>
#include <libgen.h>
#include <sys/stat.h>
#include <strings.h>
#include <string.h>
#include <id3tag.h>
#include <stdio.h>
#include <FLAC/all.h>
#include <vorbis/vorbisfile.h>

#include "main.h"
#include "callbacks.h"
#include "interface.h"
#include "mtp.h"
#include "prefs.h"
#include "dnd.h"
#include "metatag_info.h"

// Constants needed for MP3 frame calculations.
int mp3_samplerate[3][4] = {
    {22050, 24000, 16000, 50000}, // MPEG 2.0
    {44100, 48000, 32000, 50000}, // MPEG 1.0
    {11025, 12000, 8000, 50000} // MPEG 2.5
};

int mp3_bitrate[2][3][15] = {
    { // MPEG 2.0
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}, // Layer 3
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}, // Layer 2
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256} // Layer 1
    },
    { // MPEG 1.0
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}, // Layer 3
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384}, // Layer 2
        {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448} // Layer 1
    }
};

int mp3_sampleperframe[3][4] = {
    // MP2.5 , res , MP2,  MP1
       {576,    0,   576,  1152}, // Layer 3
       {1152,   0,   1152, 1152}, // Layer 2
       {384,    0,   384,  384} // Layer 1

};

// ************************************************************************************************

/**
 * Returns the Frame Text for a given tag.
 * @param tag
 * @param frame_name
 * @return
 */
gchar * ID3_getFrameText(struct id3_tag *tag, char *frame_name) {
    const id3_ucs4_t *id3_string;
    struct id3_frame *id3_frame;
    union id3_field *id3_field;
    gchar *rtn_string = NULL;
    enum id3_field_textencoding id3_field_encoding = ID3_FIELD_TEXTENCODING_ISO_8859_1;

    id3_frame = id3_tag_findframe(tag, frame_name, 0);
    if (id3_frame == NULL)
        return NULL;

    id3_field = id3_frame_field(id3_frame, 0);
    if (id3_field && (id3_field_type(id3_field) == ID3_FIELD_TYPE_TEXTENCODING)) {
        id3_field_encoding = id3_field->number.value;
    }
    //if (frame_name == ID3_FRAME_COMMENT) {
    if(g_ascii_strcasecmp(frame_name, ID3_FRAME_COMMENT) == 0){
        id3_field = id3_frame_field(id3_frame, 3);
    } else {
        id3_field = id3_frame_field(id3_frame, 1);
    }
    if (id3_field == NULL)
        return NULL;
        
    if (g_ascii_strcasecmp(frame_name, ID3_FRAME_COMMENT) == 0) {
        id3_string = id3_field_getfullstring(id3_field);
    } else {
        id3_string = id3_field_getstrings(id3_field, 0);
    }
    if (id3_string == NULL)
        return NULL;
        
    if (g_ascii_strcasecmp(frame_name, ID3_FRAME_GENRE) == 0)
        id3_string = id3_genre_name(id3_string);
        
    if (id3_field_encoding == ID3_FIELD_TEXTENCODING_ISO_8859_1) {
        rtn_string = (gchar *) id3_ucs4_latin1duplicate(id3_string);
    } else {
        rtn_string = (gchar *) id3_ucs4_utf8duplicate(id3_string);
    }
    return rtn_string;
}

// ************************************************************************************************

/**
 * Returns header infomation from a MP3 file.
 * @param mp3_file File handle to an open MP3 file.
 * @param header_info Variable to hold all the MP3 header information for this MP3 frame.
 * @return TRUE if successful, otherwise FALSE.
 */
gboolean get_mp3_header(FILE * mp3_file, MP3_header * header_info) {
    uint8_t raw_header[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint32_t framesize = 0;
    uint32_t id3_footer = 0;
    uint32_t id3_size = 0;

resync:
    if (fread(&raw_header, sizeof (uint8_t)*10, 1, mp3_file) < 1) {
        header_info->header_sync = 0;
        return FALSE;
    }
    /*
        // Print header info for debugging
        printf("Offset: %llx : ", ftell(mp3_file) - 10);
        for (int i = 0; i < 10; i++) {
            printf("0x%x ", raw_header[i]);
        }
        printf("\n");
     */

    header_info->header_sync = ((raw_header[0] << 4) | ((raw_header[1]&0xE0) >> 4));

    // Check for ID3 Tags
    while (header_info->header_sync != 0xFFE) {
        // We may have ID3 Tag.
        if (raw_header[0] == 'I' && raw_header[1] == 'D' && raw_header[2] == '3') {
            // We have a ID3 header...
            // Now skip it.
            id3_footer = (raw_header[5] >> 4) & 0x1;
            id3_size = ((uint32_t) (raw_header[6] << 21) +
                (uint32_t) (raw_header[7] << 14) +
                (uint32_t) (raw_header[8] << 7) +
                (uint32_t) (raw_header[9]));
            if (id3_footer == 1) {
                fseek(mp3_file, (id3_size + 10), SEEK_CUR);
            } else {
                fseek(mp3_file, (id3_size), SEEK_CUR);
            }
            if (fread(&raw_header, sizeof (uint8_t)*10, 1, mp3_file) < 1) {
                header_info->header_sync = 0;
                return FALSE;
            }
            /*
                        // Print header info for debugging
                        printf("Offset: %llx : ", ftell(mp3_file) - 10);
                        for (int i = 0; i < 10; i++) {
                            printf("0x%x ", raw_header[i]);
                        }
                        printf("\n");
             */
            header_info->header_sync = ((raw_header[0] << 4) | ((raw_header[1]&0xE0) >> 4));
        } else {
            // Not a valid frame, nor an ID3 frame?
            // Attempt resync;
            header_info->header_sync = 0;
            fseek(mp3_file, -9, SEEK_CUR);
            goto resync;
            //return FALSE;
        }
    }

    header_info->version = (raw_header[1] >> 3) & 0x3;
    header_info->layer = (raw_header[1] >> 1) & 0x3;
    header_info->crc = raw_header[1] & 0x1;
    header_info->bitrate = (raw_header[2] >> 4) & 0xF;
    header_info->samplerate = (raw_header[2] >> 2) & 0x3;
    header_info->padding = (raw_header[2] >> 1) & 0x1;
    header_info->private_bit = (raw_header[2]) & 0x1;
    header_info->channel_mode = (raw_header[3] >> 6) & 0x3;
    header_info->mode_extension = (raw_header[3] >> 4) & 0x3;
    header_info->copyright = (raw_header[3] >> 3) & 0x1;
    header_info->original = (raw_header[3] >> 2) & 0x1;
    header_info->emphasis = (raw_header[3]) & 0x3;

    // Sanity checks --
    if ((header_info->header_sync != 0xFFE)
        || (header_info->version == 0x1) // Reserved value
        //|| (header_info->layer == 0x1)    // We only care about layer 3 data
        || (header_info->bitrate == 0xF) // Bad value
        || (header_info->samplerate == 0x3)) { // Reserved value
        header_info->header_sync = 0;
        // Attempt resync;
        header_info->header_sync = 0;
        fseek(mp3_file, -9, SEEK_CUR);
        goto resync;
        //return FALSE;
    }
    // We have a valid header, so forward to next possible frame.
    // FrameSize = (samples per sec / 8)  * BitRate / (SampleRate + Padding).

    // Layer I
    if (header_info->layer == 3) {
        framesize = ((12 * mp3_bitrate[header_info->version & 0x1][header_info->layer - 1][header_info->bitrate] * 1000
            / mp3_samplerate[header_info->version & 0x1][header_info->samplerate]
            ) + header_info->padding) * 4;
    } else {
        // Layer 2 and Layer 3
        framesize = (mp3_sampleperframe[header_info->layer - 1][header_info->version] / 8 )     // Samples per frame.
            * mp3_bitrate[header_info->version & 0x1][header_info->layer - 1][header_info->bitrate] * 1000 // Bitrate
            / (mp3_samplerate[header_info->version & 0x1][header_info->samplerate] // Sample Rate
            ) + header_info->padding; // Padding bit
    }
    fseek(mp3_file, (framesize - 10), SEEK_CUR);
    return TRUE;
}

// ************************************************************************************************

/**
 * Get our file information for an MP3 based file.
 * @param filename
 * @param mp3_struct
 */
void get_mp3_info(gchar *filename, MP3_Info *mp3_struct) {

    FILE * mp3_file = NULL;
    struct stat sb;
    MP3_header header_info;
    uint32_t initial_bitrate = 0;
    uint32_t new_bitrate = 0;
    uint64_t total_bitrate = 0;
    uint32_t frames_sampled = 0;
    uint64_t filesize = 0;

    // Init our struct that has been passed to us, and return defaults even if
    // things go wrong.
    mp3_struct->VBR = 1;
    mp3_struct->bitrate = 0;
    mp3_struct->channels = 0;
    mp3_struct->duration = 0;

    mp3_file = fopen(filename, "r");
    if (mp3_file == NULL)
        return;

    if (stat(filename, &sb) == -1) {
        perror("stat");
        fclose(mp3_file);
        return;
    }

    filesize = sb.st_size;

    if (get_mp3_header(mp3_file, &header_info) == TRUE) {
        initial_bitrate = mp3_bitrate[header_info.version & 0x1][header_info.layer - 1][header_info.bitrate];
        total_bitrate = initial_bitrate;
        frames_sampled = 1;
        if (header_info.channel_mode == 0x3) {
            mp3_struct->channels = 1;
        } else {
            mp3_struct->channels = 2;
        }
        // Scan the full file for all frames.
        while ((ftell(mp3_file) < (int64_t) (filesize - 128)) && (get_mp3_header(mp3_file, &header_info) == TRUE)) {
            new_bitrate = mp3_bitrate[header_info.version & 0x1][header_info.layer - 1][header_info.bitrate];
            total_bitrate += new_bitrate;
            frames_sampled++;
            if (new_bitrate != initial_bitrate)
                mp3_struct->VBR = 2;
        }
        if (mp3_struct->VBR != 2) {
            mp3_struct->bitrate = initial_bitrate * 1000;
        } else {
            mp3_struct->bitrate = (total_bitrate / frames_sampled) * 1000;
        }
        //mp3_struct->duration = frames_sampled * 26;
        mp3_struct->duration = (double) frames_sampled * 26.00;
        // Each frame lasts for 26ms, so just multiple the number of frames by 26 to get our duration
    }
}

// ************************************************************************************************

/**
 * Get our track ID3 Tag information
 * @param filename filename of the MP3 file
 * @param trackinformation ID3 infomation returned via this struct.
 */
void get_id3_tags(gchar *filename, LIBMTP_track_t *trackinformation) {
    gchar * tracknumber = NULL;
    gchar * trackduration = NULL;
    MP3_Info mp3_information;

    struct id3_file * id3_file_id = id3_file_open(filename, ID3_FILE_MODE_READONLY);

    if (id3_file_id != NULL) {
        // We have a valid file, so lets get some data.
        struct id3_tag* id3_tag_id = id3_file_tag(id3_file_id);
        // We have our tag data, so now cycle through the fields.
        trackinformation->album = ID3_getFrameText(id3_tag_id, ID3_FRAME_ALBUM);
        trackinformation->title = ID3_getFrameText(id3_tag_id, ID3_FRAME_TITLE);
        trackinformation->artist = ID3_getFrameText(id3_tag_id, ID3_FRAME_ARTIST);
        trackinformation->date = ID3_getFrameText(id3_tag_id, ID3_FRAME_YEAR);
        trackinformation->genre = ID3_getFrameText(id3_tag_id, ID3_FRAME_GENRE);

        tracknumber = ID3_getFrameText(id3_tag_id, ID3_FRAME_TRACK);
        if (tracknumber != 0) {
            trackinformation->tracknumber = atoi(tracknumber);
        } else {
            trackinformation->tracknumber = 0;
        }

        // Need below if the default artist field is NULL
        if (trackinformation->artist == NULL)
            trackinformation->artist = ID3_getFrameText(id3_tag_id, "TPE2");
        if (trackinformation->artist == NULL)
            trackinformation->artist = ID3_getFrameText(id3_tag_id, "TPE3");
        if (trackinformation->artist == NULL)
            trackinformation->artist = ID3_getFrameText(id3_tag_id, "TPE4");
        if (trackinformation->artist == NULL)
            trackinformation->artist = ID3_getFrameText(id3_tag_id, "TCOM");
        // Need this if using different Year field.
        if (trackinformation->date == NULL)
            trackinformation->date = ID3_getFrameText(id3_tag_id, "TDRC");

        // Get our track duration via ID3 Tag.
        trackduration = ID3_getFrameText(id3_tag_id, "TLEN");
        if (trackduration != 0) {
            trackinformation->duration = atoi(trackduration);
        } else {
            trackinformation->duration = 0;
        }
        // Close our file for reading the fields.
        id3_file_close(id3_file_id);
    }

    // Duration, bitrate and other information
    // This information must be derived by manually decoding the MP3 file.
    get_mp3_info(filename, &mp3_information);
    trackinformation->duration = mp3_information.duration;
    trackinformation->bitrate = mp3_information.bitrate;
    trackinformation->bitratetype = mp3_information.VBR;
    trackinformation->nochannels = mp3_information.channels;
}

// ************************************************************************************************

/**
 * Return the OGG Comment for the given tag name
 * @param comments The OGG Comments data field.
 * @param name The tag name to return
 * @return gchar* to string with tag contents
 */
gchar * OGG_getFieldText(const vorbis_comment *comments, const char *name) {
    gchar ** file_comments;
    gchar ** comments_split;
    gint file_comments_count = 0;
    // We simple cycle through our comments, looking for our name, and return it's value;

    if (comments->comments > 0) {
        file_comments = comments->user_comments;
        file_comments_count = comments->comments;
        while (file_comments_count--) {
            // We have our comment, now see if it is what we are after?
            comments_split = g_strsplit(*file_comments, "=", 2);
            if (*comments_split != NULL) {
                if (g_ascii_strcasecmp(name, *comments_split) == 0) {
                    // We have our desrired tag, so return it to the user.
                    comments_split++;
                    return g_strdup(*comments_split);
                }
            }
            // Increment our pointers accordingly.
            file_comments++;
        }
    } else {
        // No comments, so return a NULL value;
        return NULL;
    }
    // We didn't find our key, so return NULL
    return NULL;
}

// ************************************************************************************************

/**
 * Get our OGG track information
 * @param filename The OGG file to extract information
 * @param trackinformation Return the Trackinformation via this variable.
 */
void get_ogg_tags(gchar *filename, LIBMTP_track_t *trackinformation) {
    OggVorbis_File *mov_file = NULL;
    vorbis_info * mov_info = NULL;
    FILE *mfile;
    vorbis_comment *mov_file_comment = NULL;
    gchar * tracknumber = NULL;

    // Attempt to open the file, and init the OggVorbis_File struct for our file.
    // Yes I know about ov_fopen(), but Solaris 10 ships with vorbis 1.0.1 which
    // doesn't have this function.
    mfile = fopen(filename, "r");
    if (mfile == NULL)
        return;

    // Allocate memory to hold the OV file information.
    mov_file = g_malloc0(sizeof (OggVorbis_File));

    if (ov_open(mfile, mov_file, NULL, 0) != 0) {
        fclose(mfile);
        return;
    }

    // Get or comment data;
    mov_file_comment = ov_comment(mov_file, -1);
    mov_info = ov_info(mov_file, -1);

    trackinformation->album = OGG_getFieldText(mov_file_comment, "ALBUM");
    trackinformation->title = OGG_getFieldText(mov_file_comment, "TITLE");
    trackinformation->artist = OGG_getFieldText(mov_file_comment, "ARTIST");
    trackinformation->date = OGG_getFieldText(mov_file_comment, "DATE");
    trackinformation->genre = OGG_getFieldText(mov_file_comment, "GENRE");
    tracknumber = OGG_getFieldText(mov_file_comment, "TRACKNUMBER");
    if (tracknumber != NULL) {
        trackinformation->tracknumber = atoi(tracknumber);
    } else {
        trackinformation->tracknumber = 0;
    }
    // Duration, bitrate and other information
    trackinformation->duration = (int) ov_time_total(mov_file, -1) * 1000;
    trackinformation->bitrate = ov_bitrate(mov_file, -1);
    trackinformation->bitratetype = 2; // VBR
    trackinformation->nochannels = mov_info->channels;
    // Clean up our data structures.
    ov_clear(mov_file);
    g_free(mov_file);
    return;
}

// ************************************************************************************************

/**
 * Get our FLAC Comment for the given tag name
 * @param tags The FLAC Comments
 * @param name The tag name.
 * @return
 */
gchar *FLAC_getFieldText(const FLAC__StreamMetadata *tags, const char *name) {
    int index = FLAC__metadata_object_vorbiscomment_find_entry_from(tags, 0, name);
    if (index < 0) {
        return NULL;
    } else {
        return strchr((const char *) tags->data.vorbis_comment.comments[index].entry, '=') + 1;
    }
}

// ************************************************************************************************

/**
 * Get our FLAC track information
 * @param filename The FLAC file to extract information
 * @param trackinformation Return the Trackinformation via this variable.
 */
void get_flac_tags(gchar *filename, LIBMTP_track_t *trackinformation) {
    FLAC__StreamMetadata *tags = NULL;
    FLAC__StreamMetadata streaminfo;
    gchar * tracknumber = 0;

    // Load in our tag information stream
    if (!FLAC__metadata_get_tags(filename, &tags))
        return;
    if (!FLAC__metadata_get_streaminfo(filename, &streaminfo)) {
        return;
    }
    // We have our tag data, get the individual fields.
    trackinformation->album = g_strdup(FLAC_getFieldText(tags, "ALBUM"));
    trackinformation->title = g_strdup(FLAC_getFieldText(tags, "TITLE"));
    trackinformation->artist = g_strdup(FLAC_getFieldText(tags, "ARTIST"));
    trackinformation->date = g_strdup(FLAC_getFieldText(tags, "DATE"));
    trackinformation->genre = g_strdup(FLAC_getFieldText(tags, "GENRE"));

    tracknumber = FLAC_getFieldText(tags, "TRACKNUMBER");
    if (tracknumber != 0) {
        trackinformation->tracknumber = atoi(tracknumber);
    } else {
        trackinformation->tracknumber = 0;
    }

    // Duration, bitrate and other information
    if((streaminfo.data.stream_info.sample_rate != 0)&&(streaminfo.data.stream_info.total_samples != 0)){
    trackinformation->duration = (streaminfo.data.stream_info.total_samples /
        streaminfo.data.stream_info.sample_rate) * 1000;
    trackinformation->bitrate = 8.0 * (float) (trackinformation->filesize) /
        (1000.0 * (float) streaminfo.data.stream_info.total_samples
        / (float) streaminfo.data.stream_info.sample_rate);
    } else {
        trackinformation->duration = 0;
        trackinformation->bitrate = 0;
    }
    trackinformation->bitratetype = 0; // Not used
    trackinformation->nochannels = streaminfo.data.stream_info.channels;

    //trackinformation->tracknumber = atoi(FLAC_getFieldText(tags, "TRACKNUMBER"));
    FLAC__metadata_object_delete(tags);
    //FLAC__metadata_object_delete(&streaminfo);
    return;
}

// ************************************************************************************************

/**
 * Get our WMA track information. WMA and WMV are both contained in a ASF Container which the
 * container header has all the information. (No need to parse the audio streams themselves).
 * @param filename The WMA file to extract information
 * @param trackinformation Return the Trackinformation via this variable.
 */
void get_asf_tags(gchar *filename, LIBMTP_track_t *trackinformation) {
    FILE *ASF_File;
    GUID Header_GUID;
    GUID Stream_GUID;
    uint32_t Header_Blocks;
    uint64_t Object_Size;
    long ASF_File_Position;

    // Content Object
    uint16_t Title_Length = 0;
    uint16_t Author_Length = 0;
    uint16_t Copyright_Length = 0;
    uint16_t Description_Length = 0;
    uint16_t Rating_Length = 0;

    gchar *Title = NULL;
    gchar *Author = NULL;

    // Extended Content Object
    uint16_t Content_Descriptors_Count = 0;
    uint16_t Descriptor_Name_Length = 0;
    gchar *Descriptor_Name = NULL;
    gchar *Descriptor_Name_UTF16 = NULL;
    uint16_t Descriptor_Value_Type = 0;
    uint16_t Descriptor_Value_Length = 0;
    uint64_t Descriptor_Value = 0;
    gchar *Descriptor_Value_Str = NULL;
    gchar *Descriptor_Value_Str_UTF16 = NULL;

    // Audio Object
    uint16_t Stream_Channels;
    uint32_t Stream_Bitrate;

    // File Object
    uint64_t Stream_Duration;


    ASF_File = fopen(filename, "r");
    if (ASF_File == NULL)
        return;

    // Get our header GUID and make sure this is it.
    size_t i = fread(&Header_GUID, sizeof (GUID), 1, ASF_File);
    if (!memcmp(&Header_GUID, &ASF_header, sizeof (GUID))) {
        // If not exit.
        fclose(ASF_File);
        return;
    }
    if(i != 1){
        // If not exit.
        fclose(ASF_File);
        return;
    }
    // Skip the rest of the header area;
    fseek(ASF_File, 8, SEEK_CUR);
    i = fread(&Header_Blocks, sizeof (uint32_t), 1, ASF_File);
    fseek(ASF_File, 2, SEEK_CUR);

    // We should be at the start of the header blocks;
    // Header_blocks has the number of header objects that we can test.
    while (Header_Blocks--) {
        i = fread(&Header_GUID, sizeof (GUID), 1, ASF_File);
        if (memcmp(&Header_GUID, &ASF_comment_header, sizeof (GUID)) == 0) {
            // We have our standard comment header block;

            // Get the size of the object, and the current file position.
            i = fread(&Object_Size, sizeof (uint64_t), 1, ASF_File);
            ASF_File_Position = ftell(ASF_File);
            // Get our field lengths.
            i = fread(&Title_Length, sizeof (uint16_t), 1, ASF_File);
            i = fread(&Author_Length, sizeof (uint16_t), 1, ASF_File);
            i = fread(&Copyright_Length, sizeof (uint16_t), 1, ASF_File);
            i = fread(&Description_Length, sizeof (uint16_t), 1, ASF_File);
            i = fread(&Rating_Length, sizeof (uint16_t), 1, ASF_File);
            // Since we only need Title and Author, we only need to alloc memory for those two.
            Title = g_malloc0(Title_Length + 0x10);
            Author = g_malloc0(Author_Length + 0x10);
            i = fread(Title, Title_Length, 1, ASF_File);
            i = fread(Author, Author_Length, 1, ASF_File);
            // Set our track information
            trackinformation->title = g_utf16_to_utf8((const gunichar2 *) Title, Title_Length, NULL, NULL, NULL);
            trackinformation->artist = g_utf16_to_utf8((const gunichar2 *) Author, Author_Length, NULL, NULL, NULL);
            // Free our memory that we used to load in the fields.
            g_free(Title);
            g_free(Author);
            Title = NULL;
            Author = NULL;
            // Set our file position so it's ready to read in the next GUID Header.
            fseek(ASF_File, ASF_File_Position, SEEK_SET);
            fseek(ASF_File, (Object_Size - sizeof (uint64_t) - sizeof (GUID)), SEEK_CUR);
        } else {
            if (memcmp(&Header_GUID, &ASF_extended_content_header, sizeof (GUID)) == 0) {
                // We have our standard comment header block;
                //g_printf("WMA: Found our extended comment block\n");
                // Get the size of the object, and the current file position.
                i = fread(&Object_Size, sizeof (uint64_t), 1, ASF_File);
                ASF_File_Position = ftell(ASF_File);
                // Get the number of Descripions field we have, as we will need to cycle through them all.
                i = fread(&Content_Descriptors_Count, sizeof (uint16_t), 1, ASF_File);
                while (Content_Descriptors_Count--) {
                    // These themselves are Objects within the main extended content header, which we need to handle.
                    // Format is:
                    // Descriptor Name Length (word)
                    // Descriptor Name (varies)
                    // Descriptor Value Type (word)
                    // Descriptor Value Length (word)
                    // Descriptor Value (varies - depend on Value Type).
                    Descriptor_Name_Length = 0;
                    Descriptor_Name = NULL;
                    Descriptor_Name_UTF16 = NULL;
                    Descriptor_Value_Type = 0;
                    Descriptor_Value_Length = 0;
                    Descriptor_Value = 0;
                    Descriptor_Value_Str = NULL;
                    Descriptor_Value_Str_UTF16 = NULL;
                    // Get our Descriptor Name.
                    i = fread(&Descriptor_Name_Length, sizeof (uint16_t), 1, ASF_File);
                    Descriptor_Name_UTF16 = g_malloc0(Descriptor_Name_Length + 0x10);
                    i = fread(Descriptor_Name_UTF16, Descriptor_Name_Length, 1, ASF_File);
                    Descriptor_Name = g_utf16_to_utf8((const gunichar2 *) Descriptor_Name_UTF16,
                        Descriptor_Name_Length, NULL, NULL, NULL);
                    // Get our Value Type and Value Length
                    i = fread(&Descriptor_Value_Type, sizeof (uint16_t), 1, ASF_File);
                    i = fread(&Descriptor_Value_Length, sizeof (uint16_t), 1, ASF_File);
                    switch (Descriptor_Value_Type) {
                        case 0: // String;
                        case 1: // Binary;
                            Descriptor_Value_Str_UTF16 = g_malloc0(Descriptor_Value_Length + 0x10);
                            i = fread(Descriptor_Value_Str_UTF16, Descriptor_Value_Length, 1, ASF_File);
                            Descriptor_Value_Str = g_utf16_to_utf8((const gunichar2 *) Descriptor_Value_Str_UTF16,
                                Descriptor_Value_Length, NULL, NULL, NULL);
                            // We have out key=value pair so lets look for our desired  keys 'WM/AlbumTitle', 'WM/Genre' and 'WM/Year'
                            if (g_ascii_strcasecmp(Descriptor_Name, "WM/AlbumTitle\0") == 0) {
                                // We have the album Title;
                                trackinformation->album = g_strdup(Descriptor_Value_Str);
                            } else {
                                if (g_ascii_strcasecmp(Descriptor_Name, "WM/Genre\0") == 0) {
                                    // We have the album Genre;
                                    trackinformation->genre = g_strdup(Descriptor_Value_Str);
                                } else {
                                    if (g_ascii_strcasecmp(Descriptor_Name, "WM/Year\0") == 0) {
                                        // We have the album Year;
                                        trackinformation->date = g_strdup(Descriptor_Value_Str);
                                    }
                                }
                            }
                            break;
                        case 2: // Boolean (DWORD)
                        case 3: // DWORD
                        case 4: // QWORD
                        case 5: // WORD
                            if (Descriptor_Value_Length > sizeof (Descriptor_Value))
                                Descriptor_Value_Length = sizeof (Descriptor_Value);
                            i = fread(&Descriptor_Value, Descriptor_Value_Length, 1, ASF_File);
                            if ((g_ascii_strcasecmp(Descriptor_Name, "WM/Track\0") == 0)) {
                                trackinformation->tracknumber = Descriptor_Value + 1;
                            } else {
                                if (g_ascii_strcasecmp(Descriptor_Name, "WM/TrackNumber\0") == 0)
                                    trackinformation->tracknumber = Descriptor_Value;
                            }
                            break;
                        default: // Unknown so skip it.
                            fseek(ASF_File, Descriptor_Value_Length, SEEK_CUR);
                            break;
                    }

                    // Free up our allocated memory;
                    g_free(Descriptor_Name);
                    g_free(Descriptor_Name_UTF16);
                    g_free(Descriptor_Value_Str);
                    g_free(Descriptor_Value_Str_UTF16);
                }

                // Set our file position so it's ready to read in the next GUID Header.
                fseek(ASF_File, ASF_File_Position, SEEK_SET);
                fseek(ASF_File, (Object_Size - sizeof (uint64_t) - sizeof (GUID)), SEEK_CUR);
            } else {
                if (memcmp(&Header_GUID, &ASF_Stream_header, sizeof (GUID)) == 0) {
                    // We have an audio header for the track information.
                    i = fread(&Object_Size, sizeof (uint64_t), 1, ASF_File);
                    ASF_File_Position = ftell(ASF_File);

                    // Read in the stream type GUID
                    i = fread(&Stream_GUID, sizeof (GUID), 1, ASF_File);
                    if (memcmp(&Stream_GUID, &ASF_Audio_Media_header, sizeof (GUID)) == 0) {
                        // We have an audio header.
                        fseek(ASF_File, 38, SEEK_CUR);
                        // We should be pointing at our audio stream data block.
                        fseek(ASF_File, sizeof (uint16_t), SEEK_CUR); // Skip CODEC ID
                        i = fread(&Stream_Channels, sizeof (uint16_t), 1, ASF_File);
                        fseek(ASF_File, 4, SEEK_CUR); // Skip Samples per second
                        i = fread(&Stream_Bitrate, sizeof (uint32_t), 1, ASF_File);

                        trackinformation->nochannels = Stream_Channels;
                        trackinformation->bitrate = Stream_Bitrate * 8; // This value is in BYTES
                        trackinformation->bitratetype = 0; // Not used
                    }
                    // Set our file position so it's ready to read in the next GUID Header.
                    fseek(ASF_File, ASF_File_Position, SEEK_SET);
                    fseek(ASF_File, (Object_Size - sizeof (uint64_t) - sizeof (GUID)), SEEK_CUR);
                } else {
                    if (memcmp(&Header_GUID, &ASF_File_Properties_header, sizeof (GUID)) == 0) {
                        // We have a file header for the track information.
                        i = fread(&Object_Size, sizeof (uint64_t), 1, ASF_File);
                        ASF_File_Position = ftell(ASF_File);
                        // Skip File ID, Filesize, Creation Date and Data Packets Count
                        fseek(ASF_File, (sizeof (GUID) + (sizeof (uint64_t) * 3)), SEEK_CUR);
                        i = fread(&Stream_Duration, sizeof (uint64_t), 1, ASF_File);
                        // Convert from 1/100ths nano sec to millisec.
                        trackinformation->duration = Stream_Duration / 10000;

                        fseek(ASF_File, ASF_File_Position, SEEK_SET);
                        fseek(ASF_File, (Object_Size - sizeof (uint64_t) - sizeof (GUID)), SEEK_CUR);
                    } else {
                        // Skip this header;
                        i = fread(&Object_Size, sizeof (uint64_t), 1, ASF_File);
                        fseek(ASF_File, (Object_Size - sizeof (uint64_t) - sizeof (GUID)), SEEK_CUR);
                    }
                }
            }
        }
    }
    fclose(ASF_File);
    return;
}
