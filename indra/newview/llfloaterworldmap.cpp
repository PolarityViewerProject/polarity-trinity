// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/** 
 * @file llfloaterworldmap.cpp
 * @author James Cook, Tom Yedwab
 * @brief LLFloaterWorldMap class implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

/*
 * Map of the entire world, with multiple background images,
 * avatar tracking, teleportation by double-click, etc.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterworldmap.h"

#include "alfloaterregiontracker.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llbutton.h"
#include "llcallingcard.h"
#include "llcombobox.h"
#include "llviewercontrol.h"
#include "llcommandhandler.h"
#include "lldraghandle.h"
//#include "llfirstuse.h"
#include "llfloaterreg.h"		// getTypedInstance()
#include "llfocusmgr.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventoryobserver.h"
#include "lllandmarklist.h"
#include "llsearcheditor.h"
#include "llnotificationsutil.h"
#include "llregionhandle.h"
#include "llscrolllistctrl.h"
#include "llslurl.h"
#include "lltextbox.h"
#include "lltracker.h"
#include "lltrans.h"
#include "llviewerinventory.h"	// LLViewerInventoryItem
#include "llviewermenu.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewertexture.h"
#include "llviewerwindow.h"
#include "llworldmap.h"
#include "llworldmapmessage.h"
#include "llworldmapview.h"
#include "llappviewer.h"
#include "llmapimagetype.h"
#include "llweb.h"

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lliconctrl.h"
#include "llsliderctrl.h"
#include "message.h"
#include "llwindow.h"			// copyTextToClipboard()
#include <algorithm>

//---------------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------------
static const F32 MAP_ZOOM_TIME = 0.2f;

// Merov: we switched from using the "world size" (which varies depending where the user went) to a fixed
// width of 512 regions max visible at a time. This makes the zoom slider works in a consistent way across
// sessions and doesn't prevent the user to pan the world if it was to grow a lot beyond that limit.
// Currently (01/26/09), this value allows the whole grid to be visible in a 1024x1024 window.
static const S32 MAX_VISIBLE_REGIONS = 512;

// It would be more logical to have this inside the method where it is used but to compile under gcc this
// struct has to be here.
struct SortRegionNames
{
	inline bool operator ()(std::pair <U64, LLSimInfo*> const& _left, std::pair <U64, LLSimInfo*> const& _right)
	{
		return(LLStringUtil::compareInsensitive(_left.second->getName(), _right.second->getName()) < 0);
	}
};

enum EPanDirection
{
	PAN_UP,
	PAN_DOWN,
	PAN_LEFT,
	PAN_RIGHT
};

// Values in pixels per region
static const F32 ZOOM_MAX = 128.f;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

// handle secondlife:///app/worldmap/{NAME}/{COORDS} URLs
class LLWorldMapHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLWorldMapHandler() : LLCommandHandler("worldmap", UNTRUSTED_THROTTLE ) { }
	
	bool handle(const LLSD& params, const LLSD& query_map,
				LLMediaCtrl* web) override
	{
		if (!LLUI::sSettingGroups["config"]->getBOOL("EnableWorldMap"))
		{
			LLNotificationsUtil::add("NoWorldMap", LLSD(), LLSD(), std::string("SwitchToStandardSkinAndQuit"));
			return true;
		}

		if (params.size() == 0)
		{
			// support the secondlife:///app/worldmap SLapp
			LLFloaterReg::showInstance("world_map", "center");
			return true;
		}
		
		// support the secondlife:///app/worldmap/{LOCATION}/{COORDS} SLapp
		const std::string region_name = LLURI::unescape(params[0].asString());
		S32 x = (params.size() > 1) ? params[1].asInteger() : 128;
		S32 y = (params.size() > 2) ? params[2].asInteger() : 128;
		S32 z = (params.size() > 3) ? params[3].asInteger() : 0;
		
		LLFloaterWorldMap::getInstance()->trackURL(region_name, x, y, z);
		LLFloaterReg::showInstance("world_map", "center");
		
		return true;
	}
};
LLWorldMapHandler gWorldMapHandler;

// SocialMap handler secondlife:///app/maptrackavatar/id
class LLMapTrackAvatarHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLMapTrackAvatarHandler() : LLCommandHandler("maptrackavatar", UNTRUSTED_THROTTLE) 
	{ 
	}
	
	bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web) override
	{
		if (!LLUI::sSettingGroups["config"]->getBOOL("EnableWorldMap"))
		{
			LLNotificationsUtil::add("NoWorldMap", LLSD(), LLSD(), std::string("SwitchToStandardSkinAndQuit"));
			return true;
		}
		
		//Make sure we have some parameters
		if (params.size() == 0)
		{
			return false;
		}
		
		//Get the ID
		LLUUID id;
		if (!id.set( params[0], FALSE ))
		{
			return false;
		}
		
		LLFloaterWorldMap::getInstance()->avatarTrackFromSlapp( id  ); 
		LLFloaterReg::showInstance( "world_map", "center" );
		
		return true;
	}
};	
LLMapTrackAvatarHandler gMapTrackAvatar;

class LLMapInventoryObserver : public LLInventoryObserver
{
public:
	LLMapInventoryObserver() {}
	virtual ~LLMapInventoryObserver() {}
	void changed(U32 mask) override;
};

void LLMapInventoryObserver::changed(U32 mask)
{
	// if there's a change we're interested in.
	if((mask & (LLInventoryObserver::CALLING_CARD | LLInventoryObserver::ADD |
				LLInventoryObserver::REMOVE)) != 0)
	{
		LLFloaterWorldMap::getInstance()->inventoryChanged();
	}
}

class LLMapFriendObserver : public LLFriendObserver
{
public:
	LLMapFriendObserver() {}
	virtual ~LLMapFriendObserver() {}
	void changed(U32 mask) override;
};

void LLMapFriendObserver::changed(U32 mask)
{
	// if there's a change we're interested in.
	if((mask & (LLFriendObserver::ADD | LLFriendObserver::REMOVE | LLFriendObserver::ONLINE | LLFriendObserver::POWERS)) != 0)
	{
		LLFloaterWorldMap::getInstance()->friendsChanged();
	}
}

//---------------------------------------------------------------------------
// Statics
//---------------------------------------------------------------------------

// Used as a pretend asset and inventory id to mean "landmark at my home location."
const LLUUID LLFloaterWorldMap::sHomeID( "10000000-0000-0000-0000-000000000001" );

//---------------------------------------------------------------------------
// Construction and destruction
//---------------------------------------------------------------------------


LLFloaterWorldMap::LLFloaterWorldMap(const LLSD& key)
:	LLFloater(key),
	mPanel(nullptr),
	mInventory(nullptr),
	mInventoryObserver(nullptr),
	mFriendObserver(nullptr),
	mCompletingRegionName(),
	mCompletingRegionPos(),
	mWaitingForTracker(FALSE),
	mIsClosing(FALSE),
	mSetToUserPosition(TRUE),
	mTrackedLocation(0,0,0),
	mTrackedStatus(LLTracker::TRACKING_NOTHING),
	mListFriendCombo(nullptr),
	mListLandmarkCombo(nullptr),
	mListSearchResults(nullptr),
	mGeneralEventsCheck(nullptr),
	mMatureEventsCheck(nullptr),
	mAdultEventsCheck(nullptr),
	mLandSaleCheck(nullptr),
	mInfoHubCheck(nullptr),
	mPeopleCheck(nullptr),
	mLandmarkIcon(nullptr),
	mLocationIcon(nullptr),
	mTrackRegionButton(nullptr),
	mCopySlurlButton(nullptr),
	mShowDestinationButton(nullptr),
	mTeleportButton(nullptr),
	mTeleportHomeButton(nullptr),
	mZoomSlider(nullptr)
{
	mFactoryMap["objects_mapview"] = LLCallbackMap(createWorldMapView, nullptr);
	
	mCommitCallbackRegistrar.add("WMap.Coordinates",	boost::bind(&LLFloaterWorldMap::onCoordinatesCommit, this));
	mCommitCallbackRegistrar.add("WMap.Location",		boost::bind(&LLFloaterWorldMap::onLocationCommit, this));
	mCommitCallbackRegistrar.add("WMap.AvatarCombo",	boost::bind(&LLFloaterWorldMap::onAvatarComboCommit, this));
	mCommitCallbackRegistrar.add("WMap.Landmark",		boost::bind(&LLFloaterWorldMap::onLandmarkComboCommit, this));
	mCommitCallbackRegistrar.add("WMap.SearchResult",	boost::bind(&LLFloaterWorldMap::onCommitSearchResult, this));
	mCommitCallbackRegistrar.add("WMap.GoHome",			boost::bind(&LLFloaterWorldMap::onGoHome, this));	
	mCommitCallbackRegistrar.add("WMap.Teleport",		boost::bind(&LLFloaterWorldMap::onClickTeleportBtn, this));	
	mCommitCallbackRegistrar.add("WMap.ShowTarget",		boost::bind(&LLFloaterWorldMap::onShowTargetBtn, this));	
	mCommitCallbackRegistrar.add("WMap.ShowAgent",		boost::bind(&LLFloaterWorldMap::onShowAgentBtn, this));		
	mCommitCallbackRegistrar.add("WMap.Clear",			boost::bind(&LLFloaterWorldMap::onClearBtn, this));		
	mCommitCallbackRegistrar.add("WMap.CopySLURL",		boost::bind(&LLFloaterWorldMap::onCopySLURL, this));
	mCommitCallbackRegistrar.add("WMap.TrackRegion",	boost::bind(&LLFloaterWorldMap::onTrackRegion, this));

	gSavedSettings.getControl("PreferredMaturity")->getSignal()->connect(boost::bind(&LLFloaterWorldMap::onChangeMaturity, this));
}

// static
void* LLFloaterWorldMap::createWorldMapView(void* data)
{
	return new LLWorldMapView();
}

BOOL LLFloaterWorldMap::postBuild()
{
	mPanel = getChild<LLPanel>("objects_mapview");
	mGeneralEventsCheck = getChild<LLCheckBoxCtrl>("event_chk");
	mMatureEventsCheck = getChild<LLCheckBoxCtrl>("events_mature_chk");
	mAdultEventsCheck = getChild<LLCheckBoxCtrl>("events_adult_chk");
	mLandSaleCheck = getChild<LLCheckBoxCtrl>("land_for_sale_chk");
	mInfoHubCheck = getChild<LLCheckBoxCtrl>("infohub_chk");
	mPeopleCheck = getChild<LLCheckBoxCtrl>("people_chk");
	mFriendsIcon = getChild<LLIconCtrl>("friends_icon");
	mLandmarkIcon = getChild<LLIconCtrl>("landmark_icon");
	mLocationIcon = getChild<LLIconCtrl>("location_icon");
	mTrackRegionButton = getChild<LLButton>("track_region");
	mCopySlurlButton = getChild<LLButton>("copy_slurl");
	mShowDestinationButton = getChild<LLButton>("Show Destination");
	mTeleportButton = getChild<LLButton>("Teleport");
	mTeleportHomeButton = getChild<LLButton>("Go Home");
	mZoomSlider = getChild<LLSliderCtrl>("zoom slider");

	LLComboBox *avatar_combo = getChild<LLComboBox>("friend combo");
	avatar_combo->selectFirstItem();
	avatar_combo->setPrearrangeCallback( boost::bind(&LLFloaterWorldMap::onAvatarComboPrearrange, this) );
	avatar_combo->setTextChangedCallback( boost::bind(&LLFloaterWorldMap::onComboTextEntry, this) );
	mListFriendCombo = dynamic_cast<LLCtrlListInterface *>(avatar_combo);
	
	LLSearchEditor *location_editor = getChild<LLSearchEditor>("location");
	location_editor->setFocusChangedCallback(boost::bind(&LLFloaterWorldMap::onLocationFocusChanged, this, _1));
	location_editor->setTextChangedCallback( boost::bind(&LLFloaterWorldMap::onSearchTextEntry, this));
	
	getChild<LLScrollListCtrl>("search_results")->setDoubleClickCallback( boost::bind(&LLFloaterWorldMap::onClickTeleportBtn, this));
	mListSearchResults = childGetListInterface("search_results");
	
	LLComboBox *landmark_combo = getChild<LLComboBox>( "landmark combo");
	landmark_combo->selectFirstItem();
	landmark_combo->setPrearrangeCallback( boost::bind(&LLFloaterWorldMap::onLandmarkComboPrearrange, this) );
	landmark_combo->setTextChangedCallback( boost::bind(&LLFloaterWorldMap::onComboTextEntry, this) );
	mListLandmarkCombo = dynamic_cast<LLCtrlListInterface *>(landmark_combo);
	
	mCurZoomVal = log(LLWorldMapView::sMapScale)/log(2.f);
	mZoomSlider->setValue(LLWorldMapView::sMapScale);
	
	setDefaultBtn(nullptr);
	
	mZoomTimer.stop();
	
	onChangeMaturity();
	
	return TRUE;
}

// virtual
LLFloaterWorldMap::~LLFloaterWorldMap()
{
	// All cleaned up by LLView destructor
	mPanel = nullptr;
	
	// Inventory deletes all observers on shutdown
	mInventory = nullptr;
	mInventoryObserver = nullptr;
	
	// avatar tracker will delete this for us.
	mFriendObserver = nullptr;
}

//static
LLFloaterWorldMap* LLFloaterWorldMap::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLFloaterWorldMap>("world_map");
}

// virtual
void LLFloaterWorldMap::onClose(bool app_quitting)
{
	// While we're not visible, discard the overlay images we're using
	LLWorldMap::getInstance()->clearImageRefs();
}

// virtual
void LLFloaterWorldMap::onOpen(const LLSD& key)
{
	bool center_on_target = (key.asString() == "center");
	
	mIsClosing = FALSE;
	
	LLWorldMapView* map_panel = (LLWorldMapView*)mPanel;
	map_panel->clearLastClick();
	
	{
		// reset pan on show, so it centers on you again
		if (!center_on_target)
		{
			LLWorldMapView::setPan(0, 0, TRUE);
		}
		map_panel->updateVisibleBlocks();
		
		// Reload items as they may have changed
		LLWorldMap::getInstance()->reloadItems();
		
		// We may already have a bounding box for the regions of the world,
		// so use that to adjust the view.
		adjustZoomSliderBounds();
		
		// Could be first show
		//LLFirstUse::useMap();
		
		// Start speculative download of landmarks
		const LLUUID landmark_folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_LANDMARK);
		LLInventoryModelBackgroundFetch::instance().start(landmark_folder_id);
		
		getChild<LLUICtrl>("location")->setFocus( TRUE);
		gFocusMgr.triggerFocusFlash();
		
		buildAvatarIDList();
		buildLandmarkIDLists();
		
		// If nothing is being tracked, set flag so the user position will be found
		mSetToUserPosition = ( LLTracker::getInstance()->getTrackingStatus() == LLTracker::TRACKING_NOTHING );
	}
	
	if (center_on_target)
	{
		centerOnTarget(FALSE);
	}
}

// static
void LLFloaterWorldMap::reloadIcons(void*)
{
	LLWorldMap::getInstance()->reloadItems();
}

// virtual
BOOL LLFloaterWorldMap::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled;
	handled = LLFloater::handleHover(x, y, mask);
	return handled;
}

BOOL LLFloaterWorldMap::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	if (!isMinimized() && isFrontmost())
	{
		if(mPanel->pointInView(x, y))
		{
			F32 slider_value = (F32)mZoomSlider->getValue().asReal();
			slider_value += ((F32)clicks * -0.3333f);
			mZoomSlider->setValue(LLSD(slider_value));
			return TRUE;
		}
	}
	
	return LLFloater::handleScrollWheel(x, y, clicks);
}


// virtual
void LLFloaterWorldMap::reshape( S32 width, S32 height, BOOL called_from_parent )
{
	LLFloater::reshape( width, height, called_from_parent );
}


// virtual
void LLFloaterWorldMap::draw()
{
	static LLUIColor map_track_color = LLUIColorTable::instance().getColor("MapTrackColor", LLColor4::white);
	static LLUIColor map_track_disabled_color = LLUIColorTable::instance().getColor("MapTrackDisabledColor", LLColor4::white);
	
	// On orientation island, users don't have a home location yet, so don't
	// let them teleport "home".  It dumps them in an often-crowed welcome
	// area (infohub) and they get confused. JC
	LLViewerRegion* regionp = gAgent.getRegion();
	bool agent_on_prelude = (regionp && regionp->isPrelude());
	bool enable_go_home = gAgent.isGodlike() || !agent_on_prelude;
	mTeleportHomeButton->setEnabled(enable_go_home);
	
	updateLocation();
	
	LLTracker::ETrackingStatus tracking_status = LLTracker::getInstance()->getTrackingStatus(); 
	if (LLTracker::TRACKING_AVATAR == tracking_status)
	{
		mFriendsIcon->setColor(map_track_color);
	}
	else
	{
		mFriendsIcon->setColor(map_track_disabled_color);
	}
	
	if (LLTracker::TRACKING_LANDMARK == tracking_status)
	{
		mLandmarkIcon->setColor(map_track_color);
	}
	else
	{
		mLandmarkIcon->setColor(map_track_disabled_color);
	}
	
	if (LLTracker::TRACKING_LOCATION == tracking_status)
	{
		mLocationIcon->setColor(map_track_color);
	}
	else
	{
		if (!mCompletingRegionName.empty())
		{
			F64 seconds = LLTimer::getElapsedSeconds();
			double value = fmod(seconds, 2);
			value = 0.5 + 0.5*cos(value * F_PI);
			LLColor4 loading_color(0.0, F32(value/2), F32(value), 1.0);
			mLocationIcon->setColor(loading_color);
		}
		else
		{
			mLocationIcon->setColor(map_track_disabled_color);
		}
	}
	
	// check for completion of tracking data
	if (mWaitingForTracker)
	{
		centerOnTarget(TRUE);
	}
	
	mTeleportButton->setEnabled((BOOL)tracking_status);
	//	getChildView("Clear")->setEnabled((BOOL)tracking_status);
	mShowDestinationButton->setEnabled((BOOL)tracking_status || LLWorldMap::getInstance()->isTracking());
	mCopySlurlButton->setEnabled((mSLURL.isValid()) );
	mTrackRegionButton->setEnabled((BOOL) tracking_status || LLWorldMap::getInstance()->isTracking());
	
	setMouseOpaque(TRUE);
	getDragHandle()->setMouseOpaque(TRUE);
	
	//RN: snaps to zoom value because interpolation caused jitter in the text rendering
	if (!mZoomTimer.getStarted() && mCurZoomVal != (F32) mZoomSlider->getValue().asReal())
	{
		mZoomTimer.start();
	}
	F32 interp = mZoomTimer.getElapsedTimeF32() / MAP_ZOOM_TIME;
	if (interp > 1.f)
	{
		interp = 1.f;
		mZoomTimer.stop();
	}
	mCurZoomVal = lerp(mCurZoomVal, (F32) mZoomSlider->getValue().asReal(), interp);
	F32 map_scale = 256.f*pow(2.f, mCurZoomVal);
	LLWorldMapView::setScale( map_scale );
	
	// Enable/disable checkboxes depending on the zoom level
	// If above threshold level (i.e. low res) -> Disable all checkboxes
	// If under threshold level (i.e. high res) -> Enable all checkboxes
	bool enable = LLWorldMapView::showRegionInfo();
	mPeopleCheck->setEnabled(enable);
	mInfoHubCheck->setEnabled(enable);
	//getChildView("telehub_chk")->setEnabled(enable);
	mLandSaleCheck->setEnabled(enable);
	mGeneralEventsCheck->setEnabled(enable);
	mMatureEventsCheck->setEnabled(enable);
	mAdultEventsCheck->setEnabled(enable);
	
	LLFloater::draw();
}


//-------------------------------------------------------------------------
// Internal utility functions
//-------------------------------------------------------------------------


void LLFloaterWorldMap::trackAvatar( const LLUUID& avatar_id, const std::string& name )
{
	LLCtrlSelectionInterface *iface = childGetSelectionInterface("friend combo");
	if (!iface) return;
	
	buildAvatarIDList();
	if(iface->setCurrentByID(avatar_id) || gAgent.isGodlike())
	{
		// *HACK: Adjust Z values automatically for liaisons & gods so
		// they swoop down when they click on the map. Requested
		// convenience.
		if(gAgent.isGodlike())
		{
			getChild<LLUICtrl>("teleport_coordinate_z")->setValue(LLSD(200.f));
		}
		// Don't re-request info if we already have it or we won't have it in time to teleport
		if (mTrackedStatus != LLTracker::TRACKING_AVATAR || name != mTrackedAvatarName)
		{
			mTrackedStatus = LLTracker::TRACKING_AVATAR;
			mTrackedAvatarName = name;
			LLTracker::getInstance()->trackAvatar(avatar_id, name);
		}
	}
	else
	{
		LLTracker::getInstance()->stopTracking();
	}
	setDefaultBtn("Teleport");
}

void LLFloaterWorldMap::trackLandmark( const LLUUID& landmark_item_id )
{
	LLCtrlSelectionInterface *iface = childGetSelectionInterface("landmark combo");
	if (!iface) return;
	
	buildLandmarkIDLists();
	BOOL found = FALSE;
	S32 idx;
	for (idx = 0; idx < mLandmarkItemIDList.size(); idx++)
	{
		if ( mLandmarkItemIDList.at(idx) == landmark_item_id)
		{
			found = TRUE;
			break;
		}
	}
	
	if (found && iface->setCurrentByID( landmark_item_id ) ) 
	{
		LLUUID asset_id = mLandmarkAssetIDList.at( idx );
		std::string name;
		LLComboBox* combo = getChild<LLComboBox>( "landmark combo");
		if (combo) name = combo->getSimple();
		mTrackedStatus = LLTracker::TRACKING_LANDMARK;
		LLTracker::getInstance()->trackLandmark(mLandmarkAssetIDList.at(idx),	// assetID
								 mLandmarkItemIDList.at( idx ), // itemID
								 name);			// name
		
		if( asset_id != sHomeID )
		{
			// start the download process
			gLandmarkList.getAsset( asset_id);
		}
		
		// We have to download both region info and landmark data, so set busy. JC
		//		getWindow()->incBusyCount();
	}
	else
	{
		LLTracker::getInstance()->stopTracking();
	}
	setDefaultBtn("Teleport");
}


void LLFloaterWorldMap::trackEvent(const LLItemInfo &event_info)
{
	mTrackedStatus = LLTracker::TRACKING_LOCATION;
	LLTracker::getInstance()->trackLocation(event_info.getGlobalPosition(), event_info.getName(), event_info.getToolTip(), LLTracker::LOCATION_EVENT);
	setDefaultBtn("Teleport");
}

void LLFloaterWorldMap::trackGenericItem(const LLItemInfo &item)
{
	mTrackedStatus = LLTracker::TRACKING_LOCATION;
	LLTracker::getInstance()->trackLocation(item.getGlobalPosition(), item.getName(), item.getToolTip(), LLTracker::LOCATION_ITEM);
	setDefaultBtn("Teleport");
}

void LLFloaterWorldMap::trackLocation(const LLVector3d& pos_global)
{
	LLSimInfo* sim_info = LLWorldMap::getInstance()->simInfoFromPosGlobal(pos_global);
	LLTracker& tracker = LLTracker::instance();
	if (!sim_info)
	{
		// We haven't found a region for that point yet, leave the tracking to the world map
		tracker.stopTracking();
		LLWorldMap::getInstance()->setTracking(pos_global);
		S32 world_x = S32(pos_global.mdV[0] / 256);
		S32 world_y = S32(pos_global.mdV[1] / 256);
		LLWorldMapMessage::getInstance()->sendMapBlockRequest(world_x, world_y, world_x, world_y, true);
		setDefaultBtn("");
		
		// clicked on a non-region - turn off coord display
		enableTeleportCoordsDisplay( false );
		
		return;
	}
	if (sim_info->isDown())
	{
		// Down region. Show the blue circle of death!
		// i.e. let the world map that this and tell it it's invalid
		tracker.stopTracking();
		LLWorldMap::getInstance()->setTracking(pos_global);
		LLWorldMap::getInstance()->setTrackingInvalid();
		setDefaultBtn("");
		
		// clicked on a down region - turn off coord display
		enableTeleportCoordsDisplay( false );
		
		return;
	}
	
	std::string sim_name = sim_info->getName();
	U32 locX, locY;
	from_region_handle(sim_info->getHandle(), &locX, &locY);
	F32 region_x = pos_global.mdV[VX] - locX;
	F32 region_y = pos_global.mdV[VY] - locY;
	std::string full_name = llformat("%s (%d, %d, %d)", 
									 sim_name.c_str(), 
									 ll_round(region_x), 
									 ll_round(region_y),
									 ll_round((F32)pos_global.mdV[VZ]));
	
	std::string tooltip("");
	mTrackedStatus = LLTracker::TRACKING_LOCATION;
	LLWorldMap::getInstance()->cancelTracking();		// The floater is taking over the tracking
	tracker.trackLocation(pos_global, full_name, tooltip);
	
	LLVector3d coord_pos = tracker.getTrackedPositionGlobal();
	updateTeleportCoordsDisplay( coord_pos );
	
	// we have a valid region - turn on coord display
	enableTeleportCoordsDisplay( true );
	
	setDefaultBtn("Teleport");
}

// enable/disable teleport destination coordinates 
void LLFloaterWorldMap::enableTeleportCoordsDisplay( bool enabled )
{
	childSetEnabled("teleport_coordinate_x", enabled );
	childSetEnabled("teleport_coordinate_y", enabled );
	childSetEnabled("teleport_coordinate_z", enabled );
}

// update display of teleport destination coordinates - pos is in global coordinates
void LLFloaterWorldMap::updateTeleportCoordsDisplay( const LLVector3d& pos )
{
	// if we're going to update their value, we should also enable them
	enableTeleportCoordsDisplay( true );
	
	// convert global specified position to a local one
	F32 region_local_x = (F32)fmod( pos.mdV[VX], (F64)REGION_WIDTH_METERS );
	F32 region_local_y = (F32)fmod( pos.mdV[VY], (F64)REGION_WIDTH_METERS );
	F32 region_local_z = (F32)llclamp( pos.mdV[VZ], 0.0, (F64)REGION_HEIGHT_METERS );

	LLSimInfo* sim_info = LLWorldMap::getInstance()->simInfoFromPosGlobal(pos);
	if (sim_info)
	{
		U32 locX, locY;
		from_region_handle(sim_info->getHandle(), &locX, &locY);
		region_local_x = pos.mdV[VX] - locX;
		region_local_y = pos.mdV[VY] - locY;
		region_local_z = (F32)pos.mdV[VZ];

		// write in the values
		childSetValue("teleport_coordinate_x", region_local_x );
		childSetValue("teleport_coordinate_y", region_local_y );
		childSetValue("teleport_coordinate_z", region_local_z );
	}
}

void LLFloaterWorldMap::updateLocation()
{
	bool gotSimName;

	LLTracker& tracker = LLTracker::instance();
	LLTracker::ETrackingStatus status = tracker.getTrackingStatus();
	
	// These values may get updated by a message, so need to check them every frame
	// The fields may be changed by the user, so only update them if the data changes
	LLVector3d pos_global = tracker.getTrackedPositionGlobal();
	if (pos_global.isExactlyZero())
	{
		LLVector3d agentPos = gAgent.getPositionGlobal();
		
		// Set to avatar's current postion if nothing is selected
		if ( status == LLTracker::TRACKING_NOTHING && mSetToUserPosition )
		{
			// Make sure we know where we are before setting the current user position
			std::string agent_sim_name;
			gotSimName = LLWorldMap::getInstance()->simNameFromPosGlobal( agentPos, agent_sim_name );
			if ( gotSimName )
			{
				mSetToUserPosition = FALSE;
				
				// Fill out the location field
				getChild<LLUICtrl>("location")->setValue(agent_sim_name);
				
				// update the coordinate display with location of avatar in region
				updateTeleportCoordsDisplay( agentPos );
				
				// Figure out where user is
				// Set the current SLURL
				mSLURL = LLSLURL(agent_sim_name, gAgent.getPositionAgent());
			}
		}
		
		return; // invalid location
	}
	std::string sim_name;
	gotSimName = LLWorldMap::getInstance()->simNameFromPosGlobal( pos_global, sim_name );
	if ((status != LLTracker::TRACKING_NOTHING) &&
		(status != mTrackedStatus || pos_global != mTrackedLocation || sim_name != mTrackedSimName))
	{
		mTrackedStatus = status;
		mTrackedLocation = pos_global;
		mTrackedSimName = sim_name;
		
		if (status == LLTracker::TRACKING_AVATAR)
		{
			// *HACK: Adjust Z values automatically for liaisons &
			// gods so they swoop down when they click on the
			// map. Requested convenience.
			if(gAgent.isGodlike())
			{
				pos_global[2] = 200;
			}
		}
		
		getChild<LLUICtrl>("location")->setValue(sim_name);
		
		// refresh coordinate display to reflect where user clicked.
		LLVector3d coord_pos = LLTracker::getInstance()->getTrackedPositionGlobal();
		updateTeleportCoordsDisplay( coord_pos );
		
		// simNameFromPosGlobal can fail, so don't give the user an invalid SLURL
		if ( gotSimName )
		{
			const LLSimInfo* sim = LLWorldMap::getInstance()->simInfoFromPosGlobal(pos_global);
			pos_global[0] = fmod(pos_global[0], sim ? sim->getSizeX() : 256);
			pos_global[1] = fmod(pos_global[1], sim ? sim->getSizeY() : 256);
			mSLURL = LLSLURL(sim_name, pos_global);
		}
		else
		{	// Empty SLURL will disable the "Copy SLURL to clipboard" button
			mSLURL = LLSLURL();
		}
	}
}

void LLFloaterWorldMap::trackURL(const std::string& region_name, S32 x_coord, S32 y_coord, S32 z_coord)
{
	LLSimInfo* sim_info = LLWorldMap::getInstance()->simInfoFromName(region_name);
	z_coord = llclamp(z_coord, 0, 8192);
	if (sim_info)
	{
		LLVector3 local_pos;
		local_pos.mV[VX] = (F32)x_coord;
		local_pos.mV[VY] = (F32)y_coord;
		local_pos.mV[VZ] = (F32)z_coord;
		LLVector3d global_pos = sim_info->getGlobalPos(local_pos);
		trackLocation(global_pos);
		setDefaultBtn("Teleport");
	}
	else
	{
		// fill in UI based on URL
		getChild<LLUICtrl>("location")->setValue(region_name);
		
		// Save local coords to highlight position after region global
		// position is returned.
		mCompletingRegionPos.set(
												   (F32)x_coord, (F32)y_coord, (F32)z_coord);
		
		// pass sim name to combo box
		mCompletingRegionName = region_name;
		LLWorldMapMessage::getInstance()->sendNamedRegionRequest(region_name);
		LLStringUtil::toLower(mCompletingRegionName);
		LLWorldMap::getInstance()->setTrackingCommit();
	}
}

void LLFloaterWorldMap::observeInventory(LLInventoryModel* model)
{
	if(mInventory)
	{
		mInventory->removeObserver(mInventoryObserver);
		delete mInventoryObserver;
		mInventory = nullptr;
		mInventoryObserver = nullptr;
	}
	if(model)
	{
		mInventory = model;
		mInventoryObserver = new LLMapInventoryObserver;
		// Inventory deletes all observers on shutdown
		mInventory->addObserver(mInventoryObserver);
		inventoryChanged();
	}
}

void LLFloaterWorldMap::inventoryChanged()
{
	LLTracker& tracker = LLTracker::instance();
	if (!tracker.getTrackedLandmarkItemID().isNull())
	{
		LLUUID item_id = tracker.getTrackedLandmarkItemID();
		buildLandmarkIDLists();
		trackLandmark(item_id);
	}
}

void LLFloaterWorldMap::observeFriends()
{
	if(!mFriendObserver)
	{
		mFriendObserver = new LLMapFriendObserver;
		LLAvatarTracker::instance().addObserver(mFriendObserver);
		friendsChanged();
	}
}

void LLFloaterWorldMap::friendsChanged()
{
	LLAvatarTracker& t = LLAvatarTracker::instance();
	const LLUUID& avatar_id = t.getAvatarID();
	buildAvatarIDList();
	if(avatar_id.notNull())
	{
		LLCtrlSelectionInterface *iface = childGetSelectionInterface("friend combo");
		const LLRelationship* buddy_info = t.getBuddyInfo(avatar_id);
		if(!iface ||
		   !iface->setCurrentByID(avatar_id) || 
		   (buddy_info && !buddy_info->isRightGrantedFrom(LLRelationship::GRANT_MAP_LOCATION)) ||
		   gAgent.isGodlike())
		{
			LLTracker::getInstance()->stopTracking();
		}
	}
}

// No longer really builds a list.  Instead, just updates mAvatarCombo.
void LLFloaterWorldMap::buildAvatarIDList()
{
	LLCtrlListInterface *list = mListFriendCombo;
	if (!list) return;
	
    // Delete all but the "None" entry
	S32 list_size = list->getItemCount();
	if (list_size > 1)
	{
		list->selectItemRange(1, -1);
		list->operateOnSelection(LLCtrlListInterface::OP_DELETE);
	}
	
	// Get all of the calling cards for avatar that are currently online
	LLCollectMappableBuddies collector;
	LLAvatarTracker::instance().applyFunctor(collector);
	LLCollectMappableBuddies::buddy_map_t::iterator it;
	LLCollectMappableBuddies::buddy_map_t::iterator end;
	it = collector.mMappable.begin();
	end = collector.mMappable.end();
	for( ; it != end; ++it)
	{
		list->addSimpleElement((*it).second, ADD_BOTTOM, (*it).first);
	}
	
	list->setCurrentByID( LLAvatarTracker::instance().getAvatarID() );
	list->selectFirstItem();
}


void LLFloaterWorldMap::buildLandmarkIDLists()
{
	LLCtrlListInterface *list = mListLandmarkCombo;
	if (!list) return;
	
    // Delete all but the "None" entry
	S32 list_size = list->getItemCount();
	if (list_size > 1)
	{
		list->selectItemRange(1, -1);
		list->operateOnSelection(LLCtrlListInterface::OP_DELETE);
	}
	
	mLandmarkItemIDList.clear();
	mLandmarkAssetIDList.clear();
	
	// Get all of the current landmarks
	mLandmarkAssetIDList.push_back( LLUUID::null );
	mLandmarkItemIDList.push_back( LLUUID::null );
	
	mLandmarkAssetIDList.push_back( sHomeID );
	mLandmarkItemIDList.push_back( sHomeID );
	
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLIsType is_landmark(LLAssetType::AT_LANDMARK);
	gInventory.collectDescendentsIf(gInventory.getRootFolderID(),
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_landmark);
	
	std::sort(items.begin(), items.end(), LLViewerInventoryItem::comparePointers());
	
	mLandmarkAssetIDList.reserve(mLandmarkAssetIDList.size() + items.size());
	mLandmarkItemIDList.reserve(mLandmarkItemIDList.size() + items.size());

	S32 count = items.size();
	for(S32 i = 0; i < count; ++i)
	{
		LLInventoryItem* item = items.at(i);
		
		list->addSimpleElement(item->getName(), ADD_BOTTOM, item->getUUID());
		
		mLandmarkAssetIDList.push_back( item->getAssetUUID() );
		mLandmarkItemIDList.push_back( item->getUUID() );
	}
	
	list->selectFirstItem();
}


F32 LLFloaterWorldMap::getDistanceToDestination(const LLVector3d &destination, 
												F32 z_attenuation) const
{
	LLVector3d delta = destination - gAgent.getPositionGlobal();
	// by attenuating the z-component we effectively 
	// give more weight to the x-y plane
	delta.mdV[VZ] *= z_attenuation;
	F32 distance = (F32)delta.magVec();
	return distance;
}


void LLFloaterWorldMap::clearLocationSelection(BOOL clear_ui, BOOL dest_reached)
{
	LLCtrlListInterface *list = mListSearchResults;
	if (list && (!dest_reached || (list->getItemCount() == 1)))
	{
		list->operateOnAll(LLCtrlListInterface::OP_DELETE);
	}
	LLWorldMap::getInstance()->cancelTracking();
	mCompletingRegionName.clear();
}


void LLFloaterWorldMap::clearLandmarkSelection(BOOL clear_ui)
{
	if (clear_ui || !childHasKeyboardFocus("landmark combo"))
	{
		LLCtrlListInterface *list = mListLandmarkCombo;
		if (list)
		{
			list->selectByValue( "None" );
		}
	}
}


void LLFloaterWorldMap::clearAvatarSelection(BOOL clear_ui)
{
	if (clear_ui || !childHasKeyboardFocus("friend combo"))
	{
		mTrackedStatus = LLTracker::TRACKING_NOTHING;
		LLCtrlListInterface *list = mListFriendCombo;
		if (list)
		{
			list->selectByValue( "None" );
		}
	}
}


// Adjust the maximally zoomed out limit of the zoom slider so you
// can see the whole world, plus a little.
void LLFloaterWorldMap::adjustZoomSliderBounds()
{
	// Merov: we switched from using the "world size" (which varies depending where the user went) to a fixed
	// width of 512 regions max visible at a time. This makes the zoom slider works in a consistent way across
	// sessions and doesn't prevent the user to pan the world if it was to grow a lot beyond that limit.
	// Currently (01/26/09), this value allows the whole grid to be visible in a 1024x1024 window.
	S32 world_width_regions	 = MAX_VISIBLE_REGIONS;
	S32 world_height_regions = MAX_VISIBLE_REGIONS;
	
	// Find how much space we have to display the world
	LLWorldMapView* map_panel = (LLWorldMapView*)mPanel;
	LLRect view_rect = map_panel->getRect();
	
	// View size in pixels
	S32 view_width = view_rect.getWidth();
	S32 view_height = view_rect.getHeight();
	
	// Pixels per region to display entire width/height
	F32 width_pixels_per_region = (F32) view_width / (F32) world_width_regions;
	F32 height_pixels_per_region = (F32) view_height / (F32) world_height_regions;
	
	F32 pixels_per_region = llmin(width_pixels_per_region,
								  height_pixels_per_region);
	
	// Round pixels per region to an even number of slider increments
	S32 slider_units = llfloor(pixels_per_region / 0.2f);
	pixels_per_region = slider_units * 0.2f;
	
	// Make sure the zoom slider can be moved at least a little bit.
	// Likewise, less than the increment pixels per region is just silly.
	pixels_per_region = llclamp(pixels_per_region, 1.f, ZOOM_MAX);
	
	F32 min_power = log(pixels_per_region/256.f)/log(2.f);
	
	mZoomSlider->setMinValue(min_power);
}


//-------------------------------------------------------------------------
// User interface widget callbacks
//-------------------------------------------------------------------------

void LLFloaterWorldMap::onGoHome()
{
	gAgent.teleportHome();
	closeFloater();
}


void LLFloaterWorldMap::onLandmarkComboPrearrange( )
{
	if( mIsClosing )
	{
		return;
	}
	
	LLCtrlListInterface *list = mListLandmarkCombo;
	if (!list) return;
	
	LLUUID current_choice = list->getCurrentID();
	
	buildLandmarkIDLists();
	
	if( current_choice.isNull() || !list->setCurrentByID( current_choice ) )
	{
		LLTracker::getInstance()->stopTracking();
	}
	
}

void LLFloaterWorldMap::onComboTextEntry()
{
	// Reset the tracking whenever we start typing into any of the search fields,
	// so that hitting <enter> does an auto-complete versus teleporting us to the
	// previously selected landmark/friend.
	LLTracker::getInstance()->stopTracking();
}

void LLFloaterWorldMap::onSearchTextEntry( )
{
	onComboTextEntry();
	updateSearchEnabled();
}


void LLFloaterWorldMap::onLandmarkComboCommit()
{
	if( mIsClosing )
	{
		return;
	}
	
	LLCtrlListInterface *list = mListLandmarkCombo;
	if (!list) return;
	
	LLUUID asset_id;
	LLUUID item_id = list->getCurrentID();
	
	LLTracker& tracker = LLTracker::instance();
	tracker.stopTracking();
	
	//RN: stopTracking() clears current combobox selection, need to reassert it here
	list->setCurrentByID(item_id);
	
	if( item_id.isNull() )
	{
	}
	else if( item_id == sHomeID )
	{
		asset_id = sHomeID;
	}
	else
	{
		LLInventoryItem* item = gInventory.getItem( item_id );
		if( item )
		{
			asset_id = item->getAssetUUID();
		}
		else
		{
			// Something went wrong, so revert to a safe value.
			item_id.setNull();
		}
	}
	
	trackLandmark( item_id);
	onShowTargetBtn();
	
	// Reset to user postion if nothing is tracked
	mSetToUserPosition = ( tracker.getTrackingStatus() == LLTracker::TRACKING_NOTHING );
}

// static 
void LLFloaterWorldMap::onAvatarComboPrearrange( )
{
	if( mIsClosing )
	{
		return;
	}
	
	LLCtrlListInterface *list = mListFriendCombo;
	if (!list) return;
	
	LLUUID current_choice;
	
	if( LLAvatarTracker::instance().haveTrackingInfo() )
	{
		current_choice = LLAvatarTracker::instance().getAvatarID();
	}
	
	buildAvatarIDList();
	
	if( !list->setCurrentByID( current_choice ) || current_choice.isNull() )
	{
		LLTracker::getInstance()->stopTracking();
	}
}

void LLFloaterWorldMap::onAvatarComboCommit()
{
	if( mIsClosing )
	{
		return;
	}
	
	LLCtrlListInterface *list = mListFriendCombo;
	if (!list) return;
	
	const LLUUID& new_avatar_id = list->getCurrentID();
	if (new_avatar_id.notNull())
	{
		std::string name;
		LLComboBox* combo = getChild<LLComboBox>("friend combo");
		if (combo) name = combo->getSimple();
		trackAvatar(new_avatar_id, name);
		onShowTargetBtn();
	}
	else
	{	// Reset to user postion if nothing is tracked
		mSetToUserPosition = (LLTracker::getInstance()->getTrackingStatus() == LLTracker::TRACKING_NOTHING);
	}
}

void LLFloaterWorldMap::avatarTrackFromSlapp( const LLUUID& id ) 
{
	trackAvatar( id, "av" );		
	onShowTargetBtn();
}

void LLFloaterWorldMap::onLocationFocusChanged( LLFocusableElement* focus )
{
	updateSearchEnabled();
}

void LLFloaterWorldMap::updateSearchEnabled()
{
	if (childHasKeyboardFocus("location") && 
		getChild<LLUICtrl>("location")->getValue().asString().length() > 0)
	{
		setDefaultBtn("DoSearch");
	}
	else
	{
		setDefaultBtn(nullptr);
	}
}

void LLFloaterWorldMap::onLocationCommit()
{
	if( mIsClosing )
	{
		return;
	}
	
	clearLocationSelection(FALSE);
	mCompletingRegionName.clear();
	mLastRegionName.clear();
	
	std::string str = getChild<LLUICtrl>("location")->getValue().asString();
	
	// Trim any leading and trailing spaces in the search target
	std::string saved_str = str;
	LLStringUtil::trim( str );
	if ( str != saved_str )
	{	// Set the value in the UI if any spaces were removed
		getChild<LLUICtrl>("location")->setValue(str);
	}

	// Don't try completing empty name (STORM-1427).
	if (str.empty())
	{
		return;
	}
	
	LLStringUtil::toLower(str);
	mCompletingRegionName = str;
	LLWorldMap::getInstance()->setTrackingCommit();
	if (str.length() >= 3)
	{
		LLWorldMapMessage::getInstance()->sendNamedRegionRequest(str);
	}
	else
	{
		str += "#";
		LLWorldMapMessage::getInstance()->sendNamedRegionRequest(str);
	}
}

void LLFloaterWorldMap::onCoordinatesCommit()
{
	if( mIsClosing )
	{
		return;
	}
	
	S32 x_coord = (S32)childGetValue("teleport_coordinate_x").asReal();
	S32 y_coord = (S32)childGetValue("teleport_coordinate_y").asReal();
	S32 z_coord = (S32)childGetValue("teleport_coordinate_z").asReal();
	
	const std::string region_name = childGetValue("location").asString();
	
	trackURL( region_name, x_coord, y_coord, z_coord );
}

void LLFloaterWorldMap::onClearBtn()
{
	mTrackedStatus = LLTracker::TRACKING_NOTHING;
	LLTracker::getInstance()->stopTracking(true);
	LLWorldMap::getInstance()->cancelTracking();
	mSLURL = LLSLURL();					// Clear the SLURL since it's invalid
	mSetToUserPosition = TRUE;	// Revert back to the current user position
}

void LLFloaterWorldMap::onShowTargetBtn()
{
	centerOnTarget(TRUE);
}

void LLFloaterWorldMap::onShowAgentBtn()
{
	LLWorldMapView::setPan( 0, 0, FALSE); // FALSE == animate
	// Set flag so user's location will be displayed if not tracking anything else
	mSetToUserPosition = TRUE;	
}

void LLFloaterWorldMap::onClickTeleportBtn()
{
	teleport();
}

void LLFloaterWorldMap::onCopySLURL()
{
	getWindow()->copyTextToClipboard(utf8str_to_wstring(mSLURL.getSLURLString()));
	
	LLSD args;
	args["SLURL"] = mSLURL.getSLURLString();
	
	LLNotificationsUtil::add("CopySLURL", args);
}

void LLFloaterWorldMap::onTrackRegion()
{
	ALFloaterRegionTracker* floaterp = LLFloaterReg::getTypedInstance<ALFloaterRegionTracker>("region_tracker");
	if (floaterp)
	{
		LLTracker& tracker = LLTracker::instance();
		if (tracker.getTrackingStatus() != LLTracker::TRACKING_NOTHING)
		{
			std::string sim_name;
			if (LLWorldMap::getInstance()->simNameFromPosGlobal(tracker.getTrackedPositionGlobal(), sim_name))
			{
				const std::string& temp_label = floaterp->getRegionLabelIfExists(sim_name);
				LLSD args, payload;
				args["REGION"] = sim_name;
				args["LABEL"] = !temp_label.empty() ? temp_label : sim_name;
				payload["name"] = sim_name;
				LLNotificationsUtil::add("RegionTrackerAdd", args, payload, boost::bind(&ALFloaterRegionTracker::onRegionAddedCallback, floaterp, _1, _2));
			}
		}
	}
}

// protected
void LLFloaterWorldMap::centerOnTarget(BOOL animate)
{
	LLVector3d pos_global;
	LLTracker& tracker = LLTracker::instance();
	if (tracker.getTrackingStatus() != LLTracker::TRACKING_NOTHING)
	{
		LLVector3d tracked_position = tracker.getTrackedPositionGlobal();
		//RN: tracker doesn't allow us to query completion, so we check for a tracking position of
		// absolute zero, and keep trying in the draw loop
		if (tracked_position.isExactlyZero())
		{
			mWaitingForTracker = TRUE;
			return;
		}
		else
		{
			// We've got the position finally, so we're no longer busy. JC
			//			getWindow()->decBusyCount();
			pos_global = tracker.getTrackedPositionGlobal() - gAgentCamera.getCameraPositionGlobal();
		}
	}
	else if(LLWorldMap::getInstance()->isTracking())
	{
		pos_global = LLWorldMap::getInstance()->getTrackedPositionGlobal() - gAgentCamera.getCameraPositionGlobal();;
		
		
		
	}
	else
	{
		// default behavior = center on agent
		pos_global.clearVec();
	}
	
	LLWorldMapView::setPan( -llfloor((F32)(pos_global.mdV[VX] * (F64)LLWorldMapView::sMapScale / REGION_WIDTH_METERS)), 
						   -llfloor((F32)(pos_global.mdV[VY] * (F64)LLWorldMapView::sMapScale / REGION_WIDTH_METERS)),
						   !animate);
	mWaitingForTracker = FALSE;
}

// protected
void LLFloaterWorldMap::fly()
{
	LLVector3d pos_global = LLTracker::getInstance()->getTrackedPositionGlobal();
	
	// Start the autopilot and close the floater, 
	// so we can see where we're flying
	if (!pos_global.isExactlyZero())
	{
		gAgent.startAutoPilotGlobal( pos_global );
		closeFloater();
	}
	else
	{
		make_ui_sound("UISndInvalidOp");
	}
}


// protected
void LLFloaterWorldMap::teleport()
{
	BOOL teleport_home = FALSE;
	LLVector3d pos_global;
	LLAvatarTracker& av_tracker = LLAvatarTracker::instance();
	
	LLTracker& tracker = LLTracker::instance();
	LLTracker::ETrackingStatus tracking_status = tracker.getTrackingStatus();
	if (LLTracker::TRACKING_AVATAR == tracking_status
		&& av_tracker.haveTrackingInfo() )
	{
		pos_global = av_tracker.getGlobalPos();
		pos_global.mdV[VZ] = getChild<LLUICtrl>("teleport_coordinate_z")->getValue();
	}
	else if ( LLTracker::TRACKING_LANDMARK == tracking_status)
	{
		if (tracker.getTrackedLandmarkAssetID() == sHomeID)
		{
			teleport_home = TRUE;
		}
		else
		{
			LLLandmark* landmark = gLandmarkList.getAsset(tracker.getTrackedLandmarkAssetID());
			LLUUID region_id;
			if(landmark
			   && !landmark->getGlobalPos(pos_global)
			   && landmark->getRegionID(region_id))
			{
				LLLandmark::requestRegionHandle(
												gMessageSystem,
												gAgent.getRegionHost(),
												region_id,
												nullptr);
			}
		}
	}
	else if ( LLTracker::TRACKING_LOCATION == tracking_status)
	{
		pos_global = tracker.getTrackedPositionGlobal();
	}
	else
	{
		make_ui_sound("UISndInvalidOp");
	}
	
	// Do the teleport, which will also close the floater
	if (teleport_home)
	{
		gAgent.teleportHome();
	}
	else if (!pos_global.isExactlyZero())
	{
		if(LLTracker::TRACKING_LANDMARK == tracking_status)
		{
			gAgent.teleportViaLandmark(tracker.getTrackedLandmarkAssetID());
		}
		else
		{
			gAgent.teleportViaLocation( pos_global );
		}
	}
}

void LLFloaterWorldMap::flyToLandmark()
{
	LLVector3d destination_pos_global;
	LLTracker& tracker = LLTracker::instance();
	if (!tracker.getTrackedLandmarkAssetID().isNull())
	{
		if (tracker.hasLandmarkPosition())
		{
			gAgent.startAutoPilotGlobal(tracker.getTrackedPositionGlobal());
		}
	}
}

void LLFloaterWorldMap::teleportToLandmark()
{
	BOOL has_destination = FALSE;
	LLUUID destination_id; // Null means "home"
	
	LLTracker& tracker = LLTracker::instance();
	if (tracker.getTrackedLandmarkAssetID() == sHomeID)
	{
		has_destination = TRUE;
	}
	else
	{
		LLLandmark* landmark = gLandmarkList.getAsset(tracker.getTrackedLandmarkAssetID());
		LLVector3d global_pos;
		if(landmark && landmark->getGlobalPos(global_pos))
		{
			destination_id = tracker.getTrackedLandmarkAssetID();
			has_destination = TRUE;
		}
		else if(landmark)
		{
			// pop up an anonymous request request.
			LLUUID region_id;
			if(landmark->getRegionID(region_id))
			{
				LLLandmark::requestRegionHandle(
												gMessageSystem,
												gAgent.getRegionHost(),
												region_id,
												nullptr);
			}
		}
	}
	
	if( has_destination )
	{
		gAgent.teleportViaLandmark( destination_id );
	}
}


void LLFloaterWorldMap::teleportToAvatar()
{
	LLAvatarTracker& av_tracker = LLAvatarTracker::instance();
	if(av_tracker.haveTrackingInfo())
	{
		LLVector3d pos_global = av_tracker.getGlobalPos();
		gAgent.teleportViaLocation( pos_global );
	}
}


void LLFloaterWorldMap::flyToAvatar()
{
	if( LLAvatarTracker::instance().haveTrackingInfo() )
	{
		gAgent.startAutoPilotGlobal( LLAvatarTracker::instance().getGlobalPos() );
	}
}

void LLFloaterWorldMap::updateSims(bool found_null_sim)
{
	if (mCompletingRegionName.empty())
	{
		return;
	}
	
	LLScrollListCtrl *list = getChild<LLScrollListCtrl>("search_results");
	list->operateOnAll(LLCtrlListInterface::OP_DELETE);
	
	S32 name_length = mCompletingRegionName.length();
	
	LLSD match;

	S32 num_results = 0;

	std::vector<std::pair <U64, LLSimInfo*> > sim_info_vec(LLWorldMap::getInstance()->getRegionMap().begin(), LLWorldMap::getInstance()->getRegionMap().end());
	std::sort(sim_info_vec.begin(), sim_info_vec.end(), SortRegionNames());

	for (const auto& sim_info_pair : sim_info_vec)
	{
		LLSimInfo* info = sim_info_pair.second;
		std::string sim_name_lower = info->getName();
		LLStringUtil::toLower(sim_name_lower);
		
		if (sim_name_lower.substr(0, name_length) == mCompletingRegionName)
		{
			if (sim_name_lower == mCompletingRegionName)
			{
				match = info->getName();
			}
			
			LLSD value;
			value["id"] = info->getName();
			value["columns"][0]["column"] = "sim_name";
			value["columns"][0]["value"] = info->getName();
			list->addElement(value);
			num_results++;
		}
	}
	
	if (found_null_sim)
	{
		mCompletingRegionName.clear();
	}
	
	if (num_results > 0)
	{
		// if match found, highlight it and go
		if (!match.isUndefined())
		{
			list->selectByValue(match);
		}
		// else select first found item
		else
		{
			list->selectFirstItem();
		}
		list->setFocus(TRUE);
		onCommitSearchResult();
	}
	else
	{
		// if we found nothing, say "none"
		list->setCommentText(LLTrans::getString("worldmap_results_none_found"));
		list->operateOnAll(LLCtrlListInterface::OP_DESELECT);
	}
}


void LLFloaterWorldMap::onCommitSearchResult()
{
	LLCtrlListInterface *list = mListSearchResults;
	if (!list) return;
	
	LLSD selected_value = list->getSelectedValue();
	std::string sim_name = selected_value.asString();
	if (sim_name.empty())
	{
		return;
	}
	LLStringUtil::toLower(sim_name);
	
	for (auto map_pair : LLWorldMap::getInstance()->getRegionMap())
	{
		LLSimInfo* info = map_pair.second;
		
		if (info->isName(sim_name))
		{
			LLVector3d pos_global = info->getGlobalOrigin();
			
			const F64 SIM_COORD_DEFAULT = 128.0;
			LLVector3 pos_local(SIM_COORD_DEFAULT, SIM_COORD_DEFAULT, 0.0f);
			
			// Did this value come from a trackURL() request?
			if (!mCompletingRegionPos.isExactlyZero())
			{
				pos_local = mCompletingRegionPos;
				mCompletingRegionPos.clear();
			}
			pos_global.mdV[VX] += (F64)pos_local.mV[VX];
			pos_global.mdV[VY] += (F64)pos_local.mV[VY];
			pos_global.mdV[VZ] = (F64)pos_local.mV[VZ];
			
			getChild<LLUICtrl>("location")->setValue(sim_name);
			trackLocation(pos_global);
			setDefaultBtn("Teleport");
			break;
		}
	}
	
	onShowTargetBtn();
}

void LLFloaterWorldMap::onChangeMaturity()
{
	bool can_access_mature = gAgent.canAccessMature();
	bool can_access_adult = gAgent.canAccessAdult();
	
	getChildView("events_mature_icon")->setVisible( can_access_mature);
	getChildView("events_mature_label")->setVisible( can_access_mature);
	mMatureEventsCheck->setVisible( can_access_mature);
	
	getChildView("events_adult_icon")->setVisible( can_access_adult);
	getChildView("events_adult_label")->setVisible( can_access_adult);
	mAdultEventsCheck->setVisible( can_access_adult);
	
	// disable mature / adult events.
	if (!can_access_mature)
	{
		gSavedSettings.setBOOL("ShowMatureEvents", FALSE);
	}
	if (!can_access_adult)
	{
		gSavedSettings.setBOOL("ShowAdultEvents", FALSE);
	}
}

void LLFloaterWorldMap::onFocusLost()
{
	gViewerWindow->showCursor();
	LLWorldMapView* map_panel = (LLWorldMapView*)mPanel;
	map_panel->mPanning = FALSE;
}
