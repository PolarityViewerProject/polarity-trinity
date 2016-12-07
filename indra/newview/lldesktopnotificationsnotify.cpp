// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/*
 * @file lldesktopnotificationsnotify.cpp
 * @brief libnotify support
 *
 * Copyright (c) 2015, Luminous Luminos <luminous@alchemyviewer.org>
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

#include "lldesktopnotificationsnotify.h"
#include "llfocusmgr.h"

#include <dlfcn.h>
#include <glib-object.h>

LLDesktopNotificationsNotify::LLDesktopNotificationsNotify()
    :handle(NULL),
     notify_init(NULL),
     notify_notification_new(NULL),
     notify_notification_show(NULL)
{
    if ((handle = dlopen("libnotify.so.4", RTLD_LAZY)))
    {
	notify_init = (notify_init_t) dlsym(handle, "notify_init");
	notify_notification_new = (notify_notification_new_t) dlsym(handle, "notify_notification_new");
	notify_notification_show = (notify_notification_show_t) dlsym(handle, "notify_notification_show");
	notify_init("Alchemy Viewer");
    }
}

LLDesktopNotificationsNotify::~LLDesktopNotificationsNotify()
{
    dlclose(handle);
    handle = NULL;
}

void LLDesktopNotificationsNotify::sendNotification(const std::string& title, const std::string& body, bool play_sound)
{
    if (handle && !gFocusMgr.getAppHasFocus())
    {
	std::string icon_path = gDirUtilp->getCurPath() + "/alchemy_icon.png";
	void* notification = notify_notification_new(title.c_str(), body.c_str(), icon_path.c_str());
	notify_notification_show(notification, NULL);
	g_object_unref(G_OBJECT(notification));
    }
}
