/** 
 * @file lldrawpoolterrain.h
 * @brief LLDrawPoolTerrain class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLDRAWPOOLTERRAIN_H
#define LL_LLDRAWPOOLTERRAIN_H

#include "lldrawpool.h"

class LLDrawPoolTerrain : public LLFacePool
{
	LLPointer<LLViewerTexture> mTexturep;
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_TEXCOORD1 |
							LLVertexBuffer::MAP_TEXCOORD2 |
							LLVertexBuffer::MAP_TEXCOORD3
	};

	U32 getVertexDataMask() override;
	static S32 getDetailMode();

	LLDrawPoolTerrain(LLViewerTexture *texturep);
	virtual ~LLDrawPoolTerrain();

	/*virtual*/ LLDrawPool *instancePool() override;

	/*virtual*/ S32 getNumDeferredPasses() override { return 1; }
	/*virtual*/ void beginDeferredPass(S32 pass) override;
	/*virtual*/ void endDeferredPass(S32 pass) override;
	/*virtual*/ void renderDeferred(S32 pass) override;

	/*virtual*/ S32 getNumShadowPasses() override { return 1; }
	/*virtual*/ void beginShadowPass(S32 pass) override;
	/*virtual*/ void endShadowPass(S32 pass) override;
	/*virtual*/ void renderShadow(S32 pass) override;

	/*virtual*/ void render(S32 pass = 0) override;
	/*virtual*/ void prerender() override;
	/*virtual*/ void beginRenderPass( S32 pass ) override;
	/*virtual*/ void endRenderPass( S32 pass ) override;
	/*virtual*/ void dirtyTextures(const std::set<LLViewerFetchedTexture*>& textures) override;
	/*virtual*/ LLViewerTexture *getTexture() override;
	/*virtual*/ LLViewerTexture *getDebugTexture() override;
	/*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display

	LLPointer<LLViewerTexture> m2DAlphaRampImagep;

	static S32 sDetailMode;
	static F32 sDetailScale; // meters per texture

protected:
	void renderSimple();
	void renderOwnership();

	void renderFull2TU();
	void renderFull4TU();
	void renderFullShader();
	void drawLoop();

private:
	void renderParcelOwnersOverlay();
};

#endif // LL_LLDRAWPOOLSIMPLE_H
