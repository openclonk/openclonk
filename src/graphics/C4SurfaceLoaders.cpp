/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

/* Extension to C4Surface that handles bitmaps in C4Group files */

#include "C4Include.h"
#include "graphics/C4Surface.h"

#include "c4group/C4GroupSet.h"
#include "c4group/C4Group.h"
#include "graphics/StdPNG.h"
#include "lib/StdColors.h"

bool C4Surface::LoadAny(C4Group &hGroup, const char *szName, bool fOwnPal, bool fNoErrIfNotFound, int iFlags)
{
	// Entry name
	char szFilename[_MAX_FNAME+1];
	SCopy(szName,szFilename,_MAX_FNAME);
	char *szExt = GetExtension(szFilename);
	if (!*szExt)
	{
		// no extension: Default to extension that is found as file in group
		const char * const extensions[] = { "png", "bmp", "jpeg", "jpg", nullptr };
		int i = 0; const char *szExt;
		while ((szExt = extensions[i++]))
		{
			EnforceExtension(szFilename, szExt);
			if (hGroup.FindEntry(szFilename)) break;
		}
	}
	// Load surface
	return Load(hGroup,szFilename,fOwnPal,fNoErrIfNotFound,iFlags);
}


bool C4Surface::LoadAny(C4GroupSet &hGroupset, const char *szName, bool fOwnPal, bool fNoErrIfNotFound, int iFlags)
{
	// Entry name
	char szFilename[_MAX_FNAME+1];
	SCopy(szName,szFilename,_MAX_FNAME);
	char *szExt = GetExtension(szFilename);
	C4Group * pGroup;
	if (!*szExt)
	{
		// no extension: Default to extension that is found as file in group
		const char * const extensions[] = { "png", "bmp", "jpeg", "jpg", nullptr };
		int i = 0; const char *szExt;
		while ((szExt = extensions[i++]))
		{
			EnforceExtension(szFilename, szExt);
			pGroup = hGroupset.FindEntry(szFilename);
			if (pGroup) break;
		}
	}
	else
		pGroup = hGroupset.FindEntry(szFilename);
	if (!pGroup) return false;
	// Load surface
	return Load(*pGroup,szFilename,fOwnPal,fNoErrIfNotFound,iFlags);
}

bool C4Surface::Load(C4Group &hGroup, const char *szFilename, bool, bool fNoErrIfNotFound, int iFlags)
{
	int ScaleToSet = 1;
	// Image is scaled?
	StdStrBuf strFilename;
	char strBasename[_MAX_FNAME + 1]; SCopy(szFilename, strBasename, _MAX_FNAME); RemoveExtension(strBasename);
	int32_t extpos; int scale;
	if (((extpos = SCharLastPos('.', strBasename)) > -1) && (sscanf(strBasename+extpos+1, "%d", &scale) == 1))
	{
		// Filename with scale information was passed. Just store scale.
		ScaleToSet = scale;
	}
	else
	{
		// a filename with out scale information was passed in
		// Look for scaled images
		const size_t base_length = std::strlen(strBasename);
		char strExtension[128 + 1]; SCopy(GetExtension(szFilename), strExtension, 128);
		if (strExtension[0])
		{
			char scaled_name[_MAX_PATH+1];
			std::string wildcard(strBasename);
			wildcard += ".*.";
			wildcard += strExtension;
			int max_scale = -1;
			if (hGroup.FindEntry(wildcard.c_str(), scaled_name))
			{
				do
				{
					int scale = -1;
					if (sscanf(scaled_name + base_length + 1, "%d", &scale) == 1)
						if (scale > max_scale)
						{
							max_scale = scale;
							ScaleToSet = max_scale;
							strFilename.Copy(scaled_name);
							szFilename = strFilename.getData();
						}
				}
				while (hGroup.FindNextEntry(wildcard.c_str(), scaled_name));
			}
		}
	}
	// Access entry
	if (!hGroup.AccessEntry(szFilename))
	{
		// file not found
		if (!fNoErrIfNotFound) LogF("%s: %s%c%s", LoadResStr("IDS_PRC_FILENOTFOUND"), hGroup.GetFullName().getData(), (char) DirectorySeparator, szFilename);
		return false;
	}
	bool fSuccess = Read(hGroup, GetExtension(szFilename), iFlags);
	// loading error? log!
	if (!fSuccess)
		LogF("%s: %s%c%s", LoadResStr("IDS_ERR_NOFILE"), hGroup.GetFullName().getData(), (char) DirectorySeparator, szFilename);
	// Work around the broken Default()-convention
	Scale = ScaleToSet;
	// done, success
	return fSuccess;
}

