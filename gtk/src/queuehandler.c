/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.c
 * Copyright (C) John Stebbins 2008-2011 <stebbins@stebbins>
 * 
 * callbacks.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "ghbcompat.h"
#include <glib/gstdio.h>
#include <gio/gio.h>
#include "hb.h"
#include "settings.h"
#include "hb-backend.h"
#include "values.h"
#include "callbacks.h"
#include "presets.h"
#include "ghb-dvd.h"

G_MODULE_EXPORT void
queue_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
	GtkTreeModel *store;
	GtkTreeIter iter, piter;
	
	g_debug("queue_list_selection_changed_cb ()");
	// A queue entry is made up of a parent and multiple
	// children that are visible when expanded.  When and entry
	// is selected, I want the parent to be selected.
	// This is purely cosmetic.
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		GtkWidget *widget = GHB_WIDGET (ud->builder, "queue_edit");
		gtk_widget_set_sensitive (widget, TRUE);
		if (gtk_tree_model_iter_parent (store, &piter, &iter))
		{
			GtkTreePath *path;
			GtkTreeView *treeview;
			
			gtk_tree_selection_select_iter (selection, &piter);
			path = gtk_tree_model_get_path (store, &piter);
			treeview = gtk_tree_selection_get_tree_view (selection);
			// Make the parent visible in scroll window if it is not.
			gtk_tree_view_scroll_to_cell (treeview, path, NULL, FALSE, 0, 0);
			gtk_tree_path_free(path);
		}
	}
	else
	{
		GtkWidget *widget = GHB_WIDGET (ud->builder, "queue_edit");
		gtk_widget_set_sensitive (widget, FALSE);
	}
}

