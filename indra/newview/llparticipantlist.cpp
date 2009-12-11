/** 
 * @file llparticipantlist.cpp
 * @brief LLParticipantList intended to update view(LLAvatarList) according to incoming messages
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

// common includes
#include "lltrans.h"
#include "llavataractions.h"
#include "llagent.h"
#include "llimview.h"

#include "llparticipantlist.h"
#include "llavatarlist.h"
#include "llspeakers.h"
#include "llviewermenu.h"

//LLParticipantList retrieves add, clear and remove events and updates view accordingly 
#if LL_MSVC
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif

class ModerationResponder : public LLHTTPClient::Responder
{
public:
	ModerationResponder(const LLUUID& session_id)
	{
		mSessionID = session_id;
	}

	virtual void error(U32 status, const std::string& reason)
	{
		llwarns << status << ": " << reason << llendl;

		if ( gIMMgr )
		{
			//403 == you're not a mod
			//should be disabled if you're not a moderator
			if ( 403 == status )
			{
				gIMMgr->showSessionEventError(
					"mute",
					"not_a_mod_error",
					mSessionID);
			}
			else
			{
				gIMMgr->showSessionEventError(
					"mute",
					"generic_request_error",
					mSessionID);
			}
		}
	}

private:
	LLUUID mSessionID;
};

LLParticipantList::LLParticipantList(LLSpeakerMgr* data_source, LLAvatarList* avatar_list,  bool use_context_menu/* = true*/):
	mSpeakerMgr(data_source),
	mAvatarList(avatar_list),
	mSortOrder(E_SORT_BY_NAME)
,	mParticipantListMenu(NULL)
{
	mSpeakerAddListener = new SpeakerAddListener(*this);
	mSpeakerRemoveListener = new SpeakerRemoveListener(*this);
	mSpeakerClearListener = new SpeakerClearListener(*this);
	mSpeakerModeratorListener = new SpeakerModeratorUpdateListener(*this);
	mSpeakerMuteListener = new SpeakerMuteListener(*this);

	mSpeakerMgr->addListener(mSpeakerAddListener, "add");
	mSpeakerMgr->addListener(mSpeakerRemoveListener, "remove");
	mSpeakerMgr->addListener(mSpeakerClearListener, "clear");
	mSpeakerMgr->addListener(mSpeakerModeratorListener, "update_moderator");

	mAvatarList->setNoItemsCommentText(LLTrans::getString("LoadingData"));
	mAvatarListDoubleClickConnection = mAvatarList->setDoubleClickCallback(boost::bind(&LLParticipantList::onAvatarListDoubleClicked, this, mAvatarList));
	mAvatarListRefreshConnection = mAvatarList->setRefreshCompleteCallback(boost::bind(&LLParticipantList::onAvatarListRefreshed, this, _1, _2));
    // Set onAvatarListDoubleClicked as default on_return action.
	mAvatarListReturnConnection = mAvatarList->setReturnCallback(boost::bind(&LLParticipantList::onAvatarListDoubleClicked, this, mAvatarList));

	if (use_context_menu)
	{
		mParticipantListMenu = new LLParticipantListMenu(*this);
		mAvatarList->setContextMenu(mParticipantListMenu);
	}
	else
	{
		mAvatarList->setContextMenu(NULL);
	}

	//Lets fill avatarList with existing speakers
	LLAvatarList::uuid_vector_t& group_members = mAvatarList->getIDs();

	LLSpeakerMgr::speaker_list_t speaker_list;
	mSpeakerMgr->getSpeakerList(&speaker_list, true);
	for(LLSpeakerMgr::speaker_list_t::iterator it = speaker_list.begin(); it != speaker_list.end(); it++)
	{
		const LLPointer<LLSpeaker>& speakerp = *it;

		addAvatarIDExceptAgent(group_members, speakerp->mID);
		if ( speakerp->mIsModerator )
		{
			mModeratorList.insert(speakerp->mID);
		}
	}
	mAvatarList->setDirty(true);
	sort();
}

