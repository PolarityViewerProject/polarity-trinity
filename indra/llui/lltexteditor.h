/**
 * @file lltexteditor.h
 * @brief LLTextEditor base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

// Text editor widget to let users enter a a multi-line ASCII document//

#ifndef LL_LLTEXTEDITOR_H
#define LL_LLTEXTEDITOR_H

#include "llrect.h"
#include "llframetimer.h"
#include "llstyle.h"
#include "lleditmenuhandler.h"
#include "llviewborder.h" // for params
#include "lltextbase.h"
#include "lltextvalidate.h"

#include "llpreeditor.h"
#include "llcontrol.h"

class LLFontGL;
class LLScrollbar;
class TextCmd;
class LLUICtrlFactory;
class LLScrollContainer;

class LLTextEditor :
	public LLTextBase,
	protected LLPreeditor
{
public:
	struct Params : public LLInitParam::Block<Params, LLTextBase::Params>
	{
		Optional<std::string>	default_text;
		Optional<LLTextValidate::validate_func_t, LLTextValidate::ValidateTextNamedFuncs>	prevalidate_callback;

		Optional<bool>			embedded_items,
								ignore_tab,
								commit_on_focus_lost,
								show_context_menu,
								enable_tooltip_paste,
								auto_indent;

		//colors
		Optional<LLUIColor>		default_color;

		Params();
	};

	void initFromParams(const Params&);
protected:
	LLTextEditor(const Params&);
	friend class LLUICtrlFactory;
public:
	//
	// Constants
	//
	static const llwchar FIRST_EMBEDDED_CHAR = 0x100000;
	static const llwchar LAST_EMBEDDED_CHAR =  0x10ffff;
	static const S32 MAX_EMBEDDED_ITEMS = LAST_EMBEDDED_CHAR - FIRST_EMBEDDED_CHAR + 1;

	virtual ~LLTextEditor();

	typedef boost::signals2::signal<void (LLTextEditor* caller)> keystroke_signal_t;

	void	setKeystrokeCallback(const keystroke_signal_t::slot_type& callback);

	void	setParseHighlights(BOOL parsing) {mParseHighlights=parsing;}

	static S32		spacesPerTab();

	// mousehandler overrides
	BOOL	handleMouseDown(S32 x, S32 y, MASK mask) override;
	BOOL	handleMouseUp(S32 x, S32 y, MASK mask) override;
	BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask) override;
	BOOL	handleHover(S32 x, S32 y, MASK mask) override;
	BOOL	handleDoubleClick(S32 x, S32 y, MASK mask ) override;
	BOOL	handleMiddleMouseDown(S32 x,S32 y,MASK mask) override;

	BOOL	handleKeyHere(KEY key, MASK mask ) override;
	BOOL	handleUnicodeCharHere(llwchar uni_char) override;

	void	onMouseCaptureLost() override;

	// view overrides
	void	draw() override;
	void	onFocusReceived() override;
	void	onFocusLost() override;
	void	onCommit() override;
	void	setEnabled(BOOL enabled) override;

	// uictrl overrides
	void	clear() override;
	void	setFocus( BOOL b ) override;
	BOOL	isDirty() const override;

	// LLEditMenuHandler interface
	void	undo() override;
	BOOL	canUndo() const override;
	void	redo() override;
	BOOL	canRedo() const override;

	void	cut() override;
	BOOL	canCut() const override;
	void	copy() override;
	BOOL	canCopy() const override;
	void	paste() override;
	BOOL	canPaste() const override;

	virtual void	updatePrimary();
	virtual void	copyPrimary();
	virtual void	pastePrimary();
	virtual BOOL	canPastePrimary() const;

	void	doDelete() override;
	BOOL	canDoDelete() const override;
	void	selectAll() override;
	BOOL	canSelectAll()	const override;

	void 			selectByCursorPosition(S32 prev_cursor_pos, S32 next_cursor_pos);

	virtual bool	canLoadOrSaveToFile();

//	void			selectNext(const std::string& search_text_in, BOOL case_insensitive, BOOL wrap = TRUE);
// [SL:KB] - Patch: UI-FloaterSearchReplace | Checked: 2010-10-29 (Catznip-3.0.0) | Added: Catznip-2.3.0
	void			selectNext(const std::string& search_text_in, BOOL case_insensitive, BOOL wrap = TRUE, BOOL search_up = FALSE);
	BOOL			replaceText(const std::string& search_text, const std::string& replace_text, BOOL case_insensitive, BOOL wrap = TRUE, BOOL search_up = FALSE);
// [/SL:KB]
//	BOOL			replaceText(const std::string& search_text, const std::string& replace_text, BOOL case_insensitive, BOOL wrap = TRUE);
	void			replaceTextAll(const std::string& search_text, const std::string& replace_text, BOOL case_insensitive);

	// Undo/redo stack
	void			blockUndo();

	// Text editing
	virtual void	makePristine();
	BOOL			isPristine() const;
	BOOL			allowsEmbeddedItems() const { return mAllowEmbeddedItems; }

	// Autoreplace (formerly part of LLLineEditor)
	typedef std::function<void(S32&, S32&, LLWString&, S32&, const LLWString&)> autoreplace_callback_t;
	autoreplace_callback_t mAutoreplaceCallback;
	void			setAutoreplaceCallback(autoreplace_callback_t cb) { mAutoreplaceCallback = cb; }

	/*virtual*/ void	onSpellCheckPerformed() override;

	//
	// Text manipulation
	//

	// inserts text at cursor
	void			insertText(const std::string &text);
	void			insertText(LLWString &text);

	void			appendWidget(const LLInlineViewSegment::Params& params, const std::string& text, bool allow_undo) override;
	// Non-undoable
	void			setText(const LLStringExplicit &utf8str, const LLStyle::Params& input_params = LLStyle::Params()) override;


	// Removes text from the end of document
	// Does not change highlight or cursor position.
	void 			removeTextFromEnd(S32 num_chars);

	BOOL			tryToRevertToPristineState();

	void			setCursorAndScrollToEnd();

	void			getCurrentLineAndColumn( S32* line, S32* col, BOOL include_wordwrap );

	// Hacky methods to make it into a word-wrapping, potentially scrolling,
	// read-only text box.
	void			setCommitOnFocusLost(BOOL b)			{ mCommitOnFocusLost = b; }

	// Hack to handle Notecards
	virtual BOOL	importBuffer(const char* buffer, S32 length );
	virtual BOOL	exportBuffer(std::string& buffer );

	const LLUUID&	getSourceID() const						{ return mSourceID; }

	const LLTextSegmentPtr	getPreviousSegment() const;
	void			getSelectedSegments(segment_vec_t& segments) const;

	void			setShowContextMenu(bool show) { mShowContextMenu = show; }
	bool			getShowContextMenu() const { return mShowContextMenu; }

	void			setPassDelete(BOOL b) { mPassDelete = b; }