static void
add_to_queue_list(signal_user_data_t *ud, GValue *settings, GtkTreeIter *piter)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkTreeStore *store;
	gchar *info;
	gint status;
	GtkTreeIter citer;
	gchar *dest, *preset, *vol_name, *basename;
	const gchar *vcodec, *container;
	gchar *fps, *vcodec_abbr;
	gint title, start_point, end_point, width, height;
	gint source_width, source_height;
	gboolean pass2 = FALSE, keep_aspect, vqtype, turbo;
	gint pic_par;
	gboolean tweaks;
	gchar *escape, *escape2;
	
	g_debug("update_queue_list ()");
	if (settings == NULL) return;
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
		
	tweaks = ghb_settings_get_boolean(settings, "allow_tweaks");
	title = ghb_settings_get_int(settings, "titlenum");
	start_point = ghb_settings_get_int(settings, "start_point");
	end_point = ghb_settings_get_int(settings, "end_point");
	vol_name = ghb_settings_get_string(settings, "volume_label");
	dest = ghb_settings_get_string(settings, "destination");
	basename = g_path_get_basename(dest);
	escape = g_markup_escape_text(basename, -1);
	escape2 = g_markup_escape_text(vol_name, -1);

	vqtype = ghb_settings_get_boolean(settings, "vquality_type_constant");
	if (!vqtype)
		pass2 = ghb_settings_get_boolean(settings, "VideoTwoPass");
	const gchar *points = "Chapters";
	if (ghb_settings_combo_int(settings, "PtoPType") == 0)
		points = "Chapters";
	else if (ghb_settings_combo_int(settings, "PtoPType") == 1)
		points = "Seconds";
	else if (ghb_settings_combo_int(settings, "PtoPType") == 2)
		points = "Frames";
	info = g_strdup_printf 
	(
		"<big><b>%s</b></big> "
		"<small>(Title %d, %s %d through %d, %d Video %s)"
		" --> %s</small>",
		 escape2, title, points, start_point, end_point, 
		 pass2 ? 2:1, pass2 ? "Passes":"Pass", escape
	);
	g_free(basename);
	g_free(escape);
	g_free(escape2);

	if (piter)
		iter = *piter;
	else
		gtk_tree_store_append(store, &iter, NULL);

	gtk_tree_store_set(store, &iter, 1, info, 2, "hb-queue-delete", -1);
	g_free(info);
	status = ghb_settings_get_int(settings, "job_status");
	switch (status)
	{
		case GHB_QUEUE_PENDING:
			gtk_tree_store_set(store, &iter, 0, "hb-queue-job", -1);
			break;
		case GHB_QUEUE_CANCELED:
			gtk_tree_store_set(store, &iter, 0, "hb-canceled", -1);
			break;
		case GHB_QUEUE_RUNNING:
			gtk_tree_store_set(store, &iter, 0, "hb-working0", -1);
			break;
		case GHB_QUEUE_DONE:
			gtk_tree_store_set(store, &iter, 0, "hb-complete", -1);
			break;
		default:
			gtk_tree_store_set(store, &iter, 0, "hb-queue-job", -1);
			break;
	}

	GString *str = g_string_new("");
	gboolean markers;
	gboolean preset_modified;
	gint mux;
	const GValue *path;

	container = ghb_settings_combo_option(settings, "FileFormat");
	mux = ghb_settings_combo_int(settings, "FileFormat");
	preset_modified = ghb_settings_get_boolean(settings, "preset_modified");
	path = ghb_settings_get_value(settings, "preset");
	preset = ghb_preset_path_string(path);
	markers = ghb_settings_get_boolean(settings, "ChapterMarkers");

	if (preset_modified)
		g_string_append_printf(str, 
			"<b>Modified Preset Based On:</b> <small>%s</small>\n", 
			preset);
	else
		g_string_append_printf(str, 
			"<b>Preset:</b> <small>%s</small>\n", 
			preset);

	if (markers)
	{
		g_string_append_printf(str, 
			"<b>Format:</b> <small>%s Container, Chapter Markers</small>\n", 
			container);
	}
	else
	{
		g_string_append_printf(str, 
			"<b>Format:</b> <small>%s Container</small>\n", container);
	}
	if (mux == HB_MUX_MP4)
	{
		gboolean ipod, http, large;

		ipod = ghb_settings_get_boolean(settings, "Mp4iPodCompatible");
		http = ghb_settings_get_boolean(settings, "Mp4HttpOptimize");
		large = ghb_settings_get_boolean(settings, "Mp4LargeFile");
		if (http || ipod || large)
		{
			g_string_append_printf(str, "<b>MP4 Options:</b><small>");
			if (ipod)
				g_string_append_printf(str, " - iPod 5G Support");
			if (http)
				g_string_append_printf(str, " - Web Optimized");
			if (large)
				g_string_append_printf(str, " - Large File Size (>4GB)");
			g_string_append_printf(str, "</small>\n");
		}
	}
	escape = g_markup_escape_text(dest, -1);
	g_string_append_printf(str, 
		"<b>Destination:</b> <small>%s</small>\n", escape);

	width = ghb_settings_get_int(settings, "scale_width");
	height = ghb_settings_get_int(settings, "scale_height");
	pic_par = ghb_settings_combo_int(settings, "PicturePAR");
	keep_aspect = ghb_settings_get_boolean(settings, "PictureKeepRatio");

	gchar *aspect_desc;
	switch (pic_par)
	{
	case 0:
	{
		if (keep_aspect)
		{
			aspect_desc = "(Aspect Preserved)";
		}
		else
		{
			aspect_desc = "(Aspect Lost)";
		}
	} break;

	case 1:
	{
		aspect_desc = "(Strict Anamorphic)";
	} break;

	case 2:
	{
		aspect_desc = "(Loose Anamorphic)";
	} break;

	case 3:
	{
		aspect_desc = "(Custom Anamorphic)";
	} break;

	default:
	{
		aspect_desc = "(Unknown)";
	} break;
	}
	vqtype = ghb_settings_get_boolean(settings, "vquality_type_constant");
	vcodec = ghb_settings_combo_option(settings, "VideoEncoder");
	vcodec_abbr = ghb_settings_get_string(settings, "VideoEncoder");

	gchar *vq_desc = "Error";
	gchar *vq_units = "";
	gchar *vqstr;
	gdouble vqvalue;
	if (!vqtype)
	{
        // Has to be bitrate
        vqvalue = ghb_settings_get_int(settings, "VideoAvgBitrate");
        vq_desc = "Bitrate:";
        vq_units = "kbps";
		vqstr = g_strdup_printf("%d", (gint)vqvalue);
	}
	else
	{
		// Constant quality
		vqvalue = ghb_settings_get_double(settings, "VideoQualitySlider");
		vq_desc = "Constant Quality:";
		vqstr = g_strdup_printf("%d", (gint)vqvalue);
		if (strcmp(vcodec_abbr, "x264") == 0)
		{
			vq_units = "(RF)";
		}
		else
		{
			vq_units = "(QP)";
		}
	}
	fps = ghb_settings_get_string(settings, "VideoFramerate");
	if (strcmp("source", fps) == 0)
	{
		g_free(fps);
		if (ghb_settings_combo_int(settings, "PictureDetelecine"))
			fps = g_strdup("Same As Source (vfr detelecine)");
		else
			fps = g_strdup("Same As Source (variable)");
	}
	else
	{
		gchar *tmp;
		tmp = g_strdup_printf("%s (constant frame rate)", fps);
		g_free(fps);
		fps = tmp;
	}
	source_width = ghb_settings_get_int(settings, "source_width");
	source_height = ghb_settings_get_int(settings, "source_height");
	g_string_append_printf(str,
		"<b>Picture:</b> Source: <small>%d x %d, Output %d x %d %s</small>\n",
		 source_width, source_height, width, height, aspect_desc);

	gint decomb, detel;
	gboolean decomb_deint;
	gboolean filters = FALSE;

	decomb_deint = ghb_settings_get_boolean(settings, "PictureDecombDeinterlace");
	decomb = ghb_settings_combo_int(settings, "PictureDecomb");
	g_string_append_printf(str, "<b>Filters:</b><small>");
	detel = ghb_settings_combo_int(settings, "PictureDetelecine");
	if (detel)
	{
		g_string_append_printf(str, " - Detelecine");
		if (detel == 1)
		{
			gchar *cust;
			cust = ghb_settings_get_string(settings, "PictureDetelecineCustom");
			g_string_append_printf(str, ": %s", cust);
			g_free(cust);
		}
		filters = TRUE;
	}
	if (decomb_deint && decomb)
	{
		g_string_append_printf(str, " - Decomb");
		if (decomb == 1)
		{
			gchar *cust;
			cust = ghb_settings_get_string(settings, "PictureDecombCustom");
			g_string_append_printf(str, ": %s", cust);
			g_free(cust);
		}
		filters = TRUE;
	}
	else if (!decomb_deint)
	{
		gint deint = ghb_settings_combo_int(settings, "PictureDeinterlace");
		if (deint)
		{
			if (deint == 1)
			{
				gchar *cust = ghb_settings_get_string(settings,
												"PictureDeinterlaceCustom");
				g_string_append_printf(str, " - Deinterlace: %s", cust);
				g_free(cust);
			}
			else
			{
				const gchar *opt = ghb_settings_combo_option(settings,
													"PictureDeinterlace");
				g_string_append_printf(str, " - Deinterlace: %s", opt);
			}
			filters = TRUE;
		}
	}
	gint denoise = ghb_settings_combo_int(settings, "PictureDenoise");
	if (denoise)
	{
		if (denoise == 1)
		{
			gchar *cust = ghb_settings_get_string(settings,
													"PictureDenoiseCustom");
			g_string_append_printf(str, " - Denoise: %s", cust);
			g_free(cust);
		}
		else
		{
			const gchar *opt = ghb_settings_combo_option(settings,
													"PictureDenoise");
			g_string_append_printf(str, " - Denoise: %s", opt);
		}
		filters = TRUE;
	}
	gint deblock = ghb_settings_get_int(settings, "PictureDeblock");
	if (deblock >= 5)
	{
		g_string_append_printf(str, " - Deblock (%d)", deblock);
		filters = TRUE;
	}
	if (ghb_settings_get_boolean(settings, "VideoGrayScale"))
	{
		g_string_append_printf(str, " - Grayscale");
		filters = TRUE;
	}
	if (!filters)
		g_string_append_printf(str, " None");
	g_string_append_printf(str, "</small>\n");

	g_string_append_printf(str,
		"<b>Video:</b> <small>%s, Framerate: %s, %s %s%s</small>\n",
		 vcodec, fps, vq_desc, vqstr, vq_units);

	turbo = ghb_settings_get_boolean(settings, "VideoTurboTwoPass");
	if (turbo)
	{
		g_string_append_printf(str, "<b>Turbo:</b> <small>On</small>\n");
	}
	if (strcmp(vcodec_abbr, "x264") == 0 ||
		strcmp(vcodec_abbr, "ffmpeg") == 0)
	{
		gchar *opts = ghb_build_advanced_opts_string(settings);
		g_string_append_printf(str, 
			"<b>Advanced Options:</b> <small>%s</small>\n", opts);
		g_free(opts);
	}
	// Add the audios
	gint count, ii;
	const GValue *audio_list;

	audio_list = ghb_settings_get_value(settings, "audio_list");
	count = ghb_array_len(audio_list);
	for (ii = 0; ii < count; ii++)
	{
		gchar *bitrate, *samplerate, *track;
		const gchar *acodec, *mix;
		GValue *asettings;
		gdouble sr;

		asettings = ghb_array_get_nth(audio_list, ii);

		acodec = ghb_settings_combo_option(asettings, "AudioEncoderActual");
		bitrate = ghb_settings_get_string(asettings, "AudioBitrate");
		sr = ghb_settings_get_double(asettings, "AudioSamplerate");
		samplerate = ghb_settings_get_string(asettings, "AudioSamplerate");
		if ((int)sr == 0)
		{
			samplerate = g_strdup("Same As Source");
		}
		else
		{
			samplerate = g_strdup_printf("%.4g", sr);
		}
		track = ghb_settings_get_string(asettings, "AudioTrackDescription");
		mix = ghb_settings_combo_option(asettings, "AudioMixdown");
		if (count == 1)
			g_string_append_printf(str, "<b>Audio:</b>");
		else if (ii == 0)
			g_string_append_printf(str, "<b>Audio:</b>\n");
		if (count != 1)
			g_string_append_printf(str, "\t");

		g_string_append_printf(str,
			"<small> %s, Encoder: %s, Mixdown: %s, SampleRate: %s, Bitrate: %s</small>\n",
			 track, acodec, mix, samplerate, bitrate);
		g_free(track);
		g_free(bitrate);
		g_free(samplerate);
	}

	// Add the audios
	const GValue *sub_list;

	sub_list = ghb_settings_get_value(settings, "subtitle_list");
	count = ghb_array_len(sub_list);
	for (ii = 0; ii < count; ii++)
	{
		GValue *settings;
		gchar *track;
		gboolean force, burn, def;
		gint source;

		settings = ghb_array_get_nth(sub_list, ii);
		track = ghb_settings_get_string(settings, "SubtitleTrackDescription");
		source = ghb_settings_get_int(settings, "SubtitleSource");
		force = ghb_settings_get_boolean(settings, "SubtitleForced");
		burn = ghb_settings_get_boolean(settings, "SubtitleBurned");
		def = ghb_settings_get_boolean(settings, "SubtitleDefaultTrack");
		if (count == 1)
			g_string_append_printf(str, "<b>Subtitle:</b>");
		else if (ii == 0)
			g_string_append_printf(str, "<b>Subtitles:</b>\n");
		if (count != 1)
			g_string_append_printf(str, "\t");

		if (source != SRTSUB)
		{
			g_string_append_printf(str,
				"<small> %s%s%s%s</small>",
			 	track, 
				force ? " (Force)":"",
				burn  ? " (Burn)":"",
				def   ? " (Default)":""
			);
		}
		else
		{
			gint offset;
			gchar *filename, *basename, *code;

			offset = ghb_settings_get_int(settings, "SrtOffset");
			filename = ghb_settings_get_string(settings, "SrtFile");
			basename = g_path_get_basename(filename);
			code = ghb_settings_get_string(settings, "SrtCodeset");
			g_string_append_printf(str,
				"<small> %s (%s), %s, Offset (ms) %d%s</small>",
			 	track, code, basename, offset,
				def   ? " (Default)":""
			);
			g_free(filename);
			g_free(basename);
			g_free(code);
		}
		if (ii < count-1)
			g_string_append_printf(str, "\n");
		g_free(track);
	}

	info = g_string_free(str, FALSE);
	gtk_tree_store_append(store, &citer, &iter);
	gtk_tree_store_set(store, &citer, 1, info, -1);
	g_free(info);
	g_free(fps);
	g_free(vcodec_abbr);
	g_free(vol_name);
	g_free(dest);
	g_free(preset);
}