bool C4Surface::Read(CStdStream &hGroup, const char * extension, int iFlags)
{
	// determine file type by file extension and load accordingly
	if (SEqualNoCase(extension, "png"))
		return ReadPNG(hGroup, iFlags);
	else if (SEqualNoCase(extension, "jpeg")
	         || SEqualNoCase(extension, "jpg"))
		return ReadJPEG(hGroup, iFlags);
	else if (SEqualNoCase(extension, "bmp"))
		return ReadBMP(hGroup, iFlags);
	else
		return false;
}

bool C4Surface::ReadPNG(CStdStream &hGroup, int iFlags)
{
	// create mem block
	int iSize=hGroup.AccessedEntrySize();
	BYTE *pData=new BYTE[iSize];
	// load file into mem
	hGroup.Read((void *) pData, iSize);
	// load as png file
	CPNGFile png;
	bool fSuccess=png.Load(pData, iSize);
	// free data
	delete [] pData;
	// abort if loading wasn't successful
	if (!fSuccess) return false;
	// create surface(s) - do not create an 8bit-buffer!
	if (!Create(png.iWdt, png.iHgt, iFlags)) return false;
	// lock for writing data
	if (!Lock()) return false;
	if (!texture)
	{
		Unlock();
		return false;
	}
	// write pixels
	// Get Texture and lock it
	if (!texture->Lock()) return false;
	int maxX = std::min(Wdt, iTexSize);
	int maxY = std::min(Hgt, iTexSize);
	for (int iY = 0; iY < maxY; ++iY)
	{
#ifndef __BIG_ENDIAN__
		if (png.iClrType == PNG_COLOR_TYPE_RGB_ALPHA)
		{
			// Optimize the easy case of a png in the same format as the display
			// 32 bit
			DWORD *pPix=(DWORD *) (((char *) texture->texLock.pBits.get()) + iY * texture->texLock.Pitch);
			memcpy (pPix, png.GetRow(iY), maxX * sizeof(*pPix));
			int iX = maxX;
			while (iX--) { if (((BYTE *)pPix)[3] == 0x00) *pPix = 0x00000000; ++pPix; }
		}
		else
#endif
		{
			// Loop through every pixel and convert
			for (int iX = 0; iX < maxX; ++iX)
			{
				uint32_t dwCol = png.GetPix(iX, iY);
				// if color is fully transparent, ensure it's black
				if (dwCol>>24 == 0x00) dwCol=0x00000000;
				// set pix in surface
				DWORD *pPix=(DWORD *) (((char *) texture->texLock.pBits.get()) + iY * texture->texLock.Pitch + iX * 4);
				*pPix=dwCol;
			}
		}
	}
	
	// unlock
	texture->Unlock();
	Unlock();
	// return if successful
	return fSuccess;
}

bool C4Surface::SavePNG(C4Group &hGroup, const char *szFilename, bool fSaveAlpha, bool fSaveOverlayOnly)
{
	// Using temporary file at C4Group temp path
	char szTemp[_MAX_PATH+1];
	SCopy(C4Group_GetTempPath(),szTemp);
	SAppend(GetFilename(szFilename),szTemp);
	MakeTempFilename(szTemp);
	// Save to temporary file
	if (!C4Surface::SavePNG(szTemp, fSaveAlpha, fSaveOverlayOnly, false)) return false;
	// Move temp file to group
	if (!hGroup.Move(szTemp,GetFilename(szFilename))) return false;
	// Success
	return true;
}

