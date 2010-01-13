/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002  Sven Eberhardt
 * Copyright (c) 2005  Günther Brammer
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */
// png file reading functionality

#include "C4Include.h"
#include <StdPNG.h>

CPNGFile *pCurrPng=NULL; // global crap for file-reading callback

// png reading proc
void PNGAPI CPNGReadFn(png_structp png_ptr, png_bytep data, size_t length)
	{
	// read from current pnt
	if (!pCurrPng) return;
	pCurrPng->Read(data, length);
	}

void CPNGFile::Read(unsigned char *pData, int iLength)
	{
	// fixme: overflow check schould be done here
	// simply copy into buffer
	memcpy(pData, pFilePtr, iLength);
	// advance file ptr
	pFilePtr+=iLength;
	}

bool CPNGFile::DoLoad()
	{
	// set current png ptr
	pCurrPng=this;
	// reset file ptr
	pFilePtr=pFile;
	// check file
	if (png_sig_cmp((unsigned char *) pFilePtr, 0, 8)) return false;
	// setup png for reading
	fWriteMode=false;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) return false;
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) return false;
	end_info = png_create_info_struct(png_ptr);
	if (!end_info) return false;
	// error handling
	if (setjmp(png_ptr->jmpbuf)) return false;
	// set file-reading proc
	png_set_read_fn(png_ptr, png_get_io_ptr(png_ptr), &CPNGReadFn);
	// read info
	png_read_info(png_ptr, info_ptr);
	// assign local vars
	png_uint_32 uWdt = iWdt, uHgt = iHgt;
	png_get_IHDR(png_ptr, info_ptr, &uWdt, &uHgt, &iBPC, &iClrType, &iIntrlcType, &iCmprType, &iFltrType);
	// convert to bgra
	if (iClrType == PNG_COLOR_TYPE_PALETTE && iBPC <= 8) png_set_expand(png_ptr);
	if (iClrType == PNG_COLOR_TYPE_GRAY && iBPC < 8) png_set_expand(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_expand(png_ptr);
	if (iBPC == 16) png_set_strip_16(png_ptr);
	if (iBPC < 8) png_set_packing(png_ptr);
	if (iClrType == PNG_COLOR_TYPE_GRAY || iClrType == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png_ptr);
	/*if (iClrType == PNG_COLOR_TYPE_RGB || iClrType == PNG_COLOR_TYPE_RGB_ALPHA)*/ png_set_bgr(png_ptr);
	// update info
	png_read_update_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &uWdt, &uHgt, &iBPC, &iClrType, &iIntrlcType, &iCmprType, &iFltrType);
	iWdt = uWdt; iHgt = uHgt;
	// get bit depth/check format
	switch (iClrType)
		{
		case PNG_COLOR_TYPE_RGB: iPixSize=3; break;
		case PNG_COLOR_TYPE_RGB_ALPHA: iPixSize=4; break;
		default: return false; // unrecognized image
		}
	// allocate mem for the whole image
	iRowSize=png_get_rowbytes(png_ptr, info_ptr);
	pImageData = new unsigned char[iRowSize*iHgt];
	// create row ptr buffer
	unsigned char **ppRowBuf = new unsigned char *[iHgt];
	unsigned char **ppRows=ppRowBuf; unsigned char *pRow=pImageData;
	for (unsigned int i=0; i<iHgt; ++i,pRow+=iRowSize) *ppRows++=pRow;
	// read image!
	png_read_image(png_ptr, ppRowBuf);
	// free row buffer
	delete [] ppRowBuf;
	// success
	return true;
	}

CPNGFile::CPNGFile()
	{
	Default();
	}

void CPNGFile::Default()
	{
	// zero fields
	pFile=NULL;
	fpFileOwned=false;
	pFilePtr=NULL;
	png_ptr=NULL;
	info_ptr=end_info=NULL;
	pImageData=NULL;
	iRowSize=0;
	iPixSize=0;
	fp=NULL;
	}

CPNGFile::~CPNGFile()
	{
	// clear
	Clear();
	}

void CPNGFile::ClearPngStructs()
	{
	// clear internal png ptrs
	if (png_ptr || info_ptr || end_info)
		{
		if (fWriteMode)
			png_destroy_write_struct(&png_ptr, &info_ptr);
		else
			png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		}
	png_ptr=NULL;
	info_ptr=end_info=NULL;
	fWriteMode=false;
	}

void CPNGFile::Clear()
	{
	// free image data
	if (pImageData) { delete [] pImageData; pImageData=NULL; }
	// clear internal png ptrs
	ClearPngStructs();
	// free file ptr if owned
	if (pFile && fpFileOwned) delete [] pFile; pFile=NULL;
	// reset fields
	fpFileOwned=false;
	pFilePtr=NULL;
	iRowSize=0;
	iPixSize=0;
	// close file if open
	if (fp) { fclose(fp); fp=NULL; }
	}

