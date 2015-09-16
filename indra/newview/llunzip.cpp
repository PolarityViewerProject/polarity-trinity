/**
 * @file llunzip.cpp
 * @brief Minizip wrapper
 *
 * Copyright (c) 2015, Cinder Roxley <cinder@sdf.org>
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "llviewerprecompiledheaders.h"
#include "llunzip.h"

S32 CASE_SENTITIVITY = 1;
size_t WRITE_BUFFER_SIZE = 8192;

LLUnZip::LLUnZip(const std::string& filename)
:	mFilename(filename)
,	mValid(false)
{
	mZipfile = open(filename);
	mValid = (mZipfile != nullptr);
}

LLUnZip::~LLUnZip()
{
	close();
}

bool LLUnZip::extract(const std::string& path)
{
	S32 error = UNZ_OK;
	unz_global_info64 gi;
	
	unzGoToFirstFile(mZipfile);
	
	error = unzGetGlobalInfo64(mZipfile, &gi);
	if (error != UNZ_OK)
	{
		LL_WARNS("UNZIP") << "Error unzipping " << mFilename << " - code: " << error << LL_ENDL;
		return false;
	}
	
	for (uLong i = 0; i < gi.number_entry ; i++)
	{
		if (extractCurrentFile(path) != UNZ_OK)
			break;
		if ((i + 1) < gi.number_entry)
		{
			error = unzGoToNextFile(mZipfile);
			if (error != UNZ_OK)
			{
				LL_WARNS("UNZIP") << "Error unzipping " << mFilename << " - code: " << error << LL_ENDL;
				break;
			}
		}
	}
	
	return true;
}

S32 LLUnZip::extractCurrentFile(const std::string& path)
{
	S32 error = UNZ_OK;
	char filename_inzip[256];
	unz_file_info64 file_info;
	
	error = unzGetCurrentFileInfo64(mZipfile, &file_info, filename_inzip, sizeof(filename_inzip), nullptr, 0, nullptr, 0);
	if (error != UNZ_OK)
	{
		LL_WARNS("UNZIP") << "Error unzipping " << mFilename << " - code: " << error << LL_ENDL;
		return error;
	}
	
	const std::string& inzip(filename_inzip);
	const std::string& write_filename = gDirUtilp->add(path, inzip);
	
	LL_INFOS("UNZIP") << "Unpacking " << inzip << LL_ENDL;
	if (LLStringUtil::endsWith(inzip, "/") || LLStringUtil::endsWith(inzip, "\\"))
	{
		if (!LLFile::isdir(write_filename))
		{
			if (LLFile::mkdir(write_filename) != 0)
				LL_WARNS("UNZIP") << "Couldn't create directory at " << write_filename << LL_ENDL;
		}
		return error;
	}
	
	void* buf;
	size_t size_buf = WRITE_BUFFER_SIZE;
	buf = (void*)malloc(size_buf);
	if (buf == nullptr)
	{
		LL_WARNS("UNZIP") << "Error allocating memory!" << LL_ENDL;
		return UNZ_INTERNALERROR;
	}

	error = unzOpenCurrentFile(mZipfile);
	if (error != UNZ_OK)
	{
		LL_WARNS("UNZIP") << "Error unzipping " << mFilename << " - code: " << error << LL_ENDL;
	}
	
	LLFILE* outfile = LLFile::fopen(write_filename, "wb");
	if (outfile == nullptr)
	{
		LL_WARNS("UNZIP") << "Error opening " << write_filename << " for writing" << LL_ENDL;
	}
	
	do
	{
		error = unzReadCurrentFile(mZipfile, buf, size_buf);
		if (error < UNZ_OK)
		{
			LL_WARNS("UNZIP") << "Error unzipping " << mFilename << " - code: " << error << LL_ENDL;
			break;
		}
		else if (error > UNZ_OK)
		{
			if (fwrite(buf, error, 1, outfile) != 1)
			{
				LL_WARNS("UNZIP") << "Error writing out " << mFilename << LL_ENDL;
				error = UNZ_ERRNO;
				break;
			}
		}
	}
	while (error > 0);
	fclose(outfile);
	
	if (error == UNZ_OK)
	{
		error = unzCloseCurrentFile(mZipfile);
		if (error != UNZ_OK)
			LL_WARNS("UNZIP") << "Error unzipping " << mFilename << " - code: " << error << LL_ENDL;
	}
	else
		unzCloseCurrentFile(mZipfile);
	
	free(buf);
	return error;
}

bool LLUnZip::extractFile(const std::string& file_to_extract, char *buf, size_t bufsize)
{
	if (unzLocateFile(mZipfile, file_to_extract.c_str(), CASE_SENTITIVITY) != UNZ_OK)
	{
		LL_WARNS("UNZIP") << file_to_extract << " was not found in " << mFilename << LL_ENDL;
		return false;
	}
	
	S32 error = UNZ_OK;
	
	char filename_inzip[256];
	unz_file_info64 file_info;
	error = unzGetCurrentFileInfo64(mZipfile, &file_info, filename_inzip, sizeof(filename_inzip), nullptr, 0, nullptr, 0);
	error = unzOpenCurrentFile(mZipfile);
	error = unzReadCurrentFile(mZipfile, buf, bufsize);
	unzCloseCurrentFile(mZipfile);
	if (error != UNZ_OK)
	{
		LL_WARNS("UNZIP") << "Error unzipping " << mFilename << " - code: " << error << LL_ENDL;
		return false;
	}
	
	return true;
}

size_t LLUnZip::getSizeFile(const std::string& file_to_size)
{
	if (unzLocateFile(mZipfile, file_to_size.c_str(), CASE_SENTITIVITY) != UNZ_OK)
	{
		LL_WARNS("UNZIP") << file_to_size << " was not found in " << mFilename << LL_ENDL;
		return 0;
	}
	
	S32 error = UNZ_OK;
	char filename_inzip[256];
	unz_file_info64 file_info;
	error = unzGetCurrentFileInfo64(mZipfile, &file_info, filename_inzip, sizeof(filename_inzip), nullptr, 0, nullptr, 0);
	if (error != UNZ_OK)
	{
		LL_WARNS("UNZIP") << "Error unzipping " << mFilename << " - code: " << error << LL_ENDL;
		return 0;
	}
	return file_info.uncompressed_size;
}

unzFile LLUnZip::open(const std::string& filename)
{
#ifdef USEWIN32IOAPI
	zlib_filefunc64_def ffunc;
#endif // USEWIN32IOAPI
	
#ifdef USEWIN32IOAPI
	fill_win32_filefunc64A(&ffunc);
	mZipfile = unzOpen2_64(filename.c_str(), &ffunc);
#else // !USEWIN32IOAPI
	mZipfile = unzOpen64(filename.c_str());
#endif // !USEWIN32IOAPI
	
	if (mZipfile == nullptr)
		LL_WARNS("UNZIP") << "Failed to open " << mFilename << LL_ENDL;
	
	return mZipfile;
}

void LLUnZip::close()
{
	unzClose(mZipfile);
	mZipfile = nullptr;
	mValid = false;
}