void
audio_list_refresh(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean done;
	gint row = 0;
	const GValue *audio_list;

	g_debug("ghb_audio_list_refresh ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
	{
		do
		{
			const gchar *track, *codec, *br, *sr, *mix;
			gchar *s_drc, *s_gain;
			gint itrack;
			gdouble drc, gain;
			GValue *asettings;

			audio_list = ghb_settings_get_value(ud->settings, "audio_list");
			if (row >= ghb_array_len(audio_list))
				return;
			asettings = ghb_array_get_nth(audio_list, row);

			track = ghb_settings_combo_option(asettings, "AudioTrack");
			itrack = ghb_settings_combo_int(asettings, "AudioTrack");
			codec = ghb_settings_combo_option(asettings, "AudioEncoderActual");
			br = ghb_settings_combo_option(asettings, "AudioBitrate");
			sr = ghb_settings_combo_option(asettings, "AudioSamplerate");
			mix = ghb_settings_combo_option(asettings, "AudioMixdown");
			gain = ghb_settings_get_double(asettings, "AudioTrackGain");
			s_gain = g_strdup_printf("%.fdB", gain);

			drc = ghb_settings_get_double(asettings, "AudioTrackDRCSlider");
			if (drc < 1.0)
				s_drc = g_strdup("Off");
			else
				s_drc = g_strdup_printf("%.1f", drc);

			gtk_list_store_set(GTK_LIST_STORE(store), &iter, 
				// These are displayed in list
				0, track,
				1, codec,
				2, br,
				3, sr,
				4, mix,
				5, s_gain,
				6, s_drc,
				-1);
			g_free(s_drc);
			g_free(s_gain);
			done = !gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
			row++;
		} while (!done);
	}
}

static gboolean
validate_settings(signal_user_data_t *ud)
{
	// Check to see if the dest file exists or is
	// already in the queue
	gchar *message, *dest;
	gint count, ii;
	gint titleindex;

	titleindex = ghb_settings_combo_int(ud->settings, "title");
	if (titleindex < 0) return FALSE;
	dest = ghb_settings_get_string(ud->settings, "destination");
	count = ghb_array_len(ud->queue);
	for (ii = 0; ii < count; ii++)
	{
		GValue *js;
		gchar *filename;

		js = ghb_array_get_nth(ud->queue, ii);
		filename = ghb_settings_get_string(js, "destination");
		if (strcmp(dest, filename) == 0)
		{
			message = g_strdup_printf(
						"Destination: %s\n\n"
						"Another queued job has specified the same destination.\n"
						"Do you want to overwrite?",
						dest);
			if (!ghb_message_dialog(GTK_MESSAGE_QUESTION, message, "Cancel", "Overwrite"))
			{
				g_free(filename);
				g_free(dest);
				g_free(message);
				return FALSE;
			}
			g_free(message);
			break;
		}
		g_free(filename);
	}
	gchar *destdir = g_path_get_dirname(dest);
	if (!g_file_test(destdir, G_FILE_TEST_IS_DIR))
	{
		message = g_strdup_printf(
					"Destination: %s\n\n"
					"This is not a valid directory.",
					destdir);
		ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
		g_free(dest);
		g_free(message);
		g_free(destdir);
		return FALSE;
	}
#if !defined(_WIN32)
	// This doesn't work properly on windows
	if (g_access(destdir, R_OK|W_OK) != 0)
	{
		message = g_strdup_printf(
					"Destination: %s\n\n"
					"Can not read or write the directory.",
					destdir);
		ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
		g_free(dest);
		g_free(message);
		g_free(destdir);
		return FALSE;
	}
#endif
	GFile *gfile;
	GFileInfo *info;
	guint64 size;
	gchar *resolved = ghb_resolve_symlink(destdir);

	gfile = g_file_new_for_path(resolved);
	info = g_file_query_filesystem_info(gfile, 
						G_FILE_ATTRIBUTE_FILESYSTEM_FREE, NULL, NULL);
	if (info != NULL)
	{
		if (g_file_info_has_attribute(info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE))
		{
			size = g_file_info_get_attribute_uint64(info, 
									G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
			
			gint64 fsize = (guint64)10 * 1024 * 1024 * 1024;
			if (size < fsize)
			{
				message = g_strdup_printf(
							"Destination filesystem is almost full: %uM free\n\n"
							"Encode may be incomplete if you proceed.\n",
							(guint)(size / (1024L*1024L)));
				if (!ghb_message_dialog(GTK_MESSAGE_QUESTION, message, "Cancel", "Proceed"))
				{
					g_free(dest);
					g_free(message);
					return FALSE;
				}
				g_free(message);
			}
		}
		g_object_unref(info);
	}
	g_object_unref(gfile);
	g_free(resolved);
	g_free(destdir);
	if (g_file_test(dest, G_FILE_TEST_EXISTS))
	{
		message = g_strdup_printf(
					"Destination: %s\n\n"
					"File already exists.\n"
					"Do you want to overwrite?",
					dest);
		if (!ghb_message_dialog(GTK_MESSAGE_QUESTION, message, "Cancel", "Overwrite"))
		{
			g_free(dest);
			g_free(message);
			return FALSE;
		}
		g_free(message);
		g_unlink(dest);
	}
	g_free(dest);
	// Validate video quality is in a reasonable range
	if (!ghb_validate_vquality(ud->settings))
	{
		return FALSE;
	}
	// Validate audio settings
	if (!ghb_validate_audio(ud))
	{
		return FALSE;
	}
	// Validate audio settings
	if (!ghb_validate_subtitles(ud))
	{
		return FALSE;
	}
	// Validate video settings
	if (!ghb_validate_video(ud))
	{
		return FALSE;
	}
	// Validate filter settings
	if (!ghb_validate_filters(ud))
	{
		return FALSE;
	}
	audio_list_refresh(ud);
	return TRUE;
}

static gboolean
queue_add(signal_user_data_t *ud)
{
	// Add settings to the queue
	GValue *settings;
	gint titleindex;
	gint titlenum;
	
	g_debug("queue_add ()");
	if (!validate_settings(ud))
	{
		return FALSE;
	}

	if (ud->queue == NULL)
		ud->queue = ghb_array_value_new(32);
	// Make a copy of current settings to be used for the new job
	settings = ghb_value_dup(ud->settings);
	ghb_settings_set_int(settings, "job_status", GHB_QUEUE_PENDING);
	ghb_settings_set_int(settings, "job_unique_id", 0);
	titleindex = ghb_settings_combo_int(settings, "title");
	titlenum = ghb_get_title_number(titleindex);
	ghb_settings_set_int(settings, "titlenum", titlenum);
	ghb_array_append(ud->queue, settings);
	add_to_queue_list(ud, settings, NULL);
	ghb_save_queue(ud->queue);
	ghb_update_pending(ud);

	return TRUE;
}

G_MODULE_EXPORT void
queue_add_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	g_debug("queue_add_clicked_cb ()");
	queue_add(ud);
}

G_MODULE_EXPORT void
queue_remove_clicked_cb(GtkWidget *widget, gchar *path, signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gint row;
	gint *indices;
	gint unique_id;
	GValue *settings;
	gint status;

	g_debug("queue_remove_clicked_cb ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
	store = gtk_tree_view_get_model(treeview);
	treepath = gtk_tree_path_new_from_string (path);
	if (gtk_tree_path_get_depth(treepath) > 1) return;
	if (gtk_tree_model_get_iter(store, &iter, treepath))
	{
		// Find the entry in the queue
		indices = gtk_tree_path_get_indices (treepath);
		row = indices[0];
		// Can only free the treepath After getting what I need from
		// indices since this points into treepath somewhere.
		gtk_tree_path_free (treepath);
		if (row < 0) return;
		if (row >= ghb_array_len(ud->queue))
			return;
		settings = ghb_array_get_nth(ud->queue, row);
		status = ghb_settings_get_int(settings, "job_status");
		if (status == GHB_QUEUE_RUNNING)
		{
			// Ask if wants to stop encode.
			if (!ghb_cancel_encode2(ud, NULL))
			{
				return;
			}
			unique_id = ghb_settings_get_int(settings, "job_unique_id");
			ghb_remove_job(unique_id);
		}
		// Remove the selected item
		gtk_tree_store_remove(GTK_TREE_STORE(store), &iter);
		// Remove the corresponding item from the queue list
		GValue *old = ghb_array_get_nth(ud->queue, row);
		ghb_value_free(old);
		ghb_array_remove(ud->queue, row);
		ghb_save_queue(ud->queue);
	}
	else
	{	
		gtk_tree_path_free (treepath);
	}
	ghb_update_pending(ud);
}

static gint
find_last_finished(GValue *queue)
{
	GValue *js;
	gint ii, count;
	gint status;
	
	g_debug("find_last_finished");
	count = ghb_array_len(queue);
	for (ii = 0; ii < count; ii++)
	{
		js = ghb_array_get_nth(queue, ii);
		status = ghb_settings_get_int(js, "job_status");
		if (status != GHB_QUEUE_DONE && status != GHB_QUEUE_RUNNING)
		{
			return ii-1;
		}
	}
	return -1;
}

// This little bit is needed to prevent the default drag motion
// handler from expanding rows if you hover over them while
// dragging.
// Also controls where valid drop locations are
G_MODULE_EXPORT gboolean
queue_drag_motion_cb(
	GtkTreeView *tv,
	GdkDragContext *ctx,
	gint x,
	gint y,
	guint time,
	signal_user_data_t *ud)
{
	GtkTreePath *path = NULL;
	GtkTreeViewDropPosition pos;
	gint *indices, row, status, finished;
	GValue *js;
	GtkTreeIter iter;
	GtkTreeView *srctv;
	GtkTreeModel *model;
	GtkTreeSelection *select;
	GtkWidget *widget;

	widget = gtk_drag_get_source_widget(ctx);
	if (widget == NULL || widget != GTK_WIDGET(tv))
		return TRUE;

	// This bit checks to see if the source is allowed to be
	// moved.  Only pending and canceled items may be moved.
	srctv = GTK_TREE_VIEW(gtk_drag_get_source_widget(ctx));
	select = gtk_tree_view_get_selection (srctv);
	gtk_tree_selection_get_selected (select, &model, &iter);
	path = gtk_tree_model_get_path (model, &iter);
	indices = gtk_tree_path_get_indices(path);
	row = indices[0];
	gtk_tree_path_free(path);
	js = ghb_array_get_nth(ud->queue, row);
	status = ghb_settings_get_int(js, "job_status");
	if (status != GHB_QUEUE_PENDING && status != GHB_QUEUE_CANCELED)
	{
		gdk_drag_status(ctx, 0, time);
		return TRUE;
	}

	// The reset checks that the destination is a valid position
	// in the list.  Can not move above any finished or running items
	gtk_tree_view_get_dest_row_at_pos (tv, x, y, &path, &pos);
	if (path == NULL)
	{
		gdk_drag_status(ctx, GDK_ACTION_MOVE, time);
		return TRUE;
	}
	// Don't allow *drop into*
	if (pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
		pos = GTK_TREE_VIEW_DROP_BEFORE;
	if (pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
		pos = GTK_TREE_VIEW_DROP_AFTER;
	// Don't allow droping int child items
	if (gtk_tree_path_get_depth(path) > 1)
	{
		gtk_tree_path_up(path);
		pos = GTK_TREE_VIEW_DROP_AFTER;
	}
	indices = gtk_tree_path_get_indices(path);
	row = indices[0];
	js = ghb_array_get_nth(ud->queue, row);

	finished = find_last_finished(ud->queue);
	if (row < finished)
	{
		gtk_tree_path_free(path);
		gdk_drag_status(ctx, 0, time);
		return TRUE;
	}
	if (pos != GTK_TREE_VIEW_DROP_AFTER && 
		row == finished)
	{
		gtk_tree_path_free(path);
		gdk_drag_status(ctx, 0, time);
		return TRUE;
	}
	gtk_tree_view_set_drag_dest_row(tv, path, pos);
	gtk_tree_path_free(path);
	gdk_drag_status(ctx, GDK_ACTION_MOVE, time);
	return TRUE;
}

G_MODULE_EXPORT void 
queue_drag_cb(
	GtkTreeView *dstwidget, 
	GdkDragContext *dc, 
	gint x, gint y, 
	GtkSelectionData *selection_data, 
	guint info, guint t, 
	signal_user_data_t *ud)
{
	GtkTreePath *path = NULL;
	//GtkTreeModel *model;
	GtkTreeViewDropPosition pos;
	GtkTreeIter dstiter, srciter;
	gint *indices, row;
	GValue *js;
	
	GtkTreeModel *dstmodel = gtk_tree_view_get_model(dstwidget);
			
	g_debug("queue_drag_cb ()");
	// This doesn't work here for some reason...
	// gtk_tree_view_get_drag_dest_row(dstwidget, &path, &pos);
	gtk_tree_view_get_dest_row_at_pos (dstwidget, x, y, &path, &pos);
	// This little hack is needed because attempting to drop after
	// the last item gives us no path or pos.
	if (path == NULL)
	{
		gint n_children;

		n_children = gtk_tree_model_iter_n_children(dstmodel, NULL);
		if (n_children)
		{
			pos = GTK_TREE_VIEW_DROP_AFTER;
			path = gtk_tree_path_new_from_indices(n_children-1, -1);
		}
		else
		{
			pos = GTK_TREE_VIEW_DROP_BEFORE;
			path = gtk_tree_path_new_from_indices(0, -1);
		}
	}
	if (path)
	{
		if (gtk_tree_path_get_depth(path) > 1)
			gtk_tree_path_up(path);
		if (gtk_tree_model_get_iter (dstmodel, &dstiter, path))
		{
			GtkTreeIter iter;
			GtkTreeView *srcwidget;
			GtkTreeModel *srcmodel;
			GtkTreeSelection *select;
			GtkTreePath *srcpath = NULL;
			GtkTreePath *dstpath = NULL;

			srcwidget = GTK_TREE_VIEW(gtk_drag_get_source_widget(dc));
			//srcmodel = gtk_tree_view_get_model(srcwidget);
			select = gtk_tree_view_get_selection (srcwidget);
			gtk_tree_selection_get_selected (select, &srcmodel, &srciter);

			srcpath = gtk_tree_model_get_path (srcmodel, &srciter);
			indices = gtk_tree_path_get_indices(srcpath);
			row = indices[0];
			gtk_tree_path_free(srcpath);
			js = ghb_array_get_nth(ud->queue, row);

			switch (pos)
			{
				case GTK_TREE_VIEW_DROP_BEFORE:
				case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
					gtk_tree_store_insert_before (GTK_TREE_STORE (dstmodel), 
													&iter, NULL, &dstiter);
					break;

				case GTK_TREE_VIEW_DROP_AFTER:
				case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
					gtk_tree_store_insert_after (GTK_TREE_STORE (dstmodel), 
													&iter, NULL, &dstiter);
					break;

				default:
					break;
			}
			// Reset job to pending
			ghb_settings_set_int(js, "job_status", GHB_QUEUE_PENDING);
			add_to_queue_list(ud, js, &iter);

			dstpath = gtk_tree_model_get_path (dstmodel, &iter);
			indices = gtk_tree_path_get_indices(dstpath);
			row = indices[0];
			gtk_tree_path_free(dstpath);
			ghb_array_insert(ud->queue, row, js);

			srcpath = gtk_tree_model_get_path (srcmodel, &srciter);
			indices = gtk_tree_path_get_indices(srcpath);
			row = indices[0];
			gtk_tree_path_free(srcpath);
			ghb_array_remove(ud->queue, row);
			gtk_tree_store_remove (GTK_TREE_STORE (srcmodel), &srciter);
			ghb_save_queue(ud->queue);
		}
		gtk_tree_path_free(path);
	}
}

void
ghb_queue_buttons_grey(signal_user_data_t *ud)
{
	GtkWidget *widget;
	GtkAction *action;
	gint queue_count;
	gint titleindex;
	gint queue_state, scan_state;
	gboolean show_start, show_stop, paused;

	queue_count = ghb_array_len(ud->queue);
	titleindex = ghb_settings_combo_int(ud->settings, "title");

	queue_state = ghb_get_queue_state();
	scan_state = ghb_get_scan_state();

	show_stop = queue_state & 
				(GHB_STATE_WORKING | GHB_STATE_SEARCHING | 
				 GHB_STATE_SCANNING | GHB_STATE_MUXING);
	show_start = !(scan_state & GHB_STATE_SCANNING) && 
					(titleindex >= 0 || queue_count > 0);


	paused = queue_state & GHB_STATE_PAUSED;

	widget = GHB_WIDGET(ud->builder, "queue_add");
	gtk_widget_set_sensitive(widget, show_start);
	action = GHB_ACTION(ud->builder, "queue_add_menu");
	gtk_action_set_sensitive(action, show_start);

	widget = GHB_WIDGET (ud->builder, "queue_start1");
	if (show_stop)
	{
		gtk_widget_set_sensitive (widget, TRUE);
		gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-stop");
		gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), "Stop");
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), "Stop Encoding");
	}
	else
	{
		gtk_widget_set_sensitive (widget, show_start);
		gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-play");
		gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), "Start");
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), "Start Encoding");
	}
	widget = GHB_WIDGET (ud->builder, "queue_start2");
	if (show_stop)
	{
		gtk_widget_set_sensitive (widget, TRUE);
		gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-stop");
		gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), "Stop");
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), "Stop Encoding");
	}
	else
	{
		gtk_widget_set_sensitive (widget, show_start);
		gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-play");
		gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), "Start");
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), "Start Encoding");
	}
	widget = GHB_WIDGET (ud->builder, "queue_pause1");
	if (paused)
	{
		gtk_widget_set_sensitive (widget, show_stop);
		gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-play");
		gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), "Resume");
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), "Resume Encoding");
	}
	else
	{
		gtk_widget_set_sensitive (widget, show_stop);
		gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-pause");
		gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), "Pause");
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), "Pause Encoding");
	}
	widget = GHB_WIDGET (ud->builder, "queue_pause2");
	if (paused)
	{
		gtk_widget_set_sensitive (widget, show_stop);
		gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-play");
		gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), "Resume");
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), "Resume Encoding");
	}
	else
	{
		gtk_widget_set_sensitive (widget, show_stop);
		gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-pause");
		gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), "Pause");
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), "Pause Encoding");
	}

	action = GHB_ACTION (ud->builder, "queue_start_menu");
	if (show_stop)
	{
		gtk_action_set_sensitive (action, TRUE);
#if GTK_CHECK_VERSION(2, 16, 0)
		gtk_action_set_icon_name(action, "hb-stop");
		gtk_action_set_label(action, "S_top Queue");
		gtk_action_set_tooltip(action, "Stop Encoding");
#else
		g_object_set_property(G_OBJECT(action), "icon-name", 
											ghb_string_value("hb-stop"));
		g_object_set_property(G_OBJECT(action), "label",
											ghb_string_value("S_top Queue"));
		g_object_set_property(G_OBJECT(action), "tooltip",
											ghb_string_value("Stop Encoding"));
#endif
	}
	else
	{
		gtk_action_set_sensitive (action, show_start);
#if GTK_CHECK_VERSION(2, 16, 0)
		gtk_action_set_icon_name(action, "hb-play");
		gtk_action_set_label(action, "_Start Queue");
		gtk_action_set_tooltip(action, "Start Encoding");
#else
		g_object_set_property(G_OBJECT(action), "icon-name", 
											ghb_string_value("hb-play"));
		g_object_set_property(G_OBJECT(action), "label",
											ghb_string_value("_Start Queue"));
		g_object_set_property(G_OBJECT(action), "tooltip",
											ghb_string_value("Start Encoding"));
#endif
	}
	action = GHB_ACTION (ud->builder, "queue_pause_menu");
	if (paused)
	{
		gtk_action_set_sensitive (action, show_start);
#if GTK_CHECK_VERSION(2, 16, 0)
		gtk_action_set_icon_name(action, "hb-play");
		gtk_action_set_label(action, "_Resume Queue");
		gtk_action_set_tooltip(action, "Resume Encoding");
#else
		g_object_set_property(G_OBJECT(action), "icon-name", 
										ghb_string_value("hb-play"));
		g_object_set_property(G_OBJECT(action), "label",
										ghb_string_value("_Resume Queue"));
		g_object_set_property(G_OBJECT(action), "tooltip",
										ghb_string_value("Resume Encoding"));
#endif
	}
	else
	{
		gtk_action_set_sensitive (action, show_stop);
#if GTK_CHECK_VERSION(2, 16, 0)
		gtk_action_set_icon_name(action, "hb-pause");
		gtk_action_set_label(action, "_Pause Queue");
		gtk_action_set_tooltip(action, "Pause Encoding");
#else
		g_object_set_property(G_OBJECT(action), "icon-name", 
										ghb_string_value("hb-pause"));
		g_object_set_property(G_OBJECT(action), "label",
										ghb_string_value("_Pause Queue"));
		g_object_set_property(G_OBJECT(action), "tooltip",
										ghb_string_value("Pause Encoding"));
#endif
	}
}

