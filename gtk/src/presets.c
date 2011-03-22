/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * presets.c
 * Copyright (C) John Stebbins 2008-2011 <stebbins@stebbins>
 * 
 * presets.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <string.h>
#include "ghbcompat.h"
#include "hb.h"
#include "settings.h"
#include "callbacks.h"
#include "audiohandler.h"
#include "subtitlehandler.h"
#include "hb-backend.h"
#include "plist.h"
#include "resources.h"
#include "presets.h"
#include "values.h"
#include "lang.h"

#define MAX_NESTED_PRESET 3

enum
{
	PRESETS_BUILTIN = 0,
	PRESETS_CUSTOM
};

static GValue *presetsPlist = NULL;
static GValue *internalPlist = NULL;
static GValue *prefsPlist = NULL;
static gboolean prefs_modified = FALSE;

static const GValue* preset_dict_get_value(GValue *dict, const gchar *key);
static void store_plist(GValue *plist, const gchar *name);
static void store_presets(void);
static void store_prefs(void);

gint
preset_path_cmp(gint *indices1, gint len1, gint *indices2, gint len2)
{
	gint ii;
	for (ii = 0; ii < len1 && ii < len2; ii++)
	{
		if (indices1[ii] != indices2[ii])
			return indices1[ii] - indices2[ii];
	}
	return len1 - len2;
}

// This only handle limited depth
GtkTreePath*
ghb_tree_path_new_from_indices(gint *indices, gint len)
{
	switch (len)
	{
		case 1:
			return gtk_tree_path_new_from_indices(
				indices[0], -1);
		case 2:
			return gtk_tree_path_new_from_indices(
				indices[0], indices[1], -1);
		case 3:
			return gtk_tree_path_new_from_indices(
				indices[0], indices[1], indices[2], -1);
		case 4:
			return gtk_tree_path_new_from_indices(
				indices[0], indices[1], indices[2], indices[3], -1);
		case 5:
			return gtk_tree_path_new_from_indices(
				indices[0], indices[1], indices[2], indices[3], indices[4], -1);
		default:
			return NULL;
	}
}

GValue*
ghb_parse_preset_path(const gchar *path)
{
	gchar **split;
	GValue *preset;
	gint ii;

	preset = ghb_array_value_new(MAX_NESTED_PRESET);
	split = g_strsplit(path, "#", MAX_NESTED_PRESET);
	for (ii = 0; split[ii] != NULL; ii++)
	{
		ghb_array_append(preset, ghb_string_value_new(split[ii]));
	}
	g_strfreev(split);
	return preset;
}

static GValue*
preset_path_from_indices(GValue *presets, gint *indices, gint len)
{
	gint ii;
	GValue *path;

	g_debug("preset_path_from_indices");
	path = ghb_array_value_new(MAX_NESTED_PRESET);
	for (ii = 0; ii < len; ii++)
	{
		GValue *dict;
		gint count, folder;
		const GValue *name;

		count = ghb_array_len(presets);
		if (indices[ii] >= count) break;
		dict = ghb_array_get_nth(presets, indices[ii]);
		name = ghb_dict_lookup(dict, "PresetName");
		if (name)
			ghb_array_append(path, ghb_value_dup(name));
		folder = ghb_value_boolean(preset_dict_get_value(dict, "Folder"));
		if (!folder)
			break;
		presets = ghb_dict_lookup(dict, "ChildrenArray");
	}
	return path;
}

gchar*
ghb_preset_path_string(const GValue *path)
{
	gint count, ii;
	GString *gstr;
	GValue *val;
	gchar *str;

	gstr = g_string_new("");
	if (path != NULL)
	{
		count = ghb_array_len(path);
		for (ii = 0; ii < count; ii++)
		{
			val = ghb_array_get_nth(path, ii);
			str = ghb_value_string(val);
			g_string_append(gstr, str);
			if (ii < count-1)
				g_string_append(gstr, "->");
			g_free(str);
		}
	}
	str = g_string_free(gstr, FALSE);
	return str;
}

void
dump_preset_path(const gchar *msg, const GValue *path)
{
	gchar *str;

	if (path)
		debug_show_type (G_VALUE_TYPE(path));
	str = ghb_preset_path_string(path);
	g_message("%s path: (%s)", msg, str);
	g_free(str);
}

void
dump_preset_indices(const gchar *msg, gint *indices, gint len)
{
	gint ii;

	g_message("%s indices: len %d", msg, len);
	for (ii = 0; ii < len; ii++)
	{
		printf("%d ", indices[ii]);
	}
	printf("\n");
}

#if 0
static gint
preset_path_cmp(const GValue *path1, const GValue *path2)
{
	gint count, ii;
	GValue *val;
	gchar *str1, *str2;
	gint result;

	count = ghb_array_len(path1);
	ii = ghb_array_len(path2);
	if (ii != count)
		return ii - count;
	for (ii = 0; ii < count; ii++)
	{
		val = ghb_array_get_nth(path1, ii);
		str1 = ghb_value_string(val);
		val = ghb_array_get_nth(path2, ii);
		str2 = ghb_value_string(val);
		result = strcmp(str1, str2);
		if (result != 0)
			return result;
		g_free(str1);
		g_free(str2);
	}
	return 0;
}
#endif

static GValue*
presets_get_dict(GValue *presets, gint *indices, gint len)
{
	gint ii, count, folder;
	GValue *dict = NULL;

	g_debug("presets_get_dict ()");
	for (ii = 0; ii < len; ii++)
	{
		count = ghb_array_len(presets);
		if (indices[ii] >= count) return NULL;
		dict = ghb_array_get_nth(presets, indices[ii]);
		if (ii < len-1)
		{
			folder = ghb_value_boolean(preset_dict_get_value(dict, "Folder"));
			if (!folder)
				return NULL;
			presets = ghb_dict_lookup(dict, "ChildrenArray");
		}
	}
	if (ii < len)
		return NULL;
	return dict;
}

static GValue*
presets_get_folder(GValue *presets, gint *indices, gint len)
{
	gint ii, count, folder;
	GValue *dict;

	g_debug("presets_get_folder ()");
	for (ii = 0; ii < len; ii++)
	{
		count = ghb_array_len(presets);
		if (indices[ii] >= count) return NULL;
		dict = ghb_array_get_nth(presets, indices[ii]);
		folder = ghb_value_boolean(preset_dict_get_value(dict, "Folder"));
		if (!folder)
			break;
		presets = ghb_dict_lookup(dict, "ChildrenArray");
	}
	if (ii < len)
		return NULL;
	return presets;
}

static GValue*
plist_get_dict(GValue *presets, const gchar *name)
{
	if (presets == NULL || name == NULL) return NULL;
	return ghb_dict_lookup(presets, name);
}

static const gchar*
preset_get_name(GValue *dict)
{
	return g_value_get_string(preset_dict_get_value(dict, "PresetName"));
}

static gboolean
preset_folder_is_open(GValue *dict)
{
	const GValue *gval;

	gval = preset_dict_get_value(dict, "FolderOpen");
	if (gval != NULL)
		return g_value_get_boolean(gval);
	return FALSE;
}

gboolean
ghb_preset_folder(GValue *dict)
{
	return ghb_value_int(preset_dict_get_value(dict, "Folder"));
}

gint
ghb_preset_type(GValue *dict)
{
	return ghb_value_int(preset_dict_get_value(dict, "Type"));
}

static void
presets_remove_nth(GValue *presets, gint pos)
{
	GValue *dict;
	gint count;
	
	if (presets == NULL || pos < 0) return;
	count = ghb_array_len(presets);
	if (pos >= count) return;
	dict = ghb_array_get_nth(presets, pos);
	ghb_array_remove(presets, pos);
	ghb_value_free(dict);
}

gboolean
ghb_presets_remove(
	GValue *presets, 
	gint *indices,
	gint len)
{
	GValue *folder = NULL;

	folder = presets_get_folder(presets, indices, len-1);
	if (folder)
		presets_remove_nth(folder, indices[len-1]);
	else
	{
		g_warning("ghb_presets_remove (): internal preset lookup error");
		return FALSE;
	}
	return TRUE;
}

static void
ghb_presets_replace(
	GValue *presets, 
	GValue *dict,
	gint *indices,
	gint len)
{
	GValue *folder = NULL;

	folder = presets_get_folder(presets, indices, len-1);
	if (folder)
		ghb_array_replace(folder, indices[len-1], dict);
	else
	{
		g_warning("ghb_presets_replace (): internal preset lookup error");
	}
}

static void
ghb_presets_insert(
	GValue *presets, 
	GValue *dict,
	gint *indices,
	gint len)
{
	GValue *folder = NULL;

	folder = presets_get_folder(presets, indices, len-1);
	if (folder)
		ghb_array_insert(folder, indices[len-1], dict);
	else
	{
		g_warning("ghb_presets_insert (): internal preset lookup error");
	}
}

static gint
presets_find_element(GValue *presets, const gchar *name)
{
	GValue *dict;
	gint count, ii;
	
	g_debug("presets_find_element () (%s)", name);
	if (presets == NULL || name == NULL) return -1;
	count = ghb_array_len(presets);
	for (ii = 0; ii < count; ii++)
	{
		const gchar *str;
		dict = ghb_array_get_nth(presets, ii);
		str = preset_get_name(dict);
		if (strcmp(name, str) == 0)
		{
			return ii;
		}
	}
	return -1;
}

static gint
single_find_pos(GValue *presets, const gchar *name, gint type)
{
	GValue *dict;
	gint count, ii, ptype, last;
	
	if (presets == NULL || name == NULL) return -1;
	last = count = ghb_array_len(presets);
	for (ii = 0; ii < count; ii++)
	{
		const gchar *str;
		dict = ghb_array_get_nth(presets, ii);
		str = preset_get_name(dict);
		ptype = ghb_value_int(preset_dict_get_value(dict, "Type"));
		if (strcasecmp(name, str) <= 0 && ptype == type)
		{
			return ii;
		}
		if (ptype == type)
			last = ii+1;
	}
	return last;
}

static gint*
presets_find_pos(const GValue *path, gint type, gint *len)
{
	GValue *nested;
	GValue *val;
	gint count, ii;
	gboolean folder;
	gint *indices = NULL;
	const gchar *name;
	GValue *dict;

	g_debug("presets_find_pos () ");
	nested = presetsPlist;
	count = ghb_array_len(path);
	indices = g_malloc(MAX_NESTED_PRESET * sizeof(gint));
	for (ii = 0; ii < count-1; ii++)
	{
		val = ghb_array_get_nth(path, ii);
		name = g_value_get_string(val);
		indices[ii] = presets_find_element(nested, name);
		if (indices[ii] == -1) return NULL;
		dict = ghb_array_get_nth(nested, indices[ii]);
		folder = ghb_value_boolean(preset_dict_get_value(dict, "Folder"));
		nested = NULL;
		if (!folder)
			break;
		nested = ghb_dict_lookup(dict, "ChildrenArray");
	}
	if (nested)
	{
		const gchar *name;

		name = g_value_get_string(ghb_array_get_nth(path, count-1));
		indices[ii] = single_find_pos(nested, name, type);
		ii++;
	}
	*len = ii;
	return indices;
}

static gint
preset_tree_depth(GValue *dict)
{
	gboolean folder;

	folder = ghb_value_boolean(preset_dict_get_value(dict, "Folder"));
	if (folder)
	{
		gint depth = 0;
		gint count, ii;
		GValue *presets;

		presets = ghb_dict_lookup(dict, "ChildrenArray");
		count = ghb_array_len(presets);
		for (ii = 0; ii < count; ii++)
		{
			gint tmp;

			dict = ghb_array_get_nth(presets, ii);
			tmp = preset_tree_depth(dict);
			depth = MAX(depth, tmp);
		}
		return depth + 1;
	}
	else
	{
		return 1;
	}
}

static gboolean
preset_is_default(GValue *dict)
{
	const GValue *val;

	val = preset_dict_get_value(dict, "Default");
	return ghb_value_boolean(val);
}

static void
presets_clear_default(GValue *presets)
{
	gint count, ii;

	count = ghb_array_len(presets);
	for (ii = 0; ii < count; ii++)
	{
		GValue *dict;
		gboolean folder;

		dict = ghb_array_get_nth(presets, ii);
		folder = ghb_value_boolean(preset_dict_get_value(dict, "Folder"));
		if (folder)
		{
			GValue *nested;

			nested = ghb_dict_lookup(dict, "ChildrenArray");
			presets_clear_default(nested);
		}
		else
		{
			if (preset_is_default(dict))
			{
				ghb_dict_insert(dict, g_strdup("Default"), 
								ghb_boolean_value_new(FALSE));
			}
		}
	}
}

static void
presets_customize(GValue *presets)
{
	gint count, ii;

	count = ghb_array_len(presets);
	for (ii = 0; ii < count; ii++)
	{
		GValue *dict;
		gboolean folder;
		gint ptype;

		dict = ghb_array_get_nth(presets, ii);

		ptype = ghb_value_int(preset_dict_get_value(dict, "Type"));
		if (ptype != PRESETS_CUSTOM)
		{
			ghb_dict_insert(dict, g_strdup("Type"), 
						ghb_int64_value_new(PRESETS_CUSTOM));
		}
		folder = ghb_value_boolean(preset_dict_get_value(dict, "Folder"));
		if (folder)
		{
			GValue *nested;

			nested = ghb_dict_lookup(dict, "ChildrenArray");
			presets_customize(nested);
		}
	}
}

static gint*
presets_find_default2(GValue *presets, gint *len)
{
	gint count, ii;
	gint *indices;

	count = ghb_array_len(presets);
	for (ii = 0; ii < count; ii++)
	{
		GValue *dict;
		gboolean folder;

		dict = ghb_array_get_nth(presets, ii);
		folder = ghb_value_boolean(preset_dict_get_value(dict, "Folder"));
		if (folder)
		{
			GValue *nested;
			gint pos = *len;

			nested = ghb_dict_lookup(dict, "ChildrenArray");
			(*len)++;
			indices = presets_find_default2(nested, len);
			if (indices)
			{
				indices[pos] = ii;
				return indices;
			}
			else
				*len = pos;
		}
		else
		{
			if (preset_is_default(dict))
			{
				indices = g_malloc(MAX_NESTED_PRESET * sizeof(gint));
				indices[*len] = ii;
				(*len)++;
				return indices;
			}
		}
	}
	return NULL;
}

static gint*
presets_find_default(GValue *presets, gint *len)
{
	*len = 0;
	return presets_find_default2(presets, len);
}

gint*
ghb_preset_indices_from_path(
	GValue *presets, 
	const GValue *path,
	gint *len)
{
	GValue *nested;
	GValue *val;
	gint count, ii;
	gint *indices = NULL;
	const gchar *name;
	GValue *dict;
	gboolean folder;

	g_debug("ghb_preset_indices_from_path () ");
	nested = presets;
	count = ghb_array_len(path);
	if (count)
		indices = g_malloc(MAX_NESTED_PRESET * sizeof(gint));
	*len = 0;
	for (ii = 0; ii < count; ii++)
	{
		val = ghb_array_get_nth(path, ii);
		name = g_value_get_string(val);
		indices[ii] = presets_find_element(nested, name);
		if (indices[ii] == -1)
		{
			g_free(indices);
			return NULL;
		}
		if (ii < count-1)
		{
			dict = ghb_array_get_nth(nested, indices[ii]);
			folder = ghb_value_boolean(preset_dict_get_value(dict, "Folder"));
			if (!folder)
			{
				g_free(indices);
				return NULL;
			}
			nested = ghb_dict_lookup(dict, "ChildrenArray");
		}
	}
	*len = ii;
	return indices;
}

