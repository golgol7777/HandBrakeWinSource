/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.h
 * Copyright (C) John Stebbins 2008-2011 <stebbins@stebbins>
 * 
 * callbacks.h is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * callbacks.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with callbacks.h.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#if !defined(_CALLBACKS_H_)
#define _CALLBACKS_H_

#if defined(_WIN32)
#include <windows.h>
#endif

#include <gtk/gtk.h>
#include "settings.h"

void ghb_check_all_depencencies(signal_user_data_t *ud);
gboolean ghb_timer_cb(gpointer data);
gboolean ghb_log_cb(GIOChannel *source, GIOCondition cond, gpointer data);
void warn_log_handler(
	const gchar *domain, GLogLevelFlags flags, const gchar *msg, gpointer ud);
void debug_log_handler(
	const gchar *domain, GLogLevelFlags flags, const gchar *msg, gpointer ud);
void ghb_hbfd(signal_user_data_t *ud, gboolean hbfd);
gboolean ghb_file_menu_add_dvd(signal_user_data_t *ud);
void ghb_udev_init(void);
gboolean ghb_message_dialog(
	GtkMessageType type, const gchar *message, 
	const gchar *no, const gchar *yes);
void ghb_error_dialog(
	GtkMessageType type, const gchar *message, const gchar *cancel);
void ghb_init_dep_map(void);
void ghb_cancel_encode(signal_user_data_t *ud, const gchar *extra_msg);
gboolean ghb_cancel_encode2(signal_user_data_t *ud, const gchar *extra_msg);
GValue* ghb_start_next_job(signal_user_data_t *ud, gboolean find_first);
void ghb_check_dependency(
	signal_user_data_t *ud, GtkWidget *widget, const gchar *alt_name);
void ghb_do_scan( signal_user_data_t *ud, const gchar *filename, 
	gint titlenum, gboolean force);
void ghb_log(gchar *log, ...);
gpointer ghb_check_update(signal_user_data_t *ud);
void ghb_uninhibit_gsm(void);
void ghb_inhibit_gsm(signal_user_data_t *ud);
#if defined(_WIN32)
void wm_drive_changed(MSG *msg, signal_user_data_t *ud);
#endif
gpointer ghb_cache_volnames(signal_user_data_t *ud);
void ghb_volname_cache_init(void);
void ghb_update_destination_extension(signal_user_data_t *ud);
void ghb_update_pending(signal_user_data_t *ud);
gboolean ghb_idle_scan(signal_user_data_t *ud);

#endif // _CALLBACKS_H_