protected:
	void			showContextMenu(S32 x, S32 y);
	void			drawPreeditMarker();

	void 			assignEmbedded(const std::string &s);

	void			removeCharOrTab();

	void			indentSelectedLines( S32 spaces );
	S32				indentLine( S32 pos, S32 spaces );
	void			unindentLineBeforeCloseBrace();

	virtual	BOOL	handleSpecialKey(const KEY key, const MASK mask);
	BOOL			handleNavigationKey(const KEY key, const MASK mask);
	BOOL			handleSelectionKey(const KEY key, const MASK mask);
	BOOL			handleControlKey(const KEY key, const MASK mask);

	BOOL			selectionContainsLineBreaks();
	void			deleteSelection(BOOL transient_operation);

	S32				prevWordPos(S32 cursorPos) const;
	S32				nextWordPos(S32 cursorPos) const;

	void			autoIndent();

	void			findEmbeddedItemSegments(S32 start, S32 end);
	void			getSegmentsInRange(segment_vec_t& segments, S32 start, S32 end, bool include_partial) const;

	virtual llwchar	pasteEmbeddedItem(llwchar ext_char) { return ext_char; }


	// Here's the method that takes and applies text commands.
	S32 			execute(TextCmd* cmd);

	// Undoable operations
	void			addChar(llwchar c); // at mCursorPos
	S32				addChar(S32 pos, llwchar wc);
	void			addLineBreakChar(BOOL group_together = FALSE);
	S32				overwriteChar(S32 pos, llwchar wc);
	void			removeChar();
	S32 			removeChar(S32 pos);
	void			removeWord(bool prev);
	S32				insert(S32 pos, const LLWString &wstr, bool group_with_next_op, LLTextSegmentPtr segment);
	S32				remove(S32 pos, S32 length, bool group_with_next_op);

	void			focusLostHelper();
	void			updateAllowingLanguageInput();
	BOOL			hasPreeditString() const;

	// Overrides LLPreeditor
	void	resetPreedit() override;
	void	updatePreedit(const LLWString &preedit_string,
						const segment_lengths_t &preedit_segment_lengths, const standouts_t &preedit_standouts, S32 caret_position) override;
	void	markAsPreedit(S32 position, S32 length) override;
	void	getPreeditRange(S32 *position, S32 *length) const override;
	void	getSelectionRange(S32 *position, S32 *length) const override;
	BOOL	getPreeditLocation(S32 query_offset, LLCoordGL *coord, LLRect *bounds, LLRect *control) const override;
	S32		getPreeditFontSize() const override;
	LLWString getPreeditString() const override { return getWText(); }
	//
	// Protected data
	//
	// Probably deserves serious thought to hiding as many of these
	// as possible behind protected accessor methods.
	//

	// Use these to determine if a click on an embedded item is a drag or not.
	S32				mMouseDownX;
	S32				mMouseDownY;

	LLWString			mPreeditWString;
	LLWString			mPreeditOverwrittenWString;
	std::vector<S32> 	mPreeditPositions;
	std::vector<BOOL> 	mPreeditStandouts;