static gint
ghb_presets_get_type(
	GValue *presets, 
	gint *indices,
	gint len)
{
	GValue *dict;
	gint type = 0;

	dict = presets_get_dict(presets, indices, len);
	if (dict)
	{
		type = ghb_preset_type(dict);
	}
	else
	{
		g_warning("ghb_presets_get_type (): internal preset lookup error");
	}
	return type;
}

static gboolean
ghb_presets_get_folder(
	GValue *presets, 
	gint *indices,
	gint len)
{
	GValue *dict;
	gboolean folder = FALSE;

	dict = presets_get_dict(presets, indices, len);
	if (dict)
	{
		folder = ghb_preset_folder(dict);
	}
	else
	{
		g_warning("ghb_presets_get_folder (): internal preset lookup error");
	}
	return folder;
}

void
presets_set_default(gint *indices, gint len)
{
	GValue *dict;
	
	g_debug("presets_set_default ()");
	presets_clear_default(presetsPlist);
	dict = presets_get_dict(presetsPlist, indices, len);
	if (dict)
	{
		ghb_dict_insert(dict, g_strdup("Default"), ghb_boolean_value_new(TRUE));
	}
	store_presets();
}

static void
presets_set_folder_open(gboolean open, gint *indices, gint len)
{
	GValue *dict;
	
	g_debug("presets_set_folder_open ()");
	dict = presets_get_dict(presetsPlist, indices, len);
	if (dict)
	{
		ghb_dict_insert(dict, g_strdup("FolderOpen"), 
						ghb_boolean_value_new(open));
	}
}

// Used for sorting dictionaries.
gint
key_cmp(gconstpointer a, gconstpointer b)
{
	gchar *stra = (gchar*)a;
	gchar *strb = (gchar*)b;

	return strcmp(stra, strb);
}

static const GValue*
preset_dict_get_value(GValue *dict, const gchar *key)
{
	const GValue *gval = NULL;

	if (dict)
	{
		gval = ghb_dict_lookup(dict, key);
	}
	if (internalPlist == NULL) return NULL;
	if (gval == NULL)
	{
		dict = plist_get_dict(internalPlist, "Presets");
		if (dict == NULL) return NULL;
		gval = ghb_dict_lookup(dict, key);
	}
	return gval;
}

const gchar*
ghb_presets_get_description(GValue *pdict)
{
	const gchar *desc;

	if (pdict == NULL) return NULL;
	desc = g_value_get_string(
			preset_dict_get_value(pdict, "PresetDescription"));
	if (desc[0] == 0) return NULL;
	return desc;
}


static void init_settings_from_dict(
	GValue *dest, GValue *internal, GValue *dict);

static void
init_settings_from_array(
	GValue *dest, 
	GValue *internal,
	GValue *array)
{
	GValue *gval, *val;
	gint count, ii;
	
	count = ghb_array_len(array);
	// The first element of the internal version is always the 
	// template for the allowed values
	gval = ghb_array_get_nth(internal, 0);
	for (ii = 0; ii < count; ii++)
	{
		val = NULL;
		val = ghb_array_get_nth(array, ii);
		if (val == NULL)
			val = gval;
		if (G_VALUE_TYPE(gval) == ghb_dict_get_type())
		{
			GValue *new_dict;
			new_dict = ghb_dict_value_new();
			ghb_array_append(dest, new_dict);
			if (G_VALUE_TYPE(val) == ghb_dict_get_type())
				init_settings_from_dict(new_dict, gval, val);
			else
				init_settings_from_dict(new_dict, gval, gval);
		}
		else if (G_VALUE_TYPE(gval) == ghb_array_get_type())
		{
			GValue *new_array;
			new_array = ghb_array_value_new(8);
			ghb_array_append(dest, new_array);
			if (G_VALUE_TYPE(val) == ghb_array_get_type())
				init_settings_from_array(new_array, gval, val);
			else
				init_settings_from_array(new_array, gval, gval);
		}
		else
		{
			ghb_array_append(dest, val);
		}
	}
}

static void
init_settings_from_dict(
	GValue *dest, 
	GValue *internal,
	GValue *dict)
{
	GHashTableIter iter;
	gchar *key;
	GValue *gval, *val;
	
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
	{
		val = NULL;
		if (dict)
			val = ghb_dict_lookup(dict, key);
		if (val == NULL)
			val = gval;
		if (G_VALUE_TYPE(gval) == ghb_dict_get_type())
		{
			GValue *new_dict;
			new_dict = ghb_dict_value_new();
			ghb_settings_take_value(dest, key, new_dict);
			if (G_VALUE_TYPE(val) == ghb_dict_get_type())
				init_settings_from_dict(new_dict, gval, val);
			else
				init_settings_from_dict(new_dict, gval, gval);
		}
		else if (G_VALUE_TYPE(gval) == ghb_array_get_type())
		{
			GValue *new_array;
			new_array = ghb_array_value_new(8);
			ghb_settings_take_value(dest, key, new_array);
			if (G_VALUE_TYPE(val) == ghb_array_get_type())
				init_settings_from_array(new_array, gval, val);
			else
				init_settings_from_array(new_array, gval, gval);
	
		}
		else
		{
			ghb_settings_set_value(dest, key, val);
		}
	}
}

void
init_ui_from_dict(
	signal_user_data_t *ud, 
	GValue *internal,
	GValue *dict)
{
	GHashTableIter iter;
	gchar *key;
	GValue *gval, *val;
	
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
	{
		val = NULL;
		if (dict)
			val = ghb_dict_lookup(dict, key);
		if (val == NULL)
			val = gval;
		ghb_ui_update(ud, key, val);
	}
}

static void
preset_to_ui(signal_user_data_t *ud, GValue *dict)
{
	g_debug("preset_to_ui()\n");
	// Initialize the ui from presets file.
	GValue *internal, *hidden;

	// Get key list from internal default presets.  This way we do not
	// load any unknown keys.
	if (internalPlist == NULL) return;
	internal = plist_get_dict(internalPlist, "Presets");
	hidden = plist_get_dict(internalPlist, "XlatPresets");
	// Setting a ui widget will cause the corresponding setting
	// to be set, but it also triggers a callback that can 
	// have the side effect of using other settings values
	// that have not yet been set.  So set *all* settings first
	// then update the ui.
	init_settings_from_dict(ud->settings, internal, dict);
	init_settings_from_dict(ud->settings, hidden, dict);
	init_ui_from_dict(ud, internal, dict);
	init_ui_from_dict(ud, hidden, dict);

	if (dict != NULL)
	{
		GValue *val;
		gboolean dd;

		val = ghb_dict_lookup(dict, "PictureDecombDeinterlace");
		if (val != NULL)
		{
			dd = ghb_value_boolean(val);
			ghb_ui_update(ud, "PictureDeinterlaceDecomb", ghb_boolean_value(!dd));
		}
		val = ghb_dict_lookup(dict, "PictureHeight");
		if (val != NULL)
		{
			ghb_ui_update(ud, "scale_height", val);
		}
		val = ghb_dict_lookup(dict, "PictureWidth");
		if (val != NULL)
		{
			ghb_ui_update(ud, "scale_width", val);
		}
	}
}

void
ghb_settings_to_ui(signal_user_data_t *ud, GValue *dict)
{
	init_ui_from_dict(ud, dict, dict);
}

static GValue *current_preset = NULL;

gboolean
ghb_preset_is_custom()
{
	const GValue *val;

	if (current_preset == NULL) return FALSE;
	val = preset_dict_get_value(current_preset, "Type");
	return (ghb_value_int(val) == 1);
}

void
ghb_set_preset_from_indices(signal_user_data_t *ud, gint *indices, gint len)
{
	GValue *dict = NULL;
	gint fallback[2] = {0, -1};

	if (indices)
		dict = presets_get_dict(presetsPlist, indices, len);
	if (dict == NULL)
	{
		indices = fallback;
		len = 1;
		dict = presets_get_dict(presetsPlist, indices, len);
	}
	if (dict == NULL)
	{
		preset_to_ui(ud, NULL);
		current_preset = NULL;
	}
	else
	{
		GValue *path;
		gboolean folder;

		current_preset = dict;
		folder = ghb_value_boolean(preset_dict_get_value(dict, "Folder"));
		if (folder)
			preset_to_ui(ud, NULL);
		else
			preset_to_ui(ud, dict);
		path = preset_path_from_indices(presetsPlist, indices, len);
		ghb_settings_set_value(ud->settings, "preset", path);
		ghb_value_free(path);
	}
}

static const GValue*
curr_preset_get_value(const gchar *key)
{
	if (current_preset == NULL) return NULL;
	return preset_dict_get_value(current_preset, key);
}

void
ghb_update_from_preset(
	signal_user_data_t *ud, 
	const gchar *key)
{
	const GValue *gval;
	
	g_debug("ghb_update_from_preset() %s", key);
	gval = curr_preset_get_value(key);
	if (gval != NULL)
	{
		ghb_ui_update(ud, key, gval);
	}
}

static void
ghb_select_preset2(
	GtkBuilder *builder, 
	gint *indices, 
	gint len)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkTreePath *path;
	
	g_debug("ghb_select_preset2()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(builder, "presets_list"));
	selection = gtk_tree_view_get_selection (treeview);
	store = gtk_tree_view_get_model (treeview);
	path = ghb_tree_path_new_from_indices(indices, len);
	if (path)
	{
		if (gtk_tree_model_get_iter(store, &iter, path))
		{
			gtk_tree_selection_select_iter (selection, &iter);
		}
		else
		{
			if (gtk_tree_model_get_iter_first(store, &iter))
				gtk_tree_selection_select_iter (selection, &iter);
		}
		gtk_tree_path_free(path);
	}
}

void
ghb_select_preset(GtkBuilder *builder, const GValue *path)
{
	gint *indices, len;

	g_debug("ghb_select_preset()");
	indices = ghb_preset_indices_from_path(presetsPlist, path, &len);
	if (indices)
	{
		ghb_select_preset2(builder, indices, len);
		g_free(indices);
	}
}

void
ghb_select_default_preset(GtkBuilder *builder)
{
	gint *indices, len;

	g_debug("ghb_select_default_preset()");
	indices = presets_find_default(presetsPlist, &len);
	if (indices)
	{
		ghb_select_preset2(builder, indices, len);
		g_free(indices);
	}
}

gchar*
ghb_get_user_config_dir(gchar *subdir)
{
	const gchar *dir;
	gchar *config;

	dir = g_get_user_config_dir();
	if (!g_file_test(dir, G_FILE_TEST_IS_DIR))
	{
		dir = g_get_home_dir();
		config = g_strdup_printf ("%s/.ghb", dir);
		if (!g_file_test(config, G_FILE_TEST_IS_DIR))
			g_mkdir (config, 0755);
	}
	else
	{
		config = g_strdup_printf ("%s/ghb", dir);
		if (!g_file_test(config, G_FILE_TEST_IS_DIR))
			g_mkdir (config, 0755);
	}
	if (subdir)
	{
		gchar **split;
		gint ii;

		split = g_strsplit(subdir, G_DIR_SEPARATOR_S, -1);
		for (ii = 0; split[ii] != NULL; ii++)
		{
			gchar *tmp;

			tmp = g_strdup_printf ("%s/%s", config, split[ii]);
			g_free(config);
			config = tmp;
			if (!g_file_test(config, G_FILE_TEST_IS_DIR))
				g_mkdir (config, 0755);
		}
	}
	return config;
}

static void
store_plist(GValue *plist, const gchar *name)
{
	gchar *config, *path;
	FILE *file;

	config = ghb_get_user_config_dir(NULL);
	path = g_strdup_printf ("%s/%s", config, name);
	file = g_fopen(path, "w");
	g_free(config);
	g_free(path);
	ghb_plist_write(file, plist);
	fclose(file);
}

static GValue*
load_plist(const gchar *name)
{
	gchar *config, *path;
	GValue *plist = NULL;

	config = ghb_get_user_config_dir(NULL);
	path = g_strdup_printf ("%s/%s", config, name);
	if (g_file_test(path, G_FILE_TEST_IS_REGULAR))
	{
		plist = ghb_plist_parse_file(path);
	}
	g_free(config);
	g_free(path);
	return plist;
}

gboolean
ghb_lock_file(const gchar *name)
{
#if !defined(_WIN32)
	gchar *config, *path;
	int fd, lock = 0;

	config = ghb_get_user_config_dir(NULL);
	path = g_strdup_printf ("%s/%s", config, name);
	fd = open(path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
	if (fd >= 0)
		lock = lockf(fd, F_TLOCK, 0);
	if (lock)
		close(fd);
	g_free(config);
	g_free(path);
	return !lock;
#else
	return 1;
#endif
}

void
ghb_write_pid_file()
{
#if !defined(_WIN32)
	gchar *config, *path;
	pid_t pid;
	FILE *fp;
	int fd, lock;

	pid = getpid();

	config = ghb_get_user_config_dir(NULL);
	path = g_strdup_printf ("%s/ghb.pid.%d", config, pid);

	fp = g_fopen(path, "w");
	fprintf(fp, "%d\n", pid);
	fclose(fp);

	fd = open(path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
	lock = lockf(fd, F_TLOCK, 0);

	g_free(config);
	g_free(path);
#endif
}

void
ghb_unlink_pid_file(int pid)
{
	gchar *config, *path;

	config = ghb_get_user_config_dir(NULL);
	path = g_strdup_printf ("%s/ghb.pid.%d", config, pid);

	if (g_file_test(path, G_FILE_TEST_IS_REGULAR))
	{
		g_unlink(path);
	}

	g_free(config);
	g_free(path);
}

int
ghb_find_pid_file()
{
	const gchar *file;
	gchar *config;

	config = ghb_get_user_config_dir(NULL);

	if (g_file_test(config, G_FILE_TEST_IS_DIR))
	{
		GDir *gdir = g_dir_open(config, 0, NULL);
		file = g_dir_read_name(gdir);
		while (file)
		{
			if (strncmp(file, "ghb.pid.", 8) == 0)
			{
				gchar *path;
				pid_t my_pid;
				int pid;

				sscanf(file, "ghb.pid.%d", &pid);
				my_pid = getpid();
				if (my_pid == pid)
				{
					file = g_dir_read_name(gdir);
					continue;
				}
				path = g_strdup_printf("%s/%s", config, file);

#if !defined(_WIN32)
				int fd, lock = 1;

				fd = open(path, O_RDWR);
				if (fd >= 0)
				{
					lock = lockf(fd, F_TLOCK, 0);
				}
				if (lock == 0)
				{
					close(fd);
					g_dir_close(gdir);
					g_unlink(path);
					g_free(path);
					g_free(config);
					return pid;
				}
				g_free(path);
				close(fd);
#else
				g_dir_close(gdir);
				g_unlink(path);
				g_free(path);
				g_free(config);
				return pid;
#endif
			}
			file = g_dir_read_name(gdir);
		}
		g_dir_close(gdir);
	}
	g_free(config);
	return -1;
}

static void
remove_plist(const gchar *name)
{
	gchar *config, *path;

	config = ghb_get_user_config_dir(NULL);
	path = g_strdup_printf ("%s/%s", config, name);
	if (g_file_test(path, G_FILE_TEST_IS_REGULAR))
	{
		g_unlink(path);
	}
	g_free(path);
	g_free(config);
}

static gboolean prefs_initializing = FALSE;

void
ghb_prefs_to_ui(signal_user_data_t *ud)
{
	const GValue *gval;
	gchar *key;
	gchar *str;
	GValue *internal, *dict;
	GHashTableIter iter;
	

	g_debug("ghb_prefs_to_ui");
	prefs_initializing = TRUE;

	// Setting a ui widget will cause the corresponding setting
	// to be set, but it also triggers a callback that can 
	// have the side effect of using other settings values
	// that have not yet been set.  So set *all* settings first
	// then update the ui.
	internal = plist_get_dict(internalPlist, "Initialization");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
	{
		ghb_ui_update(ud, key, gval);
	}

	dict = plist_get_dict(prefsPlist, "Preferences");
	internal = plist_get_dict(internalPlist, "Preferences");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
	{
		const GValue *value = NULL;
		if (dict)
			value = ghb_dict_lookup(dict, key);
		if (value == NULL)
			value = gval;
		ghb_settings_set_value(ud->settings, key, value);
	}
	internal = plist_get_dict(internalPlist, "Preferences");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
   	{
		const GValue *value = NULL;
		if (dict)
			value = ghb_dict_lookup(dict, key);
		if (value == NULL)
			value = gval;
		ghb_ui_update(ud, key, value);
   	}
	const GValue *val;
	val = ghb_settings_get_value(ud->settings, "show_presets");
	ghb_ui_update(ud, "show_presets", val);
	if (ghb_settings_get_boolean(ud->settings, "hbfd_feature"))
	{
		GtkAction *action;
		val = ghb_settings_get_value(ud->settings, "hbfd");
		ghb_ui_update(ud, "hbfd", val);
		action = GHB_ACTION (ud->builder, "hbfd");
		gtk_action_set_visible(action, TRUE);
	}
	else
	{
		ghb_ui_update(ud, "hbfd", ghb_int64_value(0));
	}
	gval = ghb_settings_get_value(ud->settings, "default_source");
	ghb_settings_set_value (ud->settings, "scan_source", gval);

	str = ghb_settings_get_string(ud->settings, "destination_dir");
	ghb_ui_update(ud, "dest_dir", ghb_string_value(str));

	gchar *file = g_strdup_printf ("new_video.mp4");
	ghb_ui_update(ud, "dest_file", ghb_string_value(file));
	g_free(str);
	g_free(file);

	prefs_initializing = FALSE;
}

void
ghb_prefs_save(GValue *settings)
{
	GValue *dict;
	GValue *pref_dict;
	GHashTableIter iter;
	gchar *key;
	const GValue *value;
	
	if (prefs_initializing) return;
	dict = plist_get_dict(internalPlist, "Preferences");
	if (dict == NULL) return;
	pref_dict = plist_get_dict(prefsPlist, "Preferences");
	if (pref_dict == NULL) return;
	ghb_dict_iter_init(&iter, dict);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&value))
	{
		value = ghb_settings_get_value(settings, key);
		if (value != NULL)
		{
			ghb_dict_insert(pref_dict, g_strdup(key), ghb_value_dup(value));
		}
	}
	store_prefs();
	prefs_modified = FALSE;
}