G_MODULE_EXPORT void
queue_list_size_allocate_cb(GtkWidget *widget, GtkAllocation *allocation, GtkCellRenderer *cell)
{
	GtkTreeViewColumn *column;
	gint width;
	
	column = gtk_tree_view_get_column (GTK_TREE_VIEW(widget), 0);
	width = gtk_tree_view_column_get_width(column);
	g_debug("col width %d alloc width %d", width, allocation->width);
	// Set new wrap-width.  Shave a little off to accomidate the icons
	// that share this column.
	if (width >= 564) // Don't allow below a certain size
		g_object_set(cell, "wrap-width", width-70, NULL);
}

G_MODULE_EXPORT void
queue_start_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GValue *js;
	gboolean running = FALSE;
	gint count, ii;
	gint status;
	gint state;

	state = ghb_get_queue_state();
	if (state & (GHB_STATE_WORKING | GHB_STATE_SEARCHING | 
				 GHB_STATE_SCANNING | GHB_STATE_MUXING))
	{
		ghb_cancel_encode(ud, "You are currently encoding.  "
								"What would you like to do?");
		return;
	}

	count = ghb_array_len(ud->queue);
	for (ii = 0; ii < count; ii++)
	{
		js = ghb_array_get_nth(ud->queue, ii);
		status = ghb_settings_get_int(js, "job_status");
		if ((status == GHB_QUEUE_RUNNING) || 
			(status == GHB_QUEUE_PENDING))
		{
			running = TRUE;
			break;
		}
	}
	if (!running)
	{
		// The queue has no running or pending jobs.
		// Add current settings to the queue, then run.
		if (!queue_add(ud))
			return;
	}
	if (state == GHB_STATE_IDLE)
	{
		// Add the first pending queue item and start
		ud->current_job = ghb_start_next_job(ud, TRUE);
	}
}