bool CPNGFile::Load(unsigned char *pFile, int iSize)
	{
	// clear any previously loaded file
	Clear();
	// store file ptr as not owned
	this->pFile = pFile;
	iFileSize=iSize;
	fpFileOwned=false;
	// perform the loading
	if (!DoLoad())
		{
		Clear();
		return false;
		}
	// reset file-field
	this->pFile = NULL; iFileSize=0;
	// success
	return true;
	}

DWORD CPNGFile::GetPix(int iX, int iY)
	{
	// image loaded?
	if (!pImageData) return 0;
	// return pixel value
	unsigned char *pPix=pImageData+iY*iRowSize+iX*iPixSize;
	switch (iClrType)
		{
		case PNG_COLOR_TYPE_RGB:
			return 0xff << 24 | RGB(pPix[0], pPix[1], pPix[2]);
		case PNG_COLOR_TYPE_RGB_ALPHA:
            return pPix[3] << 24 | RGB(pPix[0], pPix[1], pPix[2]);
		}
	return 0;
	}

bool CPNGFile::Create(int iWdt, int iHgt, bool fAlpha)
	{
	// catch invalid size
	if (iWdt<=0 || iHgt<=0) return false;
	// clear current image in mem
	Clear();
	// set dimensions
	this->iWdt=iWdt; this->iHgt=iHgt;
	// set color type
	iBPC = 8;
	iClrType = fAlpha ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB;
	iPixSize = fAlpha ? 4 : 3;
	// set interlacing, filters and stuff
	iIntrlcType = PNG_INTERLACE_NONE;
	iCmprType = PNG_COMPRESSION_TYPE_DEFAULT;
	iFltrType = PNG_FILTER_TYPE_DEFAULT;
	// calculate rowbytes
	int iBPP = (fAlpha ? 4 : 3) * iBPC;
	iRowSize = (iBPP*iWdt+7)>>3;
	// create image data
	pImageData = new unsigned char[iHgt * iRowSize];
	// success
	return true;
	}

bool CPNGFile::SetPix(int iX, int iY, DWORD dwValue)
	{
	// image created?
	if (!pImageData) return false;
	// set pixel value
	unsigned char *pPix=pImageData+iY*iRowSize+iX*iPixSize;
	switch (iClrType)
		{
		case PNG_COLOR_TYPE_RGB: // RGB: set r, g and b values
			pPix[0] = GetRValue(dwValue);
			pPix[1] = GetGValue(dwValue);
			pPix[2] = GetBValue(dwValue);
			return true;
		case PNG_COLOR_TYPE_RGB_ALPHA: // RGBA: simply set in mem
			*(unsigned long *) pPix = dwValue;
			return true;
		}
	return false;
	}

bool CPNGFile::Save(const char *szFilename)
	{
	// regular file saving - first, there has to be a buffer
	if (!pImageData) return false;
	// open the file
	fp = fopen(szFilename, "wb"); if (!fp) return false;
	// clear any previously initialized png-structs (e.g. by reading)
	ClearPngStructs();
	// reinit them for writing
	fWriteMode=true;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) { Clear(); return false; }
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) { Clear(); return false; }
	// error handling
	if (setjmp(png_ptr->jmpbuf)) { Clear(); return false; }
	// io initialization
	png_init_io(png_ptr, fp);
	// compression stuff
	png_set_filter(png_ptr, 0, PNG_FILTER_NONE | PNG_FILTER_SUB | PNG_FILTER_PAETH);
	png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
	png_set_compression_mem_level(png_ptr, 8);
	png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
	png_set_compression_window_bits(png_ptr, 15);
	png_set_compression_method(png_ptr, 8);
	// set header
	png_set_IHDR(png_ptr, info_ptr, iWdt, iHgt, iBPC, iClrType, iIntrlcType, iCmprType, iFltrType);
	// double-check our calculated row size
	int iRealRowSize=png_get_rowbytes(png_ptr, info_ptr);
	if (iRealRowSize != iRowSize)
		{
		// this won't go well, so better abort
		Clear(); return false;
		}
	// write png header
	png_write_info(png_ptr, info_ptr);
	// image data is given as bgr...
	png_set_bgr(png_ptr);
	// create row array
	unsigned char **ppRowBuf = new unsigned char *[iHgt];
	unsigned char **ppRows=ppRowBuf; unsigned char *pRow=pImageData;
	for (unsigned int i=0; i<iHgt; ++i,pRow+=iRowSize) *ppRows++=pRow;
	// write image
	png_write_image(png_ptr, ppRowBuf);
	// free row buffer
	delete [] ppRowBuf;
	// write end struct
	png_write_end(png_ptr, info_ptr);
	// finally, close the file
	fclose(fp); fp = NULL;
	// clear png structs
	ClearPngStructs();
	// success!
	return true;
	}

int CPNGFile::GetBitsPerPixel()
	{
	switch (iClrType)
		{
		case PNG_COLOR_TYPE_RGB: return 24;
		case PNG_COLOR_TYPE_RGB_ALPHA: return 32;
		}
	return 0;
	}
