/** 
 * @file llversioninfo.h
 * @brief Routines to access the viewer version and build information
 * @author Martin Reddy
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLVERSIONINFO_H
#define LL_LLVERSIONINFO_H

///
/// This API provides version information for the viewer.  This
/// includes access to the major, minor, patch, and build integer
/// values, as well as human-readable string representations. All
/// viewer code that wants to query the current version should 
/// use this API.
///
class LLVersionInfo
{
public:
	/// return the major verion number as an integer
	static S32 getMajor();

	/// return the minor verion number as an integer
	static S32 getMinor();

	/// return the patch verion number as an integer
	static S32 getPatch();

	/// return the build number as an integer
	static S32 getBuild();

	/// return the full viewer version as a string like "2.0.0.200030"
	static const std::string &getVersion();

	/// return the viewer version as a string like "2.0.0"
	static const std::string &getShortVersion();

	/// return the viewer version and channel as a string
	/// like "Second Life Release 2.0.0.200030"
	static const std::string &getChannelAndVersion();

	// <polarity> PVData
	// This function is functionally equivalent to GetChannelAndVersion as of 2015-07-22.
	// We do however define this function as its own entity to ensure that any change
	// done upstream that could affect the syntax of those functions does not break
	// the parts of our infrastructure which depend on this specific syntax.
	// This returns something like "Polarity Test 5.7.22 (33738)"
	static const std::string &getChannelAndVersionStatic();
	static const std::string &getCompiledChannel();
	// </polarity> PVData

	// return the latest Linden Lab release we merged
	static const std::string &getLastLindenRelease();

	/// return the channel name, e.g. "Second Life"
	static const std::string &getChannel();
	
    /// return the CMake build type
    static const std::string &getBuildConfig();

    /// Return the short hash of the commit this build was made from
    static const std::string &getBuildCommitHash();
    /// Return the short hash of the commit this build was made from
    static const std::string &getBuildCommitHashLong();

    /// Return the build ID for this binary
    static const std::string &getBuildNumber();

	/// reset the channel name used by the viewer.
	static void resetChannel(const std::string& channel);

    typedef enum
    {
        TEST_VIEWER,
        PROJECT_VIEWER,
        BETA_VIEWER,
        RELEASE_VIEWER
    } ViewerMaturity;
    static ViewerMaturity getViewerMaturity();
    static const std::string getBuildDate();
    static const std::string getBuildTime();
};

#endif