G_MODULE_EXPORT void
queue_pause_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	ghb_pause_queue();
}

gboolean
ghb_reload_queue(signal_user_data_t *ud)
{
	GValue *queue;
	gint unfinished = 0;
	gint count, ii;
	gint pid;
	gint status;
	GValue *settings;
	gchar *message;

	g_debug("ghb_reload_queue");

find_pid:
	pid = ghb_find_pid_file();
	if (pid < 0)
		return FALSE;

	queue = ghb_load_old_queue(pid);
	ghb_remove_old_queue_file(pid);
	// Look for unfinished entries
	count = ghb_array_len(queue);
	for (ii = 0; ii < count; ii++)
	{
		settings = ghb_array_get_nth(queue, ii);
		status = ghb_settings_get_int(settings, "job_status");
		if (status != GHB_QUEUE_DONE && status != GHB_QUEUE_CANCELED)
		{
			unfinished++;
		}
	}
	if (!unfinished)
		goto find_pid;

	if (unfinished)
	{
		message = g_strdup_printf(
					"You have %d unfinished job%s in a saved queue.\n\n"
					"Would you like to reload %s?",
					unfinished, 
					(unfinished > 1) ? "s" : "",
					(unfinished > 1) ? "them" : "it");
		if (ghb_message_dialog(GTK_MESSAGE_QUESTION, message, "No", "Yes"))
		{
			GtkWidget *widget = GHB_WIDGET (ud->builder, "queue_window");
			gtk_widget_show (widget);
			widget = GHB_WIDGET (ud->builder, "show_queue");
			gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), TRUE);

			ud->queue = queue;
			// First get rid of any old items we don't want
			for (ii = count-1; ii >= 0; ii--)
			{
				settings = ghb_array_get_nth(queue, ii);
				status = ghb_settings_get_int(settings, "job_status");
				if (status == GHB_QUEUE_DONE || status == GHB_QUEUE_CANCELED)
				{
					GValue *old = ghb_array_get_nth(queue, ii);
					ghb_value_free(old);
					ghb_array_remove(queue, ii);
				}
			}
			count = ghb_array_len(queue);
			for (ii = 0; ii < count; ii++)
			{
				settings = ghb_array_get_nth(queue, ii);
				ghb_settings_set_int(settings, "job_unique_id", 0);
				ghb_settings_set_int(settings, "job_status", GHB_QUEUE_PENDING);
				add_to_queue_list(ud, settings, NULL);
			}
			ghb_queue_buttons_grey(ud);
			ghb_save_queue(ud->queue);
		}
		else
		{
			ghb_value_free(queue);
		}
		g_free(message);
	}
	return FALSE;
}