void
ghb_pref_set(GValue *settings, const gchar *key)
{
	const GValue *value, *value2;
	
	if (prefs_initializing) return;
	value = ghb_settings_get_value(settings, key);
	if (value != NULL)
	{
		GValue *dict;
		dict = plist_get_dict(prefsPlist, "Preferences");
		if (dict == NULL) return;
		value2 = ghb_dict_lookup(dict, key);
		if (ghb_value_cmp(value, value2) != 0)
		{
			ghb_dict_insert(dict, g_strdup(key), ghb_value_dup(value));
			store_prefs();
			prefs_modified = TRUE;
		}
	}
}

void
ghb_pref_save(GValue *settings, const gchar *key)
{
	const GValue *value, *value2;
	
	if (prefs_initializing) return;
	value = ghb_settings_get_value(settings, key);
	if (value != NULL)
	{
		GValue *dict;
		dict = plist_get_dict(prefsPlist, "Preferences");
		if (dict == NULL) return;
		value2 = ghb_dict_lookup(dict, key);
		if (ghb_value_cmp(value, value2) != 0)
		{
			ghb_dict_insert(dict, g_strdup(key), ghb_value_dup(value));
			store_prefs();
			prefs_modified = FALSE;
		}
	}
}

void
ghb_prefs_store(void)
{
	if (prefs_modified)
	{
		store_prefs();
		prefs_modified = FALSE;
	}
}

void
ghb_settings_init(signal_user_data_t *ud)
{
	GValue *internal;
	GHashTableIter iter;
	gchar *key;
	GValue *gval;


	g_debug("ghb_settings_init");
	prefs_initializing = TRUE;

	internalPlist = ghb_resource_get("internal-defaults");
	// Setting a ui widget will cause the corresponding setting
	// to be set, but it also triggers a callback that can 
	// have the side effect of using other settings values
	// that have not yet been set.  So set *all* settings first
	// then update the ui.
	internal = plist_get_dict(internalPlist, "Initialization");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
	{
		ghb_settings_set_value(ud->settings, key, gval);
	}

	internal = plist_get_dict(internalPlist, "Presets");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
	{
		ghb_settings_set_value(ud->settings, key, gval);
	}

	internal = plist_get_dict(internalPlist, "Preferences");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
	{
		ghb_settings_set_value(ud->settings, key, gval);
	}
	prefs_initializing = FALSE;
}

void
ghb_settings_close()
{
	if (internalPlist)
		ghb_value_free(internalPlist);
	if (presetsPlist)
		ghb_value_free(presetsPlist);
	if (prefsPlist)
		ghb_value_free(prefsPlist);
}

#if defined(_WIN32)
gchar*
FindFirstCDROM(void)
{
	gint ii, drives;
	gchar drive[5];

	strcpy(drive, "A:" G_DIR_SEPARATOR_S);
	drives = GetLogicalDrives();
	for (ii = 0; ii < 26; ii++)
	{
		if (drives & 0x01)
		{
			guint dtype;

			drive[0] = 'A' + ii;
			dtype = GetDriveType(drive);
			if (dtype == DRIVE_CDROM)
			{
				return g_strdup(drive);
			}
		}
		drives >>= 1;
	}
	return NULL;
}
#endif

void
ghb_prefs_load(signal_user_data_t *ud)
{
	GValue *dict, *internal;
	GHashTableIter iter;
	gchar *key;
	GValue *gval, *path;
	
	g_debug("ghb_prefs_load");
	prefsPlist = load_plist("preferences");
	if (prefsPlist == NULL)
		prefsPlist = ghb_dict_value_new();
	dict = plist_get_dict(prefsPlist, "Preferences");
	internal = plist_get_dict(internalPlist, "Preferences");
	if (dict == NULL && internal)
	{
		dict = ghb_dict_value_new();
		ghb_dict_insert(prefsPlist, g_strdup("Preferences"), dict);

		// Get defaults from internal defaults 
		ghb_dict_iter_init(&iter, internal);
		// middle (void*) cast prevents gcc warning "defreferencing type-punned
		// pointer will break strict-aliasing rules"
		while (g_hash_table_iter_next(
				&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&gval))
		{
			ghb_dict_insert(dict, g_strdup(key), ghb_value_dup(gval));
		}

		const gchar *dir = g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);
		if (dir == NULL)
		{
			dir = ".";
		}
		ghb_dict_insert(dict, 
			g_strdup("ExportDirectory"), ghb_value_dup(ghb_string_value(dir)));

		dir = g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS);
		if (dir == NULL)
		{
			dir = ".";
		}
		ghb_dict_insert(dict, 
			g_strdup("destination_dir"), ghb_value_dup(ghb_string_value(dir)));

		ghb_dict_insert(dict, 
			g_strdup("SrtDir"), ghb_value_dup(ghb_string_value(dir)));
#if defined(_WIN32)
		gchar *source;

		source = FindFirstCDROM();
		if (source == NULL)
		{
			source = g_strdup("C:" G_DIR_SEPARATOR_S);
		}
		ghb_dict_insert(dict, g_strdup("default_source"), 
						ghb_value_dup(ghb_string_value(source)));
		g_free(source);
#endif
		store_prefs();
	}
	// Read legacy default_preset preference and update accordingly
	path = ghb_dict_lookup(dict, "default_preset");
	if (path)
	{
		gint *indices, len;

		if (G_VALUE_TYPE(path) == G_TYPE_STRING)
		{
			GValue *str = path;

			path = ghb_array_value_new(1);
			ghb_array_append(path, ghb_value_dup(str));
			indices = ghb_preset_indices_from_path(presetsPlist, path, &len);
			ghb_value_free(path);
		}
		else
			indices = ghb_preset_indices_from_path(presetsPlist, path, &len);

		if (indices)
		{
			presets_set_default(indices, len);
			g_free(indices);
		}
		ghb_dict_remove(dict, "default_preset");
		store_prefs();
	}
}

static const gchar*
get_preset_color(gint type, gboolean folder)
{
	const gchar *color;

	if (type == PRESETS_CUSTOM)
	{
		color = "DimGray";
		if (folder)
		{
			color = "black";
		}
	}
	else
	{
		color = "blue";
		if (folder)
		{
			color = "Navy";
		}
	}
	return color;
}

void
ghb_presets_list_init(
	signal_user_data_t *ud, 
	gint *indices,
	gint len)
{
	GtkTreeView *treeview;
	GtkTreeIter iter, titer, *piter;
	
	GtkTreeStore *store;
	const gchar *preset;
	GtkTreePath *parent_path;
	const gchar *description;
	gboolean def;
	gint count, ii;
	GValue *dict;
	gint *more_indices;
	GValue *presets = NULL;
	
	g_debug("ghb_presets_list_init ()");
	more_indices = g_malloc((len+1)*sizeof(gint));
	memcpy(more_indices, indices, len*sizeof(gint));
	presets = presets_get_folder(presetsPlist, indices, len);
	if (presets == NULL)
	{
		g_warning("Failed to find parent folder when adding child.");
		return;
	}
	count = ghb_array_len(presets);
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	parent_path = ghb_tree_path_new_from_indices(indices, len);
	if (parent_path)
	{
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &titer, parent_path);
		piter = &titer;
		gtk_tree_path_free(parent_path);
	}
	else
	{
		piter = NULL;
	}
	for (ii = 0; ii < count; ii++)
	{
		const gchar *color;
		gint type;
		gboolean folder;

		// Additional settings, add row
		dict = ghb_array_get_nth(presets, ii);
		preset = preset_get_name(dict);
		more_indices[len] = ii;
		def = preset_is_default(dict);

		description = ghb_presets_get_description(dict);
		gtk_tree_store_append(store, &iter, piter);
		type = ghb_preset_type(dict);
		folder = ghb_preset_folder(dict);
		color = get_preset_color(type, folder);
		gtk_tree_store_set(store, &iter, 0, preset, 
						   	1, def ? 800 : 400, 
						   	2, def ? 2 : 0,
						   	3, color, 
							4, description,
							5, type == PRESETS_BUILTIN ? 0 : 1,
						   	-1);
		if (def && piter)
		{
			GtkTreePath *path;
			GtkTreeIter ppiter;

			if (gtk_tree_model_iter_parent(
				GTK_TREE_MODEL(store), &ppiter, piter))
			{
				path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &ppiter);
				gtk_tree_view_expand_row(treeview, path, FALSE);
				gtk_tree_path_free(path);
			}
			path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), piter);
			gtk_tree_view_expand_row(treeview, path, FALSE);
			gtk_tree_path_free(path);
		}
		if (folder)
		{
			ghb_presets_list_init(ud, more_indices, len+1);
			if (preset_folder_is_open(dict))
			{
				GtkTreePath *path;

				if (piter != NULL)
				{
					path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), piter);
					gtk_tree_view_expand_row(treeview, path, FALSE);
					gtk_tree_path_free(path);
				}
				path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
				gtk_tree_view_expand_row(treeview, path, FALSE);
				gtk_tree_path_free(path);
			}
		}
	}
	g_free(more_indices);
}

static void
presets_list_update_item(
	signal_user_data_t *ud, 
	gint *indices,
	gint len,
	gboolean recurse)
{
	GtkTreeView *treeview;
	GtkTreeStore *store;
	GtkTreeIter iter;
	GtkTreePath *treepath;
	const gchar *name;
	const gchar *description;
	gint type;
	gboolean def, folder;
	GValue *dict;
	const gchar *color;
	
	g_debug("presets_list_update_item ()");
	dict = presets_get_dict(presetsPlist, indices, len);
	if (dict == NULL)
		return;
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	treepath = ghb_tree_path_new_from_indices(indices, len);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath);
	// Additional settings, add row
	name = preset_get_name(dict);
	def = preset_is_default(dict);

	description = ghb_presets_get_description(dict);
	type = ghb_preset_type(dict);
	folder = ghb_preset_folder(dict);
	color = get_preset_color(type, folder);
	gtk_tree_store_set(store, &iter, 0, name, 
					   	1, def ? 800 : 400, 
					   	2, def ? 2 : 0,
					   	3, color,
						4, description,
						5, type == PRESETS_BUILTIN ? 0 : 1,
					   	-1);
	if (recurse && folder)
	{
		ghb_presets_list_init(ud, indices, len);
	}
}

static void
presets_list_insert(
	signal_user_data_t *ud, 
	gint *indices,
	gint len)
{
	GtkTreeView *treeview;
	GtkTreeIter iter, titer, *piter;
	GtkTreeStore *store;
	const gchar *preset;
	const gchar *description;
	gint type;
	gboolean def, folder;
	gint count;
	GValue *presets;
	GtkTreePath *parent_path;
	GValue *dict;
	const gchar *color;
	
	g_debug("presets_list_insert ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	presets = presets_get_folder(presetsPlist, indices, len-1);
	if (presets == NULL)
	{
		g_warning("Failed to find parent folder while adding child.");
		return;
	}
	parent_path = ghb_tree_path_new_from_indices(indices, len-1);
	if (parent_path)
	{
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &titer, parent_path);
		piter = &titer;
		gtk_tree_path_free(parent_path);
	}
	else
	{
		piter = NULL;
	}
	count = ghb_array_len(presets);
	if (indices[len-1] >= count)
		return;
	// Additional settings, add row
	dict = ghb_array_get_nth(presets, indices[len-1]);
	preset = preset_get_name(dict);
	def = preset_is_default(dict);

	description = ghb_presets_get_description(dict);
	gtk_tree_store_insert(store, &iter, piter, indices[len-1]);
	type = ghb_preset_type(dict);
	folder = ghb_preset_folder(dict);
	color = get_preset_color(type, folder);
	gtk_tree_store_set(store, &iter, 0, preset, 
					   	1, def ? 800 : 400, 
					   	2, def ? 2 : 0,
					   	3, color,
						4, description,
						5, type == PRESETS_BUILTIN ? 0 : 1,
					   	-1);
	if (folder)
	{
		ghb_presets_list_init(ud, indices, len);
	}
}

static void
presets_list_remove(
	signal_user_data_t *ud, 
	gint *indices,
	gint len)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeIter iter;
	GtkTreeStore *store;
	
	g_debug("presets_list_remove ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	treepath = ghb_tree_path_new_from_indices(indices, len);
	if (treepath)
	{
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath))
			gtk_tree_store_remove(store, &iter);
		gtk_tree_path_free(treepath);
	}
}

