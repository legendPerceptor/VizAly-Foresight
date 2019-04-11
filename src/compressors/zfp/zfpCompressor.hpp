/*================================================================================
This software is open source software available under the BSD-3 license.

Copyright (c) 2017, Los Alamos National Security, LLC.
All rights reserved.

Authors:
 - Pascal Grosset
================================================================================*/
#ifdef CBENCH_HAS_ZFP

#ifndef _ZFP_COMPRESSOR_H_
#define _ZFP_COMPRESSOR_H_

#include <sstream>
#include "compressorInterface.hpp"

#include <zfp.h>

class ZFPCompressor: public CompressorInterface
{
    size_t zfpCompressedSize;

  public:
    ZFPCompressor();
    ~ZFPCompressor();

    void init();
    int compress(void *input, void *&output, std::string dataType, size_t dataTypeSize, size_t * n);
    int decompress(void *&input, void *&output, std::string dataType, size_t dataTypeSize, size_t * n);
    void close();

    zfp_type getZfpType(std::string dataType);
};


inline ZFPCompressor::ZFPCompressor()
{
    compressorName = "zfp";
}

inline ZFPCompressor::~ZFPCompressor()
{
    
}


inline void ZFPCompressor::init()
{

}


inline zfp_type ZFPCompressor::getZfpType(std::string dataType)
{
	if (dataType == "float")
		return zfp_type_float;
	else if (dataType == "double")
		return zfp_type_double;
	else if ((dataType == "int32_t") || (dataType == "int16_t"))
		return zfp_type_int32;
	else if (dataType == "int64_t")
		return zfp_type_int64;
}


inline int ZFPCompressor::compress(void *input, void *&output, std::string dataType, size_t dataTypeSize, size_t * n)
{
	size_t numel = n[0];
	for (int i = 1; i < 5; i++)
		if (n[i] != 0)
			numel *= n[i];

    // Read in json compression parameters
    double abs = 1E-3;
    std::unordered_map<std::string, std::string>::const_iterator got = compressorParameters.find("abs");
    if( got != compressorParameters.end() )
        if (compressorParameters["abs"] != "")
            abs = strConvert::to_double( compressorParameters["abs"] );

    Timer cTime; 
    cTime.start();

	zfp_type type = getZfpType( dataType );

    // allocate meta data for the 1D input array
    zfp_field* field = zfp_field_1d(input, type, numel);


    // allocate meta data for a compressed stream
    zfp_stream* zfp = zfp_stream_open(NULL);

    // set absolute error tolerance
    zfp_stream_set_accuracy(zfp, abs);

   	//allocate buffer for compressed data
    size_t bufsize = zfp_stream_maximum_size(zfp, field);
    output = malloc(bufsize);

    // associate bit stream with allocated buffer
    bitstream* stream = stream_open(output, bufsize);
    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);

    // Compress
    size_t zfpsize = zfp_compress(zfp, field);
    if (!zfpsize)
    {
        std::cout << "compression failed\n";
        return 0;
    }

    zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);

    zfpCompressedSize = zfpsize;
    cbytes = zfpsize;

    cTime.stop();


    log << "\n" << compressorName << " ~ InputBytes: " << dataTypeSize*numel << ", OutputBytes: " << cbytes << ", cRatio: " << (dataTypeSize*numel / (float)cbytes) << ", #elements: " << numel << std::endl;
    log << compressorName << " ~ CompressTime: " << cTime.getDuration() << " s " << std::endl;

    return 1;
}


inline int ZFPCompressor::decompress(void *&input, void *&output, std::string dataType, size_t dataTypeSize, size_t * n)
{
	size_t numel = n[0];
	for (int i = 1; i < 5; i++)
		if (n[i] != 0)
			numel *= n[i];

    // Read in json compression parameters
	double abs = 1E-3;
    std::unordered_map<std::string, std::string>::const_iterator got = compressorParameters.find("abs");
    if( got != compressorParameters.end() )
        if (compressorParameters["abs"] != "")
            abs = strConvert::to_double( compressorParameters["abs"] );

    Timer dTime; 
    dTime.start();
    
    zfp_type type = getZfpType( dataType );

    // allocate meta data for the 1D input array of decompressed data
    output = malloc(numel*dataTypeSize);
    zfp_field* field = zfp_field_1d(output, type, numel);


    // allocate meta data for a compressed stream
    zfp_stream* zfp = zfp_stream_open(NULL);
    zfp_stream_set_accuracy(zfp, abs);

    // allocate buffer for compressed data and transfer data
    size_t bufsize = zfp_stream_maximum_size(zfp, field);
    void  *buffer = malloc(bufsize);
    memcpy(buffer, input, zfpCompressedSize);

    // associate bit stream with allocated buffer
    bitstream* stream = stream_open(buffer, zfpCompressedSize);
    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);


    // DeCompress
    size_t zfpsize  = zfp_decompress(zfp, field);
    if (! zfpsize)
    {
        std::cout << "decompression failed\n";
        return 0;
    }
    
    free(buffer);
    zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);

    dTime.stop();

    log << compressorName << " ~ DecompressTime: " << dTime.getDuration() << " s " << std::endl;

    return 1;
}

inline void ZFPCompressor::close()
{

}

#endif // _ZFP_COMPRESSOR_H_
#endif // CBENCH_HAS_ZFP