G_MODULE_EXPORT gboolean 
queue_key_press_cb(
	GtkWidget *widget, 
	GdkEventKey *event,
	signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gint row;
	gint *indices;
	gint unique_id;
	GValue *settings;
	gint status;

	g_debug("queue_key_press_cb ()");
	if (event->keyval != GDK_KEY_Delete)
		return FALSE;
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
	store = gtk_tree_view_get_model(treeview);

	selection = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		GtkTreePath *treepath;

		treepath = gtk_tree_model_get_path (store, &iter);
		// Find the entry in the queue
		indices = gtk_tree_path_get_indices (treepath);
		row = indices[0];
		// Can only free the treepath After getting what I need from
		// indices since this points into treepath somewhere.
		gtk_tree_path_free (treepath);
		if (row < 0) return FALSE;
		if (row >= ghb_array_len(ud->queue))
			return FALSE;
		settings = ghb_array_get_nth(ud->queue, row);
		status = ghb_settings_get_int(settings, "job_status");
		if (status == GHB_QUEUE_RUNNING)
		{
			// Ask if wants to stop encode.
			if (!ghb_cancel_encode2(ud, NULL))
			{
				return TRUE;
			}
			unique_id = ghb_settings_get_int(settings, "job_unique_id");
			ghb_remove_job(unique_id);
		}
		// Remove the selected item
		gtk_tree_store_remove(GTK_TREE_STORE(store), &iter);
		// Remove the corresponding item from the queue list
		GValue *old = ghb_array_get_nth(ud->queue, row);
		ghb_value_free(old);
		ghb_array_remove(ud->queue, row);
		ghb_save_queue(ud->queue);
		return TRUE;
	}
	return FALSE;
}

