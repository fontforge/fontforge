/* Copyright (C) 2025 FontForge Contributors
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 */

#ifndef FONTFORGE_FFPROCESS_H
#define FONTFORGE_FFPROCESS_H

#include <stddef.h> /* for size_t */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Process spawning and file decompression utilities.
 *
 * Backend selection:
 * - When running as a Python module: Uses Python's subprocess/gzip/etc.
 * - When running as FontForge app: Tries POSIX first, falls back to Python
 * - When Python unavailable: POSIX only
 */

typedef enum {
    FF_PROCESS_OK = 0,
    FF_PROCESS_NOT_FOUND, /* Command not found in PATH */
    FF_PROCESS_FAILED,    /* Execution failed */
    FF_PROCESS_NO_BACKEND /* No POSIX or Python available */
} FFProcessResult;

/**
 * Run a command synchronously and capture output.
 *
 * @param argv       NULL-terminated argument array (argv[0] is the command)
 * @param workdir    Working directory for child process (NULL = inherit)
 * @param stdout_buf If non-NULL, receives malloc'd stdout (caller frees)
 * @param stderr_buf If non-NULL, receives malloc'd stderr (caller frees)
 * @return Result code
 */
FFProcessResult ff_run_command(char** argv, const char* workdir,
                               char** stdout_buf, char** stderr_buf);

/**
 * Run a command and capture binary stdout (may contain NUL bytes).
 *
 * @param argv       NULL-terminated argument array
 * @param workdir    Working directory (NULL = inherit)
 * @param out_data   Receives malloc'd output data (caller frees)
 * @param out_len    Receives length of output data
 * @return Result code
 */
FFProcessResult ff_run_command_binary(char** argv, const char* workdir,
                                      unsigned char** out_data,
                                      size_t* out_len);

/**
 * Decompress a file to a temporary file.
 *
 * Supports: .gz, .bz2, .lzma, .xz
 *
 * @param filename   Path to compressed file
 * @param out_tmpfile Receives malloc'd path to temp file (caller frees and
 * unlinks)
 * @return Result code
 */
FFProcessResult ff_decompress_to_temp(const char* filename, char** out_tmpfile);

/**
 * Extract a file from an archive to a temporary directory.
 *
 * Supports: .zip, .tar, .tar.gz, .tar.bz2, .tgz, .tbz2
 *
 * @param archive     Path to archive file
 * @param member      Specific member to extract (NULL = let user choose or
 * extract all)
 * @param out_tmpdir  Receives malloc'd path to temp directory (caller
 * frees/cleans)
 * @param out_file    Receives malloc'd path to extracted file (caller frees)
 * @return Result code
 */
FFProcessResult ff_extract_from_archive(const char* archive, const char* member,
                                        char** out_tmpdir, char** out_file);

/**
 * Check if we're running as a Python module (Python started us).
 * Used internally to decide whether to prefer Python over POSIX.
 */
int ff_running_as_python_module(void);

/* ==================== Compression API ==================== */

/**
 * Compression types.
 *
 * Note: These values do NOT match the legacy SplineFont.compression field.
 * Legacy mapping: 1=gz, 2=bz2, 3=bz(bz2), 4=Z, 5=lzma
 * Use ff_compression_type() to detect from filename extension.
 */
typedef enum {
    FF_COMPRESS_NONE = 0,
    FF_COMPRESS_GZ,   /* .gz - gzip */
    FF_COMPRESS_BZ2,  /* .bz2, .bz - bzip2 */
    FF_COMPRESS_LZMA, /* .lzma - lzma */
    FF_COMPRESS_XZ,   /* .xz - xz */
    FF_COMPRESS_Z,    /* .Z - compress */
    FF_COMPRESS_COUNT
} FFCompressionType;

/**
 * Detect compression type from filename extension.
 *
 * @param filename  Path to check
 * @return Compression type, or FF_COMPRESS_NONE if not recognized
 */
FFCompressionType ff_compression_type(const char* filename);

/**
 * Convert legacy SplineFont.compression value to FFCompressionType.
 *
 * Legacy values: 0=none, 1=gz, 2=bz2, 3=bz(bz2), 4=Z, 5=lzma
 *
 * @param sf_compression  SplineFont.compression field value
 * @return Corresponding FFCompressionType
 */
FFCompressionType ff_compression_from_legacy(int sf_compression);

/**
 * Convert FFCompressionType to legacy SplineFont.compression value.
 *
 * @param type  FFCompressionType value
 * @return Legacy value suitable for SplineFont.compression field
 */
int ff_compression_to_legacy(FFCompressionType type);

/**
 * Get the file extension for a compression type.
 *
 * @param type  Compression type
 * @return Extension string including dot (e.g., ".gz"), or "" for NONE
 */
const char* ff_compression_ext(FFCompressionType type);

/**
 * Compress a file in place.
 * The original file is replaced with the compressed version.
 *
 * @param filename  Path to file (will have extension appended)
 * @param type      Compression type to use
 * @return Result code
 */
FFProcessResult ff_compress_file(const char* filename, FFCompressionType type);

/**
 * Decompress a file in place.
 * The compressed file is replaced with the decompressed version.
 *
 * @param filename  Path to compressed file (extension will be removed)
 * @return Result code
 */
FFProcessResult ff_decompress_in_place(const char* filename);

/* ==================== MIME Type API ==================== */

/**
 * Guess MIME type from filename extension.
 *
 * Uses Python's mimetypes module if available, otherwise falls back
 * to a built-in extension table covering common image/font types.
 *
 * @param path  Path or filename to check
 * @return malloc'd MIME type string (caller frees), or NULL if unknown
 */
char* ff_guess_mime_type(const char* path);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_FFPROCESS_H */
