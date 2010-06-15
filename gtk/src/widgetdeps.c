#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <string.h>
#include "values.h"
#include "plist.h"

typedef struct
{
	const gchar *widget_name;
	const gchar *dep_name;
	const gchar *enable_value;
	const gboolean disable_if_equal;
	const gboolean hide;
} dependency_t;

static dependency_t dep_map[] =
{
	{"title", "queue_add", "none", TRUE, FALSE},
	{"title", "queue_add_menu", "none", TRUE, FALSE},
	{"title", "preview_frame", "none", TRUE, FALSE},
	{"title", "picture_label", "none", TRUE, FALSE},
	{"title", "picture_tab", "none", TRUE, FALSE},
	{"title", "chapters_label", "none", TRUE, FALSE},
	{"title", "chapters_tab", "none", TRUE, FALSE},
	{"title", "start_point", "none", TRUE, FALSE},
	{"title", "end_point", "none", TRUE, FALSE},
	{"title", "angle", "none", TRUE, FALSE},
	{"title", "angle_label", "1", TRUE, FALSE},
	{"use_dvdnav", "angle", "FALSE", TRUE, TRUE},
	{"use_dvdnav", "angle_label", "FALSE", TRUE, TRUE},
	{"angle_count", "angle", "1", TRUE, TRUE},
	{"angle_count", "angle_label", "1", TRUE, TRUE},
	{"vquality_type_bitrate", "VideoAvgBitrate", "TRUE", FALSE, FALSE},
	{"vquality_type_target", "VideoTargetSize", "TRUE", FALSE, FALSE},
	{"vquality_type_constant", "VideoQualitySlider", "TRUE", FALSE, FALSE},
	{"vquality_type_constant", "VideoTwoPass", "TRUE", TRUE, FALSE},
	{"vquality_type_constant", "VideoTurboTwoPass", "TRUE", TRUE, FALSE},
	{"VideoTwoPass", "VideoTurboTwoPass", "TRUE", FALSE, FALSE},
	{"FileFormat", "Mp4LargeFile", "mp4", FALSE, TRUE},
	{"FileFormat", "Mp4HttpOptimize", "mp4", FALSE, TRUE},
	{"FileFormat", "Mp4iPodCompatible", "mp4", FALSE, TRUE},
	{"PictureDecombDeinterlace", "PictureDeinterlace", "TRUE", TRUE, TRUE},
	{"PictureDecombDeinterlace", "PictureDeinterlaceCustom", "TRUE", TRUE, TRUE},
	{"PictureDecombDeinterlace", "PictureDeinterlaceLabel", "TRUE", TRUE, TRUE},
	{"PictureDecombDeinterlace", "PictureDecomb", "FALSE", TRUE, TRUE},
	{"PictureDecombDeinterlace", "PictureDecombCustom", "FALSE", TRUE, TRUE},
	{"PictureDecombDeinterlace", "PictureDecombLabel", "FALSE", TRUE, TRUE},
	{"PictureDeinterlace", "PictureDeinterlaceCustom", "custom", FALSE, TRUE},
	{"PictureDenoise", "PictureDenoiseCustom", "custom", FALSE, TRUE},
	{"PictureDecomb", "PictureDecombCustom", "custom", FALSE, TRUE},
	{"PictureDetelecine", "PictureDetelecineCustom", "custom", FALSE, TRUE},
	{"PictureAutoCrop", "PictureTopCrop", "FALSE", FALSE, FALSE},
	{"PictureAutoCrop", "PictureBottomCrop", "FALSE", FALSE, FALSE},
	{"PictureAutoCrop", "PictureLeftCrop", "FALSE", FALSE, FALSE},
	{"PictureAutoCrop", "PictureRightCrop", "FALSE", FALSE, FALSE},
	{"VideoEncoder", "x264_tab", "x264", FALSE, FALSE},
	{"VideoEncoder", "x264_tab_label", "x264", FALSE, FALSE},
	{"VideoEncoder", "Mp4iPodCompatible", "x264", FALSE, FALSE},
	{"AudioEncoderActual", "AudioBitrate", "ac3|dts", TRUE, FALSE},
	{"AudioEncoderActual", "AudioSamplerate", "ac3|dts", TRUE, FALSE},
	{"AudioEncoderActual", "AudioMixdown", "ac3|dts", TRUE, FALSE},
	{"AudioEncoderActual", "AudioTrackDRCSlider", "ac3|dts", TRUE, FALSE},
	{"AudioEncoderActual", "drc_label", "ac3|dts", TRUE, FALSE},
	{"x264_bframes", "x264_bpyramid", "<2", TRUE, FALSE},
	{"x264_bframes", "x264_direct", "0", TRUE, FALSE},
	{"x264_bframes", "x264_b_adapt", "0", TRUE, FALSE},
	{"x264_cabac", "x264_trellis", "TRUE", FALSE, FALSE},
	{"x264_subme", "x264_psy_rd", "<6", TRUE, FALSE},
	{"x264_subme", "x264_psy_trell", "<6", TRUE, FALSE},
	{"x264_cabac", "x264_psy_trell", "TRUE", FALSE, FALSE},
	{"x264_trellis", "x264_psy_trell", "0", TRUE, FALSE},
	{"ChapterMarkers", "chapters_list", "TRUE", FALSE, FALSE},
	{"use_source_name", "chapters_in_destination", "TRUE", FALSE, FALSE},
	{"use_source_name", "title_no_in_destination", "TRUE", FALSE, FALSE},
};

int
main(gint argc, gchar *argv[])
{
	gint ii, jj;
	GValue *top;
	gint count = sizeof(dep_map) / sizeof(dependency_t);

	g_type_init();

	top = ghb_dict_value_new();
	for (ii = 0; ii < count; ii++)
	{
		const gchar *name;
		GValue *array;

		name = dep_map[ii].widget_name;
		if (ghb_dict_lookup(top, name))
			continue;
		array = ghb_array_value_new(8);
		for (jj = 0; jj < count; jj++)
		{
			if (strcmp(name, dep_map[jj].widget_name) == 0)
			{
				ghb_array_append(array,
					ghb_value_dup(ghb_string_value(dep_map[jj].dep_name)));
			}
		}
		ghb_dict_insert(top, g_strdup(name), array);
	}
	ghb_plist_write_file("widget.deps", top);

	// reverse map
	top = ghb_dict_value_new();
	for (ii = 0; ii < count; ii++)
	{
		const gchar *name;
		GValue *array;

		name = dep_map[ii].dep_name;
		if (ghb_dict_lookup(top, name))
			continue;
		array = ghb_array_value_new(8);
		for (jj = 0; jj < count; jj++)
		{
			if (strcmp(name, dep_map[jj].dep_name) == 0)
			{
				GValue *data;
				data = ghb_array_value_new(3);
				ghb_array_append(data, ghb_value_dup(
					ghb_string_value(dep_map[jj].widget_name)));
				ghb_array_append(data, ghb_value_dup(
					ghb_string_value(dep_map[jj].enable_value)));
				ghb_array_append(data, ghb_value_dup(
					ghb_boolean_value(dep_map[jj].disable_if_equal)));
				ghb_array_append(data, ghb_value_dup(
					ghb_boolean_value(dep_map[jj].hide)));
				ghb_array_append(array, data);
			}
		}
		ghb_dict_insert(top, g_strdup(name), array);
	}
	ghb_plist_write_file("widget_reverse.deps", top);
	return 0;
}