LLParticipantList::~LLParticipantList()
{
	mAvatarListDoubleClickConnection.disconnect();
	mAvatarListRefreshConnection.disconnect();
	mAvatarListReturnConnection.disconnect();

	delete mParticipantListMenu;
	mParticipantListMenu = NULL;
}

void LLParticipantList::setSpeakingIndicatorsVisible(BOOL visible)
{
	mAvatarList->setSpeakingIndicatorsVisible(visible);
};

void LLParticipantList::onAvatarListDoubleClicked(LLAvatarList* list)
{
	// NOTE(EM): Should we check if there is multiple selection and start conference if it is so?
	LLUUID clicked_id = list->getSelectedUUID();

	if (clicked_id.isNull() || clicked_id == gAgent.getID())
		return;
	
	LLAvatarActions::startIM(clicked_id);
}

void LLParticipantList::onAvatarListRefreshed(LLUICtrl* ctrl, const LLSD& param)
{
	LLAvatarList* list = dynamic_cast<LLAvatarList*>(ctrl);
	if (list)
	{
		const std::string moderator_indicator(LLTrans::getString("IM_moderator_label")); 
		const std::size_t moderator_indicator_len = moderator_indicator.length();

		// Firstly remove moderators indicator
		std::set<LLUUID>::const_iterator
			moderator_list_it = mModeratorToRemoveList.begin(),
			moderator_list_end = mModeratorToRemoveList.end();
		for (;moderator_list_it != moderator_list_end; ++moderator_list_it)
		{
			LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*> (list->getItemByValue(*moderator_list_it));
			if ( item )
			{
				std::string name = item->getAvatarName();
				size_t found = name.find(moderator_indicator);
				if (found == std::string::npos)
				{
					name.erase(found, moderator_indicator_len);
					item->setName(name);
					item->reshapeAvatarName();
				}
			}
		}

		mModeratorToRemoveList.clear();

		// Add moderators indicator
		moderator_list_it = mModeratorList.begin();
		moderator_list_end = mModeratorList.end();
		for (;moderator_list_it != moderator_list_end; ++moderator_list_it)
		{
			LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*> (list->getItemByValue(*moderator_list_it));
			if ( item )
			{
				std::string name = item->getAvatarName();
				size_t found = name.find(moderator_indicator);
				if (found == std::string::npos)
				{
					name += " ";
					name += moderator_indicator;
					item->setName(name);
					item->reshapeAvatarName();
				}
			}
		}
	}
}

void LLParticipantList::setSortOrder(EParticipantSortOrder order)
{
	if ( mSortOrder != order )
	{
		mSortOrder = order;
		sort();
	}
}

bool LLParticipantList::onAddItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLAvatarList::uuid_vector_t& group_members = mAvatarList->getIDs();
	LLUUID uu_id = event->getValue().asUUID();

	LLAvatarList::uuid_vector_t::iterator found = std::find(group_members.begin(), group_members.end(), uu_id);
	if(found != group_members.end())
	{
		llinfos << "Already got a buddy" << llendl;
		return true;
	}

	addAvatarIDExceptAgent(group_members, uu_id);
	// Mark AvatarList as dirty one
	mAvatarList->setDirty();
	sort();
	return true;
}

bool LLParticipantList::onRemoveItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLAvatarList::uuid_vector_t& group_members = mAvatarList->getIDs();
	LLAvatarList::uuid_vector_t::iterator pos = std::find(group_members.begin(), group_members.end(), event->getValue().asUUID());
	if(pos != group_members.end())
	{
		group_members.erase(pos);
		mAvatarList->setDirty();
	}
	return true;
}

bool LLParticipantList::onClearListEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLAvatarList::uuid_vector_t& group_members = mAvatarList->getIDs();
	group_members.clear();
	mAvatarList->setDirty();
	return true;
}