static void
remove_std_presets(signal_user_data_t *ud)
{
	gint count, ii;
	gint indices = 0;

	count = ghb_array_len(presetsPlist);
	for (ii = count-1; ii >= 0; ii--)
	{
		GValue *dict;
		gint ptype;

		dict = ghb_array_get_nth(presetsPlist, ii);
		ptype = ghb_value_int(preset_dict_get_value(dict, "Type"));
		if (ptype == PRESETS_BUILTIN)
		{
			if (ghb_presets_remove(presetsPlist, &indices, 1))
			{
				presets_list_remove(ud, &indices, 1);
			}
		}
	}
}

void
ghb_save_queue(GValue *queue)
{
	pid_t pid;
	char *path;

	pid = getpid();
	path = g_strdup_printf ("queue.%d", pid);
	store_plist(queue, path);
	g_free(path);
}

GValue*
ghb_load_queue()
{
	GValue *queue;
	pid_t pid;
	char *path;

	pid = getpid();
	path = g_strdup_printf ("queue.%d", pid);
	queue = load_plist(path);
	g_free(path);
	return queue;
}

GValue*
ghb_load_old_queue(int pid)
{
	GValue *queue;
	char *path;

	path = g_strdup_printf ("queue.%d", pid);
	queue = load_plist(path);
	g_free(path);
	return queue;
}

void
ghb_remove_old_queue_file(int pid)
{
	char *path;

	path = g_strdup_printf ("queue.%d", pid);
	remove_plist(path);
	g_free(path);
}

void
ghb_remove_queue_file()
{
	pid_t pid;
	char *path;

	pid = getpid();
	path = g_strdup_printf ("queue.%d", pid);
	remove_plist(path);
	g_free(path);
}

typedef struct
{
	gchar *mac_val;
	gchar *lin_val;
} value_map_t;

static value_map_t vcodec_xlat[] =
{
	{"MPEG-4 (FFmpeg)", "ffmpeg"},
	{"MPEG-4 (XviD)", "ffmpeg"},
	{"H.264 (x264)", "x264"},
	{"VP3 (Theora)", "theora"},
	{NULL,NULL}
};

static value_map_t acodec_xlat[] =
{
	{"AAC (faac)", "faac"},
	{"AAC (CoreAudio)", "faac"},
	{"AC3 Passthru", "ac3pass"},
	{"DTS Passthru", "dtspass"},
	{"MP3 (lame)", "lame"},
	{"Vorbis (vorbis)", "vorbis"},
	{NULL,NULL}
};

value_map_t container_xlat[] =
{
	{"MP4 file", "mp4"},
	{"M4V file", "mp4"},
	{"MKV file", "mkv"},
	{"AVI file", "mkv"},
	{"OGM file", "mkv"},
	{NULL, NULL}
};

value_map_t framerate_xlat[] =
{
	{"Same as source", "source"},
	{"5", "5"},
	{"10", "10"},
	{"12", "12"},
	{"15", "15"},
	{"23.976 (NTSC Film)", "23.976"},
	{"24", "24"},
	{"25 (PAL Film/Video)", "25"},
	{"29.97 (NTSC Video)", "29.97"},
	{NULL, NULL}
};

value_map_t samplerate_xlat[] =
{
	{"Auto", "source"},
	{"22.05", "22.05"},
	{"24", "24"},
	{"32", "32"},
	{"44.1", "44.1"},
	{"48", "48"},
	{NULL, NULL}
};

value_map_t mix_xlat[] =
{
	{"Mono", "mono"},
	{"Stereo", "stereo"},
	{"Dolby Surround", "dpl1"},
	{"Dolby Pro Logic II", "dpl2"},
	{"6-channel discrete", "6ch"},
	{"AC3 Passthru", "none"},
	{NULL, NULL}
};

value_map_t deint_xlat[] =
{
	{"0", "off"},
	{"1", "custom"},
	{"2", "fast"},
	{"3", "slow"},
	{"4", "slower"},
	{NULL, NULL}
};

value_map_t denoise_xlat[] =
{
	{"0", "off"},
	{"1", "custom"},
	{"2", "weak"},
	{"3", "medium"},
	{"4", "strong"},
	{NULL, NULL}
};

value_map_t detel_xlat[] =
{
	{"0", "off"},
	{"1", "custom"},
	{"2", "default"},
	{NULL, NULL}
};

value_map_t decomb_xlat[] =
{
	{"0", "off"},
	{"1", "custom"},
	{"2", "default"},
	{NULL, NULL}
};

extern iso639_lang_t ghb_language_table[];

static GValue*
export_lang_xlat2(GValue *lin_val)
{
	GValue *gval;

	if (lin_val == NULL) return NULL;
	gint ii;
	gchar *str;

	str = ghb_value_string(lin_val);
	for (ii = 0; ghb_language_table[ii].eng_name; ii++)
	{
		if (strcmp(str, ghb_language_table[ii].iso639_2) == 0)
		{
			const gchar *lang;

			if (ghb_language_table[ii].native_name[0] != 0)
				lang = ghb_language_table[ii].native_name;
			else
				lang = ghb_language_table[ii].eng_name;

			gval = ghb_string_value_new(lang);
			g_free(str);
			return gval;
		}
	}
	g_debug("Can't map language value: (%s)", str);
	g_free(str);
	return NULL;
}

static GValue*
export_subtitle_xlat2(GValue *lin_val)
{
	gchar *str;
	GValue *gval;

	if (lin_val == NULL) return NULL;
	str = ghb_value_string(lin_val);
	if (strcmp(str, "none") == 0)
	{
		gval = ghb_string_value_new("None");
	}
	else if (strcmp(str, "auto") == 0)
	{
		gval = ghb_string_value_new("Autoselect");
	}
	else
	{
		gval = export_lang_xlat2(lin_val);
	}
	g_free(str);
	return gval;
}

static GValue*
import_lang_xlat2(GValue *mac_val)
{
	GValue *gval;

	if (mac_val == NULL) return NULL;
	gint ii;
	gchar *str;

	str = ghb_value_string(mac_val);
	for (ii = 0; ghb_language_table[ii].eng_name; ii++)
	{
		if ((strcmp(str, ghb_language_table[ii].eng_name) == 0) ||
			(strcmp(str, ghb_language_table[ii].native_name) == 0))
		{
			gval = ghb_string_value_new(ghb_language_table[ii].iso639_2);
			g_free(str);
			return gval;
		}
	}
	g_debug("Can't map language value: (%s)", str);
	g_free(str);
	return NULL;
}

static GValue*
import_subtitle_xlat2(GValue *mac_val)
{
	gchar *str;
	GValue *gval;

	if (mac_val == NULL) return NULL;
	str = ghb_value_string(mac_val);
	if (strcmp(str, "None") == 0)
	{
		gval = ghb_string_value_new("none");
	}
	else if (strcmp(str, "Autoselect") == 0)
	{
		gval = ghb_string_value_new("auto");
	}
	else
	{
		gval = import_lang_xlat2(mac_val);
	}
	g_free(str);
	return gval;
}

static GValue*
export_audio_track_xlat2(GValue *lin_val)
{
	gchar *str;
	GValue *gval = NULL;

	if (lin_val == NULL) return NULL;
	str = ghb_value_string(lin_val);
	if (strcmp(str, "none") == 0)
	{
		gval = ghb_int_value_new(1);
	}
	else
	{
		gint val = ghb_value_int(lin_val) + 1;
		gval = ghb_int_value_new(val);
	}
	g_free(str);
	return gval;
}

static GValue*
import_audio_track_xlat2(GValue *mac_val)
{
	gint val;
	gchar *str;
	GValue *gval;

	if (mac_val == NULL) return NULL;
	val = ghb_value_int(mac_val);
	if (val <= 0)
	{
		val = 0;
	}
	else
	{
		val--;
	}
	str = g_strdup_printf("%d", val);
	gval = ghb_string_value_new(str);
	g_free(str);
	return gval;
}

static GValue*
export_value_xlat2(value_map_t *value_map, GValue *lin_val, GType mac_type)
{
	GValue *gval;

	if (lin_val == NULL) return NULL;
	gint ii;
	gchar *str;
	GValue *sval;

	str = ghb_value_string(lin_val);
	for (ii = 0; value_map[ii].mac_val; ii++)
	{
		if (strcmp(str, value_map[ii].lin_val) == 0)
		{
			sval = ghb_string_value_new(value_map[ii].mac_val);
			g_free(str);
			gval = ghb_value_new(mac_type);
			if (!g_value_transform(sval, gval))
			{
				g_warning("can't transform");
				ghb_value_free(gval);
				ghb_value_free(sval);
				return NULL;
			}
			ghb_value_free(sval);
			return gval;
		}
	}
	g_debug("Can't map value: (%s)", str);
	g_free(str);
	return NULL;
}

static void
export_value_xlat(GValue *dict)
{
	GValue *lin_val, *gval;
	const gchar *key;

	key = "VideoEncoder";
	lin_val = ghb_dict_lookup(dict, key);
	gval = export_value_xlat2(vcodec_xlat, lin_val, G_TYPE_STRING);
	if (gval)
		ghb_dict_insert(dict, g_strdup(key), gval);
	key = "FileFormat";
	lin_val = ghb_dict_lookup(dict, key);
	gval = export_value_xlat2(container_xlat, lin_val, G_TYPE_STRING);
	if (gval)
		ghb_dict_insert(dict, g_strdup(key), gval);
	key = "VideoFramerate";
	lin_val = ghb_dict_lookup(dict, key);
	gval = export_value_xlat2(framerate_xlat, lin_val, G_TYPE_STRING);
	if (gval)
		ghb_dict_insert(dict, g_strdup(key), gval);
	key = "PictureDetelecine";
	lin_val = ghb_dict_lookup(dict, key);
	gval = export_value_xlat2(detel_xlat, lin_val, G_TYPE_INT);
	if (gval)
		ghb_dict_insert(dict, g_strdup(key), gval);
	key = "PictureDecomb";
	lin_val = ghb_dict_lookup(dict, key);
	gval = export_value_xlat2(decomb_xlat, lin_val, G_TYPE_INT);
	if (gval)
		ghb_dict_insert(dict, g_strdup(key), gval);
	key = "PictureDeinterlace";
	lin_val = ghb_dict_lookup(dict, key);
	gval = export_value_xlat2(deint_xlat, lin_val, G_TYPE_INT);
	if (gval)
		ghb_dict_insert(dict, g_strdup(key), gval);
	key = "PictureDenoise";
	lin_val = ghb_dict_lookup(dict, key);
	gval = export_value_xlat2(denoise_xlat, lin_val, G_TYPE_INT);
	if (gval)
		ghb_dict_insert(dict, g_strdup(key), gval);

	GValue *slist;
	GValue *sdict;
	gint count, ii;

	slist = ghb_dict_lookup(dict, "SubtitleList");
	count = ghb_array_len(slist);
	for (ii = 0; ii < count; ii++)
	{
		sdict = ghb_array_get_nth(slist, ii);
		key = "SubtitleLanguage";
		lin_val = ghb_dict_lookup(sdict, key);
		gval = export_subtitle_xlat2(lin_val);
		if (gval)
			ghb_dict_insert(sdict, g_strdup(key), gval);
	}

	GValue *alist;
	GValue *adict;

	alist = ghb_dict_lookup(dict, "AudioList");
	count = ghb_array_len(alist);
	for (ii = 0; ii < count; ii++)
	{
		adict = ghb_array_get_nth(alist, ii);
		key = "AudioTrack";
		lin_val = ghb_dict_lookup(adict, key);
		gval = export_audio_track_xlat2(lin_val);
		if (gval)
			ghb_dict_insert(adict, g_strdup(key), gval);
		key = "AudioEncoder";
		lin_val = ghb_dict_lookup(adict, key);
		gval = export_value_xlat2(acodec_xlat, lin_val, G_TYPE_STRING);
		if (gval)
			ghb_dict_insert(adict, g_strdup(key), gval);
		key = "AudioSamplerate";
		lin_val = ghb_dict_lookup(adict, key);
		gval = export_value_xlat2(samplerate_xlat, lin_val, G_TYPE_STRING);
		if (gval)
			ghb_dict_insert(adict, g_strdup(key), gval);
		key = "AudioMixdown";
		lin_val = ghb_dict_lookup(adict, key);
		gval = export_value_xlat2(mix_xlat, lin_val, G_TYPE_STRING);
		if (gval)
			ghb_dict_insert(adict, g_strdup(key), gval);
	}
}


static GValue*
import_value_xlat2(
	GValue *defaults, 
	value_map_t *value_map,
	const gchar *key, 
	GValue *mac_val)
{
	GValue *gval, *def_val;

	if (mac_val == NULL) return NULL;
	def_val = ghb_dict_lookup(defaults, key);
	if (def_val)
	{
		gint ii;
		gchar *str;
		GValue *sval;

		str = ghb_value_string(mac_val);
		for (ii = 0; value_map[ii].mac_val; ii++)
		{
			if (strcmp(str, value_map[ii].mac_val) == 0)
			{
				sval = ghb_string_value_new(value_map[ii].lin_val);
				g_free(str);
				gval = ghb_value_new(G_VALUE_TYPE(def_val));
				if (!g_value_transform(sval, gval))
				{
					g_warning("can't transform");
					ghb_value_free(gval);
					ghb_value_free(sval);
					return NULL;
				}
				ghb_value_free(sval);
				return gval;
			}
		}
		g_free(str);
	}
	else
	{
		gint ii;
		gchar *str;
		GValue *sval;

		str = ghb_value_string(mac_val);
		for (ii = 0; value_map[ii].mac_val; ii++)
		{
			if (strcmp(str, value_map[ii].mac_val) == 0)
			{
				sval = ghb_string_value_new(value_map[ii].lin_val);
				g_free(str);
				gval = ghb_value_new(G_VALUE_TYPE(mac_val));
				if (!g_value_transform(sval, gval))
				{
					g_warning("can't transform");
					ghb_value_free(gval);
					ghb_value_free(sval);
					return NULL;
				}
				ghb_value_free(sval);
				return gval;
			}
		}
		g_free(str);
	}
	return NULL;
}

