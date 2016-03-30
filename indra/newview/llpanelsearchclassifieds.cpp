/*
 * @file llpanelsearchclassifieds.cpp
 * @brief Groups search panel
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
#include "llpanelsearchclassifieds.h"
#include "llfloaterdirectory.h"

#include "llfloaterreg.h"
#include "llqueryflags.h"
#include "lltrans.h"

#include "llagent.h"
#include "llclassifiedflags.h"
#include "llclassifiedinfo.h"
#include "llcombobox.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "llsearcheditor.h"
#include "llsearchhistory.h"

static LLPanelInjector<LLPanelSearchClassifieds> t_panel_search_classifieds("panel_search_classifieds");

LLPanelSearchClassifieds::LLPanelSearchClassifieds()
	: LLPanelSearch()
	, mSearchEditor(nullptr)
	, mClassifiedsCategory(nullptr)
{
	mCommitCallbackRegistrar.add("Search.query", boost::bind(&LLPanelSearchClassifieds::onCommitSearch, this, _1));
}

BOOL LLPanelSearchClassifieds::postBuild()
{
	mClassifiedsCategory = getChild<LLComboBox>("classifieds_category");
	mClassifiedsCategory->add("All categories", LLSD("any"));
	mClassifiedsCategory->addSeparator();
	LLClassifiedInfo::cat_map::iterator iter;
	for (iter = LLClassifiedInfo::sCategories.begin();
		 iter != LLClassifiedInfo::sCategories.end();
		 ++iter)
	{
		mClassifiedsCategory->add(LLTrans::getString(iter->second));
	}
	
	mSearchEditor = getChild<LLSearchEditor>("search_bar");
	//mSearchEditor->setKeystrokeCallback(boost::bind(&LLPanelSearchClassifieds::onCommitSearch, this, _1));
	
	return TRUE;
}

void LLPanelSearchClassifieds::onCommitSearch(LLUICtrl* ctrl)
{
	LLSearchEditor* pSearchEditor = dynamic_cast<LLSearchEditor*>(ctrl);
	if (pSearchEditor)
	{
		std::string text = pSearchEditor->getText();
		LLStringUtil::trim(text);
		if (text.length() <= MIN_SEARCH_STRING_SIZE)
			LLSearchHistory::getInstance()->addEntry(text);
	}
	search();
}

void LLPanelSearchClassifieds::search()
{
	LLDirQuery query;
	query.type = SE_CLASSIFIEDS;
	query.results_per_page = 100;
	query.text = mSearchEditor->getText();
	LLStringUtil::trim(query.text);
	
	query.category_int = mClassifiedsCategory->getValue().asInteger();
	
	static LLUICachedControl<bool> inc_pg("ShowPGClassifieds", 1);
	static LLUICachedControl<bool> inc_mature("ShowMatureClassifieds", 0);
	static LLUICachedControl<bool> inc_adult("ShowAdultClassifieds", 0);
	if (!(inc_pg || inc_mature || inc_adult))
	{
		LLNotificationsUtil::add("NoContentToSearch");
		return;
	}
	query.scope = pack_classified_flags_request(/*auto_renew*/ FALSE, inc_pg, inc_mature, inc_adult);
	
	mFloater->queryDirectory(query, true);
}