GValue *ghb_queue_edit_settings = NULL;

G_MODULE_EXPORT void
queue_edit_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gint row;
	gint *indices;
	gint status;

	g_debug("queue_key_press_cb ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
	store = gtk_tree_view_get_model(treeview);

	selection = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		GtkTreePath *treepath;

		treepath = gtk_tree_model_get_path (store, &iter);
		// Find the entry in the queue
		indices = gtk_tree_path_get_indices (treepath);
		row = indices[0];
		// Can only free the treepath After getting what I need from
		// indices since this points into treepath somewhere.
		gtk_tree_path_free (treepath);
		if (row < 0) return;
		if (row >= ghb_array_len(ud->queue))
			return;
		ghb_queue_edit_settings = ghb_array_get_nth(ud->queue, row);
		status = ghb_settings_get_int(ghb_queue_edit_settings, "job_status");
		if (status == GHB_QUEUE_PENDING)
		{
			// Remove the selected item
			gtk_tree_store_remove(GTK_TREE_STORE(store), &iter);
			// Remove the corresponding item from the queue list
			ghb_array_remove(ud->queue, row);
		}
		else
		{
			ghb_queue_edit_settings = ghb_value_dup(ghb_queue_edit_settings);
		}
		gchar *source;
		source = ghb_settings_get_string(ghb_queue_edit_settings, "source");
		ghb_do_scan(ud, source, 0, FALSE);
		g_free(source);
	}
}

