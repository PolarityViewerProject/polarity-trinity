/**
 * @file llwlparamset.h
 * @brief Interface for the LLWaterParamSet class.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#ifndef LL_WATER_PARAM_SET_H
#define LL_WATER_PARAM_SET_H

#include "v4math.h"
#include "v4color.h"
#include "llviewershadermgr.h"
#include "llstringtable.h"

class LLWaterParamSet;

/// A class representing a set of parameter values for the Water shaders.
class LLWaterParamSet 
{
	friend class LLWaterParamManager;

public:
	std::string mName;	
	
private:

	LLSD mParamValues;
	std::vector<LLStaticHashedString> mParamHashedNames;

	void updateHashedNames();

public:

	LLWaterParamSet();

	/// Bind this set of parameter values to the uniforms of a particular shader.
	void update(LLGLSLShader * shader) const;

	/// set the total llsd
	void setAll(const LLSD& val);
	
	/// get the total llsd
	const LLSD& getAll();		

	/// Set a float parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param x			The float value to set.
	void set(const std::string& paramName, float x);

	/// Set a float2 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param x			The x component's value to set.
	/// \param y			The y component's value to set.
	void set(const std::string& paramName, float x, float y);

	/// Set a float3 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param x			The x component's value to set.
	/// \param y			The y component's value to set.
	/// \param z			The z component's value to set.
	void set(const std::string& paramName, float x, float y, float z);

	/// Set a float4 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param x			The x component's value to set.
	/// \param y			The y component's value to set.
	/// \param z			The z component's value to set.
	/// \param w			The w component's value to set.
	void set(const std::string& paramName, float x, float y, float z, float w);

	/// Set a float4 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param val			An array of the 4 float values to set the parameter to.
	void set(const std::string& paramName, const float * val);

	/// Set a float4 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param val			A struct of the 4 float values to set the parameter to.
	void set(const std::string& paramName, const LLVector4 & val);

	/// Set a float4 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param val			A struct of the 4 float values to set the parameter to.
	void set(const std::string& paramName, const LLColor4 & val);

	/// Get a float4 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param error		A flag to set if it's not the proper return type
	LLVector4 getVector4(const std::string& paramName, bool& error);

	/// Get a float3 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param error		A flag to set if it's not the proper return type
	LLVector3 getVector3(const std::string& paramName, bool& error);
	
	/// Get a float2 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param error		A flag to set if it's not the proper return type
	LLVector2 getVector2(const std::string& paramName, bool& error);
	
	/// Get an integer parameter
	/// \param paramName	The name of the parameter to set.
	/// \param error		A flag to set if it's not the proper return type	
	F32 getFloat(const std::string& paramName, bool& error);
		
	/// interpolate two parameter sets
	/// \param src			The parameter set to start with
	/// \param dest			The parameter set to end with
	/// \param weight		The amount to interpolate
	void mix(LLWaterParamSet& src, LLWaterParamSet& dest, 
		F32 weight);

};

inline void LLWaterParamSet::setAll(const LLSD& val)
{
	if(val.isMap()) {
		LLSD::map_const_iterator mIt = val.beginMap();
		for(; mIt != val.endMap(); mIt++)
		{
			mParamValues[mIt->first] = mIt->second;
		}
	}
	updateHashedNames();
}

inline void LLWaterParamSet::updateHashedNames()
{
	mParamHashedNames.clear();
	// Iterate through values
	for(LLSD::map_iterator iter = mParamValues.beginMap(); iter != mParamValues.endMap(); ++iter)
	{
		mParamHashedNames.push_back(LLStaticHashedString(iter->first));
	}
}

inline const LLSD& LLWaterParamSet::getAll()
{
	return mParamValues;
}

#endif // LL_WaterPARAM_SET_H
