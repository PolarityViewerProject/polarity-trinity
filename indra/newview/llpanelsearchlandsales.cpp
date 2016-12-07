// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/*
 * @file llpanelsearchlandsales.cpp
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
#include "llpanelsearchlandsales.h"
#include "llfloaterdirectory.h"

#include "llfloaterreg.h"
#include "llqueryflags.h"

#include "llagent.h"
#include "llnotificationsutil.h"
#include "llsearcheditor.h"
#include "llsearchhistory.h"
#include "llviewercontrol.h"

static LLPanelInjector<LLPanelSearchLandSales> t_panel_search_land_sales("panel_search_landsales");

LLPanelSearchLandSales::LLPanelSearchLandSales()
:	LLPanelSearch()
{
	mCommitCallbackRegistrar.add("Search.query", boost::bind(&LLPanelSearchLandSales::onCommitSearch, this, _1));
}

BOOL LLPanelSearchLandSales::postBuild()
{
	return TRUE;
}

void LLPanelSearchLandSales::onCommitSearch(LLUICtrl* ctrl)
{
	search();
}

void LLPanelSearchLandSales::search()
{
	LLDirQuery query;
	query.type = SE_LANDSALES;
	query.results_per_page = 100;
	
	static LLUICachedControl<bool> inc_pg("ShowPGLand", 1);
	static LLUICachedControl<bool> inc_mature("ShowMatureLand", 0);
	static LLUICachedControl<bool> inc_adult("ShowAdultLand", 0);
	static LLUICachedControl<bool> limit_price("FindLandPrice", 1);
	static LLUICachedControl<bool> limit_area("FindLandArea", 1);
	if (!(inc_pg || inc_mature || inc_adult))
	{
		LLNotificationsUtil::add("NoContentToSearch");
		return;
	}

	const std::string& type = gSavedSettings.getString("FindLandType");
	if (type == "All")
		query.category_int = ST_ALL;
	else if (type == "Auction")
		query.category_int = ST_AUCTION;
	else if (type == "Mainland")
		query.category_int = ST_MAINLAND;
	else if (type == "Estate")
		query.category_int = ST_ESTATE;
	
	if (gAgent.wantsPGOnly())
		query.scope |= DFQ_PG_SIMS_ONLY;
	if (inc_pg)
		query.scope |= DFQ_INC_PG;
	if (inc_mature && gAgent.canAccessMature())
		query.scope |= DFQ_INC_MATURE;
	if (inc_adult && gAgent.canAccessAdult())
		query.scope |= DFQ_INC_ADULT;
	
	const std::string& sort = gSavedSettings.getString("FindLandSort");
	if (sort == "Name")
		query.scope |= DFQ_NAME_SORT;
	else if (sort == "Price")
		query.scope |= DFQ_PRICE_SORT;
	else if (sort == "PPM")
		query.scope |= DFQ_PER_METER_SORT;
	else if (sort == "Area")
		query.scope |= DFQ_AREA_SORT;

	if (gSavedSettings.getBOOL("FindLandSortAscending"))
		query.scope |= DFQ_SORT_ASC;
	if (limit_price)
		query.scope |= DFQ_LIMIT_BY_PRICE;
	if (limit_area)
		query.scope |= DFQ_LIMIT_BY_AREA;
	query.price = childGetValue("edit_price").asInteger();
	query.area = childGetValue("edit_area").asInteger();
	
	mFloater->queryDirectory(query, true);
}