static void
import_value_xlat(GValue *dict)
{
	GValue *defaults, *mac_val, *gval;
	const gchar *key;

	defaults = plist_get_dict(internalPlist, "Presets");
	key = "VideoEncoder";
	mac_val = ghb_dict_lookup(dict, key);
	gval = import_value_xlat2(defaults, vcodec_xlat, key, mac_val);
	if (gval)
		ghb_dict_insert(dict, g_strdup(key), gval);
	key = "FileFormat";
	mac_val = ghb_dict_lookup(dict, key);
	gval = import_value_xlat2(defaults, container_xlat, key, mac_val);
	if (gval)
		ghb_dict_insert(dict, g_strdup(key), gval);
	key = "VideoFramerate";
	mac_val = ghb_dict_lookup(dict, key);
	gval = import_value_xlat2(defaults, framerate_xlat, key, mac_val);
	if (gval)
		ghb_dict_insert(dict, g_strdup(key), gval);
	key = "PictureDetelecine";
	mac_val = ghb_dict_lookup(dict, key);
	gval = import_value_xlat2(defaults, detel_xlat, key, mac_val);
	if (gval)
		ghb_dict_insert(dict, g_strdup(key), gval);
	key = "PictureDecomb";
	mac_val = ghb_dict_lookup(dict, key);
	gval = import_value_xlat2(defaults, decomb_xlat, key, mac_val);
	if (gval)
		ghb_dict_insert(dict, g_strdup(key), gval);
	key = "PictureDeinterlace";
	mac_val = ghb_dict_lookup(dict, key);
	gval = import_value_xlat2(defaults, deint_xlat, key, mac_val);
	if (gval)
		ghb_dict_insert(dict, g_strdup(key), gval);
	key = "PictureDenoise";
	mac_val = ghb_dict_lookup(dict, key);
	gval = import_value_xlat2(defaults, denoise_xlat, key, mac_val);
	if (gval)
		ghb_dict_insert(dict, g_strdup(key), gval);


	GValue *sdeflist;
	GValue *sdefaults;
	GValue *slist;
	GValue *sdict;
	gint count, ii;

	sdeflist = ghb_dict_lookup(defaults, "SubtitleList");
	if (sdeflist)
	{
		slist = ghb_dict_lookup(dict, "SubtitleList");
		if (slist)
		{
			sdefaults = ghb_array_get_nth(sdeflist, 0);
			count = ghb_array_len(slist);
			for (ii = 0; ii < count; ii++)
			{
				sdict = ghb_array_get_nth(slist, ii);
				key = "SubtitleLanguage";
				mac_val = ghb_dict_lookup(sdict, key);
				gval = import_subtitle_xlat2(mac_val);
				if (gval)
					ghb_dict_insert(sdict, g_strdup(key), gval);
			}
		
		}
		else
		{
			key = "Subtitles";
			mac_val = ghb_dict_lookup(dict, key);
			slist = ghb_array_value_new(8);
			ghb_dict_insert(dict, g_strdup("SubtitleList"), slist);
			if (mac_val)
			{
				gchar *lang;
	
				gval = import_subtitle_xlat2(mac_val);
				lang = ghb_value_string(gval);
				if (lang && strcasecmp(lang, "none") != 0 && !slist)
				{
					sdict = ghb_dict_value_new();
					ghb_array_append(slist, sdict);
					ghb_dict_insert(sdict, g_strdup("SubtitleLanguage"), gval);
					gval = ghb_dict_lookup(dict, "SubtitlesForced");
					if (gval != NULL)
					{
						ghb_dict_insert(sdict, g_strdup("SubtitleForced"), 
										ghb_value_dup(gval));
					}
					else
					{
						ghb_dict_insert(sdict, g_strdup("SubtitleForced"), 
										ghb_boolean_value_new(FALSE));
					}
					ghb_dict_insert(sdict, g_strdup("SubtitleBurned"),
									ghb_boolean_value_new(TRUE));
					ghb_dict_insert(sdict, g_strdup("SubtitleDefaultTrack"),
									ghb_boolean_value_new(FALSE));
				}
				else
				{
					ghb_value_free(gval);
				}
				if (lang)
					g_free(lang);
			}
		}
	}
	ghb_dict_remove(dict, "Subtitles");
	ghb_dict_remove(dict, "SubtitlesForced");


	GValue *alist;
	GValue *adict;
	GValue *adefaults;
	GValue *adeflist;

	adeflist = ghb_dict_lookup(defaults, "AudioList");
	if (adeflist)
	{
		adefaults = ghb_array_get_nth(adeflist, 0);
		alist = ghb_dict_lookup(dict, "AudioList");
		count = ghb_array_len(alist);
		for (ii = 0; ii < count; ii++)
		{
			adict = ghb_array_get_nth(alist, ii);
			key = "AudioTrack";
			mac_val = ghb_dict_lookup(adict, key);
			gval = import_audio_track_xlat2(mac_val);
			if (gval)
				ghb_dict_insert(adict, g_strdup(key), gval);
			key = "AudioEncoder";
			mac_val = ghb_dict_lookup(adict, key);
			gval = import_value_xlat2(adefaults, acodec_xlat, key, mac_val);
			if (gval)
				ghb_dict_insert(adict, g_strdup(key), gval);
			key = "AudioSamplerate";
			mac_val = ghb_dict_lookup(adict, key);
			gval = import_value_xlat2(adefaults, samplerate_xlat, key, mac_val);
			if (gval)
				ghb_dict_insert(adict, g_strdup(key), gval);
			key = "AudioMixdown";
			mac_val = ghb_dict_lookup(adict, key);
			gval = import_value_xlat2(adefaults, mix_xlat, key, mac_val);
			if (gval)
				ghb_dict_insert(adict, g_strdup(key), gval);

			mac_val = ghb_dict_lookup(adict, "AudioTrackDRCSlider");
			if (mac_val != NULL)
			{
				gdouble drc;
				drc = ghb_value_double(mac_val);
				if (drc < 1.0 && drc > 0.0)
				{
					ghb_dict_insert(adict, g_strdup("AudioTrackDRCSlider"), 
									ghb_double_value_new(0.0));
				}
			}
		}
	}
}

static void
import_xlat_preset(GValue *dict)
{
	gboolean uses_max;
	gint uses_pic;
	gint par;
	gint vqtype;

	g_debug("import_xlat_preset ()");
	uses_max = ghb_value_boolean(
						preset_dict_get_value(dict, "UsesMaxPictureSettings"));
	uses_pic = ghb_value_int(
						preset_dict_get_value(dict, "UsesPictureSettings"));
	par = ghb_value_int(preset_dict_get_value(dict, "PicturePAR"));
	vqtype = ghb_value_int(preset_dict_get_value(dict, "VideoQualityType"));

	if (uses_max || uses_pic == 2)
	{
		ghb_dict_insert(dict, g_strdup("autoscale"), 
						ghb_boolean_value_new(TRUE));
	}
	switch (par)
	{
	case 0:
	{
		if (ghb_dict_lookup(dict, "PictureModulus") == NULL)
			ghb_dict_insert(dict, g_strdup("PictureModulus"), 
							ghb_int_value_new(16));
	} break;
	case 1:
	{
		ghb_dict_insert(dict, g_strdup("PictureModulus"), 
						ghb_int_value_new(1));
	} break;
	case 2:
	{
		if (ghb_dict_lookup(dict, "PictureModulus") == NULL)
			ghb_dict_insert(dict, g_strdup("PictureModulus"), 
							ghb_int_value_new(16));
	} break;
	default:
	{
		if (ghb_dict_lookup(dict, "PictureModulus") == NULL)
			ghb_dict_insert(dict, g_strdup("PictureModulus"), 
							ghb_int_value_new(16));
	} break;
	}
	// VideoQualityType/0/1/2 - vquality_type_/target/bitrate/constant
	// *note: target is no longer used
	switch (vqtype)
	{
	case 0:
	{
		ghb_dict_insert(dict, g_strdup("vquality_type_bitrate"), 
						ghb_boolean_value_new(TRUE));
		ghb_dict_insert(dict, g_strdup("vquality_type_constant"), 
						ghb_boolean_value_new(FALSE));
	} break;
	case 1:
	{
		ghb_dict_insert(dict, g_strdup("vquality_type_bitrate"), 
						ghb_boolean_value_new(TRUE));
		ghb_dict_insert(dict, g_strdup("vquality_type_constant"), 
						ghb_boolean_value_new(FALSE));
	} break;
	case 2:
	{
		ghb_dict_insert(dict, g_strdup("vquality_type_bitrate"), 
						ghb_boolean_value_new(FALSE));
		ghb_dict_insert(dict, g_strdup("vquality_type_constant"), 
						ghb_boolean_value_new(TRUE));
	} break;
	default:
	{
		ghb_dict_insert(dict, g_strdup("vquality_type_bitrate"), 
						ghb_boolean_value_new(FALSE));
		ghb_dict_insert(dict, g_strdup("vquality_type_constant"), 
						ghb_boolean_value_new(TRUE));
	} break;
	}

	import_value_xlat(dict);

	GValue *mode = ghb_dict_lookup(dict, "VideoFramerateMode");
	if (mode == NULL)
	{
		GValue *fr = ghb_dict_lookup(dict, "VideoFramerate");
		if (fr)
		{
			gchar *str;
			gboolean pfr = FALSE;
			GValue *pfr_val = ghb_dict_lookup(dict, "VideoFrameratePFR");
			if (pfr_val)
			{
				pfr = ghb_value_boolean(pfr_val);
			}
			str = ghb_value_string(fr);
			if (strcmp(str, "source") == 0)
			{
				ghb_dict_insert(dict, g_strdup("VideoFramerateCFR"), 
								ghb_boolean_value_new(FALSE));
				ghb_dict_insert(dict, g_strdup("VideoFramerateVFR"), 
								ghb_boolean_value_new(TRUE));
			}
			else if (!pfr)
			{
				ghb_dict_insert(dict, g_strdup("VideoFramerateCFR"), 
								ghb_boolean_value_new(TRUE));
				ghb_dict_insert(dict, g_strdup("VideoFramerateVFR"), 
								ghb_boolean_value_new(FALSE));
			}
			else
			{
				ghb_dict_insert(dict, g_strdup("VideoFramerateCFR"), 
								ghb_boolean_value_new(FALSE));
				ghb_dict_insert(dict, g_strdup("VideoFramerateVFR"), 
								ghb_boolean_value_new(FALSE));
			}
            g_free(str);
		}
	}
	else
	{
		gchar *str;
		str = ghb_value_string(mode);
		if (strcmp(str, "cfr") == 0)
		{
				ghb_dict_insert(dict, g_strdup("VideoFramerateCFR"), 
								ghb_boolean_value_new(TRUE));
				ghb_dict_insert(dict, g_strdup("VideoFrameratePFR"), 
								ghb_boolean_value_new(FALSE));
				ghb_dict_insert(dict, g_strdup("VideoFramerateVFR"), 
								ghb_boolean_value_new(FALSE));
		}
		else if (strcmp(str, "pfr") == 0)
		{
				ghb_dict_insert(dict, g_strdup("VideoFramerateCFR"), 
								ghb_boolean_value_new(FALSE));
				ghb_dict_insert(dict, g_strdup("VideoFrameratePFR"), 
								ghb_boolean_value_new(TRUE));
				ghb_dict_insert(dict, g_strdup("VideoFramerateVFR"), 
								ghb_boolean_value_new(FALSE));
		}
		else
		{
				ghb_dict_insert(dict, g_strdup("VideoFramerateCFR"), 
								ghb_boolean_value_new(FALSE));
				ghb_dict_insert(dict, g_strdup("VideoFrameratePFR"), 
								ghb_boolean_value_new(FALSE));
				ghb_dict_insert(dict, g_strdup("VideoFramerateVFR"), 
								ghb_boolean_value_new(TRUE));
		}
		g_free(str);
	}
}

static void
import_xlat_presets(GValue *presets)
{
	gint count, ii;
	GValue *dict;
	gboolean folder;

	g_debug("import_xlat_presets ()");
	if (presets == NULL) return;
	count = ghb_array_len(presets);
	for (ii = 0; ii < count; ii++)
	{
		dict = ghb_array_get_nth(presets, ii);
		folder = ghb_value_boolean(preset_dict_get_value(dict, "Folder"));
		if (folder)
		{
			GValue *nested;

			nested = ghb_dict_lookup(dict, "ChildrenArray");
			import_xlat_presets(nested);
		}
		else
		{
			import_xlat_preset(dict);
		}
	}
}

static void
export_xlat_preset(GValue *dict)
{
	gboolean autoscale, br, constant;

	g_debug("export_xlat_prest ()");
	autoscale = ghb_value_boolean(preset_dict_get_value(dict, "autoscale"));
	br = ghb_value_boolean(
				preset_dict_get_value(dict, "vquality_type_bitrate"));
	constant = ghb_value_boolean(
				preset_dict_get_value(dict, "vquality_type_constant"));

	if (autoscale)
		ghb_dict_insert(dict, g_strdup("UsesPictureSettings"), 
						ghb_int_value_new(2));
	else
		ghb_dict_insert(dict, g_strdup("UsesPictureSettings"), 
						ghb_int_value_new(1));

	// VideoQualityType/0/1/2 - vquality_type_/target/bitrate/constant
	// *note: target is no longer used
	if (br)
	{
		ghb_dict_insert(dict, g_strdup("VideoQualityType"), 
						ghb_int_value_new(1));
	}
	else if (constant)
	{
		ghb_dict_insert(dict, g_strdup("VideoQualityType"), 
						ghb_int_value_new(2));
	}

	if (ghb_value_boolean(preset_dict_get_value(dict, "VideoFramerateCFR")))
	{
		ghb_dict_insert(dict, g_strdup("VideoFramerateMode"), 
						ghb_string_value_new("cfr"));
	}
	else if (ghb_value_boolean(preset_dict_get_value(dict, "VideoFrameratePFR")))
	{
		ghb_dict_insert(dict, g_strdup("VideoFramerateMode"), 
						ghb_string_value_new("pfr"));
	}
	else
	{
		ghb_dict_insert(dict, g_strdup("VideoFramerateMode"), 
						ghb_string_value_new("vfr"));
	}

	GValue *alist, *adict;
	gint count, ii;

	alist = ghb_dict_lookup(dict, "AudioList");
	count = ghb_array_len(alist);
	for (ii = 0; ii < count; ii++)
	{
		gdouble drc;

		adict = ghb_array_get_nth(alist, ii);
		drc = ghb_value_double(
				preset_dict_get_value(adict, "AudioTrackDRCSlider"));
		if (drc < 1.0 && drc > 0.0)
		{
			ghb_dict_insert(adict, g_strdup("AudioTrackDRCSlider"), 
							ghb_double_value_new(0.0));
		}
	}

	GValue *internal;
	GHashTableIter iter;
	gchar *key;
	GValue *value;
	internal = plist_get_dict(internalPlist, "XlatPresets");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&value))
	{
		ghb_dict_remove(dict, key);
	}

	// remove obsolete keys
	ghb_dict_remove(dict, "UsesMaxPictureSettings");
	ghb_dict_remove(dict, "VFR");
	ghb_dict_remove(dict, "VideoFrameratePFR");

	export_value_xlat(dict);
}

static void
export_xlat_presets(GValue *presets)
{
	gint count, ii;
	GValue *dict;
	gboolean folder;

	if (presets == NULL) return;
	count = ghb_array_len(presets);
	for (ii = 0; ii < count; ii++)
	{
		dict = ghb_array_get_nth(presets, ii);
		folder = ghb_value_boolean(preset_dict_get_value(dict, "Folder"));
		if (folder)
		{
			GValue *nested;

			nested = ghb_dict_lookup(dict, "ChildrenArray");
			export_xlat_presets(nested);
		}
		else
		{
			export_xlat_preset(dict);
		}
	}
}

static guint prefs_timeout_id = 0;

static gboolean
delayed_store_prefs(gpointer data)
{
	store_plist(prefsPlist, "preferences");
	prefs_timeout_id = 0;
	return FALSE;
}

static void
store_presets()
{
	GValue *export;

	export = ghb_value_dup(presetsPlist);
	export_xlat_presets(export);
	store_plist(export, "presets");
	ghb_value_free(export);
}

static void
store_prefs(void)
{
	if (prefs_timeout_id != 0)
	{
		GMainContext *mc;
		GSource *source;

		mc = g_main_context_default();
		source = g_main_context_find_source_by_id(mc, prefs_timeout_id);
		if (source != NULL)
			g_source_destroy(source);
	}
	prefs_timeout_id = g_timeout_add_seconds(1, (GSourceFunc)delayed_store_prefs, NULL);
}

