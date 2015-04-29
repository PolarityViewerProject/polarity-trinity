/**
 * @file alpanelquicksettingspulldown.h
 * @brief Quick Settings popdown panel
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Alchemy Viewer Source Code
 * Copyright (C) 2013-2014, Alchemy Viewer Project.
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
 * $/LicenseInfo$
 */

#ifndef AL_ALPANELQUICKSETTINGSPULLDOWN_H
#define AL_ALPANELQUICKSETTINGSPULLDOWN_H

#include "llpanel.h"

class LLFrameTimer;

class ALPanelQuickSettingsPulldown : public LLPanel
{
public:
	ALPanelQuickSettingsPulldown();
	/*virtual*/ void draw();
	/*virtual*/ void onMouseEnter(S32 x, S32 y, MASK mask);
	/*virtual*/ void onMouseLeave(S32 x, S32 y, MASK mask);
	/*virtual*/ void onTopLost();
	/*virtual*/ void onVisibilityChange(BOOL new_visibility);

private:
	LLFrameTimer mHoverTimer;
};

#endif // AL_ALPANELQUICKSETTINGSPULLDOWN_H