bool LLParticipantList::onModeratorUpdateEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	const LLSD& evt_data = event->getValue();
	if ( evt_data.has("id") && evt_data.has("is_moderator") )
	{
		LLUUID id = evt_data["id"];
		bool is_moderator = evt_data["is_moderator"];
		if ( id.notNull() )
		{
			if ( is_moderator )
				mModeratorList.insert(id);
			else
			{
				std::set<LLUUID>::iterator it = mModeratorList.find (id);
				if ( it != mModeratorList.end () )
				{
					mModeratorToRemoveList.insert(id);
					mModeratorList.erase(id);
				}
			}
		}
	}
	return true;
}

bool LLParticipantList::onSpeakerMuteEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLPointer<LLSpeaker> speakerp = (LLSpeaker*)event->getSource();
	if (speakerp.isNull()) return false;

	// update UI on confirmation of moderator mutes
	if (event->getValue().asString() == "voice")
	{
		LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*>(mAvatarList->getItemByValue(speakerp->mID));
		if (item)
		{
			LLOutputMonitorCtrl* indicator = item->getChild<LLOutputMonitorCtrl>("speaking_indicator");
			indicator->setIsMuted(speakerp->mModeratorMutedVoice);
		}
	}
	return true;
}

void LLParticipantList::sort()
{
	if ( !mAvatarList )
		return;

	// TODO: Implement more sorting orders after specs updating (EM)
	switch ( mSortOrder ) {
	case E_SORT_BY_NAME :
		mAvatarList->sortByName();
		break;
	default :
		llwarns << "Unrecognized sort order for " << mAvatarList->getName() << llendl;
		return;
	}
}

void LLParticipantList::addAvatarIDExceptAgent(std::vector<LLUUID>& existing_list, const LLUUID& avatar_id)
{
	if (gAgent.getID() == avatar_id) return;

	existing_list.push_back(avatar_id);
	adjustParticipant(avatar_id);
}

void LLParticipantList::adjustParticipant(const LLUUID& speaker_id)
{
	LLPointer<LLSpeaker> speakerp = mSpeakerMgr->findSpeaker(speaker_id);
	if (speakerp.isNull()) return;

	// add listener to process moderation changes
	speakerp->addListener(mSpeakerMuteListener);
}

//
// LLParticipantList::SpeakerAddListener
//
bool LLParticipantList::SpeakerAddListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	/**
	 * We need to filter speaking objects. These objects shouldn't appear in the list
	 * @c LLFloaterChat::addChat() in llviewermessage.cpp to get detailed call hierarchy
	 */
	const LLUUID& speaker_id = event->getValue().asUUID();
	LLPointer<LLSpeaker> speaker = mParent.mSpeakerMgr->findSpeaker(speaker_id);
	if(speaker.isNull() || speaker->mType == LLSpeaker::SPEAKER_OBJECT)
	{
		return false;
	}
	return mParent.onAddItemEvent(event, userdata);
}

//
// LLParticipantList::SpeakerRemoveListener
//
bool LLParticipantList::SpeakerRemoveListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onRemoveItemEvent(event, userdata);
}

//
// LLParticipantList::SpeakerClearListener
//
bool LLParticipantList::SpeakerClearListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onClearListEvent(event, userdata);
}

//
// LLParticipantList::SpeakerModeratorListener
//
bool LLParticipantList::SpeakerModeratorUpdateListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
		return mParent.onModeratorUpdateEvent(event, userdata);
}

bool LLParticipantList::SpeakerMuteListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onSpeakerMuteEvent(event, userdata);
}

LLContextMenu* LLParticipantList::LLParticipantListMenu::createMenu()
{
	// set up the callbacks for all of the avatar menu items
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	
	registrar.add("ParticipantList.ToggleAllowTextChat", boost::bind(&LLParticipantList::LLParticipantListMenu::toggleAllowTextChat, this, _2));
	registrar.add("ParticipantList.ToggleMuteText", boost::bind(&LLParticipantList::LLParticipantListMenu::toggleMuteText, this, _2));

	registrar.add("ParticipantList.ModerateVoice", boost::bind(&LLParticipantList::LLParticipantListMenu::moderateVoice, this, _2));

	enable_registrar.add("ParticipantList.EnableItem", boost::bind(&LLParticipantList::LLParticipantListMenu::enableContextMenuItem,	this, _2));
	enable_registrar.add("ParticipantList.CheckItem",  boost::bind(&LLParticipantList::LLParticipantListMenu::checkContextMenuItem,	this, _2));

	// create the context menu from the XUI
	LLContextMenu* main_menu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>(
		"menu_participant_list.xml", LLMenuGL::sMenuContainer, LLViewerMenuHolderGL::child_registry_t::instance());

	main_menu->setItemVisible("Moderator Options", isGroupModerator());
	main_menu->arrangeAndClear();

	return main_menu;
}

