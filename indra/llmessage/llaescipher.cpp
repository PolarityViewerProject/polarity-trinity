// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/*
 * @file llaescipher.cpp
 * @brief AES wrapper
 *
 * Copyright (c) 2014, Cinder Roxley <cinder@sdf.org>
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

#include "linden_common.h"
#include "llaescipher.h"
#include <openssl/evp.h>
#include <openssl/aes.h>

LLAESCipher::LLAESCipher(const U8* secret, size_t secret_size, const U8* iv, size_t iv_size)
:	LLCipher()
{
	llassert(secret);
	
	mSecretSize = secret_size;
	mSecret = new U8[mSecretSize];
	memcpy(mSecret, secret, mSecretSize);
	
	mInitialVectorSize = iv_size;
	mInitialVector = new U8[mInitialVectorSize];
	memcpy(mInitialVector, iv, mInitialVectorSize);
}

LLAESCipher::~LLAESCipher()
{
	delete [] mSecret;
	mSecret = NULL;
	
	delete [] mInitialVector;
	mInitialVector = NULL;
}

// virtual
U32 LLAESCipher::encrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len)
{
	if (!src || !src_len || !dst || !dst_len) return 0;
	if (src_len > dst_len) return 0;
	
	// OpenSSL uses "cipher contexts" to hold encryption parameters.
    EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
	
	EVP_EncryptInit_ex(context, EVP_aes_256_ofb(), NULL, NULL, NULL);
	EVP_CIPHER_CTX_set_key_length(context, (int)mSecretSize);
	
	EVP_EncryptInit_ex(context, NULL, NULL, mSecret, mInitialVector);
	
    int blocksize = EVP_CIPHER_CTX_block_size(context);
    int keylen = EVP_CIPHER_CTX_key_length(context);
    int iv_length = EVP_CIPHER_CTX_iv_length(context);
    LL_DEBUGS("Crypto") << "Blocksize " << blocksize << " keylen " << keylen << " iv_len " << iv_length << LL_ENDL;
	
	int output_len = 0;
	int temp_len = 0;
	if (!EVP_EncryptUpdate(context,
						   dst,
						   &output_len,
						   src,
						   src_len))
	{
		LL_WARNS("Crypto") << "EVP_EncryptUpdate failure" << LL_ENDL;
		goto AES_ERROR;
	}
	
	// There may be some final data left to encrypt if the input is
	// not an exact multiple of the block size.
	if (!EVP_EncryptFinal_ex(context, (unsigned char*)(dst + output_len), &temp_len))
	{
		LL_WARNS("Crypto") << "EVP_EncryptFinal failure" << LL_ENDL;
		goto AES_ERROR;
	}
	output_len += temp_len;
	
	EVP_CIPHER_CTX_free(context);
	return output_len;
	
AES_ERROR:
	EVP_CIPHER_CTX_free(context);
	return 0;
}

// virtual
U32 LLAESCipher::decrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len)
{
	if (!src || !src_len || !dst || !dst_len) return 0;
	if (src_len > dst_len) return 0;
	
	EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
	
	EVP_DecryptInit_ex(context, EVP_aes_256_ofb(), NULL, NULL, NULL);
	EVP_CIPHER_CTX_set_key_length(context, (int)mSecretSize);
	
	EVP_DecryptInit_ex(context, NULL, NULL, mSecret, mInitialVector);
	
    int blocksize = EVP_CIPHER_CTX_block_size(context);
    int keylen = EVP_CIPHER_CTX_key_length(context);
    int iv_length = EVP_CIPHER_CTX_iv_length(context);
    LL_DEBUGS("AES") << "Blocksize " << blocksize << " keylen " << keylen << " iv_len " << iv_length << LL_ENDL;
	
	int out_len = 0;
	int tmp_len = 0;
	if (!EVP_DecryptUpdate(context, dst, &out_len, src, src_len))
	{
		LL_WARNS("AES") << "EVP_DecryptUpdate failure" << LL_ENDL;
		goto AES_ERROR;
	}
	if (!EVP_DecryptFinal_ex(context, dst + out_len, &tmp_len))
	{
		LL_WARNS("AES") << "EVP_DecryptFinal failure" << LL_ENDL;
		goto AES_ERROR;
	}
	
	out_len += tmp_len;
	
	EVP_CIPHER_CTX_free(context);
	return out_len;
	
AES_ERROR:
	EVP_CIPHER_CTX_free(context);
	return 0;
}

// virtual
U32 LLAESCipher::requiredEncryptionSpace(U32 len) const
{
	len += AES_BLOCK_SIZE;
	len -= (len % AES_BLOCK_SIZE);
	return len;
}