protected:
	LLUIColor			mDefaultColor;

	bool				mAutoIndent;
	bool				mParseOnTheFly;

	void				updateLinkSegments();
	void				keepSelectionOnReturn(bool keep) { mKeepSelectionOnReturn = keep; }
	class LLViewBorder*	mBorder;

private:
	//
	// Methods
	//
	void	        pasteHelper(bool is_primary);
	void			cleanStringForPaste(LLWString & clean_string);
	void			pasteTextWithLinebreaks(LLWString & clean_string);

	void			onKeyStroke();

	// Concrete TextCmd sub-classes used by the LLTextEditor base class
	class TextCmdInsert;
	class TextCmdAddChar;
	class TextCmdOverwriteChar;
	class TextCmdRemove;

	BOOL			mBaseDocIsPristine;
	TextCmd*		mPristineCmd;

	TextCmd*		mLastCmd;

	typedef std::deque<TextCmd*> undo_stack_t;
	undo_stack_t	mUndoStack;

	BOOL			mTabsToNextField;		// if true, tab moves focus to next field, else inserts spaces
	BOOL			mCommitOnFocusLost;
	BOOL			mTakesFocus;

	BOOL			mAllowEmbeddedItems;
	bool			mShowContextMenu;
	bool			mEnableTooltipPaste;
	bool			mPassDelete;
	bool			mKeepSelectionOnReturn;	// disabling of removing selected text after pressing of Enter

	LLUUID			mSourceID;

	LLCoordGL		mLastIMEPosition;		// Last position of the IME editor

	keystroke_signal_t mKeystrokeSignal;
	LLTextValidate::validate_func_t mPrevalidateFunc;

	LLHandle<LLContextMenu> mContextMenuHandle;
}; // end class LLTextEditor

// Build time optimization, generate once in .cpp file
#ifndef LLTEXTEDITOR_CPP
extern template class LLTextEditor* LLView::getChild<class LLTextEditor>(
	const std::string& name, BOOL recurse) const;
#endif

#endif  // LL_TEXTEDITOR_H