void
ghb_presets_reload(signal_user_data_t *ud)
{
	GValue *std_presets;
	gint count, ii;
	int *indices, len;

	g_debug("ghb_presets_reload()\n");
	std_presets = ghb_resource_get("standard-presets");
	if (std_presets == NULL) return;

	remove_std_presets(ud);
	indices = presets_find_default(presetsPlist, &len);
	if (indices)
	{
		presets_clear_default(std_presets);
		g_free(indices);
	}
	// Merge the keyfile contents into our presets
	count = ghb_array_len(std_presets);
	for (ii = count-1; ii >= 0; ii--)
	{
		GValue *std_dict;
		GValue *copy_dict;
		gint indices = 0;

		std_dict = ghb_array_get_nth(std_presets, ii);
		copy_dict = ghb_value_dup(std_dict);
		ghb_dict_insert(copy_dict, g_strdup("PresetBuildNumber"), 
						ghb_int64_value_new(hb_get_build(NULL)));
		ghb_presets_insert(presetsPlist, copy_dict, &indices, 1);
		presets_list_insert(ud, &indices, 1);
	}
	import_xlat_presets(presetsPlist);
	store_presets();
}

static gboolean
check_old_presets()
{
	gint count, ii;

	count = ghb_array_len(presetsPlist);
	for (ii = count-1; ii >= 0; ii--)
	{
		GValue *dict;
		GValue *type;

		dict = ghb_array_get_nth(presetsPlist, ii);
		type = ghb_dict_lookup(dict, "Type");
		if (type == NULL)
			return TRUE;
	}
	return FALSE;
}

static void
replace_standard_presets()
{
	GValue *std_presets;
	int *indices, len;
	gint count, ii;

	count = ghb_array_len(presetsPlist);
	for (ii = count-1; ii >= 0; ii--)
	{
		GValue *dict;
		gint ptype;

		dict = ghb_array_get_nth(presetsPlist, ii);
		ptype = ghb_value_int(preset_dict_get_value(dict, "Type"));
		if (ptype == PRESETS_BUILTIN)
		{
			gint indices = 0;
			ghb_presets_remove(presetsPlist, &indices, 1);
		}
	}

	std_presets = ghb_resource_get("standard-presets");
	if (std_presets == NULL) return;

	indices = presets_find_default(presetsPlist, &len);
	if (indices)
	{
		presets_clear_default(std_presets);
		g_free(indices);
	}
	// Merge the keyfile contents into our presets
	count = ghb_array_len(std_presets);
	for (ii = count-1; ii >= 0; ii--)
	{
		GValue *std_dict;
		GValue *copy_dict;
		gint indices = 0;

		std_dict = ghb_array_get_nth(std_presets, ii);
		copy_dict = ghb_value_dup(std_dict);
		ghb_dict_insert(copy_dict, g_strdup("PresetBuildNumber"), 
						ghb_int64_value_new(hb_get_build(NULL)));
		ghb_presets_insert(presetsPlist, copy_dict, &indices, 1);
	}
	import_xlat_presets(presetsPlist);
	store_presets();
}

static void
update_standard_presets(signal_user_data_t *ud)
{
	gint count, ii;

	count = ghb_array_len(presetsPlist);
	for (ii = count-1; ii >= 0; ii--)
	{
		GValue *dict;
		const GValue *gval;
		gint64 build;
		gint type;

		dict = ghb_array_get_nth(presetsPlist, ii);
		gval = ghb_dict_lookup(dict, "Type");
		if (gval == NULL)
		{
			// Old preset that doesn't have a Type
			replace_standard_presets();
			return;
		}
			
		type = ghb_value_int(gval);
		if (type == 0)
		{
			gval = ghb_dict_lookup(dict, "PresetBuildNumber");
			if (gval == NULL)
			{
				// Old preset that doesn't have a build number
				replace_standard_presets();
				return;
			}

			build = ghb_value_int64(gval);
			if (build != hb_get_build(NULL))
			{
				// Build number does not match
				replace_standard_presets();
				return;
			}
		}
	}
	return;
}

void
ghb_presets_load(signal_user_data_t *ud)
{
	presetsPlist = load_plist("presets");
	if (presetsPlist == NULL)
	{
		presetsPlist = ghb_value_dup(ghb_resource_get("standard-presets"));
		import_xlat_presets(presetsPlist);
		store_presets();
	}
	else if (G_VALUE_TYPE(presetsPlist) == ghb_dict_get_type())
	{ // Presets is older dictionary format. Convert to array
		ghb_value_free(presetsPlist);
		presetsPlist = ghb_value_dup(ghb_resource_get("standard-presets"));
		import_xlat_presets(presetsPlist);
		store_presets();
	}
	else if (check_old_presets())
	{
		ghb_value_free(presetsPlist);
		presetsPlist = ghb_value_dup(ghb_resource_get("standard-presets"));
		import_xlat_presets(presetsPlist);
		store_presets();
	}
	update_standard_presets(ud);
	import_xlat_presets(presetsPlist);
}

static void
settings_save(signal_user_data_t *ud, const GValue *path)
{
	GValue *dict, *internal;
	GHashTableIter iter;
	gchar *key;
	GValue *value;
	gint *indices, len, count;
	gint *def_indices, def_len;
	const gchar *name;
	gboolean replace = FALSE;

	g_debug("settings_save");
	if (internalPlist == NULL) return;
	count = ghb_array_len(path);
	name = g_value_get_string(ghb_array_get_nth(path, count-1));
	indices = ghb_preset_indices_from_path(presetsPlist, path, &len);
	if (indices)
	{
		if (ghb_presets_get_folder(presetsPlist, indices, len))
		{
			gchar *message;
			message = g_strdup_printf(
						"%s: Folder already exists.\n"
						"You can not replace it with a preset.",
						name);
			ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
			g_free(message);
			return;
		}
		dict = ghb_dict_value_new();
		ghb_presets_replace(presetsPlist, dict, indices, len);
		replace = TRUE;
	}
	else
	{
		indices = presets_find_pos(path, PRESETS_CUSTOM, &len);
		if (indices)
		{
			dict = ghb_dict_value_new();
			ghb_presets_insert(presetsPlist, dict, indices, len);
		}
		else
		{
			g_warning("failed to find insert path");
			return;
		}
	}
	current_preset = dict;
	ghb_settings_set_int64(ud->settings, "Type", PRESETS_CUSTOM);
	ghb_settings_set_int64(ud->settings, "PresetBuildNumber", hb_get_build(NULL));

	internal = plist_get_dict(internalPlist, "Presets");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&value))
	{
		const GValue *gval;

		gval = ghb_settings_get_value(ud->settings, key);
		if (gval == NULL)
		{
			continue;
		}
		ghb_dict_insert(dict, g_strdup(key), ghb_value_dup(gval));
	}
	internal = plist_get_dict(internalPlist, "XlatPresets");
	ghb_dict_iter_init(&iter, internal);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&value))
	{
		const GValue *gval;

		gval = ghb_settings_get_value(ud->settings, key);
		if (gval == NULL)
		{
			continue;
		}
		ghb_dict_insert(dict, g_strdup(key), ghb_value_dup(gval));
	}
	ghb_dict_insert(dict, g_strdup("PresetName"), ghb_string_value_new(name));
	if (replace)
	{
		def_indices = presets_find_default(presetsPlist, &def_len);
		if (def_indices != NULL && 
			preset_path_cmp(indices, len, def_indices, def_len) != 0)
		{
			ghb_dict_insert(dict, g_strdup("Default"), 
							ghb_boolean_value_new(FALSE));
		}
		presets_list_update_item(ud, indices, len, FALSE);
	}
	else
	{
		ghb_dict_insert(dict, g_strdup("Default"), 
						ghb_boolean_value_new(FALSE));
		presets_list_insert(ud, indices, len);
	}
	if (!ghb_settings_get_boolean( ud->settings, "PictureWidthEnable"))
	{
		ghb_dict_remove(dict, "PictureWidth");
	}
	if (!ghb_settings_get_boolean( ud->settings, "PictureHeightEnable"))
	{
		ghb_dict_remove(dict, "PictureHeight");
	}
	ghb_dict_insert(dict, g_strdup("autoscale"), 
		ghb_boolean_value_new(
			!ghb_settings_get_boolean( ud->settings, "PictureWidthEnable") &&
			!ghb_settings_get_boolean( ud->settings, "PictureHeightEnable")
		)
	);
	store_presets();
	ud->dont_clear_presets = TRUE;
	// Make the new preset the selected item
	ghb_select_preset2(ud->builder, indices, len);
	g_free(indices);
	ud->dont_clear_presets = FALSE;
	return;
}

static void
folder_save(signal_user_data_t *ud, const GValue *path)
{
	GValue *dict, *folder;
	gint *indices, len, count;
	const gchar *name;

	count = ghb_array_len(path);
	name = g_value_get_string(ghb_array_get_nth(path, count-1));
	indices = ghb_preset_indices_from_path(presetsPlist, path, &len);
	if (indices)
	{
		if (!ghb_presets_get_folder(presetsPlist, indices, len))
		{
			gchar *message;
			message = g_strdup_printf(
						"%s: Preset already exists.\n"
						"You can not replace it with a folder.",
						name);
			ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
			g_free(message);
			g_free(indices);
			return;
		}
		// Already exists, update its description
		dict = presets_get_dict(presetsPlist, indices, len);
		ghb_dict_insert(dict, g_strdup("PresetDescription"), 
			ghb_value_dup(preset_dict_get_value(
				ud->settings, "PresetDescription")));
		presets_list_update_item(ud, indices, len, FALSE);
		g_free(indices);
		store_presets();
		return;
	}
	else
	{
		indices = presets_find_pos(path, PRESETS_CUSTOM, &len);
		if (indices)
		{
			dict = ghb_dict_value_new();
			ghb_presets_insert(presetsPlist, dict, indices, len);
		}
		else
		{
			g_warning("failed to find insert path");
			return;
		}
	}
	ghb_dict_insert(dict, g_strdup("PresetDescription"), 
		ghb_value_dup(preset_dict_get_value(
			ud->settings, "PresetDescription")));
	ghb_dict_insert(dict, g_strdup("PresetName"), ghb_string_value_new(name));
	folder = ghb_array_value_new(8);
	ghb_dict_insert(dict, g_strdup("ChildrenArray"), folder);
	ghb_dict_insert(dict, g_strdup("Type"),
							ghb_int64_value_new(PRESETS_CUSTOM));
	ghb_dict_insert(dict, g_strdup("Folder"), ghb_boolean_value_new(TRUE));

	presets_list_insert(ud, indices, len);
	g_free(indices);
	store_presets();
	return;
}

void
ghb_presets_list_default(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeIter iter;
	GtkTreeStore *store;
	gint *indices, len;
	
	g_debug("ghb_presets_list_default ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	indices = presets_find_default(presetsPlist, &len);
	if (indices == NULL) return;
	treepath = ghb_tree_path_new_from_indices(indices, len);
	if (treepath)
	{
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath))
		{
			gtk_tree_store_set(store, &iter, 
						1, 800, 
						2, 2 ,
						-1);
		}
		gtk_tree_path_free(treepath);
	}
	g_free(indices);
}

void
ghb_presets_list_clear_default(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeIter iter;
	GtkTreeStore *store;
	gint *indices, len;
	
	g_debug("ghb_presets_list_clear_default ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	indices = presets_find_default(presetsPlist, &len);
	if (indices == NULL) return;
	treepath = ghb_tree_path_new_from_indices(indices, len);
	if (treepath)
	{
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath))
		{
			gtk_tree_store_set(store, &iter, 
						1, 400, 
						2, 0 ,
						-1);
		}
		gtk_tree_path_free(treepath);
	}
	g_free(indices);
}

static void
update_audio_presets(signal_user_data_t *ud)
{
	g_debug("update_audio_presets");
	const GValue *audio_list;

	audio_list = ghb_settings_get_value(ud->settings, "audio_list");
	ghb_settings_set_value(ud->settings, "AudioList", audio_list);
}

static void
update_subtitle_presets(signal_user_data_t *ud)
{
	g_debug("update_subtitle_presets");
	const GValue *subtitle_list, *subtitle;
	GValue *slist, *dict;
	gint count, ii, source;

	subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
	slist = ghb_array_value_new(8);
	count = ghb_array_len(subtitle_list);
	for (ii = 0; ii < count; ii++)
	{
		subtitle = ghb_array_get_nth(subtitle_list, ii);
		source = ghb_settings_get_int(subtitle, "SubtitleSource");
		if (source != SRTSUB)
		{
			dict = ghb_value_dup(subtitle);
			ghb_array_append(slist, dict);
		}
	}
	ghb_settings_take_value(ud->settings, "SubtitleList", slist);
}

G_MODULE_EXPORT void
presets_menu_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkMenu *menu;

	menu = GTK_MENU(GHB_WIDGET(ud->builder, "presets_menu"));
	gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 1, 
					gtk_get_current_event_time());
}

G_MODULE_EXPORT void
preset_import_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkWidget *dialog;
	GtkResponseType response;
	gchar *exportDir;
	gchar *filename;
	GtkFileFilter *filter;

	g_debug("preset_import_clicked_cb ()");

	dialog = gtk_file_chooser_dialog_new("Import Preset", NULL,
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "All (*)");
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Presets (*.plist)");
	gtk_file_filter_add_pattern(filter, "*.plist");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	exportDir = ghb_settings_get_string(ud->settings, "ExportDirectory");
	if (exportDir == NULL || exportDir[0] == '\0')
	{
		exportDir = g_strdup(".");
	}
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), exportDir);
	g_free(exportDir);

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);
	if (response == GTK_RESPONSE_ACCEPT)
	{
		GValue *dict, *array;
		gchar  *dir;
		gint count, ii;

		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		// import the preset
		if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR))
		{
			gtk_widget_destroy(dialog);
			g_free(filename);
			return;
		}
		array = ghb_plist_parse_file(filename);

		import_xlat_presets(array);
		presets_clear_default(array);
		presets_customize(array);

		count = ghb_array_len(array);
		for (ii = 0; ii < count; ii++)
		{
			GValue *path, *name;
			gint *indices, len;
			gint index = 1;

			dict = ghb_array_get_nth(array, ii);
			path = ghb_array_value_new(1);
			name = ghb_value_dup(ghb_dict_lookup(dict, "PresetName"));
			ghb_array_append(path, name);
			indices = ghb_preset_indices_from_path(presetsPlist, path, &len);
			// Modify the preset name till we make it unique
			while (indices != NULL)
			{
				gchar *str = ghb_value_string(name);

				ghb_value_free(path);
				g_free(indices);

				str = g_strdup_printf("%s %d", str, index);
				path = ghb_array_value_new(1);
				name = ghb_string_value_new(str);
				ghb_array_append(path, name);
				g_free(str);

				index++;
				indices = ghb_preset_indices_from_path(presetsPlist, path, &len);
			}
			ghb_dict_insert(dict, g_strdup("PresetName"), ghb_value_dup(name));
			indices = presets_find_pos(path, PRESETS_CUSTOM, &len);
			ghb_presets_insert(presetsPlist, ghb_value_dup(dict), indices, len);
			presets_list_insert(ud, indices, len);
			ghb_value_free(path);
		}
		ghb_value_free(array);

		exportDir = ghb_settings_get_string(ud->settings, "ExportDirectory");
		dir = g_path_get_dirname(filename);
		if (strcmp(dir, exportDir) != 0)
		{
			ghb_settings_set_string(ud->settings, "ExportDirectory", dir);
			ghb_pref_save(ud->settings, "ExportDirectory");
		}
		g_free(filename);
		g_free(exportDir);
		g_free(dir);
		store_presets();
	}
	gtk_widget_destroy(dialog);
}