/* JPEG loading */

// Some distributions ship jpeglib.h with extern "C", others don't - gah.
extern "C"
{
/* avoid conflict with conflicting FAR typedefs */
#undef FAR
#include <jpeglib.h>
}
#include <csetjmp>

// Straight from the libjpeg example
struct my_error_mgr
{
	struct jpeg_error_mgr pub;  /* "public" fields */
	jmp_buf setjmp_buffer;  /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

static void my_error_exit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr) cinfo->err;
	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);
	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}
static void my_output_message (j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message) (cinfo, buffer);
	LogF("libjpeg: %s", buffer);
}
static void jpeg_noop (j_decompress_ptr cinfo) {}
static const unsigned char end_of_input = JPEG_EOI;
static boolean fill_input_buffer (j_decompress_ptr cinfo)
{
	// The doc says to give fake end-of-inputs if there is no more data
	cinfo->src->next_input_byte = &end_of_input;
	cinfo->src->bytes_in_buffer = 1;
	return (boolean)true;
}
static void skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	cinfo->src->next_input_byte += num_bytes;
	cinfo->src->bytes_in_buffer -= num_bytes;
	if (cinfo->src->bytes_in_buffer <= 0)
	{
		cinfo->src->next_input_byte = &end_of_input;
		cinfo->src->bytes_in_buffer = 1;
	}
}

bool C4Surface::ReadJPEG(CStdStream &hGroup, int iFlags)
{
	// create mem block
	size_t size=hGroup.AccessedEntrySize();
	unsigned char *pData=new unsigned char[size];
	// load file into mem
	hGroup.Read(pData, size);
	// stuff for libjpeg
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	JSAMPARRAY buffer;    /* Output row buffer */
	int row_stride;   /* physical row width in output buffer */
	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	jerr.pub.output_message = my_output_message;
	// apparantly, this is needed so libjpeg does not exit() the engine away
	if (setjmp(jerr.setjmp_buffer))
	{
		// some fatal error
		jpeg_destroy_decompress(&cinfo);
		delete [] pData;
		return false;
	}
	jpeg_create_decompress(&cinfo);

	// no fancy function calling
	jpeg_source_mgr blub;
	cinfo.src = &blub;
	blub.next_input_byte = pData;
	blub.bytes_in_buffer = size;
	blub.init_source = jpeg_noop;
	blub.fill_input_buffer = fill_input_buffer;
	blub.skip_input_data = skip_input_data;
	blub.resync_to_restart = jpeg_resync_to_restart;
	blub.term_source = jpeg_noop;

	// a missing image is an error
	jpeg_read_header(&cinfo, (boolean)true);

	// Let libjpeg convert for us
	cinfo.out_color_space = JCS_RGB;
	jpeg_start_decompress(&cinfo);

	// create surface(s) - do not create an 8bit-buffer!
	if (!Create(cinfo.output_width, cinfo.output_height, iFlags)) return false;
	// JSAMPLEs per row in output buffer
	row_stride = cinfo.output_width * cinfo.output_components;
	// Make a one-row-high sample array that will go away at jpeg_destroy_decompress
	buffer = (*cinfo.mem->alloc_sarray)
	         ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
	// lock for writing data
	if (!Lock()) return false;
	while (cinfo.output_scanline < cinfo.output_height)
	{
		// read an 1-row-array of scanlines
		jpeg_read_scanlines(&cinfo, buffer, 1);
		// put the data in the image
		for (unsigned int i = 0; i < cinfo.output_width; ++i)
		{
			const unsigned char * const start = buffer[0] + i * cinfo.output_components;
			const uint32_t c = C4RGB(*start, *(start + 1), *(start + 2));
			SetPixDw(i, cinfo.output_scanline - 1, c);
		}
	}
	// unlock
	Unlock();
	// clean up
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	// free data
	delete [] pData;
	// return if successful
	return true;
}