void LLParticipantList::LLParticipantListMenu::show(LLView* spawning_view, const std::vector<LLUUID>& uuids, S32 x, S32 y)
{
	LLPanelPeopleMenus::ContextMenu::show(spawning_view, uuids, x, y);

	if (uuids.size() == 0) return;

	const LLUUID speaker_id = mUUIDs.front();
	BOOL is_muted = isMuted(speaker_id);

	if (is_muted)
	{
		LLMenuGL::sMenuContainer->childSetVisible("ModerateVoiceMuteSelected", false);
		LLMenuGL::sMenuContainer->childSetVisible("ModerateVoiceMuteOthers", false);
	}
	else
	{
		LLMenuGL::sMenuContainer->childSetVisible("ModerateVoiceUnMuteSelected", false);
		LLMenuGL::sMenuContainer->childSetVisible("ModerateVoiceUnMuteOthers", false);
	}
}

void LLParticipantList::LLParticipantListMenu::toggleAllowTextChat(const LLSD& userdata)
{
	const LLUUID speaker_id = mUUIDs.front();

	std::string url = gAgent.getRegion()->getCapability("ChatSessionRequest");
	LLSD data;
	data["method"] = "mute update";
	data["session-id"] = mParent.mSpeakerMgr->getSessionID();
	data["params"] = LLSD::emptyMap();
	data["params"]["agent_id"] = speaker_id;
	data["params"]["mute_info"] = LLSD::emptyMap();
	//current value represents ability to type, so invert
	data["params"]["mute_info"]["text"] = !mParent.mSpeakerMgr->findSpeaker(speaker_id)->mModeratorMutedText;

	LLHTTPClient::post(
		url,
		data,
		new ModerationResponder(mParent.mSpeakerMgr->getSessionID()));
}

void LLParticipantList::LLParticipantListMenu::toggleMute(const LLSD& userdata, U32 flags)
{
	const LLUUID speaker_id = mUUIDs.front();
	BOOL is_muted = LLMuteList::getInstance()->isMuted(speaker_id, flags);
	std::string name;

	//fill in name using voice client's copy of name cache
	LLPointer<LLSpeaker> speakerp = mParent.mSpeakerMgr->findSpeaker(speaker_id);
	if (speakerp.isNull())
	{
		return;
	}

	name = speakerp->mDisplayName;

	LLMute mute(speaker_id, name, speakerp->mType == LLSpeaker::SPEAKER_AGENT ? LLMute::AGENT : LLMute::OBJECT);

	if (!is_muted)
	{
		LLMuteList::getInstance()->add(mute, flags);
	}
	else
	{
		LLMuteList::getInstance()->remove(mute, flags);
	}
}

void LLParticipantList::LLParticipantListMenu::toggleMuteText(const LLSD& userdata)
{
	toggleMute(userdata, LLMute::flagTextChat);
}

void LLParticipantList::LLParticipantListMenu::toggleMuteVoice(const LLSD& userdata)
{
	toggleMute(userdata, LLMute::flagVoiceChat);
}

bool LLParticipantList::LLParticipantListMenu::isGroupModerator()
{
	// Agent is in Group Call
	bool is_in_group = gAgent.isInGroup(mParent.mSpeakerMgr->getSessionID());
	// Agent is Moderator
	bool is_moderator = mParent.mSpeakerMgr->findSpeaker(gAgentID)->mIsModerator;
	return is_in_group && is_moderator;
}

