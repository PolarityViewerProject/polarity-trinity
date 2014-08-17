/*
 * @file llpanelprofilelegacy.cpp
 * @brief Legacy protocol avatar profile panel
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

#include "llviewerprecompiledheaders.h"
#include "llpanelprofilelegacy.h"

// libraries
#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"
#include "llavatarnamecache.h"
#include "llcheckboxctrl.h"
#include "llflatlistview.h"
#include "lllineeditor.h"
#include "llloadingindicator.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lltrans.h"

// newview
#include "llagent.h"
#include "llagentdata.h"
#include "llavataractions.h"
#include "llcallingcard.h" // for LLAvatarTracker
#include "lldateutil.h"
#include "llfloaterreporter.h"
#include "llgroupactions.h"
#include "llpanelclassified.h"
#include "llpanelpicks.h"
#include "llpanelpick.h"
#include "llmutelist.h"
#include "llsidetraypanelcontainer.h"
#include "llslurl.h"

// These are order-senstitive so don't fk with 'em!
static const std::array<std::string, 8> sWantCheckboxes{{"wanna_build", "wanna_explore", "wanna_yiff", "wanna_work", "wanna_group", "wanna_buy", "wanna_sell", "wanna_hire"}};
static const std::array<std::string, 6> sSkillsCheckboxes{{"can_texture", "can_architect", "can_model", "can_event", "can_script", "can_characters"}};

static LLPanelInjector<LLPanelProfileLegacy> t_panel_lprofile("panel_profile_legacy_sidetray");
static LLPanelInjector<LLPanelProfileLegacy::LLPanelProfileGroups> t_panel_group("panel_profile_groups");
static LLPanelInjector<LLPanelProfileLegacy::LLPanelProfilePicks> t_panel_picks("panel_profile_picks");

LLPanelProfileLegacy::LLPanelProfileLegacy()
:	LLPanelProfileTab()
,	mPanelGroups(NULL)
,	mPanelPicks(NULL)
{
	mCommitCallbackRegistrar.add("Profile.CommitInterest", boost::bind(&LLPanelProfileLegacy::onCommitInterest, this));
	mCommitCallbackRegistrar.add("Profile.CommitProperties", boost::bind(&LLPanelProfileLegacy::onCommitAvatarProperties, this));
	mCommitCallbackRegistrar.add("Profile.CommitRights", boost::bind(&LLPanelProfileLegacy::onCommitRights, this));
	mCommitCallbackRegistrar.add("Profile.CopyData", boost::bind(&LLPanelProfileLegacy::copyData, this, _2));
	mCommitCallbackRegistrar.add("Profile.Action", boost::bind(&LLPanelProfileLegacy::onCommitAction, this, _2));
	mEnableCallbackRegistrar.add("Profile.Enable", boost::bind(&LLPanelProfileLegacy::isActionEnabled, this, _2));
}

LLPanelProfileLegacy::~LLPanelProfileLegacy()
{
	if (LLAvatarPropertiesProcessor::instanceExists() && getAvatarId().notNull())
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(), this);
	if (mAvatarNameCacheConnection.connected())
		mAvatarNameCacheConnection.disconnect();
}

// virtual
BOOL LLPanelProfileLegacy::postBuild()
{
	mPanelGroups = static_cast<LLPanelProfileLegacy::LLPanelProfileGroups*>(getChild<LLUICtrl>("groups_tab_panel"));
	mPanelPicks = static_cast<LLPanelProfileLegacy::LLPanelProfilePicks*>(getChild<LLUICtrl>("picks_tab_panel"));
	mPanelPicks->setProfilePanel(this);
	
	childSetCommitCallback("back", boost::bind(&LLPanelProfileLegacy::onBackBtnClick, this), NULL);
	childSetCommitCallback("sl_about", boost::bind(&LLPanelProfileLegacy::onCommitAvatarProperties, this), NULL);
	childSetCommitCallback("fl_about", boost::bind(&LLPanelProfileLegacy::onCommitAvatarProperties, this), NULL);
	childSetCommitCallback("sl_profile_pic", boost::bind(&LLPanelProfileLegacy::onCommitAvatarProperties, this), NULL);
	childSetCommitCallback("fl_profile_pic", boost::bind(&LLPanelProfileLegacy::onCommitAvatarProperties, this), NULL);
	childSetCommitCallback("notes", boost::bind(&LLPanelProfileLegacy::onCommitNotes, this), NULL);
	return TRUE;
}

void LLPanelProfileLegacy::onOpen(const LLSD& key)
{
	if (!key.has("avatar_id")) return;
	const LLUUID av_id = key["avatar_id"].asUUID();
	
	if (key.has("action"))
	{
		// *TODO: Actions, if any
		return;
	}
	
	setAvatarId(av_id);
	
	mPanelGroups->onOpen(LLSD(av_id));
	mPanelPicks->onOpen(LLSD(av_id));
	// Oh joy!
	bool is_self = (getAvatarId() == gAgentID);
	childSetEnabled("sl_profile_pic", is_self);
	childSetEnabled("fl_profile_pic", is_self);
	childSetEnabled("sl_about", is_self);
	childSetEnabled("fl_about", is_self);
	childSetVisible("www", !is_self);
	childSetVisible("www_edit", is_self);
	childSetVisible("allow_publish", is_self);
	childSetEnabled("wanna_something", is_self);
	childSetEnabled("can_something", is_self);
	childSetEnabled("languages", is_self);
	for (const std::string& checkbox: sWantCheckboxes)
		childSetEnabled(checkbox, is_self);
	for (const std::string& checkbox: sSkillsCheckboxes)
		childSetEnabled(checkbox, is_self);
	
	updateData();
	resetControls();
	
	getChild<LLAccordionCtrl>("avatar_accordion")->expandDefaultTab();
}

void LLPanelProfileLegacy::resetControls()
{
	LLButton* button = getChild<LLButton>("btn_chat");
	button->setEnabled(getAvatarId() != gAgentID);
	button = getChild<LLButton>("btn_friend");
	button->setEnabled(getAvatarId() != gAgentID);
	button->setLabel(getString((LLAvatarTracker::instance().getBuddyInfo(getAvatarId()) == NULL)
							   ? "add_friend" : "remove_friend"));
	button = getChild<LLButton>("btn_block");
	button->setEnabled(getAvatarId() != gAgentID);
	button->setLabel(LLTrans::getString(LLMuteList::getInstance()->isMuted(getAvatarId())
										? "UnmuteAvatar"
										: "MuteAvatar"));
}

void LLPanelProfileLegacy::updateData()
{
	setProgress(true);
	LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesRequest(getAvatarId());
	LLAvatarPropertiesProcessor::getInstance()->sendAvatarNotesRequest(getAvatarId());
	mAvatarNameCacheConnection = LLAvatarNameCache::get(getAvatarId(),
														boost::bind(&LLPanelProfileLegacy::onAvatarNameCache, this, _1, _2));
	const LLRelationship* relation = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
	if (relation && getAvatarId() != gAgentID)
	{
		childSetVisible("permissions_label", true);
		childSetVisible("allow_show_online", true);
		childSetVisible("allow_mapping", true);
		childSetVisible("allow_object_perms", true);
		S32 rights = relation->getRightsGrantedTo();
		getChild<LLCheckBoxCtrl>("allow_show_online")->setValue(rights & LLRelationship::GRANT_ONLINE_STATUS ? TRUE : FALSE);
		getChild<LLCheckBoxCtrl>("allow_mapping")->setValue(rights & LLRelationship::GRANT_MAP_LOCATION ? TRUE : FALSE);
		getChild<LLCheckBoxCtrl>("allow_object_perms")->setValue(rights & LLRelationship::GRANT_MODIFY_OBJECTS ? TRUE : FALSE);
	}
	else
	{
		childSetVisible("permissions_label", false);
		childSetVisible("allow_show_online", false);
		childSetVisible("allow_mapping", false);
		childSetVisible("allow_object_perms", false);
	}
}

void LLPanelProfileLegacy::onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
{
	getChild<LLTextBase>("avatar_name")->setText(av_name.getCompleteName());
}

void LLPanelProfileLegacy::processProperties(void* data, EAvatarProcessorType type)
{
	if (!data) return;
	switch(type)
	{
		case APT_PROPERTIES:
		{
			const LLAvatarData* pData = static_cast<const LLAvatarData*>(data);
			if (!pData || pData->avatar_id != getAvatarId()) return;
			getChild<LLTextureCtrl>("sl_profile_pic")->setValue(pData->image_id);
			getChild<LLTextureCtrl>("fl_profile_pic")->setValue(pData->fl_image_id);
			if (pData->partner_id.notNull())
			{
				getChild<LLTextBase>("partner_info")->setText(LLSLURL("agent", pData->partner_id, "inspect").getSLURLString());
				getChild<LLTextBase>("partner_label")->setVisible(TRUE);
				getChild<LLTextBase>("partner_info")->setVisible(TRUE);
			}
			else
			{
				getChild<LLTextBase>("partner_label")->setVisible(FALSE);
				getChild<LLTextBase>("partner_info")->setVisible(FALSE);
			}
			getChild<LLTextEditor>("sl_about")->setText(pData->about_text);
			getChild<LLTextEditor>("fl_about")->setText(pData->fl_about_text);
			getChild<LLLineEditor>("uuid_editor")->setText(pData->agent_id.asString());
			getChild<LLTextBase>("www")->setText(pData->profile_url);
			getChild<LLLineEditor>("www_edit")->setText(pData->profile_url);
			
			LLStringUtil::format_map_t args;
			std::string birth_date = LLTrans::getString("AvatarBirthDateFormat");
			LLStringUtil::format(birth_date, LLSD().with("datetime", static_cast<S32>(pData->born_on.secondsSinceEpoch())));
			args["[AGE]"] = LLDateUtil::ageFromDate(pData->born_on, LLDate::now());
			args["[REZDAY]"] = birth_date;
			args["[ACCOUNT_TYPE]"] = LLAvatarPropertiesProcessor::accountType(pData);
			args["[PAYMENT_INFO]"] = LLAvatarPropertiesProcessor::paymentInfo(pData);
			args["[AGE_VERIFIED]"] = pData->flags & AVATAR_AGEVERIFIED ? getString("age_verified") : LLStringUtil::null;
			LLSD formatted_info(getString("account_info_fmt", args));
			getChild<LLTextBase>("account_info")->setValue(formatted_info);
			formatted_info = LLSD(getString("rezday_fmt", args));
			getChild<LLTextBase>("rezday")->setValue(formatted_info);
			childSetVisible("cake", pData->born_on.toHTTPDateString(LLStringExplicit("%d %b")) ==
							LLDate::now().toHTTPDateString(LLStringExplicit("%d %b")));
			getChild<LLCheckBoxCtrl>("allow_publish")->setValue(static_cast<bool>(pData->flags & AVATAR_ALLOW_PUBLISH));
			getChild<LLUICtrl>("online")->setVisible(static_cast<BOOL>(pData->flags & AVATAR_ONLINE ||
																	   pData->avatar_id == gAgentID));
			break;
		}
		case APT_NOTES:
		{
			const LLAvatarNotes* pData = static_cast<const LLAvatarNotes*>(data);
			if (!pData || pData->target_id != getAvatarId()) return;
			getChild<LLTextEditor>("notes")->setValue(pData->notes);
			break;
		}
		case APT_INTERESTS:
		{
			const LLAvatarInterests* pData = static_cast<const LLAvatarInterests*>(data);
			if (!pData || pData->avatar_id != getAvatarId()) return;

			for (U32 i = 0; i < sWantCheckboxes.size(); ++i)
			{
				getChild<LLCheckBoxCtrl>(sWantCheckboxes.at(i))->setValue(pData->want_to_mask & (1<<i) ? TRUE : FALSE);
			}
			
			for (U32 i = 0; i < sSkillsCheckboxes.size(); ++i)
			{
				getChild<LLCheckBoxCtrl>(sSkillsCheckboxes.at(i))->setValue(pData->skills_mask & (1<<i) ? TRUE : FALSE);
			}
			getChild<LLLineEditor>("wanna_something")->setText(pData->want_to_text);
			getChild<LLLineEditor>("can_something")->setText(pData->skills_text);
			getChild<LLLineEditor>("languages")->setText(pData->languages_text);
			break;
		}
		case APT_GROUPS:
		{
			LLAvatarGroups* pData = static_cast<LLAvatarGroups*>(data);
			if(!pData || getAvatarId() != pData->avatar_id) return;
			
			showAccordion("avatar_groups_tab", pData->group_list.size());
			break;
		}
		// These are handled by their respective panels
		case APT_PICKS:
		case APT_CLASSIFIEDS:
		case APT_PICK_INFO:
		case APT_CLASSIFIED_INFO:
		// No idea what this message is...
		case APT_TEXTURES:
		default:
			break;
	}
	setProgress(false);
}

void LLPanelProfileLegacy::setProgress(bool started)
{
	LLLoadingIndicator* indicator = getChild<LLLoadingIndicator>("progress_indicator");
	indicator->setVisible(started);
	if (started)
		indicator->start();
	else
		indicator->stop();
}

void LLPanelProfileLegacy::showAccordion(const std::string& name, bool show)
{
	LLAccordionCtrlTab* tab = getChild<LLAccordionCtrlTab>(name);
	tab->setVisible(show);
	getChild<LLAccordionCtrl>("avatar_accordion")->arrange();
}

void LLPanelProfileLegacy::onCommitAction(const LLSD& userdata)
{
	const std::string action = userdata.asString();
	if (action == "friend")
	{
		if (LLAvatarTracker::instance().getBuddyInfo(getAvatarId()) == NULL)
			LLAvatarActions::requestFriendshipDialog(getAvatarId());
		else
			LLAvatarActions::removeFriendDialog(getAvatarId());
		resetControls();
	}
	else if (action == "block")
	{
		LLAvatarActions::toggleBlock(getAvatarId());
		resetControls();
	}
	else if (action == "chat")
		LLAvatarActions::startIM(getAvatarId());
	else if (action == "webprofile")
		LLAvatarActions::showWebProfile(getAvatarId());
	else if (action == "call")
		LLAvatarActions::startCall(getAvatarId());
	else if (action == "share")
		LLAvatarActions::share(getAvatarId());
	else if (action == "teleport")
		LLAvatarActions::offerTeleport(getAvatarId());
	else if (action == "req_teleport")
		LLAvatarActions::teleportRequest(getAvatarId());
	else if (action == "map")
		LLAvatarActions::showOnMap(getAvatarId());
	else if (action == "pay")
		LLAvatarActions::pay(getAvatarId());
	else if (action == "report_abuse")
		LLFloaterReporter::showFromObject(getAvatarId());
	else
		LL_WARNS("LegacyProfiles") << "Unhandled action: " << action << LL_ENDL;
}

void LLPanelProfileLegacy::copyData(const LLSD& userdata)
{
	const std::string& param = userdata.asString();
	if (param == "copy_name")
		LLAvatarActions::copyData(getAvatarId(), LLAvatarActions::E_DATA_NAME);
	else if (param == "copy_slurl")
		LLAvatarActions::copyData(getAvatarId(), LLAvatarActions::E_DATA_SLURL);
	else if (param == "copy_key")
		LLAvatarActions::copyData(getAvatarId(), LLAvatarActions::E_DATA_UUID);
	else
		LL_WARNS("LegacyProfiles") << "Unhandled action: " << param << LL_ENDL;
}

bool LLPanelProfileLegacy::isActionEnabled(const LLSD& userdata)
{
	bool action_enabled = false;
	const std::string check = userdata.asString();
	if (check == "can_has_telefono")
		action_enabled = (LLAvatarActions::canCall() && getAvatarId() != gAgentID);
	else if (check == "can_has_teleport")
		action_enabled = (LLAvatarActions::canOfferTeleport(getAvatarId()) && getAvatarId() != gAgentID);
	else if (check == "can_has_map")
	{
		action_enabled = (LLAvatarTracker::instance().isBuddyOnline(getAvatarId())
						  && LLAvatarActions::isAgentMappable(getAvatarId()))
		|| gAgent.isGodlike();
	}
	else if (check == "can_has_pay")
		action_enabled = (getAvatarId() != gAgentID);
	else if (check == "can_share")
		action_enabled = (getAvatarId() != gAgentID);
	else if (check == "can_drama")
		action_enabled = (getAvatarId() != gAgentID);
	else
		LL_INFOS("LegacyProfiles") << "Unhandled check " << check << LL_ENDL;
	return action_enabled;
}

void LLPanelProfileLegacy::onCommitAvatarProperties()
{
	LLAvatarData data = LLAvatarData();
	
	data.avatar_id = gAgentID;
	data.image_id = getChild<LLTextureCtrl>("sl_profile_pic")->getImageAssetID();
	data.fl_image_id = getChild<LLTextureCtrl>("fl_profile_pic")->getImageAssetID();
	data.about_text = getChild<LLTextEditor>("sl_about")->getText();
	data.fl_about_text = getChild<LLTextEditor>("fl_about")->getText();
	data.profile_url = getChild<LLLineEditor>("www_edit")->getText();
	data.allow_publish = getChild<LLCheckBoxCtrl>("allow_publish")->getValue().asBoolean();
	
	LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesUpdate(&data);
}

void LLPanelProfileLegacy::onCommitInterest()
{
	LLAvatarInterests data = LLAvatarInterests();
	
	data.want_to_mask = 0x0;
	data.skills_mask = 0x0;
	data.want_to_text = getChild<LLLineEditor>("wanna_something")->getText();
	data.skills_text = getChild<LLLineEditor>("can_something")->getText();
	data.languages_text = getChild<LLLineEditor>("languages")->getText();
	for (U32 i = 0; i < sWantCheckboxes.size(); ++i)
	{
		if(getChild<LLCheckBoxCtrl>(sWantCheckboxes.at(i))->getValue().asBoolean())
			data.want_to_mask |= 1<<i;
	}
	for (U32 i = 0; i < sSkillsCheckboxes.size(); ++i)
	{
		if(getChild<LLCheckBoxCtrl>(sSkillsCheckboxes.at(i))->getValue().asBoolean())
			data.skills_mask |= 1<<i;
	}
	
	LLAvatarPropertiesProcessor::getInstance()->sendInterestsUpdate(&data);
}

void LLPanelProfileLegacy::onCommitNotes()
{
	const std::string notes = getChild<LLTextEditor>("notes")->getValue().asString();
	LLAvatarPropertiesProcessor::getInstance()->sendNotes(getAvatarId(), notes);
}

void LLPanelProfileLegacy::onBackBtnClick()
{
	LLSideTrayPanelContainer* parent = dynamic_cast<LLSideTrayPanelContainer*>(getParent());
	if(parent)
	{
		parent->openPreviousPanel();
	}
}

void LLPanelProfileLegacy::onCommitRights()
{
	if (!LLAvatarActions::isFriend(getAvatarId())) return;
	S32 flags = 0;
	if (getChild<LLCheckBoxCtrl>("allow_show_online")->getValue().asBoolean())
		flags |= LLRelationship::GRANT_ONLINE_STATUS;
	if (getChild<LLCheckBoxCtrl>("allow_mapping")->getValue().asBoolean())
		flags |= LLRelationship::GRANT_MAP_LOCATION;
	if (getChild<LLCheckBoxCtrl>("allow_object_perms")->getValue().asBoolean())
		flags |= LLRelationship::GRANT_MODIFY_OBJECTS;
	
	LLAvatarPropertiesProcessor::getInstance()->sendFriendRights(getAvatarId(), flags);
}

void LLPanelProfileLegacy::openPanel(LLPanel* panel, const LLSD& params)
{
	// Hide currently visible panel.
	//mChildStack.push();
	
	// Add the panel or bring it to front.
	if (panel->getParent() != this)
	{
		addChild(panel);
	}
	else
	{
		sendChildToFront(panel);
	}
	
	panel->setVisible(TRUE);
	panel->setFocus(TRUE); // prevent losing focus by the floater
	panel->onOpen(params);
	
	LLRect new_rect = getRect();
	panel->reshape(new_rect.getWidth(), new_rect.getHeight());
	new_rect.setLeftTopAndSize(0, new_rect.getHeight(), new_rect.getWidth(), new_rect.getHeight());
	panel->setRect(new_rect);
}

void LLPanelProfileLegacy::closePanel(LLPanel* panel)
{
	panel->setVisible(FALSE);
	
	if (panel->getParent() == this)
	{
		removeChild(panel);
		
		// Make the underlying panel visible.
		//mChildStack.pop();
		
		// Prevent losing focus by the floater
		const child_list_t* child_list = getChildList();
		if (child_list->size() > 0)
		{
			child_list->front()->setFocus(TRUE);
		}
		else
		{
			LL_WARNS() << "No underlying panel to focus." << LL_ENDL;
		}
	}
}

// LLPanelProfilePicks //

LLPanelProfileLegacy::LLPanelProfilePicks::LLPanelProfilePicks()
:	LLPanelProfileTab()
,	mProfilePanel(NULL)
,	mPicksList(NULL)
,	mClassifiedsList(NULL)
{
	
}

BOOL LLPanelProfileLegacy::LLPanelProfilePicks::postBuild()
{
	mPicksList = getChild<LLFlatListView>("picks_list");
	mClassifiedsList = getChild<LLFlatListView>("classifieds_list");
	return TRUE;
}

void LLPanelProfileLegacy::LLPanelProfilePicks::onOpen(const LLSD& key)
{
	const LLUUID id(key.asUUID());
	setAvatarId(id);
	
	updateData();
}

void LLPanelProfileLegacy::LLPanelProfilePicks::updateData()
{
	mPicksList->clear();
	mClassifiedsList->clear();
	LLAvatarPropertiesProcessor::getInstance()->sendAvatarPicksRequest(getAvatarId());
	LLAvatarPropertiesProcessor::getInstance()->sendAvatarClassifiedsRequest(getAvatarId());
}

void LLPanelProfileLegacy::LLPanelProfilePicks::processProperties(void* data, EAvatarProcessorType type)
{
	if (APT_PICKS == type)
	{
		LLAvatarPicks* avatar_picks = static_cast<LLAvatarPicks*>(data);
		if (!avatar_picks || getAvatarId() != avatar_picks->target_id) return;
		for (const LLAvatarPicks::pick_data_t& pick: avatar_picks->picks_list)
		{
			const LLUUID pick_id = pick.first;
			const std::string pick_name = pick.second;
			
			LLPickItem* picture = LLPickItem::create();
			picture->childSetAction("info_chevron", boost::bind(&LLPanelProfileLegacy::LLPanelProfilePicks::onClickInfo, this));
			picture->setPickName(pick_name);
			picture->setPickId(pick_id);
			picture->setCreatorId(getAvatarId());
			
			LLAvatarPropertiesProcessor::instance().addObserver(getAvatarId(), picture);
			picture->update();
			
			LLSD pick_value = LLSD();
			pick_value.insert(PICK_ID, pick_id);
			pick_value.insert(PICK_NAME, pick_name);
			pick_value.insert(PICK_CREATOR_ID, getAvatarId());
			
			mPicksList->addItem(picture, pick_value);
		}
		showAccordion("tab_picks", mPicksList->size());
	}
	else if (APT_CLASSIFIEDS == type)
	{
		LLAvatarClassifieds* c_info = static_cast<LLAvatarClassifieds*>(data);
		if (!c_info || getAvatarId() != c_info->target_id) return;
		for (const LLAvatarClassifieds::classified_data& c_data: c_info->classifieds_list)
		{
			LLClassifiedItem* c_item = new LLClassifiedItem(getAvatarId(), c_data.classified_id);
			c_item->childSetAction("info_chevron", boost::bind(&LLPanelProfileLegacy::LLPanelProfilePicks::onClickInfo, this));
			c_item->setClassifiedName(c_data.name);
			
			LLSD pick_value = LLSD();
			pick_value.insert(CLASSIFIED_ID, c_data.classified_id);
			pick_value.insert(CLASSIFIED_NAME, c_data.name);
			
			if (!findClassifiedById(c_data.classified_id))
			{
				mClassifiedsList->addItem(c_item, pick_value);
			}

		}
		showAccordion("tab_classifieds", mClassifiedsList->size());
	}
}

void LLPanelProfileLegacy::LLPanelProfilePicks::showAccordion(const std::string& name, bool show)
{
	getChild<LLAccordionCtrlTab>(name)->setVisible(show);
	LLAccordionCtrl* acc = getChild<LLAccordionCtrl>("accordion");
	acc->arrange();
}

LLClassifiedItem *LLPanelProfileLegacy::LLPanelProfilePicks::findClassifiedById(const LLUUID& classified_id)
{
	// HACK - find item by classified id.  Should be a better way.
	std::vector<LLPanel*> items;
	mClassifiedsList->getItems(items);
	LLClassifiedItem* c_item = NULL;
	for(LLPanel* it: items)
	{
		LLClassifiedItem *test_item = dynamic_cast<LLClassifiedItem*>(it);
		if (test_item && test_item->getClassifiedId() == classified_id)
		{
			c_item = test_item;
			break;
		}
	}
	return c_item;
}

void LLPanelProfileLegacy::LLPanelProfilePicks::onClickInfo()
{
	if(mClassifiedsList->numSelected() > 0)
		openClassifiedInfo();
	else if(mPicksList->numSelected() > 0)
		openPickInfo();
}

void LLPanelProfileLegacy::LLPanelProfilePicks::openPickInfo()
{
	LLSD selected_value = mPicksList->getSelectedValue();
	if (selected_value.isUndefined()) return;
	
	LLPickItem* pick = (LLPickItem*)mPicksList->getSelectedItem();
	LLSD params;
	params["pick_id"] = pick->getPickId();
	params["avatar_id"] = pick->getCreatorId();
	params["snapshot_id"] = pick->getSnapshotId();
	params["pick_name"] = pick->getPickName();
	params["pick_desc"] = pick->getPickDesc();
	
	if(!mPanelPickInfo)
	{
		mPanelPickInfo = LLPanelPickInfo::create();
		mPanelPickInfo->setExitCallback(boost::bind(&LLPanelProfilePicks::onPanelPickClose, this, mPanelPickInfo));
		mPanelPickInfo->setVisible(FALSE);
	}
	
	getProfilePanel()->openPanel(mPanelPickInfo, params);
}

void LLPanelProfileLegacy::LLPanelProfilePicks::openClassifiedInfo()
{
	LLSD selected_value = mClassifiedsList->getSelectedValue();
	if (selected_value.isUndefined()) return;
	
	LLClassifiedItem* c_item = getSelectedClassifiedItem();
	LLSD params;
	params["classified_id"] = c_item->getClassifiedId();
	params["classified_creator_id"] = c_item->getAvatarId();
	params["classified_snapshot_id"] = c_item->getSnapshotId();
	params["classified_name"] = c_item->getClassifiedName();
	params["classified_desc"] = c_item->getDescription();
	params["from_search"] = false;
	
	if (!mPanelClassifiedInfo)
	{
		mPanelClassifiedInfo = LLPanelClassifiedInfo::create();
		mPanelClassifiedInfo->setExitCallback(boost::bind(&LLPanelProfilePicks::onPanelClassifiedClose, this, mPanelClassifiedInfo));
		mPanelClassifiedInfo->setVisible(FALSE);
	}
	
	getProfilePanel()->openPanel(mPanelClassifiedInfo, params);
}

LLClassifiedItem* LLPanelProfileLegacy::LLPanelProfilePicks::getSelectedClassifiedItem()
{
	LLPanel* selected_item = mClassifiedsList->getSelectedItem();
	if (!selected_item) return NULL;
	
	return dynamic_cast<LLClassifiedItem*>(selected_item);
}

void LLPanelProfileLegacy::LLPanelProfilePicks::onPanelClassifiedClose(LLPanelClassifiedInfo* panel)
{
	if(panel->getInfoLoaded() && !panel->isDirty())
	{
		std::vector<LLSD> values;
		mClassifiedsList->getValues(values);
		for(size_t n = 0; n < values.size(); ++n)
		{
			LLUUID c_id = values[n][CLASSIFIED_ID].asUUID();
			if(panel->getClassifiedId() == c_id)
			{
				LLClassifiedItem* c_item = dynamic_cast<LLClassifiedItem*>(mClassifiedsList->getItemByValue(values[n]));
				llassert(c_item);
				if (c_item)
				{
					c_item->setClassifiedName(panel->getClassifiedName());
					c_item->setDescription(panel->getDescription());
					c_item->setSnapshotId(panel->getSnapshotId());
				}
			}
		}
	}
	
	onPanelPickClose(panel);
}

void LLPanelProfileLegacy::LLPanelProfilePicks::onPanelPickClose(LLPanel* panel)
{
	getProfilePanel()->closePanel(panel);
}

void LLPanelProfileLegacy::LLPanelProfilePicks::setProfilePanel(LLPanelProfileLegacy* profile_panel)
{
	mProfilePanel = profile_panel;
}

inline LLPanelProfileLegacy* LLPanelProfileLegacy::LLPanelProfilePicks::getProfilePanel()
{
	llassert_always(NULL != mProfilePanel);
	return mProfilePanel;
}

// LLPanelProfileGroups //

LLPanelProfileLegacy::LLPanelProfileGroups::LLPanelProfileGroups()
:	LLPanelProfileTab()
,	mGroupsList(NULL)
,	mGroupsText(NULL)
{
	
}

BOOL LLPanelProfileLegacy::LLPanelProfileGroups::postBuild()
{
	mGroupsList = getChild<LLFlatListView>("groups_detail_list");
	mGroupsText = getChild<LLTextBase>("groups_panel_text");
	return TRUE;
}

void LLPanelProfileLegacy::LLPanelProfileGroups::onOpen(const LLSD& key)
{
	const LLUUID id(key.asUUID());
	setAvatarId(id);
	
	updateData();
}

void LLPanelProfileLegacy::LLPanelProfileGroups::updateData()
{
	mGroupsText->setVisible(TRUE);
	mGroupsList->clear();
	LLAvatarPropertiesProcessor::getInstance()->sendAvatarGroupsRequest(getAvatarId());
}

void LLPanelProfileLegacy::LLPanelProfileGroups::processProperties(void* data, EAvatarProcessorType type)
{
	if (APT_GROUPS != type) return;
	LLAvatarGroups* avatar_groups = static_cast<LLAvatarGroups*>(data);
	if(!avatar_groups || getAvatarId() != avatar_groups->avatar_id) return;
	
	for (const LLAvatarGroups::LLGroupData& data: avatar_groups->group_list)
	{
		LLProfileGroupItem* item = LLProfileGroupItem::create();
		item->childSetAction("info_chevron", boost::bind(&LLPanelProfileLegacy::LLPanelProfileGroups::showGroup, this, data.group_id));
		item->init(data);
		
		LLSD item_value = LLSD();
		item_value.insert("group_id", data.group_id);
		item_value.insert("group_name", data.group_name);
		item_value.insert("group_icon", data.group_insignia_id);
		item_value.insert("group_desc", LLStringUtil::null);
		
		mGroupsList->addItem(item, item_value);
	}
}

void LLPanelProfileLegacy::LLPanelProfileGroups::showGroup(const LLUUID& id)
{
	LLGroupActions::show(id); //*FIXME: lol
}

// LLProfileGroupItem //

LLProfileGroupItem::LLProfileGroupItem()
:	LLPanel()
,	mInsignia(LLUUID::null)
,	mGroupName(LLStringUtil::null)
,	mCharter(LLStringUtil::null)
{
	buildFromFile("panel_profile_group_list_item.xml");
}

LLProfileGroupItem::~LLProfileGroupItem()
{
	LLGroupMgr::getInstance()->removeObserver(this);
}

//static
LLProfileGroupItem* LLProfileGroupItem::create()
{
	return new LLProfileGroupItem();
}

void LLProfileGroupItem::init(const LLAvatarGroups::LLGroupData& data)
{
	setId(data.group_id);
	setName(data.group_name);
	setInsignia(data.group_insignia_id);
	LLGroupMgr::getInstance()->addObserver(this);
	LLGroupMgr::getInstance()->sendGroupPropertiesRequest(data.group_id);
	
	LLTextureCtrl* picture = getChild<LLTextureCtrl>("picture");
	picture->setImageAssetID(data.group_insignia_id);
}

BOOL LLProfileGroupItem::postBuild()
{
	setMouseEnterCallback(boost::bind(&set_child_visible, this, "hovered_icon", true));
	setMouseLeaveCallback(boost::bind(&set_child_visible, this, "hovered_icon", false));
	return TRUE;
}

void LLProfileGroupItem::setValue(const LLSD& value)
{
	if (!value.isMap()) return;;
	if (!value.has("selected")) return;
	getChildView("selected_icon")->setVisible( value["selected"]);
}

void LLProfileGroupItem::setId(const LLUUID& id)
{
	mID = id;
}

void LLProfileGroupItem::setInsignia(const LLUUID& id)
{
	mInsignia = id;
	getChild<LLTextureCtrl>("picture")->setImageAssetID(id);
}

void LLProfileGroupItem::setName(const std::string& name)
{
	mGroupName = name;
	getChild<LLUICtrl>("name")->setValue(name);
}

void LLProfileGroupItem::setCharter(const std::string& charter)
{
	mCharter = charter;
	getChild<LLUICtrl>("description")->setValue(charter);
}

void LLProfileGroupItem::changed(LLGroupChange gc)
{
	if (gc != GC_PROPERTIES) return;
	LLGroupMgrGroupData* group_data = LLGroupMgr::getInstance()->getGroupData(mID);
	if (group_data)
	{
		setCharter(group_data->mCharter);
	}
	LLGroupMgr::getInstance()->removeObserver(this);
}