G_MODULE_EXPORT void
preset_export_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkWidget *dialog;
	GtkResponseType response;
	GValue *preset;
	const gchar *name = "";
	gint count, *indices, len;
	gchar *exportDir;
	gchar *filename;

	g_debug("preset_export_clicked_cb ()");
	preset = ghb_settings_get_value (ud->settings, "preset_selection");
	if (preset == NULL)
		return;

	count = ghb_array_len(preset);
	if (count <= 0)
		return;

	name = g_value_get_string(ghb_array_get_nth(preset, count-1));

	dialog = gtk_file_chooser_dialog_new("Export Preset", NULL,
				GTK_FILE_CHOOSER_ACTION_SAVE,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				NULL);

	exportDir = ghb_settings_get_string(ud->settings, "ExportDirectory");
	if (exportDir == NULL || exportDir[0] == '\0')
	{
		exportDir = g_strdup(".");
	}
	filename = g_strdup_printf("%s.plist", name);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), exportDir);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), filename);
	g_free(filename);
	g_free(exportDir);

	indices = ghb_preset_indices_from_path(presetsPlist, preset, &len);
	if (indices == NULL)
		return;

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);
	if (response == GTK_RESPONSE_ACCEPT)
	{
		GValue *export, *dict, *array;
		FILE *file;
		gchar  *dir;

		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		// export the preset
		dict = presets_get_dict(presetsPlist, indices, len);

		export = ghb_value_dup(dict);
		array = ghb_array_value_new(1);
		ghb_array_append(array, export);
		presets_clear_default(array);
		presets_customize(array);
		export_xlat_presets(array);

		file = g_fopen(filename, "w");
		if (file != NULL)
		{
			ghb_plist_write(file, array);
			fclose(file);
		}
		ghb_value_free(array);

		exportDir = ghb_settings_get_string(ud->settings, "ExportDirectory");
		dir = g_path_get_dirname(filename);
		if (strcmp(dir, exportDir) != 0)
		{
			ghb_settings_set_string(ud->settings, "ExportDirectory", dir);
			ghb_pref_save(ud->settings, "ExportDirectory");
		}
		g_free(exportDir);
		g_free(dir);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
	g_free(indices);
}

G_MODULE_EXPORT void
presets_new_folder_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkWidget *dialog;
	GtkEntry *entry;
	GtkTextView *desc;
	GtkResponseType response;
	GValue *preset, *dict;
	const gchar *name = "";
	const gchar *description = "";
	gint count, *indices, len;

	g_debug("presets_new_folder_clicked_cb ()");
	preset = ghb_settings_get_value (ud->settings, "preset_selection");

	count = ghb_array_len(preset);
	if (count > 0)
		name = g_value_get_string(ghb_array_get_nth(preset, count-1));
	else
		count = 1;

	indices = ghb_preset_indices_from_path(presetsPlist, preset, &len);
	dict = presets_get_dict(presetsPlist, indices, len);
	if (dict != NULL)
	{
		description = g_value_get_string(
							ghb_dict_lookup(dict, "PresetDescription"));
		ghb_ui_update(ud, "FolderDescription", ghb_string_value(description));
	}

	desc = GTK_TEXT_VIEW(GHB_WIDGET(ud->builder, "FolderDescription"));
	dialog = GHB_WIDGET(ud->builder, "preset_new_folder_dialog");
	entry = GTK_ENTRY(GHB_WIDGET(ud->builder, "FolderName"));
	gtk_entry_set_text(entry, name);
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);
	if (response == GTK_RESPONSE_OK)
	{
		// save the preset
		const gchar *name = gtk_entry_get_text(entry);
		GValue *dest;

		if (count > MAX_NESTED_PRESET-1)
			count = MAX_NESTED_PRESET-1;

		dest = ghb_array_value_new(MAX_NESTED_PRESET);
		if (indices != NULL)
		{
			gint ptype;

			ptype = ghb_presets_get_type(presetsPlist, indices, len);
			if (ptype == PRESETS_CUSTOM)
			{
				ghb_array_copy(dest, preset, count-1);
			}
		}
		ghb_array_append(dest, ghb_string_value_new(name));
		GValue *val = ghb_widget_value(GTK_WIDGET(desc));
		ghb_settings_set_value(ud->settings, "PresetDescription", val);
		folder_save(ud, dest);
		ghb_value_free(dest);
	}
	if (indices != NULL)
		g_free(indices);
}

G_MODULE_EXPORT void
presets_save_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkWidget *dialog;
	GtkEntry *entry;
	GtkTextView *desc;
	GtkResponseType response;
	GValue *preset;
	const gchar *name = "";
	gint count, *indices, len;

	g_debug("presets_save_clicked_cb ()");
	preset = ghb_settings_get_value (ud->settings, "preset_selection");

	count = ghb_array_len(preset);
	if (count > 0)
		name = g_value_get_string(ghb_array_get_nth(preset, count-1));
	else
		count = 1;

	desc = GTK_TEXT_VIEW(GHB_WIDGET(ud->builder, "PresetDescription"));
	int width = ghb_settings_get_int(ud->settings, "PictureWidth");
	int height = ghb_settings_get_int(ud->settings, "PictureHeight");
	gboolean autoscale = ghb_settings_get_boolean(ud->settings, "autoscale");
	ghb_ui_update(ud, "PictureWidthEnable", 
		ghb_boolean_value(width!=0&&!autoscale));
	ghb_ui_update(ud, "PictureHeightEnable", 
		ghb_boolean_value(height!=0&&!autoscale));
	if (!width)
	{
		width = ghb_settings_get_int(ud->settings, "scale_width");
		ghb_ui_update(ud, "PictureWidth", ghb_int_value(width));
	}
	if (!height)
	{
		height = ghb_settings_get_int(ud->settings, "scale_height");
		ghb_ui_update(ud, "PictureHeight", ghb_int_value(height));
	}
	dialog = GHB_WIDGET(ud->builder, "preset_save_dialog");
	entry = GTK_ENTRY(GHB_WIDGET(ud->builder, "PresetName"));
	gtk_entry_set_text(entry, name);
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);
	if (response == GTK_RESPONSE_OK)
	{
		// save the preset
		const gchar *name = gtk_entry_get_text(entry);
		GValue *dest;

		dest = ghb_array_value_new(MAX_NESTED_PRESET);
		indices = ghb_preset_indices_from_path(presetsPlist, preset, &len);
		if (indices)
		{
			gint ptype;

			ptype = ghb_presets_get_type(presetsPlist, indices, len);
			if (ptype == PRESETS_CUSTOM)
			{
				ghb_array_copy(dest, preset, count-1);
			}
			g_free(indices);
		}
		ghb_array_append(dest, ghb_string_value_new(name));

		ghb_widget_to_setting(ud->settings, GTK_WIDGET(desc));

		// Construct the audio settings presets from the current audio list
		update_audio_presets(ud);
		update_subtitle_presets(ud);
		settings_save(ud, dest);
		ghb_value_free(dest);
	}
}

G_MODULE_EXPORT void
preset_type_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	ghb_widget_to_setting(ud->settings, widget);
}

G_MODULE_EXPORT void
presets_restore_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GValue *preset;

	g_debug("presets_restore_clicked_cb ()");
	// Reload only the standard presets
	ghb_presets_reload(ud);
	// Updating the presets list shuffles things around
	// need to make sure the proper preset is selected
	preset = ghb_settings_get_value (ud->settings, "preset");
	ghb_select_preset(ud->builder, preset);
}

G_MODULE_EXPORT void
presets_remove_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gchar *preset;
	GtkResponseType response;

	g_debug("presets_remove_clicked_cb ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	selection = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		GtkWidget *dialog;
		GtkTreePath *path;
		gint *indices, len;
		gboolean folder;

		gtk_tree_model_get(store, &iter, 0, &preset, -1);
		path = gtk_tree_model_get_path(store, &iter);
		indices = gtk_tree_path_get_indices(path);
		len = gtk_tree_path_get_depth(path);

		folder = ghb_presets_get_folder(presetsPlist, indices, len);
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
							GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
							"Confirm deletion of %s:\n\n%s", 
							folder ? "folder" : "preset",
							preset);
		response = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy (dialog);
		if (response == GTK_RESPONSE_YES)
		{
			GtkTreeIter nextIter = iter;
			gboolean valid = TRUE;
			if (!gtk_tree_model_iter_next(store, &nextIter))
			{
				if (!gtk_tree_model_iter_parent(store, &nextIter, &iter))
				{
					valid = FALSE;
				}
			}
			// Remove the selected item
			// First unselect it so that selecting the new item works properly
			gtk_tree_selection_unselect_iter (selection, &iter);
			if (ghb_presets_remove(presetsPlist, indices, len))
			{
				store_presets();
				presets_list_remove(ud, indices, len);
			}
			if (!valid)
				valid = gtk_tree_model_get_iter_first(store, &nextIter);
			if (valid)
			{
				gtk_tree_path_free(path);
				path = gtk_tree_model_get_path(store, &nextIter);
				indices = gtk_tree_path_get_indices(path);
				len = gtk_tree_path_get_depth(path);
				ghb_select_preset2(ud->builder, indices, len);
			}
		}
		g_free(preset);
		gtk_tree_path_free(path);
	}
}

// controls where valid drop locations are
G_MODULE_EXPORT gboolean
presets_drag_motion_cb(
	GtkTreeView *tv,
	GdkDragContext *ctx,
	gint x,
	gint y,
	guint time,
	signal_user_data_t *ud)
{
	GtkTreePath *path = NULL;
	GtkTreeViewDropPosition drop_pos;
	gint *indices, len;
	GtkTreeIter iter;
	GtkTreeView *srctv;
	GtkTreeModel *model;
	GtkTreeSelection *select;
	gint src_ptype, dst_ptype;
	gboolean src_folder, dst_folder;
	GValue *preset;
	gint tree_depth, ii;
	GtkWidget *widget;

	widget = gtk_drag_get_source_widget(ctx);
	if (widget == NULL || widget != GTK_WIDGET(tv))
		return TRUE;

	// Get the type of the object being dragged
	srctv = GTK_TREE_VIEW(gtk_drag_get_source_widget(ctx));
	select = gtk_tree_view_get_selection (srctv);
	gtk_tree_selection_get_selected (select, &model, &iter);
	path = gtk_tree_model_get_path (model, &iter);
	indices = gtk_tree_path_get_indices(path);
	len = gtk_tree_path_get_depth(path);

	preset = presets_get_dict(presetsPlist, indices, len);
	tree_depth = preset_tree_depth(preset);

	src_ptype = ghb_presets_get_type(presetsPlist, indices, len);
	src_folder = ghb_presets_get_folder(presetsPlist, indices, len);
	gtk_tree_path_free(path);

	if (src_folder && tree_depth == 1)
		tree_depth = 2;

	// The rest checks that the destination is a valid position
	// in the list.
	gtk_tree_view_get_dest_row_at_pos (tv, x, y, &path, &drop_pos);
	if (path == NULL)
	{
		gdk_drag_status(ctx, 0, time);
		return TRUE;
	}
	// Don't allow repositioning of builtin presets
	if (src_ptype != PRESETS_CUSTOM)
	{
		gdk_drag_status(ctx, 0, time);
		return TRUE;
	}

	len = gtk_tree_path_get_depth(path);
	if (len+tree_depth-1 >= MAX_NESTED_PRESET)
	{
		if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
			drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
		if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
			drop_pos = GTK_TREE_VIEW_DROP_AFTER;
	}
	for (ii = len+tree_depth-1; ii > MAX_NESTED_PRESET; ii--)
		gtk_tree_path_up(path);
	indices = gtk_tree_path_get_indices(path);
	len = gtk_tree_path_get_depth(path);
	dst_ptype = ghb_presets_get_type(presetsPlist, indices, len);
	dst_folder = ghb_presets_get_folder(presetsPlist, indices, len);
	// Don't allow mixing custom presets in the builtins
	if (dst_ptype != PRESETS_CUSTOM)
	{
		gdk_drag_status(ctx, 0, time);
		return TRUE;
	}

	// Only allow *drop into* for folders
	if (!dst_folder)
	{ 
		if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
			drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
		if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
			drop_pos = GTK_TREE_VIEW_DROP_AFTER;
	}

	len = gtk_tree_path_get_depth(path);
	gtk_tree_view_set_drag_dest_row(tv, path, drop_pos);
	gtk_tree_path_free(path);
	gdk_drag_status(ctx, GDK_ACTION_MOVE, time);
	return TRUE;
}

G_MODULE_EXPORT void 
presets_drag_cb(
	GtkTreeView *dstwidget, 
	GdkDragContext *dc, 
	gint x, gint y, 
	GtkSelectionData *selection_data, 
	guint info, guint t, 
	signal_user_data_t *ud)
{
	GtkTreePath *path = NULL;
	GtkTreeViewDropPosition drop_pos;
	GtkTreeIter dstiter, srciter;
	gint *dst_indices, dst_len, *src_indices, src_len;
	gint src_ptype;
	gboolean src_folder, dst_folder;
	
	GtkTreeModel *dstmodel = gtk_tree_view_get_model(dstwidget);
			
	g_debug("preset_drag_cb ()");
	// This doesn't work here for some reason...
	// gtk_tree_view_get_drag_dest_row(dstwidget, &path, &drop_pos);
	gtk_tree_view_get_dest_row_at_pos (dstwidget, x, y, &path, &drop_pos);
	// This little hack is needed because attempting to drop after
	// the last item gives us no path or drop_pos.
	if (path == NULL)
	{
		gint n_children;

		n_children = gtk_tree_model_iter_n_children(dstmodel, NULL);
		if (n_children)
		{
			drop_pos = GTK_TREE_VIEW_DROP_AFTER;
			path = gtk_tree_path_new_from_indices(n_children-1, -1);
		}
		else
		{
			drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
			path = gtk_tree_path_new_from_indices(0, -1);
		}
	}
	if (path)
	{
		GtkTreeView *srcwidget;
		GtkTreeModel *srcmodel;
		GtkTreeSelection *select;
		GtkTreePath *srcpath = NULL;
		GValue *preset;
		gint tree_depth, ii;

		srcwidget = GTK_TREE_VIEW(gtk_drag_get_source_widget(dc));
		select = gtk_tree_view_get_selection (srcwidget);
		gtk_tree_selection_get_selected (select, &srcmodel, &srciter);

		srcpath = gtk_tree_model_get_path (srcmodel, &srciter);
		src_indices = gtk_tree_path_get_indices(srcpath);
		src_len = gtk_tree_path_get_depth(srcpath);
		src_ptype = ghb_presets_get_type(presetsPlist, src_indices, src_len);
		src_folder = ghb_presets_get_folder(presetsPlist, src_indices, src_len);
		preset = ghb_value_dup(
					presets_get_dict(presetsPlist, src_indices, src_len));
		gtk_tree_path_free(srcpath);

		// Don't allow repositioning of builtin presets
		if (src_ptype != PRESETS_CUSTOM)
			return;

		tree_depth = preset_tree_depth(preset);
		if (src_folder && tree_depth == 1)
			tree_depth = 2;

		dst_len = gtk_tree_path_get_depth(path);
		if (dst_len+tree_depth-1 >= MAX_NESTED_PRESET)
		{
			if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
				drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
			if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
				drop_pos = GTK_TREE_VIEW_DROP_AFTER;
		}

		for (ii = dst_len+tree_depth-1; ii > MAX_NESTED_PRESET; ii--)
			gtk_tree_path_up(path);
		dst_indices = gtk_tree_path_get_indices(path);
		dst_len = gtk_tree_path_get_depth(path);
		dst_folder = ghb_presets_get_folder(presetsPlist, dst_indices, dst_len);
		// Only allow *drop into* for folders
		if (!dst_folder)
		{ 
			if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
				drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
			if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
				drop_pos = GTK_TREE_VIEW_DROP_AFTER;
		}
		if (gtk_tree_model_get_iter (dstmodel, &dstiter, path))
		{
			GtkTreeIter iter;
			GtkTreePath *dstpath = NULL;

			switch (drop_pos)
			{
				case GTK_TREE_VIEW_DROP_BEFORE:
					gtk_tree_store_insert_before(GTK_TREE_STORE (dstmodel), 
												&iter, NULL, &dstiter);
					break;

				case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
					gtk_tree_store_insert(GTK_TREE_STORE (dstmodel), 
												&iter, &dstiter, 0);
					break;

				case GTK_TREE_VIEW_DROP_AFTER:
					gtk_tree_store_insert_after(GTK_TREE_STORE (dstmodel), 
												&iter, NULL, &dstiter);
					break;

				case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
					gtk_tree_store_insert_after(GTK_TREE_STORE (dstmodel), 
												&iter, &dstiter, 0);
					break;

				default:
					break;
			}

			dstpath = gtk_tree_model_get_path (dstmodel, &iter);
			dst_indices = gtk_tree_path_get_indices(dstpath);
			dst_len = gtk_tree_path_get_depth(dstpath);
			ghb_presets_insert(presetsPlist, preset, dst_indices, dst_len);
			gtk_tree_path_free(dstpath);

			srcpath = gtk_tree_model_get_path (srcmodel, &srciter);
			src_indices = gtk_tree_path_get_indices(srcpath);
			src_len = gtk_tree_path_get_depth(srcpath);
			ghb_presets_remove(presetsPlist, src_indices, src_len);
			gtk_tree_path_free(srcpath);

			gtk_tree_store_remove (GTK_TREE_STORE (srcmodel), &srciter);

			dstpath = gtk_tree_model_get_path (dstmodel, &iter);
			dst_indices = gtk_tree_path_get_indices(dstpath);
			dst_len = gtk_tree_path_get_depth(dstpath);
			presets_list_update_item(ud, dst_indices, dst_len, TRUE);
			gtk_tree_path_free(dstpath);

			store_presets();
		}
		gtk_tree_path_free(path);
	}
}

