/** 
 * @file lldrawpooltree.h
 * @brief LLDrawPoolTree class definition
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

#ifndef LL_LLDRAWPOOLTREE_H
#define LL_LLDRAWPOOLTREE_H

#include "lldrawpool.h"

class LLDrawPoolTree : public LLFacePool
{
	LLPointer<LLViewerTexture> mTexturep;
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0							
	};

	U32 getVertexDataMask() override { return VERTEX_DATA_MASK; }

	LLDrawPoolTree(LLViewerTexture *texturep);

	/*virtual*/ LLDrawPool *instancePool() override;

	/*virtual*/ void prerender() override;

	/*virtual*/ S32 getNumDeferredPasses() override { return 1; }
	/*virtual*/ void beginDeferredPass(S32 pass) override;
	/*virtual*/ void endDeferredPass(S32 pass) override;
	/*virtual*/ void renderDeferred(S32 pass) override;

	/*virtual*/ S32 getNumShadowPasses() override { return 1; }
	/*virtual*/ void beginShadowPass(S32 pass) override;
	/*virtual*/ void endShadowPass(S32 pass) override;
	/*virtual*/ void renderShadow(S32 pass) override;

	/*virtual*/ void beginRenderPass( S32 pass ) override;
	/*virtual*/ void render(S32 pass = 0) override;
	/*virtual*/ void endRenderPass( S32 pass ) override;
	/*virtual*/ S32	 getNumPasses() override { return 1; }
	/*virtual*/ BOOL verify() const override;
	/*virtual*/ LLViewerTexture *getTexture() override;
	/*virtual*/ LLViewerTexture *getDebugTexture() override;
	/*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display

	static S32 sDiffTex;
};

#endif // LL_LLDRAWPOOLTREE_H