bool LLParticipantList::LLParticipantListMenu::isMuted(const LLUUID& avatar_id)
{
	LLPointer<LLSpeaker> selected_speakerp = mParent.mSpeakerMgr->findSpeaker(avatar_id);
	if (!selected_speakerp) return true;

	return selected_speakerp->mStatus == LLSpeaker::STATUS_MUTED;
}

void LLParticipantList::LLParticipantListMenu::moderateVoice(const LLSD& userdata)
{
	if (!gAgent.getRegion()) return;

	bool moderate_selected = userdata.asString() == "selected";
	const LLUUID& selected_avatar_id = mUUIDs.front();
	bool is_muted = isMuted(selected_avatar_id);

	if (moderate_selected)
	{
		moderateVoiceParticipant(selected_avatar_id, is_muted);
	}
	else
	{
		moderateVoiceOtherParticipants(selected_avatar_id, is_muted);
	}
}

void LLParticipantList::LLParticipantListMenu::moderateVoiceParticipant(const LLUUID& avatar_id, bool unmute)
{
	if (gAgentID == avatar_id) return; // do not process myself

	LLPointer<LLSpeaker> speakerp = mParent.mSpeakerMgr->findSpeaker(avatar_id);
	if (!speakerp) return;

	// *NOTE: mantipov: probably this condition will be incorrect when avatar will be blocked for
	// text chat via moderation (LLSpeaker::mModeratorMutedText == TRUE)
	bool is_in_voice = speakerp->mStatus <= LLSpeaker::STATUS_VOICE_ACTIVE || speakerp->mStatus == LLSpeaker::STATUS_MUTED;

	// do not send voice moderation changes for avatars not in voice channel
	if (!is_in_voice) return;

	std::string url = gAgent.getRegion()->getCapability("ChatSessionRequest");
	LLSD data;
	data["method"] = "mute update";
	data["session-id"] = mParent.mSpeakerMgr->getSessionID();
	data["params"] = LLSD::emptyMap();
	data["params"]["agent_id"] = avatar_id;
	data["params"]["mute_info"] = LLSD::emptyMap();
	data["params"]["mute_info"]["voice"] = !unmute;

	LLHTTPClient::post(
		url,
		data,
		new ModerationResponder(mParent.mSpeakerMgr->getSessionID()));
}

void LLParticipantList::LLParticipantListMenu::moderateVoiceOtherParticipants(const LLUUID& excluded_avatar_id, bool unmute)
{
	LLSpeakerMgr::speaker_list_t speakers;
	mParent.mSpeakerMgr->getSpeakerList(&speakers, FALSE);

	for (LLSpeakerMgr::speaker_list_t::iterator iter = speakers.begin();
		iter != speakers.end(); ++iter)
	{
		LLSpeaker* speakerp = (*iter).get();
		LLUUID speaker_id = speakerp->mID;

		if (excluded_avatar_id == speaker_id) continue;

		moderateVoiceParticipant(speaker_id, unmute);
	}
}

bool LLParticipantList::LLParticipantListMenu::enableContextMenuItem(const LLSD& userdata)
{
	std::string item = userdata.asString();
	if (item == "can_mute_text")
	{
		return mUUIDs.front() != gAgentID;
	}
	else
		if (item == "can_allow_text_chat" || "can_moderate_voice" == item)
		{
			return isGroupModerator();
		}
	return true;
}

bool LLParticipantList::LLParticipantListMenu::checkContextMenuItem(const LLSD& userdata)
{
	std::string item = userdata.asString();
	const LLUUID& id = mUUIDs.front();
	if (item == "is_muted")
		return LLMuteList::getInstance()->isMuted(id, LLMute::flagTextChat); 
	else
		if (item == "is_allowed_text_chat")
		{
			LLPointer<LLSpeaker> selected_speakerp = mParent.mSpeakerMgr->findSpeaker(id);

			if (selected_speakerp.notNull())
			{
				return !selected_speakerp->mModeratorMutedText;
			}
		}
	return false;
}

//EOF