void
presets_row_expanded_cb(
	GtkTreeView *treeview, 
	GtkTreeIter *iter, 
	GtkTreePath *path, 
	signal_user_data_t *ud)
{
	gint *indices, len;
	gboolean expanded, folder;
	GValue *dict;

	expanded = gtk_tree_view_row_expanded(treeview, path);
	indices = gtk_tree_path_get_indices(path);
	len = gtk_tree_path_get_depth(path);
	dict = presets_get_dict(presetsPlist, indices, len);
	if (preset_folder_is_open(dict))
	{
		if (expanded)
			return;
	}
	else if (!expanded)
	{
		return;
	}
	folder = ghb_presets_get_folder(presetsPlist, indices, len);
	if (folder)
	{
		presets_set_folder_open(expanded, indices, len);
	}

	// Collapsing parent folder collapses all children
	if (!expanded)
	{
		GValue *presets = NULL;
		gint *more_indices, count, ii;

		more_indices = g_malloc((len+1)*sizeof(gint));
		memcpy(more_indices, indices, len*sizeof(gint));

		presets = presets_get_folder(presetsPlist, indices, len);
		count = ghb_array_len(presets);
		for (ii = 0; ii < count; ii++)
		{
			dict = ghb_array_get_nth(presets, ii);
			folder = ghb_preset_folder(dict);
			if (folder)
			{
				more_indices[len] = ii;
				presets_set_folder_open(expanded, more_indices, len+1);
			}
		}
		g_free(more_indices);
	}
	store_presets();
}

static void
preset_update_title_deps(signal_user_data_t *ud, ghb_title_info_t *tinfo)
{
	GtkWidget *widget;

	ghb_ui_update(ud, "scale_width", 
			ghb_int64_value(tinfo->width - tinfo->crop[2] - tinfo->crop[3]));
	// If anamorphic or keep_aspect, the hight will be automatically calculated
	gboolean keep_aspect;
	gint pic_par;
	keep_aspect = ghb_settings_get_boolean(ud->settings, "PictureKeepRatio");
	pic_par = ghb_settings_combo_int(ud->settings, "PicturePAR");
	if (!(keep_aspect || pic_par) || pic_par == 3)
	{
		ghb_ui_update(ud, "scale_height", 
			ghb_int64_value(tinfo->height - tinfo->crop[0] - tinfo->crop[1]));
	}

	// Set the limits of cropping.  hb_set_anamorphic_size crashes if
	// you pass it a cropped width or height == 0.
	gint bound;
	bound = tinfo->height / 2 - 2;
	widget = GHB_WIDGET (ud->builder, "PictureTopCrop");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	widget = GHB_WIDGET (ud->builder, "PictureBottomCrop");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	bound = tinfo->width / 2 - 2;
	widget = GHB_WIDGET (ud->builder, "PictureLeftCrop");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	widget = GHB_WIDGET (ud->builder, "PictureRightCrop");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	if (ghb_settings_get_boolean(ud->settings, "PictureAutoCrop"))
	{
		ghb_ui_update(ud, "PictureTopCrop", ghb_int64_value(tinfo->crop[0]));
		ghb_ui_update(ud, "PictureBottomCrop", ghb_int64_value(tinfo->crop[1]));
		ghb_ui_update(ud, "PictureLeftCrop", ghb_int64_value(tinfo->crop[2]));
		ghb_ui_update(ud, "PictureRightCrop", ghb_int64_value(tinfo->crop[3]));
	}
}

void
ghb_refresh_preset(signal_user_data_t *ud)
{
	ghb_title_info_t tinfo;
	GValue *preset;
	gint *indices, len;

	g_debug("ghb_refresh_preset ()");
	preset = ghb_settings_get_value(ud->settings, "preset_selection");
	indices = ghb_preset_indices_from_path(presetsPlist, preset, &len);
	if (indices)
	{
		gboolean folder;

		folder = ghb_presets_get_folder(presetsPlist, indices, len);
		if (!folder)
		{
			ud->dont_clear_presets = TRUE;
			ud->scale_busy = TRUE;
			// Temporarily set the video_quality range to (0,100)
			// This is needed so the video_quality value does not get
			// truncated when set.  The range will be readjusted below
			GtkWidget *qp = GHB_WIDGET(ud->builder, "VideoQualitySlider");
			gtk_range_set_range (GTK_RANGE(qp), 0, 100);
			gtk_scale_set_digits(GTK_SCALE(qp), 3);
			// Clear the audio list prior to changing the preset.  Existing 
			// audio can cause the container extension to be automatically 
			// changed when it shouldn't be
			ghb_clear_audio_list(ud);
			ghb_set_preset_from_indices(ud, indices, len);
			gint titleindex;
			titleindex = ghb_settings_combo_int(ud->settings, "title");
			ghb_set_pref_audio(titleindex, ud);
			ghb_set_pref_subtitle(titleindex, ud);
			ghb_settings_set_boolean(ud->settings, "preset_modified", FALSE);
			if (ghb_get_title_info (&tinfo, titleindex))
			{
				preset_update_title_deps(ud, &tinfo);
			}
			ud->scale_busy = FALSE;
			ghb_set_scale (ud, GHB_PIC_KEEP_PAR|GHB_PIC_USE_MAX);
			ud->dont_clear_presets = FALSE;

			gdouble vqmin, vqmax, step, page;
			gint digits;
			gboolean inverted;

			ghb_vquality_range(ud, &vqmin, &vqmax, &step, 
								&page, &digits, &inverted);
			gtk_range_set_range (GTK_RANGE(qp), vqmin, vqmax);
			gtk_range_set_increments (GTK_RANGE(qp), step, page);
			gtk_scale_set_digits(GTK_SCALE(qp), digits);
			gtk_range_set_inverted (GTK_RANGE(qp), inverted);

			gchar *text;
			gint crop[4];
			GtkWidget *crop_widget;
			crop[0] = ghb_settings_get_int(ud->settings, "PictureTopCrop");
			crop[1] = ghb_settings_get_int(ud->settings, "PictureBottomCrop");
			crop[2] = ghb_settings_get_int(ud->settings, "PictureLeftCrop");
			crop[3] = ghb_settings_get_int(ud->settings, "PictureRightCrop");
			crop_widget = GHB_WIDGET (ud->builder, "crop_values");
			text = g_strdup_printf("%d:%d:%d:%d", 
									crop[0], crop[1], crop[2], crop[3]);
			gtk_label_set_text (GTK_LABEL(crop_widget), text);
			g_free(text);
		}
	}
}

G_MODULE_EXPORT void
presets_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	ghb_title_info_t tinfo;
	GtkWidget *widget;
	
	g_debug("presets_list_selection_changed_cb ()");
	widget = GHB_WIDGET (ud->builder, "presets_remove");
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		GtkTreePath *treepath;
		gint *indices, len;
		GValue *path;
		gboolean folder;

		treepath = gtk_tree_model_get_path(store, &iter);
		indices = gtk_tree_path_get_indices(treepath);
		len = gtk_tree_path_get_depth(treepath);

		path = preset_path_from_indices(presetsPlist, indices, len);
		ghb_settings_take_value(ud->settings, "preset_selection", path);

		folder = ghb_presets_get_folder(presetsPlist, indices, len);
		if (!folder)
		{
			ud->dont_clear_presets = TRUE;
			ud->scale_busy = TRUE;
			// Temporarily set the video_quality range to (0,100)
			// This is needed so the video_quality value does not get
			// truncated when set.  The range will be readjusted below
			GtkWidget *qp = GHB_WIDGET(ud->builder, "VideoQualitySlider");
			gtk_range_set_range (GTK_RANGE(qp), 0, 100);
			gtk_scale_set_digits(GTK_SCALE(qp), 3);
			// Clear the audio list prior to changing the preset.  Existing 
			// audio can cause the container extension to be automatically 
			// changed when it shouldn't be
			ghb_clear_audio_list(ud);
			ghb_set_preset_from_indices(ud, indices, len);
			gint titleindex;
			titleindex = ghb_settings_combo_int(ud->settings, "title");
			ghb_set_pref_audio(titleindex, ud);
			ghb_set_pref_subtitle(titleindex, ud);
			ghb_settings_set_boolean(ud->settings, "preset_modified", FALSE);
			if (ghb_get_title_info (&tinfo, titleindex))
			{
				preset_update_title_deps(ud, &tinfo);
			}
			ud->scale_busy = FALSE;
			ghb_set_scale (ud, GHB_PIC_KEEP_PAR|GHB_PIC_USE_MAX);
			ud->dont_clear_presets = FALSE;

			gdouble vqmin, vqmax, step, page;
			gint digits;
			gboolean inverted;

			ghb_vquality_range(ud, &vqmin, &vqmax, &step, 
								&page, &digits, &inverted);
			gtk_range_set_range (GTK_RANGE(qp), vqmin, vqmax);
			gtk_range_set_increments (GTK_RANGE(qp), step, page);
			gtk_scale_set_digits(GTK_SCALE(qp), digits);
			gtk_range_set_inverted (GTK_RANGE(qp), inverted);

			gchar *text;
			gint crop[4];
			GtkWidget *crop_widget;
			crop[0] = ghb_settings_get_int(ud->settings, "PictureTopCrop");
			crop[1] = ghb_settings_get_int(ud->settings, "PictureBottomCrop");
			crop[2] = ghb_settings_get_int(ud->settings, "PictureLeftCrop");
			crop[3] = ghb_settings_get_int(ud->settings, "PictureRightCrop");
			crop_widget = GHB_WIDGET (ud->builder, "crop_values");
			text = g_strdup_printf("%d:%d:%d:%d", 
									crop[0], crop[1], crop[2], crop[3]);
			gtk_label_set_text (GTK_LABEL(crop_widget), text);
			g_free(text);
		}
		gtk_tree_path_free(treepath);
		gtk_widget_set_sensitive(widget, TRUE);
	}
	else
	{
		g_debug("No selection???  Perhaps unselected.");
		gtk_widget_set_sensitive(widget, FALSE);
	}
	if (ghb_settings_combo_int(ud->settings, "PtoPType") == 0)
	{
		gint start, end;
		start = ghb_settings_get_int(ud->settings, "start_point");
		end = ghb_settings_get_int(ud->settings, "end_point");
		widget = GHB_WIDGET (ud->builder, "ChapterMarkers");
		gtk_widget_set_sensitive(widget, TRUE);
		if (start == end)
		{
			ud->dont_clear_presets = TRUE;
			ghb_ui_update(ud, "ChapterMarkers", ghb_boolean_value(FALSE));
			ud->dont_clear_presets = FALSE;
			gtk_widget_set_sensitive(widget, FALSE);
		}
	}
}

void
ghb_clear_presets_selection(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	
	if (ud->dont_clear_presets) return;
	g_debug("ghb_clear_presets_selection()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	selection = gtk_tree_view_get_selection (treeview);
	gtk_tree_selection_unselect_all (selection);
	ghb_settings_set_boolean(ud->settings, "preset_modified", TRUE);
}

G_MODULE_EXPORT void
presets_frame_size_allocate_cb(GtkWidget *widget, GtkAllocation *allocation, signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		GtkTreePath *path;
		path = gtk_tree_model_get_path (store, &iter);
		// Make the parent visible in scroll window if it is not.
		gtk_tree_view_scroll_to_cell (treeview, path, NULL, FALSE, 0, 0);
		gtk_tree_path_free(path);
	}
}

G_MODULE_EXPORT void
presets_default_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GValue *preset;
	gint *indices, len;

	g_debug("presets_default_clicked_cb ()");
	preset = ghb_settings_get_value(ud->settings, "preset_selection");
	indices = ghb_preset_indices_from_path(presetsPlist, preset, &len);
	if (indices)
	{
		if (!ghb_presets_get_folder(presetsPlist, indices, len))
		{
			ghb_presets_list_clear_default(ud);
			presets_set_default(indices, len);
			ghb_presets_list_default(ud);
		}
		g_free(indices);
	}
}

G_MODULE_EXPORT void
preset_edited_cb(
	GtkCellRendererText *cell, 
	gchar *path, 
	gchar *text, 
	signal_user_data_t *ud)
{
	GtkTreePath *treepath;
	GtkTreeStore *store;
	GtkTreeView *treeview;
	GtkTreeIter iter;
	gint *indices, len, count;
	GValue *dict;
	GValue *preset, *dest;
	
	g_debug("preset_edited_cb ()");
	g_debug("path (%s)", path);
	g_debug("text (%s)", text);

	preset = ghb_settings_get_value (ud->settings, "preset_selection");
	dest = ghb_array_value_new(MAX_NESTED_PRESET);
	count = ghb_array_len(preset);
	ghb_array_copy(dest, preset, count-1);
	ghb_array_append(dest, ghb_string_value_new(text));
	indices = ghb_preset_indices_from_path(presetsPlist, dest, &len);
	ghb_value_free(dest);
	if (indices != NULL)
	{
		// Already exists
		g_free(indices);
		return;
	}

	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	treepath = gtk_tree_path_new_from_string (path);
	indices = gtk_tree_path_get_indices(treepath);
	len = gtk_tree_path_get_depth(treepath);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath);
	gtk_tree_store_set(store, &iter, 0, text, -1);

	dict = presets_get_dict(presetsPlist, indices, len);
	ghb_dict_insert(dict, g_strdup("PresetName"), ghb_string_value_new(text));
	store_presets();
	gtk_tree_path_free (treepath);
}

