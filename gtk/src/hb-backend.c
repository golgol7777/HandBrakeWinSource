/***************************************************************************
 *            hb-backend.c
 *
 *  Fri Mar 28 10:38:44 2008
 *  Copyright  2008-2011  John Stebbins
 *  <john at stebbins dot name>
 ****************************************************************************/

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
#define _GNU_SOURCE
#include <limits.h>
#include <math.h>
#include "hb.h"
#include "ghbcompat.h"
#include <glib/gstdio.h>
#include "hb-backend.h"
#include "settings.h"
#include "callbacks.h"
#include "subtitlehandler.h"
#include "audiohandler.h"
#include "x264handler.h"
#include "preview.h"
#include "values.h"
#include "lang.h"

typedef struct
{
	const gchar *option;
	const gchar *shortOpt;
	gdouble ivalue;
	const gchar *svalue;
} options_map_t;

typedef struct
{
	gint count;
	options_map_t *map;
} combo_opts_t;

static gchar **index_str = NULL;
static gint index_str_size = 0;

static void 
index_str_init(gint max_index)
{
	int ii;

	if (max_index+1 > index_str_size)
	{
		index_str = realloc(index_str, (max_index+1) * sizeof(char*));
		for (ii = index_str_size; ii <= max_index; ii++)
		{
			index_str[ii] = g_strdup_printf("%d", ii);
		}
		index_str_size = max_index + 1;
	}
}

static options_map_t d_point_to_point_opts[] =
{
	{"Chapters:",       "chapter", 0, "0"},
	{"Seconds:", "time",    1, "1"},
	{"Frames:",   "frame",   2, "2"},
};
combo_opts_t point_to_point_opts =
{
	sizeof(d_point_to_point_opts)/sizeof(options_map_t),
	d_point_to_point_opts
};

static options_map_t d_when_complete_opts[] =
{
	{"Do Nothing",            "nothing",  0, "0"},
	{"Show Notification",     "notify",   1, "1"},
	{"Quit Handbrake",        "quit",     4, "4"},
	{"Put Computer To Sleep", "sleep",    2, "2"},
	{"Shutdown Computer",     "shutdown", 3, "3"},
};
combo_opts_t when_complete_opts =
{
	sizeof(d_when_complete_opts)/sizeof(options_map_t),
	d_when_complete_opts
};

static options_map_t d_par_opts[] =
{
	{"Off", "0", 0, "0"},
	{"Strict", "1", 1, "1"},
	{"Loose", "2", 2, "2"},
	{"Custom", "3", 3, "3"},
};
combo_opts_t par_opts =
{
	sizeof(d_par_opts)/sizeof(options_map_t),
	d_par_opts
};

static options_map_t d_alignment_opts[] =
{
	{"2", "2", 2, "2"},
	{"4", "4", 4, "4"},
	{"8", "8", 8, "8"},
	{"16", "16", 16, "16"},
};
combo_opts_t alignment_opts =
{
	sizeof(d_alignment_opts)/sizeof(options_map_t),
	d_alignment_opts
};

static options_map_t d_logging_opts[] =
{
	{"0", "0", 0, "0"},
	{"1", "1", 1, "1"},
	{"2", "2", 2, "2"},
};
combo_opts_t logging_opts =
{
	sizeof(d_logging_opts)/sizeof(options_map_t),
	d_logging_opts
};

static options_map_t d_log_longevity_opts[] =
{
	{"Week",     "week",     7, "7"},
	{"Month",    "month",    30, "30"},
	{"Year",     "year",     365, "365"},
	{"Immortal", "immortal", 366, "366"},
};
combo_opts_t log_longevity_opts =
{
	sizeof(d_log_longevity_opts)/sizeof(options_map_t),
	d_log_longevity_opts
};

static options_map_t d_appcast_update_opts[] =
{
	{"Never", "never", 0, "never"},
	{"Daily", "daily", 1, "daily"},
	{"Weekly", "weekly", 2, "weekly"},
	{"Monthly", "monthly", 3, "monthly"},
};
combo_opts_t appcast_update_opts =
{
	sizeof(d_appcast_update_opts)/sizeof(options_map_t),
	d_appcast_update_opts
};

static options_map_t d_vqual_granularity_opts[] =
{
	{"0.2",  "0.2",  0.2,  "0.2"},
	{"0.25", "0.25", 0.25, "0.25"},
	{"0.5",  "0.5",  0.5,  "0.5"},
	{"1",    "1",    1,    "1"},
};
combo_opts_t vqual_granularity_opts =
{
	sizeof(d_vqual_granularity_opts)/sizeof(options_map_t),
	d_vqual_granularity_opts
};

static options_map_t d_container_opts[] =
{
	{"MKV", "mkv", HB_MUX_MKV, "mkv"},
	{"MP4", "mp4", HB_MUX_MP4, "mp4"},
};
combo_opts_t container_opts =
{
	sizeof(d_container_opts)/sizeof(options_map_t),
	d_container_opts
};

static options_map_t d_detel_opts[] =
{
	{"Off",    "off",   0, ""},
	{"Custom", "custom", 1, ""},
	{"Default","default",2, NULL},
};
combo_opts_t detel_opts =
{
	sizeof(d_detel_opts)/sizeof(options_map_t),
	d_detel_opts
};

static options_map_t d_decomb_opts[] =
{
	{"Off",    "off",   0, ""},
	{"Custom", "custom", 1, ""},
	{"Default","default",2, NULL},
};
combo_opts_t decomb_opts =
{
	sizeof(d_decomb_opts)/sizeof(options_map_t),
	d_decomb_opts
};

static options_map_t d_deint_opts[] =
{
	{"Off",    "off",   0, ""},
	{"Custom", "custom", 1, ""},
	{"Fast",   "fast",   2, "-1:-1:-1:0:1"},
	{"Slow",   "slow",   3, "2:-1:-1:0:1"},
	{"Slower", "slower", 4, "0:-1:-1:0:1"},
};
combo_opts_t deint_opts =
{
	sizeof(d_deint_opts)/sizeof(options_map_t),
	d_deint_opts
};

static options_map_t d_denoise_opts[] =
{
	{"Off",    "off",   0, ""},
	{"Custom", "custom", 1, ""},
	{"Weak",   "weak",   2, "2:1:2:3"},
	{"Medium", "medium", 3, "3:2:2:3"},
	{"Strong", "strong", 4, "7:7:5:5"},
};
combo_opts_t denoise_opts =
{
	sizeof(d_denoise_opts)/sizeof(options_map_t),
	d_denoise_opts
};

static options_map_t d_vcodec_opts[] =
{
	{"H.264 (x264)",    "x264",   HB_VCODEC_X264, ""},
	{"MPEG-4 (FFmpeg)", "ffmpeg", HB_VCODEC_FFMPEG_MPEG4, ""},
	{"MPEG-2 (FFmpeg)", "ffmpeg2",HB_VCODEC_FFMPEG_MPEG2, ""},
	{"VP3 (Theora)",    "theora", HB_VCODEC_THEORA, ""},
};
combo_opts_t vcodec_opts =
{
	sizeof(d_vcodec_opts)/sizeof(options_map_t),
	d_vcodec_opts
};

static options_map_t d_acodec_opts[] =
{
	{"AAC (faac)",      "faac",    HB_ACODEC_FAAC,     "faac"},
	{"MP3 (lame)",      "lame",    HB_ACODEC_LAME,     "lame"},
	{"Vorbis",          "vorbis",  HB_ACODEC_VORBIS,   "vorbis"},
	{"AC3 (ffmpeg)",    "ac3",     HB_ACODEC_AC3,      "ac3"},
	{"AC3 (pass-thru)", "ac3pass", HB_ACODEC_AC3_PASS, "ac3pass"},
	{"DTS (pass-thru)", "dtspass", HB_ACODEC_DCA_PASS, "dtspass"},
	{"Choose For Me",   "auto",    HB_ACODEC_ANY,      "auto"},
};
combo_opts_t acodec_opts =
{
	sizeof(d_acodec_opts)/sizeof(options_map_t),
	d_acodec_opts
};

static options_map_t d_direct_opts[] =
{
	{"None",      "none",     0, "none"},
	{"Spatial",   "spatial",  1, "spatial"},
	{"Temporal",  "temporal", 2, "temporal"},
	{"Automatic", "auto",     3, "auto"},
};
combo_opts_t direct_opts =
{
	sizeof(d_direct_opts)/sizeof(options_map_t),
	d_direct_opts
};

static options_map_t d_badapt_opts[] =
{
	{"Off",             "0", 0, "0"},
	{"Fast",            "1", 1, "1"},
	{"Optimal",         "2", 2, "2"},
};
combo_opts_t badapt_opts =
{
	sizeof(d_badapt_opts)/sizeof(options_map_t),
	d_badapt_opts
};

static options_map_t d_bpyramid_opts[] =
{
	{"Off",    "none",   0, "none"},
	{"Strict", "strict", 1, "strict"},
	{"Normal", "normal", 2, "normal"},
};
combo_opts_t bpyramid_opts =
{
	sizeof(d_bpyramid_opts)/sizeof(options_map_t),
	d_bpyramid_opts
};

static options_map_t d_weightp_opts[] =
{
	{"Off",    "0", 0, "0"},
	{"Simple", "1", 1, "1"},
	{"Smart",  "2", 2, "2"},
};
combo_opts_t weightp_opts =
{
	sizeof(d_weightp_opts)/sizeof(options_map_t),
	d_weightp_opts
};

static options_map_t d_me_opts[] =
{
	{"Diamond",              "dia",  0, "dia"},
	{"Hexagon",              "hex",  1, "hex"},
	{"Uneven Multi-Hexagon", "umh",  2, "umh"},
	{"Exhaustive",           "esa",  3, "esa"},
	{"Hadamard Exhaustive",  "tesa", 4, "tesa"},
};
combo_opts_t me_opts =
{
	sizeof(d_me_opts)/sizeof(options_map_t),
	d_me_opts
};

static options_map_t d_subme_opts[] =
{
	{"0: SAD, no subpel",          "0", 0, "0"},
	{"1: SAD, qpel",               "1", 1, "1"},
	{"2: SATD, qpel",              "2", 2, "2"},
	{"3: SATD: multi-qpel",        "3", 3, "3"},
	{"4: SATD, qpel on all",       "4", 4, "4"},
	{"5: SATD, multi-qpel on all", "5", 5, "5"},
	{"6: RD in I/P-frames",        "6", 6, "6"},
	{"7: RD in all frames",        "7", 7, "7"},
	{"8: RD refine in I/P-frames", "8", 8, "8"},
	{"9: RD refine in all frames", "9", 9, "9"},
	{"10: QPRD in all frames",     "10", 10, "10"},
};
combo_opts_t subme_opts =
{
	sizeof(d_subme_opts)/sizeof(options_map_t),
	d_subme_opts
};

static options_map_t d_analyse_opts[] =
{
	{"Most", "p8x8,b8x8,i8x8,i4x4", 0, "p8x8,b8x8,i8x8,i4x4"},
	{"None", "none", 1, "none"},
	{"Some", "i4x4,i8x8", 2, "i4x4,i8x8"},
	{"All",  "all",  3, "all"},
	{"Custom",  "custom",  4, "all"},
};
combo_opts_t analyse_opts =
{
	sizeof(d_analyse_opts)/sizeof(options_map_t),
	d_analyse_opts
};

static options_map_t d_trellis_opts[] =
{
	{"Off",         "0", 0, "0"},
	{"Encode only", "1", 1, "1"},
	{"Always",      "2", 2, "2"},
};
combo_opts_t trellis_opts =
{
	sizeof(d_trellis_opts)/sizeof(options_map_t),
	d_trellis_opts
};

combo_opts_t subtitle_opts =
{
	0,
	NULL
};

combo_opts_t title_opts =
{
	0,
	NULL
};

combo_opts_t audio_track_opts =
{
	0,
	NULL
};

typedef struct
{
	const gchar *name;
	combo_opts_t *opts;
} combo_name_map_t;

combo_name_map_t combo_name_map[] =
{
	{"PtoPType", &point_to_point_opts},
	{"WhenComplete", &when_complete_opts},
	{"PicturePAR", &par_opts},
	{"PictureModulus", &alignment_opts},
	{"LoggingLevel", &logging_opts},
	{"LogLongevity", &log_longevity_opts},
	{"check_updates", &appcast_update_opts},
	{"VideoQualityGranularity", &vqual_granularity_opts},
	{"FileFormat", &container_opts},
	{"PictureDeinterlace", &deint_opts},
	{"PictureDecomb", &decomb_opts},
	{"PictureDetelecine", &detel_opts},
	{"PictureDenoise", &denoise_opts},
	{"VideoEncoder", &vcodec_opts},
	{"AudioEncoder", &acodec_opts},
	{"AudioEncoderActual", &acodec_opts},
	{"x264_direct", &direct_opts},
	{"x264_b_adapt", &badapt_opts},
	{"x264_bpyramid", &bpyramid_opts},
	{"x264_weighted_pframes", &weightp_opts},
	{"x264_me", &me_opts},
	{"x264_subme", &subme_opts},
	{"x264_analyse", &analyse_opts},
	{"x264_trellis", &trellis_opts},
	{"SubtitleTrack", &subtitle_opts},
	{"title", &title_opts},
	{"AudioTrack", &audio_track_opts},
	{NULL, NULL}
};

const gchar *srt_codeset_table[] =
{
	"ANSI_X3.4-1968",
	"ANSI_X3.4-1986",
	"ANSI_X3.4",
	"ANSI_X3.110-1983",
	"ANSI_X3.110",
	"ASCII",
	"ECMA-114",
	"ECMA-118",
	"ECMA-128",
	"ECMA-CYRILLIC",
	"IEC_P27-1",
	"ISO-8859-1",
	"ISO-8859-2",
	"ISO-8859-3",
	"ISO-8859-4",
	"ISO-8859-5",
	"ISO-8859-6",
	"ISO-8859-7",
	"ISO-8859-8",
	"ISO-8859-9",
	"ISO-8859-9E",
	"ISO-8859-10",
	"ISO-8859-11",
	"ISO-8859-13",
	"ISO-8859-14",
	"ISO-8859-15",
	"ISO-8859-16",
	"UTF-7",
	"UTF-8",
	"UTF-16",
	"UTF-16LE",
	"UTF-16BE",
	"UTF-32",
	"UTF-32LE",
	"UTF-32BE",
	NULL
};
#define	SRT_TABLE_SIZE (sizeof(srt_codeset_table)/ sizeof(char*)-1)

#if 0
typedef struct iso639_lang_t
{
    char * eng_name;        /* Description in English */
    char * native_name;     /* Description in native language */
    char * iso639_1;       /* ISO-639-1 (2 characters) code */
    char * iso639_2;        /* ISO-639-2/t (3 character) code */
    char * iso639_2b;       /* ISO-639-2/b code (if different from above) */
} iso639_lang_t;
#endif

const iso639_lang_t ghb_language_table[] =
{ 
	{ "Any", "", "zz", "und" },
	{ "Afar", "", "aa", "aar" },
	{ "Abkhazian", "", "ab", "abk" },
	{ "Afrikaans", "", "af", "afr" },
	{ "Akan", "", "ak", "aka" },
	{ "Albanian", "", "sq", "sqi", "alb" },
	{ "Amharic", "", "am", "amh" },
	{ "Arabic", "", "ar", "ara" },
	{ "Aragonese", "", "an", "arg" },
	{ "Armenian", "", "hy", "hye", "arm" },
	{ "Assamese", "", "as", "asm" },
	{ "Avaric", "", "av", "ava" },
	{ "Avestan", "", "ae", "ave" },
	{ "Aymara", "", "ay", "aym" },
	{ "Azerbaijani", "", "az", "aze" },
	{ "Bashkir", "", "ba", "bak" },
	{ "Bambara", "", "bm", "bam" },
	{ "Basque", "", "eu", "eus", "baq" },
	{ "Belarusian", "", "be", "bel" },
	{ "Bengali", "", "bn", "ben" },
	{ "Bihari", "", "bh", "bih" },
	{ "Bislama", "", "bi", "bis" },
	{ "Bosnian", "", "bs", "bos" },
	{ "Breton", "", "br", "bre" },
	{ "Bulgarian", "", "bg", "bul" },
	{ "Burmese", "", "my", "mya", "bur" },
	{ "Catalan", "", "ca", "cat" },
	{ "Chamorro", "", "ch", "cha" },
	{ "Chechen", "", "ce", "che" },
	{ "Chinese", "", "zh", "zho", "chi" },
	{ "Church Slavic", "", "cu", "chu" },
	{ "Chuvash", "", "cv", "chv" },
	{ "Cornish", "", "kw", "cor" },
	{ "Corsican", "", "co", "cos" },
	{ "Cree", "", "cr", "cre" },
	{ "Czech", "", "cs", "ces", "cze" },
	{ "Danish", "Dansk", "da", "dan" },
	{ "German", "Deutsch", "de", "deu", "ger" },
	{ "Divehi", "", "dv", "div" },
	{ "Dzongkha", "", "dz", "dzo" },
	{ "English", "English", "en", "eng" },
	{ "Spanish", "Espanol", "es", "spa" },
	{ "Esperanto", "", "eo", "epo" },
	{ "Estonian", "", "et", "est" },
	{ "Ewe", "", "ee", "ewe" },
	{ "Faroese", "", "fo", "fao" },
	{ "Fijian", "", "fj", "fij" },
	{ "French", "Francais", "fr", "fra", "fre" },
	{ "Western Frisian", "", "fy", "fry" },
	{ "Fulah", "", "ff", "ful" },
	{ "Georgian", "", "ka", "kat", "geo" },
	{ "Gaelic (Scots)", "", "gd", "gla" },
	{ "Irish", "", "ga", "gle" },
	{ "Galician", "", "gl", "glg" },
	{ "Manx", "", "gv", "glv" },
	{ "Greek, Modern", "", "el", "ell", "gre" },
	{ "Guarani", "", "gn", "grn" },
	{ "Gujarati", "", "gu", "guj" },
	{ "Haitian", "", "ht", "hat" },
	{ "Hausa", "", "ha", "hau" },
	{ "Hebrew", "", "he", "heb" },
	{ "Herero", "", "hz", "her" },
	{ "Hindi", "", "hi", "hin" },
	{ "Hiri Motu", "", "ho", "hmo" },
	{ "Croatian", "Hrvatski", "hr", "hrv", "scr" },
	{ "Igbo", "", "ig", "ibo" },
	{ "Ido", "", "io", "ido" },
	{ "Icelandic", "Islenska", "is", "isl", "ice" },
	{ "Sichuan Yi", "", "ii", "iii" },
	{ "Inuktitut", "", "iu", "iku" },
	{ "Interlingue", "", "ie", "ile" },
	{ "Interlingua", "", "ia", "ina" },
	{ "Indonesian", "", "id", "ind" },
	{ "Inupiaq", "", "ik", "ipk" },
	{ "Italian", "Italiano", "it", "ita" },
	{ "Javanese", "", "jv", "jav" },
	{ "Japanese", "", "ja", "jpn" },
	{ "Kalaallisut", "", "kl", "kal" },
	{ "Kannada", "", "kn", "kan" },
	{ "Kashmiri", "", "ks", "kas" },
	{ "Kanuri", "", "kr", "kau" },
	{ "Kazakh", "", "kk", "kaz" },
	{ "Central Khmer", "", "km", "khm" },
	{ "Kikuyu", "", "ki", "kik" },
	{ "Kinyarwanda", "", "rw", "kin" },
	{ "Kirghiz", "", "ky", "kir" },
	{ "Komi", "", "kv", "kom" },
	{ "Kongo", "", "kg", "kon" },
	{ "Korean", "", "ko", "kor" },
	{ "Kuanyama", "", "kj", "kua" },
	{ "Kurdish", "", "ku", "kur" },
	{ "Lao", "", "lo", "lao" },
	{ "Latin", "", "la", "lat" },
	{ "Latvian", "", "lv", "lav" },
	{ "Limburgan", "", "li", "lim" },
	{ "Lingala", "", "ln", "lin" },
	{ "Lithuanian", "", "lt", "lit" },
	{ "Luxembourgish", "", "lb", "ltz" },
	{ "Luba-Katanga", "", "lu", "lub" },
	{ "Ganda", "", "lg", "lug" },
	{ "Macedonian", "", "mk", "mkd", "mac" },
	{ "Hungarian", "Magyar", "hu", "hun" },
	{ "Marshallese", "", "mh", "mah" },
	{ "Malayalam", "", "ml", "mal" },
	{ "Maori", "", "mi", "mri", "mao" },
	{ "Marathi", "", "mr", "mar" },
	{ "Malay", "", "ms", "msa", "msa" },
	{ "Malagasy", "", "mg", "mlg" },
	{ "Maltese", "", "mt", "mlt" },
	{ "Moldavian", "", "mo", "mol" },
	{ "Mongolian", "", "mn", "mon" },
	{ "Nauru", "", "na", "nau" },
	{ "Navajo", "", "nv", "nav" },
	{ "Dutch", "Nederlands", "nl", "nld", "dut" },
	{ "Ndebele, South", "", "nr", "nbl" },
	{ "Ndebele, North", "", "nd", "nde" },
	{ "Ndonga", "", "ng", "ndo" },
	{ "Nepali", "", "ne", "nep" },
	{ "Norwegian", "Norsk", "no", "nor" },
	{ "Norwegian Nynorsk", "", "nn", "nno" },
	{ "Norwegian Bokmål", "", "nb", "nob" },
	{ "Chichewa; Nyanja", "", "ny", "nya" },
	{ "Occitan", "", "oc", "oci" },
	{ "Ojibwa", "", "oj", "oji" },
	{ "Oriya", "", "or", "ori" },
	{ "Oromo", "", "om", "orm" },
	{ "Ossetian", "", "os", "oss" },
	{ "Panjabi", "", "pa", "pan" },
	{ "Persian", "", "fa", "fas", "per" },
	{ "Pali", "", "pi", "pli" },
	{ "Polish", "", "pl", "pol" },
	{ "Portuguese", "Portugues", "pt", "por" },
	{ "Pushto", "", "ps", "pus" },
	{ "Quechua", "", "qu", "que" },
	{ "Romansh", "", "rm", "roh" },
	{ "Romanian", "", "ro", "ron", "rum" },
	{ "Rundi", "", "rn", "run" },
	{ "Russian", "", "ru", "rus" },
	{ "Sango", "", "sg", "sag" },
	{ "Sanskrit", "", "sa", "san" },
	{ "Serbian", "", "sr", "srp", "scc" },
	{ "Sinhala", "", "si", "sin" },
	{ "Slovak", "", "sk", "slk", "slo" },
	{ "Slovenian", "", "sl", "slv" },
	{ "Northern Sami", "", "se", "sme" },
	{ "Samoan", "", "sm", "smo" },
	{ "Shona", "", "sn", "sna" },
	{ "Sindhi", "", "sd", "snd" },
	{ "Somali", "", "so", "som" },
	{ "Sotho, Southern", "", "st", "sot" },
	{ "Sardinian", "", "sc", "srd" },
	{ "Swati", "", "ss", "ssw" },
	{ "Sundanese", "", "su", "sun" },
	{ "Finnish", "Suomi", "fi", "fin" },
	{ "Swahili", "", "sw", "swa" },
	{ "Swedish", "Svenska", "sv", "swe" },
	{ "Tahitian", "", "ty", "tah" },
	{ "Tamil", "", "ta", "tam" },
	{ "Tatar", "", "tt", "tat" },
	{ "Telugu", "", "te", "tel" },
	{ "Tajik", "", "tg", "tgk" },
	{ "Tagalog", "", "tl", "tgl" },
	{ "Thai", "", "th", "tha" },
	{ "Tibetan", "", "bo", "bod", "tib" },
	{ "Tigrinya", "", "ti", "tir" },
	{ "Tonga", "", "to", "ton" },
	{ "Tswana", "", "tn", "tsn" },
	{ "Tsonga", "", "ts", "tso" },
	{ "Turkmen", "", "tk", "tuk" },
	{ "Turkish", "", "tr", "tur" },
	{ "Twi", "", "tw", "twi" },
	{ "Uighur", "", "ug", "uig" },
	{ "Ukrainian", "", "uk", "ukr" },
	{ "Urdu", "", "ur", "urd" },
	{ "Uzbek", "", "uz", "uzb" },
	{ "Venda", "", "ve", "ven" },
	{ "Vietnamese", "", "vi", "vie" },
	{ "Volapük", "", "vo", "vol" },
	{ "Welsh", "", "cy", "cym", "wel" },
	{ "Walloon", "", "wa", "wln" },
	{ "Wolof", "", "wo", "wol" },
	{ "Xhosa", "", "xh", "xho" },
	{ "Yiddish", "", "yi", "yid" },
	{ "Yoruba", "", "yo", "yor" },
	{ "Zhuang", "", "za", "zha" },
	{ "Zulu", "", "zu", "zul" },
	{NULL, NULL, NULL, NULL}
};
#define	LANG_TABLE_SIZE (sizeof(ghb_language_table)/ sizeof(iso639_lang_t)-1)

static void audio_bitrate_opts_set(GtkBuilder *builder, const gchar *name);

static void
del_tree(const gchar *name, gboolean del_top)
{
	const gchar *file;

	if (g_file_test(name, G_FILE_TEST_IS_DIR))
	{
		GDir *gdir = g_dir_open(name, 0, NULL);
		file = g_dir_read_name(gdir);
		while (file)
		{
			gchar *path;
			path = g_strdup_printf("%s/%s", name, file);
			del_tree(path, TRUE);
			g_free(path);
			file = g_dir_read_name(gdir);
		}
		if (del_top)
			g_rmdir(name);
		g_dir_close(gdir);
	}
	else
	{
		g_unlink(name);
	}
}

const gchar*
ghb_version()
{
	return hb_get_version(NULL);
}

void
ghb_vquality_range(
	signal_user_data_t *ud, 
	gdouble *min, 
	gdouble *max,
	gdouble *step,
	gdouble *page,
	gint *digits,
	gboolean *inverted)
{
	gint vcodec = ghb_settings_combo_int(ud->settings, "VideoEncoder");
	*page = 10;
	*digits = 0;
	switch (vcodec)
	{
		case HB_VCODEC_X264:
		{
			*min = 0;
			*max = 51;
			*step = ghb_settings_combo_double(ud->settings, 
											"VideoQualityGranularity");
			if (*step == 0.2 || *step == 0.5)
				*digits = 1;
			else if (*step == 0.25)
				*digits = 2;
			*inverted = TRUE;
		} break;

		case HB_VCODEC_FFMPEG_MPEG2:
		case HB_VCODEC_FFMPEG_MPEG4:
		{
			*min = 1;
			*max = 31;
			*step = 1;
			*inverted = TRUE;
		} break;

		case HB_VCODEC_THEORA:
		{
			*min = 0;
			*max = 63;
			*step = 1;
			*inverted = FALSE;
		} break;

		default:
		{
			*min = 0;
			*max = 100;
			*step = 1;
			*inverted = FALSE;
		} break;
	}
}

gint
find_combo_entry(combo_opts_t *opts, const GValue *gval)
{
	gint ii;

	if (G_VALUE_TYPE(gval) == G_TYPE_STRING)
	{
		gchar *str;
		str = ghb_value_string(gval);
		for (ii = 0; ii < opts->count; ii++)
		{
			if (strcmp(opts->map[ii].shortOpt, str) == 0)
			{
				break;
			}
		}
		g_free(str);
		return ii;
	}
	else if (G_VALUE_TYPE(gval) == G_TYPE_DOUBLE)
	{
		gdouble val;
		val = ghb_value_double(gval);
		for (ii = 0; ii < opts->count; ii++)
		{
			if (opts->map[ii].ivalue == val)
			{
				break;
			}
		}
		return ii;
	}
	else if (G_VALUE_TYPE(gval) == G_TYPE_INT ||
			 G_VALUE_TYPE(gval) == G_TYPE_BOOLEAN ||
			 G_VALUE_TYPE(gval) == G_TYPE_INT64)
	{
		gint64 val;
		val = ghb_value_int64(gval);
		for (ii = 0; ii < opts->count; ii++)
		{
			if ((gint64)opts->map[ii].ivalue == val)
			{
				break;
			}
		}
		return ii;
	}
	return opts->count;
}

static const gchar*
lookup_generic_string(combo_opts_t *opts, const GValue *gval)
{
	gint ii;
	const gchar *result = "";

	ii = find_combo_entry(opts, gval);
	if (ii < opts->count)
	{
		result = opts->map[ii].svalue;
	}
	return result;
}

static gint
lookup_generic_int(combo_opts_t *opts, const GValue *gval)
{
	gint ii;
	gint result = -1;

	ii = find_combo_entry(opts, gval);
	if (ii < opts->count)
	{
		result = opts->map[ii].ivalue;
	}
	return result;
}

static gdouble
lookup_generic_double(combo_opts_t *opts, const GValue *gval)
{
	gint ii;
	gdouble result = -1;

	ii = find_combo_entry(opts, gval);
	if (ii < opts->count)
	{
		result = opts->map[ii].ivalue;
	}
	return result;
}

static const gchar*
lookup_generic_option(combo_opts_t *opts, const GValue *gval)
{
	gint ii;
	const gchar *result = "";

	ii = find_combo_entry(opts, gval);
	if (ii < opts->count)
	{
		result = opts->map[ii].option;
	}
	return result;
}

static gint
lookup_mix_int(const GValue *mix)
{
	gint ii;
	gint result = 0;


	if (G_VALUE_TYPE(mix) == G_TYPE_STRING)
	{
		gchar * str = ghb_value_string(mix);
		for (ii = 0; ii < hb_audio_mixdowns_count; ii++)
		{
			if (strcmp(hb_audio_mixdowns[ii].short_name, str) == 0)
			{
				result = hb_audio_mixdowns[ii].amixdown;
				break;
			}
		}
		g_free(str);
	}
	else if (G_VALUE_TYPE(mix) == G_TYPE_INT ||
			 G_VALUE_TYPE(mix) == G_TYPE_INT64 ||
			 G_VALUE_TYPE(mix) == G_TYPE_DOUBLE)
	{
		gint val = ghb_value_int(mix);
		for (ii = 0; ii < hb_audio_mixdowns_count; ii++)
		{
			if (hb_audio_mixdowns[ii].amixdown == val)
			{
				result = hb_audio_mixdowns[ii].amixdown;
				break;
			}
		}
	}
	return result;
}

static const gchar*
lookup_mix_option(const GValue *mix)
{
	gint ii;
	gchar *result = "None";


	if (G_VALUE_TYPE(mix) == G_TYPE_STRING)
	{
		gchar *str = ghb_value_string(mix);
		for (ii = 0; ii < hb_audio_mixdowns_count; ii++)
		{
			if (strcmp(hb_audio_mixdowns[ii].short_name, str) == 0)
			{
				result = hb_audio_mixdowns[ii].human_readable_name;
				break;
			}
		}
		g_free(str);
	}
	else if (G_VALUE_TYPE(mix) == G_TYPE_INT ||
			 G_VALUE_TYPE(mix) == G_TYPE_INT64 ||
			 G_VALUE_TYPE(mix) == G_TYPE_DOUBLE)
	{
		gint val = ghb_value_int(mix);
		for (ii = 0; ii < hb_audio_mixdowns_count; ii++)
		{
			if (hb_audio_mixdowns[ii].amixdown == val)
			{
				result = hb_audio_mixdowns[ii].human_readable_name;
				break;
			}
		}
	}
	return result;
}

static const gchar*
lookup_mix_string(const GValue *mix)
{
	gint ii;
	gchar *result = "None";


	if (G_VALUE_TYPE(mix) == G_TYPE_STRING)
	{
		gchar *str = ghb_value_string(mix);
		for (ii = 0; ii < hb_audio_mixdowns_count; ii++)
		{
			if (strcmp(hb_audio_mixdowns[ii].short_name, str) == 0)
			{
				result = hb_audio_mixdowns[ii].short_name;
				break;
			}
		}
		g_free(str);
	}
	else if (G_VALUE_TYPE(mix) == G_TYPE_INT ||
			 G_VALUE_TYPE(mix) == G_TYPE_INT64 ||
			 G_VALUE_TYPE(mix) == G_TYPE_DOUBLE)
	{
		gint val = ghb_value_int(mix);
		for (ii = 0; ii < hb_audio_mixdowns_count; ii++)
		{
			if (hb_audio_mixdowns[ii].amixdown == val)
			{
				result = hb_audio_mixdowns[ii].short_name;
				break;
			}
		}
	}
	return result;
}

static gint
lookup_video_rate_int(const GValue *vrate)
{
	gint ii;
	gchar *str;
	gint result = 0;

	str = ghb_value_string(vrate);
	for (ii = 0; ii < hb_video_rates_count; ii++)
	{
		if (strcmp(hb_video_rates[ii].string, str) == 0)
		{
			result = hb_video_rates[ii].rate;
			break;
		}
	}
	g_free(str);
	// Default to "same as source"
	return result;
}

static const gchar*
lookup_video_rate_option(const GValue *vrate)
{
	gint ii;
	gchar *str;
	const gchar *result = "Same as source";

	str = ghb_value_string(vrate);
	for (ii = 0; ii < hb_video_rates_count; ii++)
	{
		if (strcmp(hb_video_rates[ii].string, str) == 0)
		{
			result = hb_video_rates[ii].string;
			break;
		}
	}
	g_free(str);
	// Default to "same as source"
	return result;
}

static gint
lookup_audio_rate_int(const GValue *rate)
{
	gint ii;
	gint result = 0;

	if (G_VALUE_TYPE(rate) == G_TYPE_STRING)
	{
		// Coincidentally, the string "source" will return 0
		// which is our flag to use "same as source"
		gchar * str = ghb_value_string(rate);
		for (ii = 0; ii < hb_audio_rates_count; ii++)
		{
			if (strcmp(hb_audio_rates[ii].string, str) == 0)
			{
				result = hb_audio_rates[ii].rate;
				break;
			}
		}
		g_free(str);
	}
	else if (G_VALUE_TYPE(rate) == G_TYPE_INT ||
			 G_VALUE_TYPE(rate) == G_TYPE_INT64 ||
			 G_VALUE_TYPE(rate) == G_TYPE_DOUBLE)
	{
		for (ii = 0; ii < hb_audio_rates_count; ii++)
		{
			gint val = ghb_value_int(rate);
			if (val == hb_audio_rates[ii].rate)
			{
				result = hb_audio_rates[ii].rate;
				break;
			}
		}
	}
	return result;
}

static const gchar*
lookup_audio_rate_option(const GValue *rate)
{
	gint ii;
	const gchar *result = "Same as source";

	if (G_VALUE_TYPE(rate) == G_TYPE_STRING)
	{
		// Coincidentally, the string "source" will return 0
		// which is our flag to use "same as source"
		gchar *str = ghb_value_string(rate);
		for (ii = 0; ii < hb_audio_rates_count; ii++)
		{
			if (strcmp(hb_audio_rates[ii].string, str) == 0)
			{
				result = hb_audio_rates[ii].string;
				break;
			}
		}
		g_free(str);
	}
	else if (G_VALUE_TYPE(rate) == G_TYPE_INT ||
			 G_VALUE_TYPE(rate) == G_TYPE_INT64 ||
			 G_VALUE_TYPE(rate) == G_TYPE_DOUBLE)
	{
		for (ii = 0; ii < hb_audio_rates_count; ii++)
		{
			gint val = ghb_value_int(rate);
			if (val == hb_audio_rates[ii].rate)
			{
				result = hb_audio_rates[ii].string;
				break;
			}
		}
	}
	return result;
}

gint
ghb_find_closest_audio_rate(gint rate)
{
	gint ii;
	gint result;

	result = 0;
	for (ii = 0; ii < hb_audio_rates_count; ii++)
	{
		if (rate <= hb_audio_rates[ii].rate)
		{
			result = hb_audio_rates[ii].rate;
			break;
		}
	}
	return result;
}

hb_rate_t *ghb_audio_bitrates;
int ghb_audio_bitrates_count;

static gint
lookup_audio_bitrate_int(const GValue *rate)
{
	gint ii;
	gint result = 0;

	if (G_VALUE_TYPE(rate) == G_TYPE_STRING)
	{
		// Coincidentally, the string "source" will return 0
		// which is our flag to use "same as source"
		gchar *str = ghb_value_string(rate);
		for (ii = 0; ii < ghb_audio_bitrates_count; ii++)
		{
			if (strcmp(ghb_audio_bitrates[ii].string, str) == 0)
			{
				result = ghb_audio_bitrates[ii].rate;
				break;
			}
		}
		g_free(str);
	}
	else if (G_VALUE_TYPE(rate) == G_TYPE_INT ||
			 G_VALUE_TYPE(rate) == G_TYPE_INT64 ||
			 G_VALUE_TYPE(rate) == G_TYPE_DOUBLE)
	{
		gint val = ghb_value_int(rate);
		for (ii = 0; ii < ghb_audio_bitrates_count; ii++)
		{
			if (ghb_audio_bitrates[ii].rate == val)
			{
				result = ghb_audio_bitrates[ii].rate;
				break;
			}
		}
	}
	return result;
}

static const gchar*
lookup_audio_bitrate_option(const GValue *rate)
{
	gint ii;
	const gchar *result = "Same as source";

	if (G_VALUE_TYPE(rate) == G_TYPE_STRING)
	{
		// Coincidentally, the string "source" will return 0
		// which is our flag to use "same as source"
		gchar *str = ghb_value_string(rate);
		for (ii = 0; ii < ghb_audio_bitrates_count; ii++)
		{
			if (strcmp(ghb_audio_bitrates[ii].string, str) == 0)
			{
				result = ghb_audio_bitrates[ii].string;
				break;
			}
		}
		g_free(str);
	}
	else if (G_VALUE_TYPE(rate) == G_TYPE_INT ||
			 G_VALUE_TYPE(rate) == G_TYPE_INT64 ||
			 G_VALUE_TYPE(rate) == G_TYPE_DOUBLE)
	{
		gint val = ghb_value_int(rate);
		for (ii = 0; ii < ghb_audio_bitrates_count; ii++)
		{
			if (ghb_audio_bitrates[ii].rate == val)
			{
				result = ghb_audio_bitrates[ii].string;
				break;
			}
		}
	}
	return result;
}

static gint
lookup_audio_lang_int(const GValue *rate)
{
	gint ii;
	gchar *str;
	gint result = 0;

	// Coincidentally, the string "source" will return 0
	// which is our flag to use "same as source"
	str = ghb_value_string(rate);
	for (ii = 0; ii < LANG_TABLE_SIZE; ii++)
	{
		if (strcmp(ghb_language_table[ii].iso639_2, str) == 0)
		{
			result = ii;
			break;
		}
	}
	g_free(str);
	return result;
}

static const gchar*
lookup_audio_lang_option(const GValue *rate)
{
	gint ii;
	gchar *str;
	const gchar *result = "Same as source";

	// Coincidentally, the string "source" will return 0
	// which is our flag to use "same as source"
	str = ghb_value_string(rate);
	for (ii = 0; ii < LANG_TABLE_SIZE; ii++)
	{
		if (strcmp(ghb_language_table[ii].iso639_2, str) == 0)
		{
			if (ghb_language_table[ii].native_name[0] != 0)
				result = ghb_language_table[ii].native_name;
			else
				result = ghb_language_table[ii].eng_name;
			break;
		}
	}
	g_free(str);
	return result;
}

GValue*
ghb_lookup_acodec_value(gint val)
{
	GValue *value = NULL;
	gint ii;

	for (ii = 0; ii < acodec_opts.count; ii++)
	{
		if ((int)acodec_opts.map[ii].ivalue == val)
		{
			value = ghb_string_value_new(acodec_opts.map[ii].shortOpt);
			return value;
		}
	}
	value = ghb_string_value_new("auto");
	return value;
}

static GValue*
get_amix_value(gint val)
{
	GValue *value = NULL;
	gint ii;

	for (ii = 0; ii < hb_audio_mixdowns_count; ii++)
	{
		if (hb_audio_mixdowns[ii].amixdown == val)
		{
			value = ghb_string_value_new(hb_audio_mixdowns[ii].short_name);
			break;
		}
	}
	return value;
}

// Handle for libhb.  Gets set by ghb_backend_init()
static hb_handle_t * h_scan = NULL;
static hb_handle_t * h_queue = NULL;

extern void hb_get_temporary_directory(char path[512]);

gchar*
ghb_get_tmp_dir()
{
	char dir[512];

	hb_get_temporary_directory(dir);
	return g_strdup(dir);
}

void
ghb_hb_cleanup(gboolean partial)
{
	char dir[512];

	hb_get_temporary_directory(dir);
	del_tree(dir, !partial);
}

gint
ghb_subtitle_track_source(signal_user_data_t *ud, gint track)
{
	gint titleindex;

	if (track == -2)
		return SRTSUB;
	if (track < 0)
		return VOBSUB;
	titleindex = ghb_settings_combo_int(ud->settings, "title");
	if (titleindex < 0)
		return VOBSUB;

	hb_list_t  * list;
	hb_title_t * title;
	hb_subtitle_t * sub;
	
	if (h_scan == NULL) return VOBSUB;
	list = hb_get_titles( h_scan );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		return VOBSUB;
	}
	title = hb_list_item( list, titleindex );
	if (title == NULL) return VOBSUB;	// Bad titleindex
	sub = hb_list_item( title->list_subtitle, track);
	if (sub != NULL)
		return sub->source;
	else
		return VOBSUB;
}

const char*
ghb_subtitle_source_name(gint source)
{
	const gchar * name = "Unknown";
	switch (source)
	{
		case VOBSUB:
			name = "VOBSUB";
			break;
		case TX3GSUB:
			name = "TX3G";
			break;
		case UTF8SUB:
			name = "UTF8";
			break;
		case CC708SUB:
		case CC608SUB:
			name = "CC";
			break;
		case SRTSUB:
			name = "SRT";
			break;
		case SSASUB:
			name = "SSA";
			break;
		default:
			break;
	}
	return name;
}

const char*
ghb_subtitle_track_source_name(signal_user_data_t *ud, gint track)
{
	gint titleindex;
	const gchar * name = "Unknown";

	if (track == -2)
	{
		name = "SRT";
		goto done;
	}
	if (track == -1)
	{
		name = "Bitmap";
		goto done;
	}

	titleindex = ghb_settings_combo_int(ud->settings, "title");
	if (titleindex < 0)
		goto done;

	hb_list_t  * list;
	hb_title_t * title;
	hb_subtitle_t * sub;
	
	if (h_scan == NULL) 
		goto done;
	list = hb_get_titles( h_scan );
	if( !hb_list_count( list ) )
		goto done;

	title = hb_list_item( list, titleindex );
	if (title == NULL)
		goto done;

	sub = hb_list_item( title->list_subtitle, track);
	if (sub != NULL)
	{
		name = ghb_subtitle_source_name(sub->source);
	}

done:
	return name;
}

gchar*
ghb_subtitle_track_lang(signal_user_data_t *ud, gint track)
{
	gint titleindex;

	titleindex = ghb_settings_combo_int(ud->settings, "title");
	if (titleindex < 0)
		goto fail;
	if (track == -1)
		return ghb_get_user_audio_lang(ud, titleindex, 0);
	if (track < 0)
		goto fail;

	hb_list_t  * list;
	hb_title_t * title;
	hb_subtitle_t * sub;
	
	if (h_scan == NULL)
		goto fail;

	list = hb_get_titles( h_scan );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		goto fail;
	}
	title = hb_list_item( list, titleindex );
	if (title == NULL) 	// Bad titleindex
		goto fail;
	sub = hb_list_item( title->list_subtitle, track);
	if (sub != NULL)
	   	return g_strdup(sub->iso639_2);

fail:
	return g_strdup("und");
}

gint
ghb_get_title_number(gint titleindex)
{
	hb_list_t  * list;
	hb_title_t * title;
	
	if (h_scan == NULL) return 1;
	list = hb_get_titles( h_scan );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		return 1;
	}
	title = hb_list_item( list, titleindex );
	if (title == NULL) return 1;	// Bad titleindex
	return title->index;
}

static hb_audio_config_t*
get_hb_audio(hb_handle_t *h, gint titleindex, gint track)
{
	hb_list_t  * list;
	hb_title_t * title;
    hb_audio_config_t *audio = NULL;
	
    if (h == NULL) return NULL;
	list = hb_get_titles( h );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		return NULL;
	}
    title = hb_list_item( list, titleindex );
	if (title == NULL) return NULL;	// Bad titleindex
	if (!hb_list_count(title->list_audio))
	{
		return NULL;
	}
    audio = (hb_audio_config_t *)hb_list_audio_config_item(title->list_audio, track);
	return audio;
}

static gint
search_rates(hb_rate_t *rates, gint rate, gint count)
{
	gint ii;
	for (ii = 0; ii < count; ii++)
	{
		if (rates[ii].rate == rate)
			return ii;
	}
	return -1;
}

static gboolean find_combo_item_by_int(GtkTreeModel *store, gint value, GtkTreeIter *iter);

static GtkListStore*
get_combo_box_store(GtkBuilder *builder, const gchar *name)
{
	GtkComboBox *combo;
	GtkListStore *store;

	g_debug("get_combo_box_store() %s\n", name);
	// First modify the combobox model to allow greying out of options
	combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
	store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
	return store;
}

static void
grey_combo_box_item(GtkBuilder *builder, const gchar *name, gint value, gboolean grey)
{
	GtkListStore *store;
	GtkTreeIter iter;
	
	store = get_combo_box_store(builder, name);
	if (find_combo_item_by_int(GTK_TREE_MODEL(store), value, &iter))
	{
		gtk_list_store_set(store, &iter, 
						   1, !grey, 
						   -1);
	}
}

void
ghb_grey_combo_options(GtkBuilder *builder)
{
	GtkWidget *widget;
	gint container, track, titleindex, acodec;
    hb_audio_config_t *aconfig = NULL;
	GValue *gval;
	
	widget = GHB_WIDGET (builder, "title");
	gval = ghb_widget_value(widget);
	titleindex = ghb_lookup_combo_int("title", gval);
	ghb_value_free(gval);
	widget = GHB_WIDGET (builder, "AudioTrack");
	gval = ghb_widget_value(widget);
	track = ghb_lookup_combo_int("AudioTrack", gval);
	ghb_value_free(gval);
	aconfig = get_hb_audio(h_scan, titleindex, track);
	widget = GHB_WIDGET (builder, "FileFormat");
	gval = ghb_widget_value(widget);
	container = ghb_lookup_combo_int("FileFormat", gval);
	ghb_value_free(gval);

	grey_combo_box_item(builder, "x264_analyse", 4, TRUE);
	grey_combo_box_item(builder, "AudioEncoder", HB_ACODEC_FAAC, FALSE);
	grey_combo_box_item(builder, "AudioEncoder", HB_ACODEC_LAME, FALSE);
	grey_combo_box_item(builder, "AudioEncoder", HB_ACODEC_VORBIS, FALSE);

	gboolean allow_dca = TRUE;
	allow_dca = (container != HB_MUX_MP4);

	grey_combo_box_item(builder, "AudioEncoder", HB_ACODEC_AC3_PASS, FALSE);
	if (allow_dca)
		grey_combo_box_item(builder, "AudioEncoder", HB_ACODEC_DCA_PASS, FALSE);
	else
		grey_combo_box_item(builder, "AudioEncoder", HB_ACODEC_DCA_PASS, TRUE);

	if (aconfig && aconfig->in.codec != HB_ACODEC_AC3)
	{
		grey_combo_box_item(builder, "AudioEncoder", HB_ACODEC_AC3_PASS, TRUE);
	}
	if (aconfig && aconfig->in.codec != HB_ACODEC_DCA)
	{
		grey_combo_box_item(builder, "AudioEncoder", HB_ACODEC_DCA_PASS, TRUE);
	}
	grey_combo_box_item(builder, "VideoEncoder", HB_VCODEC_THEORA, FALSE);

	widget = GHB_WIDGET (builder, "AudioEncoder");
	gval = ghb_widget_value(widget);
	acodec = ghb_lookup_combo_int("AudioEncoder", gval);
	ghb_value_free(gval);
	grey_combo_box_item(builder, "AudioMixdown", 0, TRUE);
	if (container == HB_MUX_MP4)
	{
		grey_combo_box_item(builder, "AudioEncoder", HB_ACODEC_VORBIS, TRUE);
		grey_combo_box_item(builder, "VideoEncoder", HB_VCODEC_THEORA, TRUE);
	}

	gboolean allow_mono = TRUE;
	gboolean allow_stereo = TRUE;
	gboolean allow_dolby = TRUE;
	gboolean allow_dpl2 = TRUE;
	gboolean allow_6ch = TRUE;
	allow_mono = TRUE;
	allow_6ch = acodec & ~HB_ACODEC_LAME;
	if (aconfig)
	{
		gint best = hb_get_best_mixdown(acodec, aconfig->in.channel_layout, 0);

		allow_stereo = best >= HB_AMIXDOWN_STEREO;
		allow_dolby = best >= HB_AMIXDOWN_DOLBY;
		allow_dpl2 = best >= HB_AMIXDOWN_DOLBYPLII;
		allow_6ch = best >= HB_AMIXDOWN_6CH;
	}
	grey_combo_box_item(builder, "AudioMixdown", HB_AMIXDOWN_MONO, !allow_mono);
	grey_combo_box_item(builder, "AudioMixdown", HB_AMIXDOWN_STEREO, !allow_stereo);
	grey_combo_box_item(builder, "AudioMixdown", HB_AMIXDOWN_DOLBY, !allow_dolby);
	grey_combo_box_item(builder, "AudioMixdown", HB_AMIXDOWN_DOLBYPLII, !allow_dpl2);
	grey_combo_box_item(builder, "AudioMixdown", HB_AMIXDOWN_6CH, !allow_6ch);
}

gint
ghb_get_best_mix(hb_audio_config_t *aconfig, gint acodec, gint mix)
{
	int layout;
	layout = aconfig ? aconfig->in.channel_layout : 
						HB_INPUT_CH_LAYOUT_3F2R | HB_INPUT_CH_LAYOUT_HAS_LFE;
	return hb_get_best_mixdown( acodec, layout, mix );
}

// Set up the model for the combo box
static void
init_combo_box(GtkBuilder *builder, const gchar *name)
{
	GtkComboBox *combo;
	GtkListStore *store;
	GtkCellRenderer *cell;

	g_debug("init_combo_box() %s\n", name);
	// First modify the combobox model to allow greying out of options
	combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
	if (combo == NULL)
		return;
	// Store contains:
	// 1 - String to display
	// 2 - bool indicating whether the entry is selectable (grey or not)
	// 3 - String that is used for presets
	// 4 - Int value determined by backend
	// 5 - String value determined by backend
	store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_BOOLEAN, 
							   G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_STRING);
	gtk_combo_box_set_model(combo, GTK_TREE_MODEL(store));

	if (G_OBJECT_TYPE(combo) == GTK_TYPE_COMBO_BOX)
	{
		gtk_cell_layout_clear(GTK_CELL_LAYOUT(combo));
    	cell = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
    	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), cell, TRUE);
    	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), cell,
      		"markup", 0, "sensitive", 1, NULL);
	}
	else
	{ // Combo box entry
		gtk_combo_box_entry_set_text_column(GTK_COMBO_BOX_ENTRY(combo), 0);
	}
}	

static void
audio_samplerate_opts_set(GtkBuilder *builder, const gchar *name, hb_rate_t *rates, gint count)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gint ii;
	gchar *str;
	
	g_debug("audio_samplerate_opts_set ()\n");
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	// Add an item for "Same As Source"
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
					   0, "<small>Same as source</small>", 
					   1, TRUE, 
					   2, "source", 
					   3, 0.0, 
					   4, "source", 
					   -1);
	for (ii = 0; ii < count; ii++)
	{
		gtk_list_store_append(store, &iter);
		str = g_strdup_printf("<small>%s</small>", rates[ii].string);
		gtk_list_store_set(store, &iter, 
						   0, str,
						   1, TRUE, 
						   2, rates[ii].string, 
						   3, (gdouble)rates[ii].rate, 
						   4, rates[ii].string, 
						   -1);
		g_free(str);
	}
}

static void
video_rate_opts_set(GtkBuilder *builder, const gchar *name, hb_rate_t *rates, gint count)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gint ii;
	
	g_debug("video_rate_opts_set ()\n");
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	// Add an item for "Same As Source"
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
					   0, "Same as source", 
					   1, TRUE, 
					   2, "source", 
					   3, 0.0, 
					   4, "source", 
					   -1);
	for (ii = 0; ii < count; ii++)
	{
		gchar *desc = "";
		gchar *option;
		if (strcmp(rates[ii].string, "23.976") == 0)
		{
			desc = "(NTSC Film)";
		}
		else if (strcmp(rates[ii].string, "25") == 0)
		{
			desc = "(PAL Film/Video)";
		}
		else if (strcmp(rates[ii].string, "29.97") == 0)
		{
			desc = "(NTSC Video)";
		}
		option = g_strdup_printf ("%s %s", rates[ii].string, desc);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, option, 
						   1, TRUE, 
						   2, rates[ii].string, 
						   3, (gdouble)rates[ii].rate, 
						   4, rates[ii].string, 
						   -1);
		g_free(option);
	}
}

static void
mix_opts_set(GtkBuilder *builder, const gchar *name)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gint ii;
	gchar *str;
	
	g_debug("mix_opts_set ()\n");
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
					   0, "<small>None</small>", 
					   1, TRUE, 
					   2, "none", 
					   3, 0.0, 
					   4, "none", 
					   -1);
	for (ii = 0; ii < hb_audio_mixdowns_count; ii++)
	{
		gtk_list_store_append(store, &iter);
		str = g_strdup_printf("<small>%s</small>",
			hb_audio_mixdowns[ii].human_readable_name);
		gtk_list_store_set(store, &iter, 
						   0, str,
						   1, TRUE, 
						   2, hb_audio_mixdowns[ii].short_name, 
						   3, (gdouble)hb_audio_mixdowns[ii].amixdown, 
						   4, hb_audio_mixdowns[ii].internal_name, 
						   -1);
		g_free(str);
	}
}

static void
srt_codeset_opts_set(GtkBuilder *builder, const gchar *name)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gint ii;
	
	g_debug("srt_codeset_opts_set ()\n");
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	for (ii = 0; ii < SRT_TABLE_SIZE; ii++)
	{
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, srt_codeset_table[ii],
						   1, TRUE, 
						   2, srt_codeset_table[ii],
						   3, (gdouble)ii, 
						   4, srt_codeset_table[ii],
						   -1);
	}
	GtkComboBoxEntry *cbe;

	cbe = GTK_COMBO_BOX_ENTRY(GHB_WIDGET(builder, name));
	//gtk_combo_box_entry_set_text_column(cbe, 0);
}

static void
language_opts_set(GtkBuilder *builder, const gchar *name)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gint ii;
	
	g_debug("language_opts_set ()\n");
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	for (ii = 0; ii < LANG_TABLE_SIZE; ii++)
	{
		const gchar *lang;

		if (ghb_language_table[ii].native_name[0] != 0)
			lang = ghb_language_table[ii].native_name;
		else
			lang = ghb_language_table[ii].eng_name;
		
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, lang,
						   1, TRUE, 
						   2, ghb_language_table[ii].iso639_2, 
						   3, (gdouble)ii, 
						   4, ghb_language_table[ii].iso639_1, 
						   -1);
	}
}

static gchar **titles = NULL;

void
title_opts_set(GtkBuilder *builder, const gchar *name)
{
	GtkTreeIter iter;
	GtkListStore *store;
	hb_list_t  * list = NULL;
	hb_title_t * title = NULL;
	gint ii;
	gint count = 0;
	
	g_debug("title_opts_set ()\n");
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	if (h_scan != NULL)
	{
		list = hb_get_titles( h_scan );
		count = hb_list_count( list );
		if (count > 100) count = 100;
	}
	if (titles) g_strfreev(titles);
	if (title_opts.map) g_free(title_opts.map);
	if (count > 0)
	{
		title_opts.count = count;
		title_opts.map = g_malloc(count*sizeof(options_map_t));
		titles = g_malloc((count+1) * sizeof(gchar*));
	}
	else
	{
		title_opts.count = 1;
		title_opts.map = g_malloc(sizeof(options_map_t));
		titles = NULL;
	}
	if( count <= 0 )
	{
		// No titles.  Fill in a default.
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, "No Titles", 
						   1, TRUE, 
						   2, "none", 
						   3, -1.0, 
						   4, "none", 
						   -1);
		title_opts.map[0].option = "No Titles";
		title_opts.map[0].shortOpt = "none";
		title_opts.map[0].ivalue = -1;
		title_opts.map[0].svalue = "none";
		return;
	}
	for (ii = 0; ii < count; ii++)
	{
		title = (hb_title_t*)hb_list_item(list, ii);
		if (title->type == HB_STREAM_TYPE || title->type == HB_FF_STREAM_TYPE)
		{
			if (title->duration != 0)
			{
                char *tmp;
				tmp  = g_strdup_printf ("%d - %02dh%02dm%02ds - %s",
					title->index, title->hours, title->minutes, title->seconds, 
					title->name);
                titles[ii] = g_markup_escape_text(tmp, -1);
                g_free(tmp);
			}
			else
			{
                char *tmp;
				tmp  = g_strdup_printf ("%d - %s", 
										title->index, title->name);
                titles[ii] = g_markup_escape_text(tmp, -1);
                g_free(tmp);
			}
		}
		else
		{
			if (title->duration != 0)
			{
				titles[ii]  = g_strdup_printf ("%d - %02dh%02dm%02ds",
					title->index, title->hours, title->minutes, title->seconds);
			}
			else
			{
				titles[ii]  = g_strdup_printf ("%d - Unknown Length", 
										title->index);
			}
		}
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, titles[ii], 
						   1, TRUE, 
						   2, titles[ii], 
						   3, (gdouble)ii, 
						   4, titles[ii], 
						   -1);
		title_opts.map[ii].option = titles[ii];
		title_opts.map[ii].shortOpt = titles[ii];
		title_opts.map[ii].ivalue = ii;
		title_opts.map[ii].svalue = titles[ii];
	}
	titles[ii] = NULL;
}

static gboolean
find_combo_item_by_int(GtkTreeModel *store, gint value, GtkTreeIter *iter)
{
	gdouble ivalue;
	gboolean foundit = FALSE;
	
	if (gtk_tree_model_get_iter_first (store, iter))
	{
		do
		{
			gtk_tree_model_get(store, iter, 3, &ivalue, -1);
			if (value == (gint)ivalue)
			{
				foundit = TRUE;
				break;
			}
		} while (gtk_tree_model_iter_next (store, iter));
	}
	return foundit;
}

void
audio_track_opts_set(GtkBuilder *builder, const gchar *name, gint titleindex)
{
	GtkTreeIter iter;
	GtkListStore *store;
	hb_list_t  * list = NULL;
	hb_title_t * title = NULL;
    hb_audio_config_t * audio;
	gint ii;
	gint count = 0;
	gchar *str;
	
	g_debug("audio_track_opts_set ()\n");
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	if (h_scan != NULL)
	{
		list = hb_get_titles( h_scan );
	    title = (hb_title_t*)hb_list_item( list, titleindex );
		if (title != NULL)
		{
			count = hb_list_count( title->list_audio );
		}
	}
	if (count > 100) count = 100;
	if (audio_track_opts.map) g_free(audio_track_opts.map);
	if (count > 0)
	{
		audio_track_opts.count = count;
		audio_track_opts.map = g_malloc(count*sizeof(options_map_t));
	}
	else
	{
		audio_track_opts.count = 1;
		audio_track_opts.map = g_malloc(sizeof(options_map_t));
	}
	if( count <= 0 )
	{
		// No audio. set some default
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, "<small>No Audio</small>", 
						   1, TRUE, 
						   2, "none", 
						   3, -1.0, 
						   4, "none", 
						   -1);
		audio_track_opts.map[0].option = "No Audio";
		audio_track_opts.map[0].shortOpt = "none";
		audio_track_opts.map[0].ivalue = -1;
		audio_track_opts.map[0].svalue = "none";
		return;
	}
	index_str_init(count-1);
	for (ii = 0; ii < count; ii++)
	{
        audio = (hb_audio_config_t *) hb_list_audio_config_item( title->list_audio, ii );
		gtk_list_store_append(store, &iter);
		str = g_strdup_printf("<small>%s</small>", audio->lang.description);
		gtk_list_store_set(store, &iter, 
						   0, str,
						   1, TRUE, 
						   2, index_str[ii], 
						   3, (gdouble)ii, 
						   4, index_str[ii], 
						   -1);
		g_free(str);
		audio_track_opts.map[ii].option = audio->lang.description,
		audio_track_opts.map[ii].shortOpt = index_str[ii];
		audio_track_opts.map[ii].ivalue = ii;
		audio_track_opts.map[ii].svalue = index_str[ii];
	}
}

void
subtitle_track_opts_set(GtkBuilder *builder, const gchar *name, gint titleindex)
{
	GtkTreeIter iter;
	GtkListStore *store;
	hb_list_t  * list = NULL;
	hb_title_t * title = NULL;
	hb_subtitle_t * subtitle;
	gint ii, count = 0;
	static char ** options = NULL;
	
	g_debug("subtitle_track_opts_set ()\n");
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	if (h_scan != NULL)
	{
		list = hb_get_titles( h_scan );
	    title = (hb_title_t*)hb_list_item( list, titleindex );
		if (title != NULL)
		{
			count = hb_list_count( title->list_subtitle );
		}
	}
	if (count > 100) count = 100;
	if (subtitle_opts.map) g_free(subtitle_opts.map);
	if (count > 0)
	{
		subtitle_opts.count = count+1;
		subtitle_opts.map = g_malloc((count+1)*sizeof(options_map_t));
	}
	else
	{
		subtitle_opts.count = LANG_TABLE_SIZE+1;
		subtitle_opts.map = g_malloc((LANG_TABLE_SIZE+1)*sizeof(options_map_t));
	}
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
					   0, "Foreign Audio Search", 
					   1, TRUE, 
					   2, "-1", 
					   3, -1.0, 
					   4, "auto", 
					   -1);
	subtitle_opts.map[0].option = "Foreign Audio Search";
	subtitle_opts.map[0].shortOpt = "-1";
	subtitle_opts.map[0].ivalue = -1;
	subtitle_opts.map[0].svalue = "auto";
	if (count > 0)
	{
		if (options != NULL)
			g_strfreev(options);
		options = g_malloc((count+1)*sizeof(gchar*));
		index_str_init(count-1);
		for (ii = 0; ii < count; ii++)
		{
       		subtitle = (hb_subtitle_t *)hb_list_item(title->list_subtitle, ii);
			options[ii] = g_strdup_printf("%d - %s (%s)", ii+1, 
				subtitle->lang, 
				ghb_subtitle_source_name(subtitle->source));
			subtitle_opts.map[ii+1].option = options[ii];
			subtitle_opts.map[ii+1].shortOpt = index_str[ii];
			subtitle_opts.map[ii+1].ivalue = ii;
			subtitle_opts.map[ii+1].svalue = subtitle->iso639_2;
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 
					   	0, options[ii], 
					   	1, TRUE, 
					   	2, index_str[ii], 
					   	3, (gdouble)ii, 
					   	4, subtitle->iso639_2, 
					   	-1);
		}
		options[count] = NULL;
	}
	else
	{
		index_str_init(LANG_TABLE_SIZE-1);
		for (ii = 0; ii < LANG_TABLE_SIZE; ii++)
		{
			const gchar *lang;

			if (ghb_language_table[ii].native_name[0] != 0)
				lang = ghb_language_table[ii].native_name;
			else
				lang = ghb_language_table[ii].eng_name;

			subtitle_opts.map[ii+1].option = lang;
			subtitle_opts.map[ii+1].shortOpt = index_str[ii];
			subtitle_opts.map[ii+1].ivalue = ii;
			subtitle_opts.map[ii+1].svalue = ghb_language_table[ii].iso639_2;
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 
					0, lang,
					1, TRUE, 
					2, index_str[ii],
					3, (gdouble)ii, 
					4, ghb_language_table[ii].iso639_2, 
					-1);
		}
	}
}

gint
ghb_longest_title()
{
	hb_list_t  * list;
	hb_title_t * title;
	gint ii;
	gint count = 0;
	gint titleindex = 0;
	gint feature;
	
	g_debug("ghb_longest_title ()\n");
	if (h_scan == NULL) return 0;
	list = hb_get_titles( h_scan );
	count = hb_list_count( list );
	if (count > 100) count = 100;
	if (count < 1) return 0;
	title = (hb_title_t*)hb_list_item(list, 0);
	feature = title->job->feature;
	for (ii = 0; ii < count; ii++)
	{
		title = (hb_title_t*)hb_list_item(list, ii);
		if (title->index == feature)
		{
			return ii;
		}
	}
	return titleindex;
}

gchar*
ghb_get_source_audio_lang(gint titleindex, gint track)
{
	hb_list_t  * list;
	hb_title_t * title;
    hb_audio_config_t * audio;
	gchar *lang = NULL;
	
	g_debug("ghb_lookup_1st_audio_lang ()\n");
	if (h_scan == NULL) 
		return NULL;
	list = hb_get_titles( h_scan );
    title = (hb_title_t*)hb_list_item( list, titleindex );
	if (title == NULL)
		return NULL;
	if (hb_list_count( title->list_audio ) <= track)
		return NULL;

	audio = hb_list_audio_config_item(title->list_audio, track);
	if (audio == NULL)
		return NULL;

	lang = g_strdup(audio->lang.iso639_2);
	return lang;
}

static gboolean*
get_track_used(gint acodec, GHashTable *track_indices, gint count)
{
	gboolean *used;

	used = g_hash_table_lookup(track_indices, &acodec);
	if (used == NULL)
	{
		gint *key;

		used = g_malloc0(count * sizeof(gboolean));
		key = g_malloc(sizeof(gint));
		*key = acodec;
		g_hash_table_insert(track_indices, key, used);
	}
	return used;
}

gint
ghb_find_audio_track(
	gint titleindex, 
	const gchar *lang, 
	gint acodec,
	gint fallback_acodec,
	GHashTable *track_indices)
{
	hb_list_t  * list;
	hb_title_t * title;
	hb_audio_config_t * audio;
	gint ii;
	gint count = 0;
	gint track = -1;
	gint max_chan;
	gboolean *used = NULL;
	gboolean *passthru_used;
	gint try_acodec;
	gint passthru_acodec;
	gboolean passthru;
	gint channels;
	
	g_debug("find_audio_track ()\n");
	if (h_scan == NULL) return -1;
	list = hb_get_titles( h_scan );
	title = (hb_title_t*)hb_list_item( list, titleindex );
	if (title != NULL)
	{
		count = hb_list_count( title->list_audio );
	}
	if (count > 10) count = 10;
	// Try to find an item that matches the preferred language and
	// the passthru codec type
	max_chan = 0;
	passthru = (acodec & HB_ACODEC_PASS_FLAG) != 0;
	if (passthru)
	{
		for (ii = 0; ii < count; ii++)
		{
			audio = (hb_audio_config_t*)hb_list_audio_config_item( 
													title->list_audio, ii );
			passthru_acodec = HB_ACODEC_PASS_MASK & acodec & audio->in.codec;
			// Is the source track use a passthru capable codec?
			if (passthru_acodec == 0)
				continue;
			used = get_track_used(passthru_acodec, track_indices, count);
			// Has the track already been used with this codec?
			if (used[ii])
				continue;

			channels = HB_INPUT_CH_LAYOUT_GET_DISCRETE_COUNT(
													audio->in.channel_layout);
			// Find a track that is not visually impaired or dirctor's
			// commentary, and has the highest channel count.
			if ((audio->lang.type < 2) &&
				((strcmp(lang, audio->lang.iso639_2) == 0) ||
				(strcmp(lang, "und") == 0)))
			{
				if (channels > max_chan)
				{
					track = ii;
					max_chan = channels;
				}
			}
		}
		try_acodec = fallback_acodec;
	}
	else
	{
		try_acodec = acodec;
	}
	if (track > -1)
	{
		used[track] = TRUE;
		return track;
	}
	// Try to find an item that matches the preferred language
	max_chan = 0;
	used = get_track_used(try_acodec, track_indices, count);
	for (ii = 0; ii < count; ii++)
	{
		// Has the track already been used with this codec?
		if (used[ii])
			continue;
		audio = (hb_audio_config_t*)hb_list_audio_config_item( 
												title->list_audio, ii );
		passthru_acodec = HB_ACODEC_PASS_MASK & audio->in.codec;
		if (passthru_acodec && passthru)
		{
			passthru_used = get_track_used(passthru_acodec, track_indices, count);
			// Has the track already been used with this codec for passthru?
			if (passthru_used[ii])
				continue;
		}
		channels = HB_INPUT_CH_LAYOUT_GET_DISCRETE_COUNT(
												audio->in.channel_layout);
		// Find a track that is not visually impaired or dirctor's commentary
		if ((audio->lang.type < 2) &&
			((strcmp(lang, audio->lang.iso639_2) == 0) ||
			(strcmp(lang, "und") == 0)))
		{
			if (channels > max_chan)
			{
				track = ii;
				max_chan = channels;
			}
		}
	}
	if (track > -1)
	{
		used[track] = TRUE;
		return track;
	}
	// Try to fine an item that does not match the preferred language and
	// matches the passthru codec type
	max_chan = 0;
	if (passthru)
	{
		for (ii = 0; ii < count; ii++)
		{
			audio = (hb_audio_config_t*)hb_list_audio_config_item( 
													title->list_audio, ii );
			passthru_acodec = HB_ACODEC_PASS_MASK & acodec & audio->in.codec;
			// Is the source track use a passthru capable codec?
			if (passthru_acodec == 0)
				continue;
			used = get_track_used(passthru_acodec, track_indices, count);
			// Has the track already been used with this codec?
			if (used[ii])
				continue;

			channels = HB_INPUT_CH_LAYOUT_GET_DISCRETE_COUNT(
													audio->in.channel_layout);
			// Find a track that is not visually impaired or dirctor's
			// commentary, and has the highest channel count.
			if (audio->lang.type < 2)
			{
				if (channels > max_chan)
				{
					track = ii;
					max_chan = channels;
				}
			}
		}
		try_acodec = fallback_acodec;
	}
	else
	{
		try_acodec = acodec;
	}
	if (track > -1)
	{
		used[track] = TRUE;
		return track;
	}
	// Try to fine an item that does not match the preferred language
	max_chan = 0;
	used = get_track_used(try_acodec, track_indices, count);
	for (ii = 0; ii < count; ii++)
	{
		// Has the track already been used with this codec?
		if (used[ii])
			continue;
		audio = (hb_audio_config_t*)hb_list_audio_config_item( 
													title->list_audio, ii );
		passthru_acodec = HB_ACODEC_PASS_MASK & audio->in.codec;
		channels = HB_INPUT_CH_LAYOUT_GET_DISCRETE_COUNT(
												audio->in.channel_layout);
		if (passthru_acodec && passthru)
		{
			passthru_used = get_track_used(passthru_acodec, track_indices, count);
			// Has the track already been used with this codec for passthru?
			if (passthru_used[ii])
				continue;
		}
		// Find a track that is not visually impaired or dirctor's commentary
		if (audio->lang.type < 2)
		{
			if (channels > max_chan)
			{
				track = ii;
				max_chan = channels;
			}
		}
	}
	if (track > -1)
	{
		used[track] = TRUE;
		return track;
	}
	// Last ditch, anything goes
	for (ii = 0; ii < count; ii++)
	{
		audio = (hb_audio_config_t*)hb_list_audio_config_item( 
												title->list_audio, ii );
		passthru_acodec = HB_ACODEC_PASS_MASK & audio->in.codec;
		if (passthru_acodec && passthru)
		{
			passthru_used = get_track_used(passthru_acodec, track_indices, count);
			// Has the track already been used with this codec for passthru?
			if (passthru_used[ii])
				continue;
		}
		// Has the track already been used with this codec?
		if (!used[ii])
		{
			track = ii;
			break;
		}
	}
	if (track > -1)
	{
		used[track] = TRUE;
	}
	return track;
}

gint
ghb_find_pref_subtitle_track(const gchar *lang)
{
	gint ii, count;
	count = subtitle_opts.count;
	for (ii = 0; ii < count; ii++)
	{
		if (strcmp(lang, subtitle_opts.map[ii].svalue) == 0)
		{
			return subtitle_opts.map[ii].ivalue;
		}
	}
	return -2;
}

gint
ghb_find_cc_track(gint titleindex)
{
	hb_list_t  * list;
	hb_title_t * title;
	hb_subtitle_t * subtitle;
	gint count, ii;
	
	g_debug("ghb_find_cc_track ()\n");
	if (h_scan == NULL) return -2;
	list = hb_get_titles( h_scan );
	title = (hb_title_t*)hb_list_item( list, titleindex );
	if (title != NULL)
	{
		count = hb_list_count( title->list_subtitle );
		// Try to find an item that matches the preferred language
		for (ii = 0; ii < count; ii++)
		{
       		subtitle = (hb_subtitle_t*)hb_list_item( title->list_subtitle, ii );
			if (subtitle->source == CC608SUB || subtitle->source == CC708SUB)
				return ii;
		}
	}
	return -2;
}

static gboolean
canForce(int source)
{
	return (source == VOBSUB);
}

static gboolean
canBurn(int source)
{
	return (source == VOBSUB || source == SSASUB);
}

gint
ghb_find_subtitle_track(
	gint          titleindex, 
	const gchar * lang, 
	gboolean      burn,
	gboolean      force,
	gint          source,
	GHashTable  * track_indices)
{
	hb_list_t  * list;
	hb_title_t * title;
	hb_subtitle_t * subtitle;
	gint count, ii;
	gboolean *used;
	
	g_debug("find_subtitle_track ()\n");
	if (strcmp(lang, "auto") == 0)
		return -1;
	if (h_scan == NULL) return -1;
	list = hb_get_titles( h_scan );
	title = (hb_title_t*)hb_list_item( list, titleindex );
	if (title != NULL)
	{
		count = hb_list_count( title->list_subtitle );
		used = g_hash_table_lookup(track_indices, lang);
		if (used == NULL)
		{
			used = g_malloc0(count * sizeof(gboolean));
			g_hash_table_insert(track_indices, g_strdup(lang), used);
		}
		// Try to find an item that matches the preferred language and source
		for (ii = 0; ii < count; ii++)
		{
			if (used[ii])
				continue;

       		subtitle = (hb_subtitle_t*)hb_list_item( title->list_subtitle, ii );
			if (source == subtitle->source &&
				((strcmp(lang, subtitle->iso639_2) == 0) ||
				 (strcmp(lang, "und") == 0)))
			{
				used[ii] = TRUE;
				return ii;
			}
		}
		// Try to find an item that matches the preferred language
		for (ii = 0; ii < count; ii++)
		{
			if (used[ii])
				continue;

       		subtitle = (hb_subtitle_t*)hb_list_item( title->list_subtitle, ii );
			if (((!force || (force && canForce(subtitle->source))) &&
				 (!burn  || (burn  &&  canBurn(subtitle->source)))) &&
				((strcmp(lang, subtitle->iso639_2) == 0) ||
				 (strcmp(lang, "und") == 0)))
			{
				used[ii] = TRUE;
				return ii;
			}
		}
	}
	return -2;
}

static void
generic_opts_set(GtkBuilder *builder, const gchar *name, combo_opts_t *opts)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gint ii;
	
	g_debug("generic_opts_set ()\n");
	if (name == NULL || opts == NULL) return;
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	for (ii = 0; ii < opts->count; ii++)
	{
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, opts->map[ii].option, 
						   1, TRUE, 
						   2, opts->map[ii].shortOpt, 
						   3, opts->map[ii].ivalue, 
						   4, opts->map[ii].svalue, 
						   -1);
	}
}

static void
small_opts_set(GtkBuilder *builder, const gchar *name, combo_opts_t *opts)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gint ii;
	gchar *str;
	
	g_debug("small_opts_set ()\n");
	if (name == NULL || opts == NULL) return;
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	for (ii = 0; ii < opts->count; ii++)
	{
		gtk_list_store_append(store, &iter);
		str = g_strdup_printf("<small>%s</small>", opts->map[ii].option);
		gtk_list_store_set(store, &iter, 
						   0, str,
						   1, TRUE, 
						   2, opts->map[ii].shortOpt, 
						   3, opts->map[ii].ivalue, 
						   4, opts->map[ii].svalue, 
						   -1);
		g_free(str);
	}
}

combo_opts_t*
find_combo_table(const gchar *name)
{
	gint ii;

	for (ii = 0; combo_name_map[ii].name != NULL; ii++)
	{
		if (strcmp(name, combo_name_map[ii].name) == 0)
		{
			return combo_name_map[ii].opts;
		}
	}
	return NULL;
}

gint
ghb_lookup_combo_int(const gchar *name, const GValue *gval)
{
	if (gval == NULL)
		return 0;
	if (strcmp(name, "AudioBitrate") == 0)
		return lookup_audio_bitrate_int(gval);
	else if (strcmp(name, "AudioSamplerate") == 0)
		return lookup_audio_rate_int(gval);
	else if (strcmp(name, "VideoFramerate") == 0)
		return lookup_video_rate_int(gval);
	else if (strcmp(name, "AudioMixdown") == 0)
		return lookup_mix_int(gval);
	else if (strcmp(name, "SrtLanguage") == 0)
		return lookup_audio_lang_int(gval);
	else if (strcmp(name, "PreferredLanguage") == 0)
		return lookup_audio_lang_int(gval);
	else
	{
		return lookup_generic_int(find_combo_table(name), gval);
	}
	g_warning("ghb_lookup_combo_int() couldn't find %s", name);
	return 0;
}

gdouble
ghb_lookup_combo_double(const gchar *name, const GValue *gval)
{
	if (gval == NULL)
		return 0;
	if (strcmp(name, "AudioBitrate") == 0)
		return lookup_audio_bitrate_int(gval);
	else if (strcmp(name, "AudioSamplerate") == 0)
		return lookup_audio_rate_int(gval);
	else if (strcmp(name, "VideoFramerate") == 0)
		return lookup_video_rate_int(gval);
	else if (strcmp(name, "AudioMixdown") == 0)
		return lookup_mix_int(gval);
	else if (strcmp(name, "SrtLanguage") == 0)
		return lookup_audio_lang_int(gval);
	else if (strcmp(name, "PreferredLanguage") == 0)
		return lookup_audio_lang_int(gval);
	else
	{
		return lookup_generic_double(find_combo_table(name), gval);
	}
	g_warning("ghb_lookup_combo_double() couldn't find %s", name);
	return 0;
}

const gchar*
ghb_lookup_combo_option(const gchar *name, const GValue *gval)
{
	if (gval == NULL)
		return NULL;
	if (strcmp(name, "AudioBitrate") == 0)
		return lookup_audio_bitrate_option(gval);
	else if (strcmp(name, "AudioSamplerate") == 0)
		return lookup_audio_rate_option(gval);
	else if (strcmp(name, "VideoFramerate") == 0)
		return lookup_video_rate_option(gval);
	else if (strcmp(name, "AudioMixdown") == 0)
		return lookup_mix_option(gval);
	else if (strcmp(name, "SrtLanguage") == 0)
		return lookup_audio_lang_option(gval);
	else if (strcmp(name, "PreferredLanguage") == 0)
		return lookup_audio_lang_option(gval);
	else
	{
		return lookup_generic_option(find_combo_table(name), gval);
	}
	g_warning("ghb_lookup_combo_int() couldn't find %s", name);
	return NULL;
}

const gchar*
ghb_lookup_combo_string(const gchar *name, const GValue *gval)
{
	if (gval == NULL)
		return NULL;
	if (strcmp(name, "AudioBitrate") == 0)
		return lookup_audio_bitrate_option(gval);
	else if (strcmp(name, "AudioSamplerate") == 0)
		return lookup_audio_rate_option(gval);
	else if (strcmp(name, "VideoFramerate") == 0)
		return lookup_video_rate_option(gval);
	else if (strcmp(name, "AudioMixdown") == 0)
		return lookup_mix_string(gval);
	else if (strcmp(name, "SrtLanguage") == 0)
		return lookup_audio_lang_option(gval);
	else if (strcmp(name, "PreferredLanguage") == 0)
		return lookup_audio_lang_option(gval);
	else
	{
		return lookup_generic_string(find_combo_table(name), gval);
	}
	g_warning("ghb_lookup_combo_int() couldn't find %s", name);
	return NULL;
}

void
ghb_update_ui_combo_box(
	signal_user_data_t *ud, 
	const gchar *name, 
	gint user_data, 
	gboolean all)
{
	GtkComboBox *combo = NULL;
	gint signal_id;
	gint handler_id = 0;

	if (name != NULL)
	{		
		g_debug("ghb_update_ui_combo_box() %s\n", name);
		// Clearing a combo box causes a rash of "changed" events, even when
		// the active item is -1 (inactive).  To control things, I'm disabling
		// the event till things are settled down.
		combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
		signal_id = g_signal_lookup("changed", GTK_TYPE_COMBO_BOX);
		if (signal_id > 0)
		{
			// Valid signal id found.  This should always succeed.
			handler_id = g_signal_handler_find ((gpointer)combo, G_SIGNAL_MATCH_ID, 
												signal_id, 0, 0, 0, 0);
			if (handler_id > 0)
			{
				// This should also always succeed
				g_signal_handler_block ((gpointer)combo, handler_id);
			}
		}
	}	
	if (all)
	{
		audio_bitrate_opts_set(ud->builder, "AudioBitrate");
		audio_samplerate_opts_set(ud->builder, "AudioSamplerate", hb_audio_rates, hb_audio_rates_count);
		video_rate_opts_set(ud->builder, "VideoFramerate", hb_video_rates, hb_video_rates_count);
		mix_opts_set(ud->builder, "AudioMixdown");
		language_opts_set(ud->builder, "SrtLanguage");
		language_opts_set(ud->builder, "PreferredLanguage");
		srt_codeset_opts_set(ud->builder, "SrtCodeset");
		title_opts_set(ud->builder, "title");
		audio_track_opts_set(ud->builder, "AudioTrack", user_data);
		subtitle_track_opts_set(ud->builder, "SubtitleTrack", user_data);
		generic_opts_set(ud->builder, "VideoQualityGranularity", &vqual_granularity_opts);
		generic_opts_set(ud->builder, "PtoPType", &point_to_point_opts);
		generic_opts_set(ud->builder, "WhenComplete", &when_complete_opts);
		generic_opts_set(ud->builder, "PicturePAR", &par_opts);
		generic_opts_set(ud->builder, "PictureModulus", &alignment_opts);
		generic_opts_set(ud->builder, "LoggingLevel", &logging_opts);
		generic_opts_set(ud->builder, "LogLongevity", &log_longevity_opts);
		generic_opts_set(ud->builder, "check_updates", &appcast_update_opts);
		generic_opts_set(ud->builder, "FileFormat", &container_opts);
		generic_opts_set(ud->builder, "PictureDeinterlace", &deint_opts);
		generic_opts_set(ud->builder, "PictureDetelecine", &detel_opts);
		generic_opts_set(ud->builder, "PictureDecomb", &decomb_opts);
		generic_opts_set(ud->builder, "PictureDenoise", &denoise_opts);
		generic_opts_set(ud->builder, "VideoEncoder", &vcodec_opts);
		small_opts_set(ud->builder, "AudioEncoder", &acodec_opts);
		small_opts_set(ud->builder, "x264_direct", &direct_opts);
		small_opts_set(ud->builder, "x264_b_adapt", &badapt_opts);
		small_opts_set(ud->builder, "x264_bpyramid", &bpyramid_opts);
		small_opts_set(ud->builder, "x264_weighted_pframes", &weightp_opts);
		small_opts_set(ud->builder, "x264_me", &me_opts);
		small_opts_set(ud->builder, "x264_subme", &subme_opts);
		small_opts_set(ud->builder, "x264_analyse", &analyse_opts);
		small_opts_set(ud->builder, "x264_trellis", &trellis_opts);
	}
	else
	{
		if (strcmp(name, "AudioBitrate") == 0)
			audio_bitrate_opts_set(ud->builder, "AudioBitrate");
		else if (strcmp(name, "AudioSamplerate") == 0)
			audio_samplerate_opts_set(ud->builder, "AudioSamplerate", hb_audio_rates, hb_audio_rates_count);
		else if (strcmp(name, "VideoFramerate") == 0)
			video_rate_opts_set(ud->builder, "VideoFramerate", hb_video_rates, hb_video_rates_count);
		else if (strcmp(name, "AudioMixdown") == 0)
			mix_opts_set(ud->builder, "AudioMixdown");
		else if (strcmp(name, "SrtLanguage") == 0)
			language_opts_set(ud->builder, "SrtLanguage");
		else if (strcmp(name, "PreferredLanguage") == 0)
			language_opts_set(ud->builder, "PreferredLanguage");
		else if (strcmp(name, "SrtCodeset") == 0)
			srt_codeset_opts_set(ud->builder, "SrtCodeset");
		else if (strcmp(name, "title") == 0)
			title_opts_set(ud->builder, "title");
		else if (strcmp(name, "SubtitleTrack") == 0)
			subtitle_track_opts_set(ud->builder, "SubtitleTrack", user_data);
		else if (strcmp(name, "AudioTrack") == 0)
			audio_track_opts_set(ud->builder, "AudioTrack", user_data);
		else
			generic_opts_set(ud->builder, name, find_combo_table(name));
	}
	if (handler_id > 0)
	{
		g_signal_handler_unblock ((gpointer)combo, handler_id);
	}
}
	
static void
init_ui_combo_boxes(GtkBuilder *builder)
{
	gint ii;

	init_combo_box(builder, "AudioBitrate");
	init_combo_box(builder, "AudioSamplerate");
	init_combo_box(builder, "VideoFramerate");
	init_combo_box(builder, "AudioMixdown");
	init_combo_box(builder, "SrtLanguage");
	init_combo_box(builder, "PreferredLanguage");
	init_combo_box(builder, "SrtCodeset");
	init_combo_box(builder, "title");
	init_combo_box(builder, "AudioTrack");
	for (ii = 0; combo_name_map[ii].name != NULL; ii++)
	{
		init_combo_box(builder, combo_name_map[ii].name);
	}
}
	
static const char * turbo_lavc_opts = "";

static const char * turbo_x264_opts = 
	"ref=1:subme=2:me=dia:analyse=none:trellis=0:"
	"no-fast-pskip=0:8x8dct=0";

// Construct the advanced options string
// The result is allocated, so someone must free it at some point.
gchar*
ghb_build_advanced_opts_string(GValue *settings)
{
	gchar *result;

	gint vcodec = ghb_settings_combo_int(settings, "VideoEncoder");

	switch (vcodec)
	{
		case HB_VCODEC_X264:
		{
			gchar *opts = ghb_settings_get_string(settings, "x264Option");
			if (opts != NULL)
			{
				result = opts;
			}
			else
			{
				result = g_strdup("");
			}
		} break;

		case HB_VCODEC_FFMPEG_MPEG2:
		case HB_VCODEC_FFMPEG_MPEG4:
		{
			gchar *opts = ghb_settings_get_string(settings, "lavcOption");
			if (opts != NULL)
			{
				result = opts;
			}
			else
			{
				result = g_strdup("");
			}
		} break;

		case HB_VCODEC_THEORA:
		default:
		{
			result = g_strdup("");
		} break;
	}
	return result;
}

void
ghb_part_duration(gint tt, gint sc, gint ec, gint *hh, gint *mm, gint *ss)
{
	hb_list_t  * list;
	hb_title_t * title;
    hb_chapter_t * chapter;
	gint count, c;
	gint64 duration;
	
	*hh = *mm = *ss = 0;
	if (h_scan == NULL) return;
	list = hb_get_titles( h_scan );
    title = (hb_title_t*)hb_list_item( list, tt );
	if (title == NULL) return;

	*hh = title->hours;
	*mm = title->minutes;
	*ss = title->seconds;

	count = hb_list_count(title->list_chapter);
	if (sc > count) sc = count;
	if (ec > count) ec = count;

	if (sc == 1 && ec == count)
		return;

	duration = 0;
	for (c = sc; c <= ec; c++)
	{
		chapter = hb_list_item(title->list_chapter, c-1);
		duration += chapter->duration;
	}

	*hh =   duration / 90000 / 3600;
	*mm = ((duration / 90000) % 3600) / 60;
	*ss =  (duration / 90000) % 60;
}

void
ghb_get_chapter_duration(gint ti, gint ii, gint *hh, gint *mm, gint *ss)
{
	hb_list_t  * list;
	hb_title_t * title;
    hb_chapter_t * chapter;
	gint count;
	
	g_debug("ghb_get_chapter_duration (title = %d)\n", ti);
	*hh = *mm = *ss = 0;
	if (h_scan == NULL) return;
	list = hb_get_titles( h_scan );
    title = (hb_title_t*)hb_list_item( list, ti );
	if (title == NULL) return;
	count = hb_list_count( title->list_chapter );
	if (ii >= count) return;
	chapter = hb_list_item(title->list_chapter, ii);
	if (chapter == NULL) return;
	*hh = chapter->hours;
	*mm = chapter->minutes;
	*ss = chapter->seconds;
}

GValue*
ghb_get_chapters(gint titleindex)
{
	hb_list_t  * list;
	hb_title_t * title;
    hb_chapter_t * chapter;
	gint count, ii;
	GValue *chapters = NULL;
	
	g_debug("ghb_get_chapters (title = %d)\n", titleindex);
	if (h_scan == NULL) return NULL;
	list = hb_get_titles( h_scan );
    title = (hb_title_t*)hb_list_item( list, titleindex );
	if (title == NULL) return NULL;
	count = hb_list_count( title->list_chapter );
	chapters = ghb_array_value_new(count);
	for (ii = 0; ii < count; ii++)
	{
		chapter = hb_list_item(title->list_chapter, ii);
		if (chapter == NULL) break;
		if (chapter->title == NULL || chapter->title[0] == 0)
		{
			gchar *str;
			str = g_strdup_printf ("Chapter %2d", ii+1);
			ghb_array_append(chapters, ghb_string_value_new(str));
			g_free(str);
		}
		else 
		{
			ghb_array_append(chapters, ghb_string_value_new(chapter->title));
		}
	}
	return chapters;
}

gboolean
ghb_ac3_in_audio_list(const GValue *audio_list)
{
	gint count, ii;

	count = ghb_array_len(audio_list);
	for (ii = 0; ii < count; ii++)
	{
		GValue *asettings;
		gint acodec;

		asettings = ghb_array_get_nth(audio_list, ii);
		acodec = ghb_settings_combo_int(asettings, "AudioEncoder");
		if (acodec & HB_ACODEC_AC3)
			return TRUE;
	}
	return FALSE;
}

static void
audio_bitrate_opts_add(GtkBuilder *builder, const gchar *name, gint rate)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gchar *str;
	
	g_debug("audio_bitrate_opts_add ()\n");

	if (rate < 8) return;

	if (ghb_audio_bitrates[hb_audio_bitrates_count].string)
	{
		g_free(ghb_audio_bitrates[hb_audio_bitrates_count].string);
	}
	ghb_audio_bitrates[hb_audio_bitrates_count].rate = rate;
	ghb_audio_bitrates[hb_audio_bitrates_count].string = 
		g_strdup_printf("%d", rate);
	ghb_audio_bitrates_count = hb_audio_bitrates_count + 1;

	store = get_combo_box_store(builder, name);
	if (!find_combo_item_by_int(GTK_TREE_MODEL(store), rate, &iter))
	{
		str = g_strdup_printf ("<small>%d</small>", rate);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, str, 
						   1, TRUE, 
						   2, ghb_audio_bitrates[hb_audio_bitrates_count].string, 
						   3, (gdouble)rate, 
						   4, ghb_audio_bitrates[hb_audio_bitrates_count].string, 
						   -1);
		g_free(str);
	}
}

static void
audio_bitrate_opts_clean(
	GtkBuilder *builder, 
	const gchar *name, 
	gint first_rate, 
	gint last_rate)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gdouble ivalue;
	gboolean done = FALSE;
	gint ii = 0;
	guint last = (guint)last_rate;
	guint first = (guint)first_rate;
	
	ghb_audio_bitrates_count = hb_audio_bitrates_count;

	g_debug("audio_bitrate_opts_clean ()\n");
	store = get_combo_box_store(builder, name);
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL(store), &iter))
	{
		do
		{
			gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 3, &ivalue, -1);
			if (search_rates(
				ghb_audio_bitrates, ivalue, ghb_audio_bitrates_count) < 0)
			{
				done = !gtk_list_store_remove(store, &iter);
			}
			else if (ivalue < first || ivalue > last)
			{
				ii++;
				gtk_list_store_set(store, &iter, 1, FALSE, -1);
				done = !gtk_tree_model_iter_next (GTK_TREE_MODEL(store), &iter);
			}
			else
			{
				ii++;
				gtk_list_store_set(store, &iter, 1, TRUE, -1);
				done = !gtk_tree_model_iter_next (GTK_TREE_MODEL(store), &iter);
			}
		} while (!done);
	}
}

static void
audio_bitrate_opts_set(GtkBuilder *builder, const gchar *name)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gint ii;
	gchar *str;
	
	ghb_audio_bitrates_count = hb_audio_bitrates_count;
	ghb_audio_bitrates = calloc(hb_audio_bitrates_count+1, sizeof(hb_rate_t));

	for (ii = 0; ii < hb_audio_bitrates_count; ii++)
	{
		ghb_audio_bitrates[ii] = hb_audio_bitrates[ii];
	}

	g_debug("audio_bitrate_opts_set ()\n");
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	for (ii = 0; ii < ghb_audio_bitrates_count; ii++)
	{
		gtk_list_store_append(store, &iter);
		str = g_strdup_printf ("<small>%s</small>", 
			ghb_audio_bitrates[ii].string);
		gtk_list_store_set(store, &iter, 
						   0, str,
						   1, TRUE, 
						   2, ghb_audio_bitrates[ii].string, 
						   3, (gdouble)ghb_audio_bitrates[ii].rate, 
						   4, ghb_audio_bitrates[ii].string, 
						   -1);
		g_free(str);
	}
}

void
ghb_set_passthru_bitrate_opts(GtkBuilder *builder, gint bitrate)
{
	audio_bitrate_opts_add(builder, "AudioBitrate", bitrate);
}

void
ghb_set_default_bitrate_opts(
	GtkBuilder *builder, 
	gint first_rate, 
	gint last_rate)
{
	audio_bitrate_opts_clean(builder, "AudioBitrate", first_rate, last_rate);
}

static ghb_status_t hb_status;

void
ghb_combo_init(signal_user_data_t *ud)
{
	// Set up the list model for the combos
	init_ui_combo_boxes(ud->builder);
	// Populate all the combos
	ghb_update_ui_combo_box(ud, NULL, 0, TRUE);
}

void
ghb_backend_init(gint debug)
{
    /* Init libhb */
    h_scan = hb_init( debug, 0 );
    h_queue = hb_init( debug, 0 );
}

void
ghb_backend_close()
{
	hb_close(&h_queue);
	hb_close(&h_scan);
	hb_global_close();
}

void ghb_backend_scan_stop()
{
    hb_scan_stop( h_scan );
}

void
ghb_backend_scan(const gchar *path, gint titleindex, gint preview_count, uint64_t min_duration)
{
    hb_scan( h_scan, path, titleindex, preview_count, 1, min_duration );
	hb_status.scan.state |= GHB_STATE_SCANNING;
	// initialize count and cur to something that won't cause FPE
	// when computing progress
	hb_status.scan.title_count = 1;
	hb_status.scan.title_cur = 0;
}

void
ghb_backend_queue_scan(const gchar *path, gint titlenum)
{
	g_debug("ghb_backend_queue_scan()");
	hb_scan( h_queue, path, titlenum, 10, 0, 0 );
	hb_status.queue.state |= GHB_STATE_SCANNING;
}

gint
ghb_get_scan_state()
{
	return hb_status.scan.state;
}

gint
ghb_get_queue_state()
{
	return hb_status.queue.state;
}

void
ghb_clear_scan_state(gint state)
{
	hb_status.scan.state &= ~state;
}

void
ghb_clear_queue_state(gint state)
{
	hb_status.queue.state &= ~state;
}

void
ghb_set_scan_state(gint state)
{
	hb_status.scan.state |= state;
}

void
ghb_set_queue_state(gint state)
{
	hb_status.queue.state |= state;
}

void
ghb_get_status(ghb_status_t *status)
{
	memcpy(status, &hb_status, sizeof(ghb_status_t));
}

void 
ghb_track_status()
{
    hb_state_t s_scan;
    hb_state_t s_queue;

	if (h_scan == NULL) return;
    hb_get_state( h_scan, &s_scan );
	switch( s_scan.state )
    {
#define p s_scan.param.scanning
        case HB_STATE_SCANNING:
		{
			hb_status.scan.state |= GHB_STATE_SCANNING;
			hb_status.scan.title_count = p.title_count;
			hb_status.scan.title_cur = p.title_cur;
		} break;
#undef p

        case HB_STATE_SCANDONE:
        {
			hb_status.scan.state &= ~GHB_STATE_SCANNING;
			hb_status.scan.state |= GHB_STATE_SCANDONE;
        } break;

#define p s_scan.param.working
        case HB_STATE_WORKING:
			hb_status.scan.state |= GHB_STATE_WORKING;
			hb_status.scan.state &= ~GHB_STATE_PAUSED;
			hb_status.scan.job_cur = p.job_cur;
			hb_status.scan.job_count = p.job_count;
			hb_status.scan.progress = p.progress;
			hb_status.scan.rate_cur = p.rate_cur;
			hb_status.scan.rate_avg = p.rate_avg;
			hb_status.scan.hours = p.hours;
			hb_status.scan.minutes = p.minutes;
			hb_status.scan.seconds = p.seconds;
			hb_status.scan.unique_id = p.sequence_id & 0xFFFFFF;
            break;
#undef p

        case HB_STATE_PAUSED:
			hb_status.scan.state |= GHB_STATE_PAUSED;
            break;
				
        case HB_STATE_MUXING:
        {
			hb_status.scan.state |= GHB_STATE_MUXING;
        } break;

#define p s_scan.param.workdone
        case HB_STATE_WORKDONE:
		{
            hb_job_t *job;

			hb_status.scan.state |= GHB_STATE_WORKDONE;
			hb_status.scan.state &= ~GHB_STATE_MUXING;
			hb_status.scan.state &= ~GHB_STATE_PAUSED;
			hb_status.scan.state &= ~GHB_STATE_WORKING;
			switch (p.error)
			{
			case HB_ERROR_NONE:
				hb_status.scan.error = GHB_ERROR_NONE;
				break;
			case HB_ERROR_CANCELED:
				hb_status.scan.error = GHB_ERROR_CANCELED;
				break;
			default:
				hb_status.scan.error = GHB_ERROR_FAIL;
				break;
			}
			// Delete all remaining jobs of this encode.
			// An encode can be composed of multiple associated jobs.
			// When a job is stopped, libhb removes it from the job list,
			// but does not remove other jobs that may be associated with it.
			// Associated jobs are taged in the sequence id.
            while ((job = hb_job(h_scan, 0)) != NULL) 
                hb_rem( h_scan, job );
		} break;
#undef p
    }
    hb_get_state( h_queue, &s_queue );
	switch( s_queue.state )
    {
#define p s_queue.param.scanning
        case HB_STATE_SCANNING:
		{
			hb_status.queue.state |= GHB_STATE_SCANNING;
			hb_status.queue.title_count = p.title_count;
			hb_status.queue.title_cur = p.title_cur;
		} break;
#undef p

        case HB_STATE_SCANDONE:
        {
			hb_status.queue.state &= ~GHB_STATE_SCANNING;
			hb_status.queue.state |= GHB_STATE_SCANDONE;
        } break;

#define p s_queue.param.working
        case HB_STATE_WORKING:
			hb_status.queue.state |= GHB_STATE_WORKING;
			hb_status.queue.state &= ~GHB_STATE_PAUSED;
			hb_status.queue.state &= ~GHB_STATE_SEARCHING;
			hb_status.queue.job_cur = p.job_cur;
			hb_status.queue.job_count = p.job_count;
			hb_status.queue.progress = p.progress;
			hb_status.queue.rate_cur = p.rate_cur;
			hb_status.queue.rate_avg = p.rate_avg;
			hb_status.queue.hours = p.hours;
			hb_status.queue.minutes = p.minutes;
			hb_status.queue.seconds = p.seconds;
			hb_status.queue.unique_id = p.sequence_id & 0xFFFFFF;
            break;

        case HB_STATE_SEARCHING:
			hb_status.queue.state |= GHB_STATE_SEARCHING;
			hb_status.queue.state &= ~GHB_STATE_WORKING;
			hb_status.queue.state &= ~GHB_STATE_PAUSED;
			hb_status.queue.job_cur = p.job_cur;
			hb_status.queue.job_count = p.job_count;
			hb_status.queue.progress = p.progress;
			hb_status.queue.rate_cur = p.rate_cur;
			hb_status.queue.rate_avg = p.rate_avg;
			hb_status.queue.hours = p.hours;
			hb_status.queue.minutes = p.minutes;
			hb_status.queue.seconds = p.seconds;
			hb_status.queue.unique_id = p.sequence_id & 0xFFFFFF;
            break;
#undef p

        case HB_STATE_PAUSED:
			hb_status.queue.state |= GHB_STATE_PAUSED;
            break;
				
        case HB_STATE_MUXING:
        {
			hb_status.queue.state |= GHB_STATE_MUXING;
        } break;

#define p s_queue.param.workdone
        case HB_STATE_WORKDONE:
		{
            hb_job_t *job;

			hb_status.queue.state |= GHB_STATE_WORKDONE;
			hb_status.queue.state &= ~GHB_STATE_MUXING;
			hb_status.queue.state &= ~GHB_STATE_PAUSED;
			hb_status.queue.state &= ~GHB_STATE_WORKING;
			hb_status.queue.state &= ~GHB_STATE_SEARCHING;
			switch (p.error)
			{
			case HB_ERROR_NONE:
				hb_status.queue.error = GHB_ERROR_NONE;
				break;
			case HB_ERROR_CANCELED:
				hb_status.queue.error = GHB_ERROR_CANCELED;
				break;
			default:
				hb_status.queue.error = GHB_ERROR_FAIL;
				break;
			}
			// Delete all remaining jobs of this encode.
			// An encode can be composed of multiple associated jobs.
			// When a job is stopped, libhb removes it from the job list,
			// but does not remove other jobs that may be associated with it.
			// Associated jobs are taged in the sequence id.
            while ((job = hb_job(h_queue, 0)) != NULL) 
                hb_rem( h_queue, job );
		} break;
#undef p
    }
}

gboolean
ghb_get_title_info(ghb_title_info_t *tinfo, gint titleindex)
{
	hb_list_t  * list;
	hb_title_t * title;
	
    if (h_scan == NULL) return FALSE;
	list = hb_get_titles( h_scan );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		return FALSE;
	}

	title = hb_list_item( list, titleindex );
	if (title == NULL) return FALSE;	// Bad titleindex
	tinfo->index = titleindex;
	tinfo->width = title->width;
	tinfo->height = title->height;
	memcpy(tinfo->crop, title->crop, 4 * sizeof(int));
	// Don't allow crop to 0
	if (title->crop[0] + title->crop[1] >= title->height)
		title->crop[0] = title->crop[1] = 0;
	if (title->crop[2] + title->crop[3] >= title->width)
		title->crop[2] = title->crop[3] = 0;
	tinfo->num_chapters = hb_list_count(title->list_chapter);
	tinfo->rate_base = title->rate_base;
	tinfo->rate = title->rate;
	tinfo->interlaced = title->detected_interlacing;
	hb_reduce(&(tinfo->aspect_n), &(tinfo->aspect_d), 
				title->width * title->pixel_aspect_width, 
				title->height * title->pixel_aspect_height);
	tinfo->hours = title->hours;
	tinfo->minutes = title->minutes;
	tinfo->seconds = title->seconds;
	tinfo->duration = title->duration;

	tinfo->angle_count = title->angle_count;
	tinfo->path = title->path;
	tinfo->name = title->name;
	tinfo->type = title->type;
	return TRUE;
}

hb_audio_config_t*
ghb_get_scan_audio_info(gint titleindex, gint audioindex)
{
    hb_audio_config_t *aconfig;
	
	aconfig = get_hb_audio(h_scan, titleindex, audioindex);
	return aconfig;
}

gboolean
ghb_audio_is_passthru(gint acodec)
{
	g_debug("ghb_audio_is_passthru () \n");
	return (acodec & HB_ACODEC_PASS_FLAG) != 0;
}

gboolean
ghb_audio_can_passthru(gint acodec)
{
	g_debug("ghb_audio_can_passthru () \n");
	return (acodec & HB_ACODEC_PASS_MASK) != 0;
}

gint
ghb_get_default_acodec()
{
	return HB_ACODEC_FAAC;
}

static void
picture_settings_deps(signal_user_data_t *ud)
{
	gboolean autoscale, keep_aspect, enable_keep_aspect;
	gboolean enable_scale_width, enable_scale_height;
	gboolean enable_disp_width, enable_disp_height, enable_par;
	gint pic_par;
	GtkWidget *widget;

	pic_par = ghb_settings_combo_int(ud->settings, "PicturePAR");
	if (pic_par == 1)
	{
		ghb_ui_update(ud, "autoscale", ghb_boolean_value(TRUE));
		ghb_ui_update(ud, "PictureModulus", ghb_int_value(2));
		ghb_ui_update(ud, "PictureLooseCrop", ghb_boolean_value(TRUE));
	}
	enable_keep_aspect = (pic_par != 1 && pic_par != 2);
	if (!enable_keep_aspect)
	{
		ghb_ui_update(ud, "PictureKeepRatio", ghb_boolean_value(TRUE));
	}
	keep_aspect = ghb_settings_get_boolean(ud->settings, "PictureKeepRatio");
	autoscale = ghb_settings_get_boolean(ud->settings, "autoscale");

	enable_scale_width = !autoscale && (pic_par != 1);
	enable_scale_height = !autoscale && (pic_par != 1);
	enable_disp_width = (pic_par == 3) && !keep_aspect;
	enable_par = (pic_par == 3) && !keep_aspect;
	enable_disp_height = FALSE;

	widget = GHB_WIDGET(ud->builder, "PictureModulus");
	gtk_widget_set_sensitive(widget, pic_par != 1);
	widget = GHB_WIDGET(ud->builder, "PictureLooseCrop");
	gtk_widget_set_sensitive(widget, pic_par != 1);
	widget = GHB_WIDGET(ud->builder, "scale_width");
	gtk_widget_set_sensitive(widget, enable_scale_width);
	widget = GHB_WIDGET(ud->builder, "scale_height");
	gtk_widget_set_sensitive(widget, enable_scale_height);
	widget = GHB_WIDGET(ud->builder, "PictureDisplayWidth");
	gtk_widget_set_sensitive(widget, enable_disp_width);
	widget = GHB_WIDGET(ud->builder, "PictureDisplayHeight");
	gtk_widget_set_sensitive(widget, enable_disp_height);
	widget = GHB_WIDGET(ud->builder, "PicturePARWidth");
	gtk_widget_set_sensitive(widget, enable_par);
	widget = GHB_WIDGET(ud->builder, "PicturePARHeight");
	gtk_widget_set_sensitive(widget, enable_par);
	widget = GHB_WIDGET(ud->builder, "PictureKeepRatio");
	gtk_widget_set_sensitive(widget, enable_keep_aspect);
	widget = GHB_WIDGET(ud->builder, "autoscale");
	gtk_widget_set_sensitive(widget, pic_par != 1);
}

void
ghb_limit_rational( gint *num, gint *den, gint limit )
{
    if (*num < limit && *den < limit)
        return;

    if (*num > *den)
    {
        gdouble factor = (double)limit / *num;
        *num = limit;
        *den = factor * *den;
    }
    else
    {
        gdouble factor = (double)limit / *den;
        *den = limit;
        *num = factor * *num;
    }
}

void
ghb_set_scale(signal_user_data_t *ud, gint mode)
{
	hb_list_t  * list;
	hb_title_t * title;
	hb_job_t   * job;
	gboolean keep_aspect;
	gint pic_par;
	gboolean autocrop, autoscale, noscale;
	gint crop[4], width, height, par_width, par_height;
	gint crop_width, crop_height;
	gint aspect_n, aspect_d;
	gboolean keep_width = (mode & GHB_PIC_KEEP_WIDTH);
	gboolean keep_height = (mode & GHB_PIC_KEEP_HEIGHT);
	gint step;
	GtkWidget *widget;
	gint mod;
	gint max_width = 0;
	gint max_height = 0;
	
	g_debug("ghb_set_scale ()\n");
	picture_settings_deps(ud);
	if (h_scan == NULL) return;
	list = hb_get_titles( h_scan );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		return;
	}
	gint titleindex;

	titleindex = ghb_settings_combo_int(ud->settings, "title");
	title = hb_list_item( list, titleindex );
	if (title == NULL) return;
	job   = title->job;
	if (job == NULL) return;

	if (ud->scale_busy) return;
	ud->scale_busy = TRUE;

	// First configure widgets
	mod = ghb_settings_combo_int(ud->settings, "PictureModulus");
	pic_par = ghb_settings_combo_int(ud->settings, "PicturePAR");
	keep_aspect = ghb_settings_get_boolean(ud->settings, "PictureKeepRatio");
	autocrop = ghb_settings_get_boolean(ud->settings, "PictureAutoCrop");
	autoscale = ghb_settings_get_boolean(ud->settings, "autoscale");
	// "Noscale" is a flag that says we prefer to crop extra to satisfy
	// alignment constraints rather than scaling to satisfy them.
	noscale = ghb_settings_get_boolean(ud->settings, "PictureLooseCrop");
	// Align dimensions to either 16 or 2 pixels
	// The scaler crashes if the dimensions are not divisible by 2
	// x264 also will not accept dims that are not multiple of 2
	if (autoscale)
	{
		keep_width = FALSE;
		keep_height = FALSE;
	}
	// Step needs to be at least 2 because odd widths cause scaler crash
	step = mod;
	widget = GHB_WIDGET (ud->builder, "scale_width");
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), step, 16);
	widget = GHB_WIDGET (ud->builder, "scale_height");
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), step, 16);
	if (noscale)
	{
		widget = GHB_WIDGET (ud->builder, "PictureTopCrop");
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), step, 16);
		widget = GHB_WIDGET (ud->builder, "PictureBottomCrop");
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), step, 16);
		widget = GHB_WIDGET (ud->builder, "PictureLeftCrop");
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), step, 16);
		widget = GHB_WIDGET (ud->builder, "PictureRightCrop");
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), step, 16);
	}
	else
	{
		widget = GHB_WIDGET (ud->builder, "PictureTopCrop");
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), 1, 16);
		widget = GHB_WIDGET (ud->builder, "PictureBottomCrop");
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), 1, 16);
		widget = GHB_WIDGET (ud->builder, "PictureLeftCrop");
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), 1, 16);
		widget = GHB_WIDGET (ud->builder, "PictureRightCrop");
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), 1, 16);
	}
	ghb_title_info_t tinfo;
	ghb_get_title_info (&tinfo, titleindex);
	if (autocrop)
	{
		crop[0] = tinfo.crop[0];
		crop[1] = tinfo.crop[1];
		crop[2] = tinfo.crop[2];
		crop[3] = tinfo.crop[3];
		ghb_ui_update(ud, "PictureTopCrop", ghb_int64_value(crop[0]));
		ghb_ui_update(ud, "PictureBottomCrop", ghb_int64_value(crop[1]));
		ghb_ui_update(ud, "PictureLeftCrop", ghb_int64_value(crop[2]));
		ghb_ui_update(ud, "PictureRightCrop", ghb_int64_value(crop[3]));
	}
	else
	{
		crop[0] = ghb_settings_get_int(ud->settings, "PictureTopCrop");
		crop[1] = ghb_settings_get_int(ud->settings, "PictureBottomCrop");
		crop[2] = ghb_settings_get_int(ud->settings, "PictureLeftCrop");
		crop[3] = ghb_settings_get_int(ud->settings, "PictureRightCrop");
	}
	if (noscale)
	{
		gint need1, need2;

		// Adjust the cropping to accomplish the desired width and height
		crop_width = tinfo.width - crop[2] - crop[3];
		crop_height = tinfo.height - crop[0] - crop[1];
		width = MOD_DOWN(crop_width, mod);
		height = MOD_DOWN(crop_height, mod);

		need1 = (crop_height - height) / 2;
		need2 = crop_height - height - need1;
		crop[0] += need1;
		crop[1] += need2;
		need1 = (crop_width - width) / 2;
		need2 = crop_width - width - need1;
		crop[2] += need1;
		crop[3] += need2;
		ghb_ui_update(ud, "PictureTopCrop", ghb_int64_value(crop[0]));
		ghb_ui_update(ud, "PictureBottomCrop", ghb_int64_value(crop[1]));
		ghb_ui_update(ud, "PictureLeftCrop", ghb_int64_value(crop[2]));
		ghb_ui_update(ud, "PictureRightCrop", ghb_int64_value(crop[3]));
	}
	hb_reduce(&aspect_n, &aspect_d, 
				title->width * title->pixel_aspect_width, 
				title->height * title->pixel_aspect_height);
	crop_width = title->width - crop[2] - crop[3];
	crop_height = title->height - crop[0] - crop[1];
	if (autoscale)
	{
		width = crop_width;
		height = crop_height;
	}
	else
	{
		width = ghb_settings_get_int(ud->settings, "scale_width");
		height = ghb_settings_get_int(ud->settings, "scale_height");
		if (mode & GHB_PIC_USE_MAX)
		{
			max_width = MOD_DOWN(
				ghb_settings_get_int(ud->settings, "PictureWidth"), mod);
			max_height = MOD_DOWN(
				ghb_settings_get_int(ud->settings, "PictureHeight"), mod);
		}
	}
	g_debug("max_width %d, max_height %d\n", max_width, max_height);

	if (width < 16)
		width = title->width - crop[2] - crop[3];
	if (height < 16)
		height = title->height - crop[0] - crop[1];

	width = MOD_ROUND(width, mod);
	height = MOD_ROUND(height, mod);

	job->anamorphic.mode = pic_par;
	if (pic_par)
	{
		// The scaler crashes if the dimensions are not divisible by 2
		// Align mod 2.  And so does something in x264_encoder_headers()
		job->modulus = mod;
		job->anamorphic.par_width = title->pixel_aspect_width;
		job->anamorphic.par_height = title->pixel_aspect_height;
		job->anamorphic.dar_width = 0;
		job->anamorphic.dar_height = 0;

		if (keep_height && pic_par == 2)
			width = ((double)height * crop_width / crop_height);
		job->width = width;
		job->height = height;
		job->maxWidth = max_width;
		job->maxHeight = max_height;
		job->crop[0] = crop[0];	job->crop[1] = crop[1];
		job->crop[2] = crop[2];	job->crop[3] = crop[3];
		if (job->anamorphic.mode == 3 && !keep_aspect)
		{
			job->anamorphic.keep_display_aspect = 0;
			if (mode & GHB_PIC_KEEP_PAR)
			{
				job->anamorphic.par_width = 
					ghb_settings_get_int(ud->settings, "PicturePARWidth");
				job->anamorphic.par_height = 
					ghb_settings_get_int(ud->settings, "PicturePARHeight");
			}
			else
			{
				job->anamorphic.dar_width = 
					ghb_settings_get_int(ud->settings, 
										"PictureDisplayWidth");
				job->anamorphic.dar_height = height;
			}
		}
		else
		{
			job->anamorphic.keep_display_aspect = 1;
		}
		// hb_set_anamorphic_size will adjust par, dar, and width/height
		// to conform to job parameters that have been set, including 
		// maxWidth and maxHeight
		hb_set_anamorphic_size( job, &width, &height, 
								&par_width, &par_height );
		if (job->anamorphic.mode == 3 && !keep_aspect && 
			mode & GHB_PIC_KEEP_PAR)
		{
			// hb_set_anamorphic_size reduces the par, which we
			// don't want in this case because the user is
			// explicitely specifying it.
			par_width = ghb_settings_get_int(ud->settings, 
											"PicturePARWidth");
			par_height = ghb_settings_get_int(ud->settings, 
												"PicturePARHeight");
		}
	}
	else 
	{
		// Adjust dims according to max values
		if (max_height)
			height = MIN(height, max_height);
		if (max_width)
			width = MIN(width, max_width);

		if (keep_aspect)
		{
			gdouble par;
			gint new_width, new_height;
			
			// Compute pixel aspect ration.  
			par = (gdouble)(title->height * aspect_n) / (title->width * aspect_d);
			// Must scale so that par becomes 1:1
			// Try to keep largest dimension
			new_height = (crop_height * ((gdouble)width/crop_width) / par);
			new_width = (crop_width * ((gdouble)height/crop_height) * par);

			if (max_width && new_width > max_width)
			{
				height = new_height;
			}
			else if (max_height && new_height > max_height)
			{
				width = new_width;
			}
			else if (keep_width)
			{
				height = new_height;
			}
			else if (keep_height)
			{
				width = new_width;
			}
			else if (width > new_width)
			{
				height = new_height;
			}
			else
			{
				width = new_width;
			}
			g_debug("new w %d h %d\n", width, height);
		}
		width = MOD_ROUND(width, mod);
		height = MOD_ROUND(height, mod);
		if (max_height)
			height = MIN(height, max_height);
		if (max_width)
			width = MIN(width, max_width);
		par_width = par_height = 1;
	}
	ghb_ui_update(ud, "scale_width", ghb_int64_value(width));
	ghb_ui_update(ud, "scale_height", ghb_int64_value(height));

	gint disp_width, dar_width, dar_height;
	gchar *str;

	disp_width = ((gdouble)par_width / par_height) * width + 0.5;
	hb_reduce(&dar_width, &dar_height, disp_width, height);
    ghb_limit_rational(&par_width, &par_height, 65535);
	gint iaspect = dar_width * 9 / dar_height;
	if (dar_width > 2 * dar_height)
	{
		str = g_strdup_printf("%.2f : 1", (gdouble)dar_width / dar_height);
	}
	else if (iaspect <= 16 && iaspect >= 15)
	{
		str = g_strdup_printf("%.2f : 9", (gdouble)dar_width * 9 / dar_height);
	}
	else if (iaspect <= 12 && iaspect >= 11)
	{
		str = g_strdup_printf("%.2f : 3", (gdouble)dar_width * 3 / dar_height);
	}
	else
	{
		str = g_strdup_printf("%d : %d", dar_width, dar_height);
	}
	ghb_ui_update(ud, "display_aspect", ghb_string_value(str));
	g_free(str);
	ghb_ui_update(ud, "PicturePARWidth", ghb_int64_value(par_width));
	ghb_ui_update(ud, "PicturePARHeight", ghb_int64_value(par_height));
	ghb_ui_update(ud, "PictureDisplayWidth", ghb_int64_value(disp_width));
	ghb_ui_update(ud, "PictureDisplayHeight", ghb_int64_value(height));
	ud->scale_busy = FALSE;
}

static void
set_preview_job_settings(hb_job_t *job, GValue *settings)
{
	job->crop[0] = ghb_settings_get_int(settings, "PictureTopCrop");
	job->crop[1] = ghb_settings_get_int(settings, "PictureBottomCrop");
	job->crop[2] = ghb_settings_get_int(settings, "PictureLeftCrop");
	job->crop[3] = ghb_settings_get_int(settings, "PictureRightCrop");

	job->anamorphic.mode = ghb_settings_combo_int(settings, "PicturePAR");
	job->modulus = 
		ghb_settings_combo_int(settings, "PictureModulus");
	job->width = ghb_settings_get_int(settings, "scale_width");
	job->height = ghb_settings_get_int(settings, "scale_height");
	if (ghb_settings_get_boolean(settings, "show_crop"))
	{
		gdouble xscale = (gdouble)job->width / 
			(gdouble)(job->title->width - job->crop[2] - job->crop[3]);
		gdouble yscale = (gdouble)job->height / 
			(gdouble)(job->title->height - job->crop[0] - job->crop[1]);
	
		job->width += xscale * (job->crop[2] + job->crop[3]);
		job->height += yscale * (job->crop[0] + job->crop[1]);
		job->crop[0] = 0;
		job->crop[1] = 0;
		job->crop[2] = 0;
		job->crop[3] = 0;
		job->modulus = 2;
	}

	gboolean decomb_deint = ghb_settings_get_boolean(settings, "PictureDecombDeinterlace");
	if (decomb_deint)
	{
		gint decomb = ghb_settings_combo_int(settings, "PictureDecomb");
		job->deinterlace = (decomb == 0) ? 0 : 1;
	}
	else
	{
		gint deint = ghb_settings_combo_int(settings, "PictureDeinterlace");
		job->deinterlace = (deint == 0) ? 0 : 1;
	}

	gboolean keep_aspect;
	keep_aspect = ghb_settings_get_boolean(settings, "PictureKeepRatio");
	if (job->anamorphic.mode)
	{
		job->anamorphic.par_width = job->title->pixel_aspect_width;
		job->anamorphic.par_height = job->title->pixel_aspect_height;
		job->anamorphic.dar_width = 0;
		job->anamorphic.dar_height = 0;

		if (job->anamorphic.mode == 3 && !keep_aspect)
		{
			job->anamorphic.keep_display_aspect = 0;
			job->anamorphic.par_width = 
				ghb_settings_get_int(settings, "PicturePARWidth");
			job->anamorphic.par_height = 
				ghb_settings_get_int(settings, "PicturePARHeight");
		}
		else
		{
			job->anamorphic.keep_display_aspect = 1;
		}
	}
}

gboolean
ghb_validate_filter_string(const gchar *str, gint max_fields)
{
	gint fields = 0;
	gchar *end;
	gdouble val;

	if (str == NULL || *str == 0) return TRUE;
	while (*str)
	{
		val = g_strtod(str, &end);
		if (str != end)
		{ // Found a numeric value
			fields++;
			// negative max_fields means infinate
			if (max_fields >= 0 && fields > max_fields) return FALSE;
			if (*end == 0)
				return TRUE;
			if (*end != ':')
				return FALSE;
			str = end + 1;
		}
		else
			return FALSE;
	}
	return FALSE;
}

gboolean
ghb_validate_filters(signal_user_data_t *ud)
{
	gchar *str;
	gint index;
	gchar *message;

	gboolean decomb_deint = ghb_settings_get_boolean(ud->settings, "PictureDecombDeinterlace");
	// deinte
	index = ghb_settings_combo_int(ud->settings, "PictureDeinterlace");
	if (!decomb_deint && index == 1)
	{
		str = ghb_settings_get_string(ud->settings, "PictureDeinterlaceCustom");
		if (!ghb_validate_filter_string(str, -1))
		{
			message = g_strdup_printf(
						"Invalid Deinterlace Settings:\n\n%s\n",
						str);
			ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
			g_free(message);
			g_free(str);
			return FALSE;
		}
		g_free(str);
	}
	// detel
	index = ghb_settings_combo_int(ud->settings, "PictureDetelecine");
	if (index == 1)
	{
		str = ghb_settings_get_string(ud->settings, "PictureDetelecineCustom");
		if (!ghb_validate_filter_string(str, -1))
		{
			message = g_strdup_printf(
						"Invalid Detelecine Settings:\n\n%s\n",
						str);
			ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
			g_free(message);
			g_free(str);
			return FALSE;
		}
		g_free(str);
	}
	// decomb
	index = ghb_settings_combo_int(ud->settings, "PictureDecomb");
	if (decomb_deint && index == 1)
	{
		str = ghb_settings_get_string(ud->settings, "PictureDecombCustom");
		if (!ghb_validate_filter_string(str, -1))
		{
			message = g_strdup_printf(
						"Invalid Decomb Settings:\n\n%s\n",
						str);
			ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
			g_free(message);
			g_free(str);
			return FALSE;
		}
		g_free(str);
	}
	// denois
	index = ghb_settings_combo_int(ud->settings, "PictureDenoise");
	if (index == 1)
	{
		str = ghb_settings_get_string(ud->settings, "PictureDenoiseCustom");
		if (!ghb_validate_filter_string(str, -1))
		{
			message = g_strdup_printf(
						"Invalid Denoise Settings:\n\n%s\n",
						str);
			ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
			g_free(str);
			g_free(message);
			return FALSE;
		}
		g_free(str);
	}
	return TRUE;
}

gboolean
ghb_validate_video(signal_user_data_t *ud)
{
	gint vcodec, mux;
	gchar *message;

	mux = ghb_settings_combo_int(ud->settings, "FileFormat");
	vcodec = ghb_settings_combo_int(ud->settings, "VideoEncoder");
	if ((mux == HB_MUX_MP4) && (vcodec == HB_VCODEC_THEORA))
	{
		// mp4/theora combination is not supported.
		message = g_strdup_printf(
					"Theora is not supported in the MP4 container.\n\n"
					"You should choose a different video codec or container.\n"
					"If you continue, FFMPEG will be chosen for you.");
		if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, "Cancel", "Continue"))
		{
			g_free(message);
			return FALSE;
		}
		g_free(message);
		vcodec = HB_VCODEC_FFMPEG_MPEG4;
		ghb_ui_update(ud, "VideoEncoder", ghb_int64_value(vcodec));
	}
	return TRUE;
}

gboolean
ghb_validate_subtitles(signal_user_data_t *ud)
{
	hb_list_t  * list;
	hb_title_t * title;
	gchar *message;

	if (h_scan == NULL) return FALSE;
	list = hb_get_titles( h_scan );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		g_message("No title found.\n");
		return FALSE;
	}

	gint titleindex;

	titleindex = ghb_settings_combo_int(ud->settings, "title");
    title = hb_list_item( list, titleindex );
	if (title == NULL) return FALSE;

	const GValue *slist, *settings;
	gint count, ii, source;
	gboolean burned, one_burned = FALSE;

	slist = ghb_settings_get_value(ud->settings, "subtitle_list");
	count = ghb_array_len(slist);
	for (ii = 0; ii < count; ii++)
	{
		settings = ghb_array_get_nth(slist, ii);
		source = ghb_settings_get_int(settings, "SubtitleSource");
		burned = ghb_settings_get_boolean(settings, "SubtitleBurned");
		if (burned && one_burned)
		{
			// MP4 can only handle burned vobsubs.  make sure there isn't
			// already something burned in the list
			message = g_strdup_printf(
			"Only one subtitle may be burned into the video.\n\n"
				"You should change your subtitle selections.\n"
				"If you continue, some subtitles will be lost.");
			if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, "Cancel", "Continue"))
			{
				g_free(message);
				return FALSE;
			}
			g_free(message);
			break;
		}
		else if (burned)
		{
			one_burned = TRUE;
		}
		if (source == SRTSUB)
		{
			gchar *filename;

			filename = ghb_settings_get_string(settings, "SrtFile");
			if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR))
			{
				message = g_strdup_printf(
				"Srt file does not exist or not a regular file.\n\n"
					"You should choose a valid file.\n"
					"If you continue, this subtitle will be ignored.");
				if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, 
					"Cancel", "Continue"))
				{
					g_free(message);
					return FALSE;
				}
				g_free(message);
				break;
			}
		}
	}
	return TRUE;
}

gint
ghb_select_audio_codec(GValue *settings, hb_audio_config_t *aconfig, gint acodec)
{
	gint mux = ghb_settings_combo_int(settings, "FileFormat");

	guint32 in_codec = aconfig ? aconfig->in.codec : HB_ACODEC_MASK;
	if (mux == HB_MUX_MP4)
	{
		if ((acodec & in_codec & HB_ACODEC_AC3))
		{
			return acodec & (in_codec | HB_ACODEC_PASS_FLAG);
		}
		else if (acodec & HB_ACODEC_AC3)
		{
			return HB_ACODEC_AC3;
		}
		else if (acodec & HB_ACODEC_LAME)
		{
			return HB_ACODEC_LAME;
		}
		else if (acodec & HB_ACODEC_FAAC)
		{
			return HB_ACODEC_FAAC;
		}
		else
		{
			return HB_ACODEC_FAAC;
		}
	}
	else
	{
		if ((acodec & in_codec & HB_ACODEC_PASS_MASK))
		{
			return acodec & (in_codec | HB_ACODEC_PASS_FLAG);
		}
		else if (acodec & HB_ACODEC_AC3)
		{
			return HB_ACODEC_AC3;
		}
		else if (acodec & HB_ACODEC_VORBIS)
		{
			return HB_ACODEC_VORBIS;
		}
		else if (acodec & HB_ACODEC_LAME)
		{
			return HB_ACODEC_LAME;
		}
		else if (acodec & HB_ACODEC_FAAC)
		{
			return HB_ACODEC_FAAC;
		}
		else
		{
			return HB_ACODEC_LAME;
		}
	}
}

gboolean
ghb_validate_audio(signal_user_data_t *ud)
{
	hb_list_t  * list;
	hb_title_t * title;
	gchar *message;
	GValue *value;

	if (h_scan == NULL) return FALSE;
	list = hb_get_titles( h_scan );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		g_message("No title found.\n");
		return FALSE;
	}

	gint titleindex;

	titleindex = ghb_settings_combo_int(ud->settings, "title");
    title = hb_list_item( list, titleindex );
	if (title == NULL) return FALSE;
	gint mux = ghb_settings_combo_int(ud->settings, "FileFormat");

	const GValue *audio_list;
	gint count, ii;

	audio_list = ghb_settings_get_value(ud->settings, "audio_list");
	count = ghb_array_len(audio_list);
	for (ii = 0; ii < count; ii++)
	{
		GValue *asettings;
	    hb_audio_config_t *aconfig;

		asettings = ghb_array_get_nth(audio_list, ii);
		gint track = ghb_settings_combo_int(asettings, "AudioTrack");
		gint codec = ghb_settings_combo_int(asettings, "AudioEncoder");
		if (codec == HB_ACODEC_ANY)
			continue;

        aconfig = (hb_audio_config_t *) hb_list_audio_config_item(
											title->list_audio, track );
		if ( ghb_audio_is_passthru(codec) &&
			!(ghb_audio_can_passthru(aconfig->in.codec) && 
			 (aconfig->in.codec & codec)))
		{
			// Not supported.  AC3 is passthrough only, so input must be AC3
			message = g_strdup_printf(
						"The source does not support Pass-Thru.\n\n"
						"You should choose a different audio codec.\n"
						"If you continue, one will be chosen for you.");
			if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, "Cancel", "Continue"))
			{
				g_free(message);
				return FALSE;
			}
			g_free(message);
			if ((codec & HB_ACODEC_AC3) ||
				aconfig->in.codec == HB_ACODEC_DCA)
			{
				codec = HB_ACODEC_AC3;
			}
			else if (mux == HB_MUX_MKV)
			{
				codec = HB_ACODEC_LAME;
			}
			else
			{
				codec = HB_ACODEC_FAAC;
			}
			value = ghb_lookup_acodec_value(codec);
			ghb_settings_take_value(asettings, "AudioEncoder", value);
		}
		gchar *a_unsup = NULL;
		gchar *mux_s = NULL;
		if (mux == HB_MUX_MP4)
		{ 
			mux_s = "MP4";
			// mp4/vorbis|DTS combination is not supported.
			if (codec == HB_ACODEC_VORBIS)
			{
				a_unsup = "Vorbis";
				codec = HB_ACODEC_FAAC;
			}
			if (codec == HB_ACODEC_DCA)
			{
				a_unsup = "DTS";
				codec = HB_ACODEC_AC3;
			}
		}
		if (a_unsup)
		{
			message = g_strdup_printf(
						"%s is not supported in the %s container.\n\n"
						"You should choose a different audio codec.\n"
						"If you continue, one will be chosen for you.", a_unsup, mux_s);
			if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, "Cancel", "Continue"))
			{
				g_free(message);
				return FALSE;
			}
			g_free(message);
			value = ghb_lookup_acodec_value(codec);
			ghb_settings_take_value(asettings, "AudioEncoder", value);
		}

		gint mix = ghb_settings_combo_int (asettings, "AudioMixdown");
		gboolean allow_mono = TRUE;
		gboolean allow_stereo = TRUE;
		gboolean allow_dolby = TRUE;
		gboolean allow_dpl2 = TRUE;
		gboolean allow_6ch = TRUE;
		allow_mono = TRUE;

		gint best = hb_get_best_mixdown(codec, aconfig->in.channel_layout, 0);

		allow_stereo = best >= HB_AMIXDOWN_STEREO;
		allow_dolby = best >= HB_AMIXDOWN_DOLBY;
		allow_dpl2 = best >= HB_AMIXDOWN_DOLBYPLII;
		allow_6ch = best >= HB_AMIXDOWN_6CH;

		gchar *mix_unsup = NULL;
		if (mix == HB_AMIXDOWN_MONO && !allow_mono)
		{
			mix_unsup = "mono";
		}
		if (mix == HB_AMIXDOWN_STEREO && !allow_stereo)
		{
			mix_unsup = "stereo";
		}
		if (mix == HB_AMIXDOWN_DOLBY && !allow_dolby)
		{
			mix_unsup = "Dolby";
		}
		if (mix == HB_AMIXDOWN_DOLBYPLII && !allow_dpl2)
		{
			mix_unsup = "Dolby Pro Logic II";
		}
		if (mix == HB_AMIXDOWN_6CH && !allow_6ch)
		{
			mix_unsup = "6 Channel";
		}
		if (mix_unsup)
		{
			message = g_strdup_printf(
						"The source audio does not support %s mixdown.\n\n"
						"You should choose a different mixdown.\n"
						"If you continue, one will be chosen for you.", mix_unsup);
			if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, "Cancel", "Continue"))
			{
				g_free(message);
				return FALSE;
			}
			g_free(message);
			mix = ghb_get_best_mix(aconfig, codec, mix);
			value = get_amix_value(mix);
			ghb_settings_take_value(asettings, "AudioMixdown", value);
		}
	}
	return TRUE;
}

gboolean
ghb_validate_vquality(GValue *settings)
{
	gint vcodec;
	gchar *message;
	gint min, max;

	if (ghb_settings_get_boolean(settings, "nocheckvquality")) return TRUE;
	vcodec = ghb_settings_combo_int(settings, "VideoEncoder");
	gdouble vquality;
	vquality = ghb_settings_get_double(settings, "VideoQualitySlider");
	if (ghb_settings_get_boolean(settings, "vquality_type_constant"))
	{
		switch (vcodec)
		{
			case HB_VCODEC_X264:
			{
				min = 16;
				max = 30;
			} break;

			case HB_VCODEC_FFMPEG_MPEG2:
			case HB_VCODEC_FFMPEG_MPEG4:
			{
				min = 1;
				max = 8;
			} break;

			case HB_VCODEC_THEORA:
			{
				min = 0;
				max = 63;
			} break;

			default:
			{
				min = 48;
				max = 62;
			} break;
		}
		if (vcodec == HB_VCODEC_X264 && vquality == 0.0)
		{
			message = g_strdup_printf(
						"Warning: lossless h.264 selected\n\n"
						"Lossless h.264 is not well supported by\n"
						"many players and editors.\n\n"
                        "It will produce enormous output files.\n\n"
						"Are you sure you wish to use this setting?");
			if (!ghb_message_dialog(GTK_MESSAGE_QUESTION, message, 
									"Cancel", "Continue"))
			{
				g_free(message);
				return FALSE;
			}
			g_free(message);
		}
		else if (vquality < min || vquality > max)
		{
			message = g_strdup_printf(
						"Interesting video quality choice: %d\n\n"
						"Typical values range from %d to %d.\n\n"
						"Are you sure you wish to use this setting?",
						(gint)vquality, min, max);
			if (!ghb_message_dialog(GTK_MESSAGE_QUESTION, message, 
									"Cancel", "Continue"))
			{
				g_free(message);
				return FALSE;
			}
			g_free(message);
		}
	}
	return TRUE;
}

static void
add_job(hb_handle_t *h, GValue *js, gint unique_id, gint titleindex)
{
	hb_list_t  * list;
	hb_title_t * title;
	hb_job_t   * job;
	static gchar *advanced_opts;
	gint sub_id = 0;
	gboolean tweaks = FALSE;
	gchar *detel_str = NULL;
	gchar *decomb_str = NULL;
	gchar *deint_str = NULL;
	gchar *deblock_str = NULL;
	gchar *denoise_str = NULL;
	gchar *dest_str = NULL;

	g_debug("add_job()\n");
	if (h == NULL) return;
	list = hb_get_titles( h );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		return;
	}

    title = hb_list_item( list, titleindex );
	if (title == NULL) return;

	/* Set job settings */
	job   = title->job;
	if (job == NULL) return;

	job->angle = ghb_settings_get_int(js, "angle");
	job->start_at_preview = ghb_settings_get_int(js, "start_frame") + 1;
	if (job->start_at_preview)
	{
		job->seek_points = ghb_settings_get_int(js, "preview_count");
		job->pts_to_stop = ghb_settings_get_int(js, "live_duration") * 90000LL;
	}

	tweaks = ghb_settings_get_boolean(js, "allow_tweaks");
	job->mux = ghb_settings_combo_int(js, "FileFormat");
	if (job->mux == HB_MUX_MP4)
	{
		job->largeFileSize = ghb_settings_get_boolean(js, "Mp4LargeFile");
		job->mp4_optimize = ghb_settings_get_boolean(js, "Mp4HttpOptimize");
	}
	else
	{
		job->largeFileSize = FALSE;
		job->mp4_optimize = FALSE;
	}
	if (!job->start_at_preview)
	{
		gint start, end;
		gint num_chapters = hb_list_count(title->list_chapter);
		gint duration = title->duration / 90000;
		job->chapter_markers = FALSE;
		job->chapter_start = 1;
		job->chapter_end = num_chapters;

		if (ghb_settings_combo_int(js, "PtoPType") == 0)
		{
			start = ghb_settings_get_int(js, "start_point");
			end = ghb_settings_get_int(js, "end_point");
			job->chapter_start = MIN( num_chapters, start );
			job->chapter_end   = MAX( job->chapter_start, end );

		}
		if (ghb_settings_combo_int(js, "PtoPType") == 1)
		{
			start = ghb_settings_get_int(js, "start_point");
			end = ghb_settings_get_int(js, "end_point");
			job->pts_to_start = (int64_t)MIN(duration-1, start) * 90000;
			job->pts_to_stop = (int64_t)MAX(start+1, end) * 90000 - 
								job->pts_to_start;
		}
		if (ghb_settings_combo_int(js, "PtoPType") == 2)
		{
			start = ghb_settings_get_int(js, "start_point");
			end = ghb_settings_get_int(js, "end_point");
			gint64 max_frames;
			max_frames = (gint64)duration * title->rate / title->rate_base;
			job->frame_to_start = (int64_t)MIN(max_frames-1, start-1);
			job->frame_to_stop = (int64_t)MAX(start, end-1) - 
								 job->frame_to_start;
		}
		if (job->chapter_start != job->chapter_end)
		{
			job->chapter_markers = ghb_settings_get_boolean(js, "ChapterMarkers");
		}
		if (job->chapter_start == job->chapter_end)
			job->chapter_markers = 0;
		if ( job->chapter_markers )
		{
			GValue *chapters;
			GValue *chapter;
			gint chap;
			gint count;

			chapters = ghb_settings_get_value(js, "chapter_list");
			count = ghb_array_len(chapters);
			for(chap = 0; chap < count; chap++)
			{
				hb_chapter_t * chapter_s;
				gchar *name;

				name = NULL;
				chapter = ghb_array_get_nth(chapters, chap);
				name = ghb_value_string(chapter); 
				if (name == NULL)
				{
					name = g_strdup_printf ("Chapter %2d", chap+1);
				}
				chapter_s = hb_list_item( job->title->list_chapter, chap);
				strncpy(chapter_s->title, name, 1023);
				chapter_s->title[1023] = '\0';
				g_free(name);
			}
		}
	}
	job->crop[0] = ghb_settings_get_int(js, "PictureTopCrop");
	job->crop[1] = ghb_settings_get_int(js, "PictureBottomCrop");
	job->crop[2] = ghb_settings_get_int(js, "PictureLeftCrop");
	job->crop[3] = ghb_settings_get_int(js, "PictureRightCrop");

	
	gboolean decomb_deint = ghb_settings_get_boolean(js, "PictureDecombDeinterlace");
	gint decomb = ghb_settings_combo_int(js, "PictureDecomb");
	gint deint = ghb_settings_combo_int(js, "PictureDeinterlace");
	if (!decomb_deint)
		job->deinterlace = (deint != 0) ? 1 : 0;
	else
		job->deinterlace = 0;
    job->grayscale   = ghb_settings_get_boolean(js, "VideoGrayScale");

	gboolean keep_aspect;
	keep_aspect = ghb_settings_get_boolean(js, "PictureKeepRatio");
	job->anamorphic.mode = ghb_settings_combo_int(js, "PicturePAR");
	job->modulus = ghb_settings_combo_int(js, "PictureModulus");
	if (job->anamorphic.mode)
	{
		job->anamorphic.par_width = title->pixel_aspect_width;
		job->anamorphic.par_height = title->pixel_aspect_height;
		job->anamorphic.dar_width = 0;
		job->anamorphic.dar_height = 0;

		if (job->anamorphic.mode == 3 && !keep_aspect)
		{
			job->anamorphic.keep_display_aspect = 0;
			job->anamorphic.par_width = 
				ghb_settings_get_int(js, "PicturePARWidth");
			job->anamorphic.par_height = 
				ghb_settings_get_int(js, "PicturePARHeight");
		}
		else
		{
			job->anamorphic.keep_display_aspect = 1;
		}
	}

	/* Add selected filters */
	job->filters = hb_list_init();
	gint detel = ghb_settings_combo_int(js, "PictureDetelecine");
	if ( detel )
	{
		if (detel != 1)
		{
			if (detel_opts.map[detel].svalue != NULL)
				detel_str = g_strdup(detel_opts.map[detel].svalue);
		}
		else
			detel_str = ghb_settings_get_string(js, "PictureDetelecineCustom");
		hb_filter_detelecine.settings = detel_str;
		hb_list_add( job->filters, &hb_filter_detelecine );
	}
	if ( decomb_deint && decomb )
	{
		if (decomb != 1)
		{
			if (decomb_opts.map[decomb].svalue != NULL)
				decomb_str = g_strdup(decomb_opts.map[decomb].svalue);
		}
		else
			decomb_str = ghb_settings_get_string(js, "PictureDecombCustom");
		hb_filter_decomb.settings = decomb_str;
		hb_list_add( job->filters, &hb_filter_decomb );
	}
	if( job->deinterlace )
	{
		if (deint != 1)
		{
			if (deint_opts.map[deint].svalue != NULL)
				deint_str = g_strdup(deint_opts.map[deint].svalue);
		}
		else
			deint_str = ghb_settings_get_string(js, "PictureDeinterlaceCustom");
		hb_filter_deinterlace.settings = deint_str;
		hb_list_add( job->filters, &hb_filter_deinterlace );
	}
	gint deblock = ghb_settings_get_int(js, "PictureDeblock");
	if( deblock >= 5 )
	{
		deblock_str = g_strdup_printf("%d", deblock);
		hb_filter_deblock.settings = deblock_str;
		hb_list_add( job->filters, &hb_filter_deblock );
	}
	gint denoise = ghb_settings_combo_int(js, "PictureDenoise");
	if( denoise )
	{
		if (denoise != 1)
		{
			if (denoise_opts.map[denoise].svalue != NULL)
				denoise_str = g_strdup(denoise_opts.map[denoise].svalue);
		}
		else
			denoise_str = ghb_settings_get_string(js, "PictureDenoiseCustom");
		hb_filter_denoise.settings = denoise_str;
		hb_list_add( job->filters, &hb_filter_denoise );
	}
	job->width = ghb_settings_get_int(js, "scale_width");
	job->height = ghb_settings_get_int(js, "scale_height");

	job->vcodec = ghb_settings_combo_int(js, "VideoEncoder");
	if ((job->mux == HB_MUX_MP4 ) && (job->vcodec == HB_VCODEC_THEORA))
	{
		// mp4/theora combination is not supported.
		job->vcodec = HB_VCODEC_FFMPEG_MPEG4;
	}
	if ((job->vcodec == HB_VCODEC_X264) && (job->mux == HB_MUX_MP4))
	{
		job->ipod_atom = ghb_settings_get_boolean(js, "Mp4iPodCompatible");
	}
	if (ghb_settings_get_boolean(js, "vquality_type_constant"))
	{
		gdouble vquality;
		vquality = ghb_settings_get_double(js, "VideoQualitySlider");
		job->vquality =  vquality;
		job->vbitrate = 0;
	}
	else if (ghb_settings_get_boolean(js, "vquality_type_bitrate"))
	{
		job->vquality = -1.0;
		job->vbitrate = ghb_settings_get_int(js, "VideoAvgBitrate");
	}

	gint vrate = ghb_settings_combo_int(js, "VideoFramerate");
	if( vrate == 0 )
	{
		job->vrate = title->rate;
		job->vrate_base = title->rate_base;
	}
	else
	{
		job->vrate = 27000000;
		job->vrate_base = vrate;
	}
	if (ghb_settings_get_boolean(js, "VideoFrameratePFR"))
		job->cfr = 2;
	else if (ghb_settings_get_boolean(js, "VideoFramerateCFR"))
		job->cfr = 1;
	else
		job->cfr = 0;

	const GValue *audio_list;
	gint count, ii;
	gint tcount = 0;
	
	audio_list = ghb_settings_get_value(js, "audio_list");
	count = ghb_array_len(audio_list);
	for (ii = 0; ii < count; ii++)
	{
		GValue *asettings;
	    hb_audio_config_t audio;
	    hb_audio_config_t *aconfig;
		gint acodec;

		hb_audio_config_init(&audio);
		asettings = ghb_array_get_nth(audio_list, ii);
		audio.in.track = ghb_settings_get_int(asettings, "AudioTrack");
		audio.out.track = tcount;

        aconfig = (hb_audio_config_t *) hb_list_audio_config_item(
									title->list_audio, audio.in.track );

		acodec = ghb_settings_combo_int(asettings, "AudioEncoder");
		audio.out.codec = ghb_select_audio_codec(js, aconfig, acodec);

		audio.out.gain = 
			ghb_settings_get_double(asettings, "AudioTrackGain");

        audio.out.dynamic_range_compression = 
			ghb_settings_get_double(asettings, "AudioTrackDRCSlider");
        if (audio.out.dynamic_range_compression < 1.0)
        	audio.out.dynamic_range_compression = 0.0;

		// It would be better if this were done in libhb for us, but its not yet.
		if (ghb_audio_is_passthru(audio.out.codec))
		{
			audio.out.mixdown = 0;
		}
		else
		{
			audio.out.mixdown = ghb_settings_combo_int(asettings, "AudioMixdown");
			// Make sure the mixdown is valid and pick a new one if not.
			audio.out.mixdown = ghb_get_best_mix(aconfig, audio.out.codec, 
													audio.out.mixdown);
			audio.out.bitrate = 
				ghb_settings_combo_int(asettings, "AudioBitrate");
			gint srate = ghb_settings_combo_int(asettings, "AudioSamplerate");
			if (srate == 0)	// 0 is same as source
				audio.out.samplerate = aconfig->in.samplerate;
			else
				audio.out.samplerate = srate;

			audio.out.bitrate = hb_get_best_audio_bitrate(
				audio.out.codec, audio.out.bitrate, 
				audio.out.samplerate, audio.out.mixdown);
		}

		// Add it to the jobs audio list
        hb_audio_add( job, &audio );
		tcount++;
	}

	dest_str = ghb_settings_get_string(js, "destination");
	job->file = dest_str;

	const GValue *subtitle_list;
	gint subtitle;
	gboolean force, burned, def, one_burned = FALSE;
	
	ghb_settings_set_boolean(js, "subtitle_scan", FALSE);
	subtitle_list = ghb_settings_get_value(js, "subtitle_list");
	count = ghb_array_len(subtitle_list);
	for (ii = 0; ii < count; ii++)
	{
		GValue *ssettings;
		gint source;

		ssettings = ghb_array_get_nth(subtitle_list, ii);

		force = ghb_settings_get_boolean(ssettings, "SubtitleForced");
		burned = ghb_settings_get_boolean(ssettings, "SubtitleBurned");
		def = ghb_settings_get_boolean(ssettings, "SubtitleDefaultTrack");
		source = ghb_settings_get_int(ssettings, "SubtitleSource");

		if (source == SRTSUB)
		{
    		hb_subtitle_config_t sub_config;
			gchar *filename, *lang, *code;

			filename = ghb_settings_get_string(ssettings, "SrtFile");
			if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR))
			{
				continue;
			}
			sub_config.offset = ghb_settings_get_int(ssettings, "SrtOffset");
			lang = ghb_settings_get_string(ssettings, "SrtLanguage");
			code = ghb_settings_get_string(ssettings, "SrtCodeset");
			strncpy(sub_config.src_filename, filename, 255);
			sub_config.src_filename[255] = 0;
			strncpy(sub_config.src_codeset, code, 39);
			sub_config.src_codeset[39] = 0;
			sub_config.force = 0;
			sub_config.dest = PASSTHRUSUB;
			sub_config.default_track = def;

			hb_srt_add( job, &sub_config, lang);

			g_free(filename);
			g_free(lang);
			g_free(code);
			continue;
		}

		subtitle = ghb_settings_get_int(ssettings, "SubtitleTrack");
		if (subtitle == -1)
		{
			if (!burned)
			{
				job->select_subtitle_config.dest = PASSTHRUSUB;
			}
			else if (burned)
			{
				// Only allow one subtitle to be burned into the video
				if (one_burned)
					continue;
				job->select_subtitle_config.dest = RENDERSUB;
				one_burned = TRUE;
			}
			job->select_subtitle_config.force = force;
			job->select_subtitle_config.default_track = def;
			job->indepth_scan = 1;
			ghb_settings_set_boolean(js, "subtitle_scan", TRUE);
		}
		else if (subtitle >= 0)
		{
    		hb_subtitle_t * subt;
    		hb_subtitle_config_t sub_config;

       		subt = (hb_subtitle_t *)hb_list_item(title->list_subtitle, subtitle);
			if (subt != NULL)
			{
				sub_config = subt->config;
				if (!burned)
				{
					sub_config.dest = PASSTHRUSUB;
				}
				else if ( burned && canBurn(subt->source) )
				{
					// Only allow one subtitle to be burned into the video
					if (one_burned)
						continue;
					sub_config.dest = RENDERSUB;
					one_burned = TRUE;
				}
				sub_config.force = force;
				sub_config.default_track = def;
        		hb_subtitle_add( job, &sub_config, subtitle );
			}
		}
	}

	// TODO: libhb holds onto a reference to the advanced_opts and is not
	// finished with it until encoding the job is done.  But I can't
	// find a way to get at the job before it is removed in order to
	// free up the memory I am allocating here.
	// The short story is THIS LEAKS.
	advanced_opts = ghb_build_advanced_opts_string(js);
	
	if( advanced_opts && *advanced_opts == '\0' )
	{
		g_free(advanced_opts);
		advanced_opts = NULL;
	}

	if (job->indepth_scan == 1)
	{
		// Subtitle scan. Look for subtitle matching audio language

		/*
		 * When subtitle scan is enabled do a fast pre-scan job
		 * which will determine which subtitles to enable, if any.
		 */
		job->pass = -1;
		job->indepth_scan = 1;
		job->advanced_opts = NULL;

		/*
		 * Add the pre-scan job
		 */
		job->sequence_id = (unique_id & 0xFFFFFF) | (sub_id++ << 24);
		hb_add( h, job );
	}

	if( ghb_settings_get_boolean(js, "VideoTwoPass") &&
		!ghb_settings_get_boolean(js, "vquality_type_constant"))
	{
		/*
		 * If subtitle_scan is enabled then only turn it on
		 * for the second pass and then off again for the
		 * second.
		 */
		job->pass = 1;
		job->indepth_scan = 0;

		/*
		 * If turbo options have been selected then append them
		 * to the advanced_opts now (size includes one ':' and the '\0')
		 */
		if( ghb_settings_get_boolean(js, "VideoTurboTwoPass") )
		{
			gchar *tmp_advanced_opts;
			gchar *extra_opts;

			if (job->vcodec == HB_VCODEC_X264)
			{
				gint badapt;

				badapt = ghb_lookup_badapt(advanced_opts);
				if (badapt == 2)
				{
					extra_opts = g_strdup_printf("%s", turbo_x264_opts);
				}
				else
				{
					extra_opts = g_strdup_printf("%s:weightb=0", turbo_x264_opts);
				}
			}
			else if (job->vcodec == HB_VCODEC_FFMPEG_MPEG4)
			{
				extra_opts = g_strdup_printf("%s", turbo_lavc_opts);
			}
			else
			{
				extra_opts = g_strdup("");
			}

			if ( advanced_opts )
			{
				tmp_advanced_opts = g_strdup_printf("%s:%s", advanced_opts, extra_opts);
			} 
			else 
			{
				/*
				 * No advanced_opts to modify, but apply the turbo options
				 * anyway as they may be modifying defaults
				 */
				tmp_advanced_opts = g_strdup_printf("%s", extra_opts);
			}
			g_free(extra_opts);

			job->advanced_opts = tmp_advanced_opts;
		}
		else
		{
			job->advanced_opts = advanced_opts;
		}
		job->sequence_id = (unique_id & 0xFFFFFF) | (sub_id++ << 24);
		hb_add( h, job );
		//if (job->advanced_opts != NULL)
		//	g_free(job->advanced_opts);

		job->pass = 2;
		/*
		 * On the second pass we turn off subtitle scan so that we
		 * can actually encode using any subtitles that were auto
		 * selected in the first pass (using the whacky select-subtitle
		 * attribute of the job).
		 */
		job->indepth_scan = 0;
		job->advanced_opts = advanced_opts;
		job->sequence_id = (unique_id & 0xFFFFFF) | (sub_id++ << 24);
		hb_add( h, job );
		//if (job->advanced_opts != NULL)
		//	g_free(job->advanced_opts);
	}
	else
	{
		job->advanced_opts = advanced_opts;
		job->indepth_scan = 0;
		job->pass = 0;
		job->sequence_id = (unique_id & 0xFFFFFF) | (sub_id++ << 24);
		hb_add( h, job );
		//if (job->advanced_opts != NULL)
		//	g_free(job->advanced_opts);
	}

	// clean up audio list
	gint num_audio_tracks = hb_list_count(job->list_audio);
	for(ii = 0; ii < num_audio_tracks; ii++)
	{
		hb_audio_t *audio = (hb_audio_t*)hb_list_item(job->list_audio, 0);
		hb_list_rem(job->list_audio, audio);
		free(audio);
	}

	// clean up subtitle list
	gint num_subtitle_tracks = hb_list_count(job->list_subtitle);
	for(ii = 0; ii < num_subtitle_tracks; ii++)
	{
		hb_subtitle_t *subtitle = hb_list_item(job->list_subtitle, 0);
		hb_list_rem(job->list_subtitle, subtitle);
		free(subtitle);
	}

	if (detel_str) g_free(detel_str);
	if (decomb_str) g_free(decomb_str);
	if (deint_str) g_free(deint_str);
	if (deblock_str) g_free(deblock_str);
	if (denoise_str) g_free(denoise_str);
	if (dest_str) g_free(dest_str);
}

void
ghb_add_job(GValue *js, gint unique_id)
{
	// Since I'm doing a scan of the single title I want just prior 
	// to adding the job, there is only the one title to choose from.
	add_job(h_queue, js, unique_id, 0);
}

void
ghb_add_live_job(GValue *js, gint unique_id)
{
	// Since I'm doing a scan of the single title I want just prior 
	// to adding the job, there is only the one title to choose from.
	gint titleindex = ghb_settings_combo_int(js, "title");
	add_job(h_scan, js, unique_id, titleindex);
}

void
ghb_remove_job(gint unique_id)
{
    hb_job_t * job;
    gint ii;
	
	// Multiples passes all get the same id
	// remove them all.
	// Go backwards through list, so reordering doesn't screw me.
	ii = hb_count(h_queue) - 1;
    while ((job = hb_job(h_queue, ii--)) != NULL)
    {
        if ((job->sequence_id & 0xFFFFFF) == unique_id)
			hb_rem(h_queue, job);
    }
}

void
ghb_start_queue()
{
	hb_start( h_queue );
}

void
ghb_stop_queue()
{
	hb_stop( h_queue );
}

void
ghb_start_live_encode()
{
	hb_start( h_scan );
}

void
ghb_stop_live_encode()
{
	hb_stop( h_scan );
}

void
ghb_pause_queue()
{
    hb_state_t s;
    hb_get_state2( h_queue, &s );

    if( s.state == HB_STATE_PAUSED )
    {
		hb_status.queue.state &= ~GHB_STATE_PAUSED;
		hb_resume( h_queue );
    }
    else
    {
		hb_status.queue.state |= GHB_STATE_PAUSED;
		hb_pause( h_queue );
    }
}

static void
vert_line(
	GdkPixbuf * pb, 
	guint8 r, 
	guint8 g, 
	guint8 b, 
	gint x, 
	gint y, 
	gint len, 
	gint width)
{
	guint8 *pixels = gdk_pixbuf_get_pixels (pb);
	guint8 *dst;
	gint ii, jj;
	gint channels = gdk_pixbuf_get_n_channels (pb);
	gint stride = gdk_pixbuf_get_rowstride (pb);

	for (jj = 0; jj < width; jj++)
	{
		dst = pixels + y * stride + (x+jj) * channels;
		for (ii = 0; ii < len; ii++)
		{
			dst[0] = r;
			dst[1] = g;
			dst[2] = b;
			dst += stride;
		}
	}
}

static void
horz_line(
	GdkPixbuf * pb, 
	guint8 r, 
	guint8 g, 
	guint8 b, 
	gint x, 
	gint y, 
	gint len,
	gint width)
{
	guint8 *pixels = gdk_pixbuf_get_pixels (pb);
	guint8 *dst;
	gint ii, jj;
	gint channels = gdk_pixbuf_get_n_channels (pb);
	gint stride = gdk_pixbuf_get_rowstride (pb);

	for (jj = 0; jj < width; jj++)
	{
		dst = pixels + (y+jj) * stride + x * channels;
		for (ii = 0; ii < len; ii++)
		{
			dst[0] = r;
			dst[1] = g;
			dst[2] = b;
			dst += channels;
		}
	}
}

static void
hash_pixbuf(
	GdkPixbuf * pb,
	gint        x,
	gint        y,
	gint        w,
	gint        h,
	gint        step,
	gint		orientation)
{
	gint ii, jj;
	gint line_width = 8;
	struct
	{
		guint8 r;
		guint8 g;
		guint8 b;
	} c[4] = 
	{{0x80, 0x80, 0x80},{0xC0, 0x80, 0x70},{0x80, 0xA0, 0x80},{0x70, 0x80, 0xA0}};

	if (!orientation)
	{
		// vertical lines
		for (ii = x, jj = 0; ii+line_width < x+w; ii += step, jj++)
		{
			vert_line(pb, c[jj&3].r, c[jj&3].g, c[jj&3].b, ii, y, h, line_width);
		}
	}
	else
	{
		// horizontal lines
		for (ii = y, jj = 0; ii+line_width < y+h; ii += step, jj++)
		{
			horz_line(pb, c[jj&3].r, c[jj&3].g, c[jj&3].b, x, ii, w, line_width);
		}
	}
}

GdkPixbuf*
ghb_get_preview_image(
	gint titleindex, 
	gint index, 
	signal_user_data_t *ud,
	gint *out_width,
	gint *out_height)
{
	GValue *settings;
	hb_title_t *title;
	hb_list_t  *list;
	
	settings = ud->settings;
	list = hb_get_titles( h_scan );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		return NULL;
	}
    title = hb_list_item( list, titleindex );
	if (title == NULL) return NULL;
	if (title->job == NULL) return NULL;
	set_preview_job_settings(title->job, settings);

	// hb_get_preview doesn't compensate for anamorphic, so lets
	// calculate scale factors
	gint width, height, par_width = 1, par_height = 1;
	gint pic_par = ghb_settings_combo_int(settings, "PicturePAR");
	if (pic_par)
	{
		hb_set_anamorphic_size( title->job, &width, &height, 
								&par_width, &par_height );
	}

	// Make sure we have a big enough buffer to receive the image from libhb
	gint dstWidth = title->job->width;
	gint dstHeight= title->job->height;

	static guint8 *buffer = NULL;
	static gint bufferSize = 0;
	gint newSize;

	newSize = dstWidth * dstHeight * 4;
	if( bufferSize < newSize )
	{
		bufferSize = newSize;
		buffer     = (guint8*) g_realloc( buffer, bufferSize );
	}
	hb_get_preview( h_scan, title, index, buffer );

	// Create an GdkPixbuf and copy the libhb image into it, converting it from
	// libhb's format something suitable.
	
	// The image data returned by hb_get_preview is 4 bytes per pixel, 
	// BGRA format. Alpha is ignored.

	GdkPixbuf *preview = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, dstWidth, dstHeight);
	guint8 *pixels = gdk_pixbuf_get_pixels (preview);
	
	guint32 *src = (guint32*)buffer;
	guint8 *dst = pixels;

	gint ii, jj;
	gint channels = gdk_pixbuf_get_n_channels (preview);
	gint stride = gdk_pixbuf_get_rowstride (preview);
	guint8 *tmp;

	for (ii = 0; ii < dstHeight; ii++)
	{
		tmp = dst;
		for (jj = 0; jj < dstWidth; jj++)
		{
			tmp[0] = src[0] >> 16;
			tmp[1] = src[0] >> 8;
			tmp[2] = src[0] >> 0;
			tmp += channels;
			src++;
		}
		dst += stride;
	}
	gint w = ghb_settings_get_int(settings, "scale_width");
	gint h = ghb_settings_get_int(settings, "scale_height");
	ghb_par_scale(ud, &w, &h, par_width, par_height);

	gint c0, c1, c2, c3;
	c0 = ghb_settings_get_int(settings, "PictureTopCrop");
	c1 = ghb_settings_get_int(settings, "PictureBottomCrop");
	c2 = ghb_settings_get_int(settings, "PictureLeftCrop");
	c3 = ghb_settings_get_int(settings, "PictureRightCrop");

	gdouble xscale = (gdouble)w / (gdouble)(title->width - c2 - c3);
	gdouble yscale = (gdouble)h / (gdouble)(title->height - c0 - c1);
	
	ghb_par_scale(ud, &dstWidth, &dstHeight, par_width, par_height);
	*out_width = w;
	*out_height = h;
	if (ghb_settings_get_boolean(settings, "reduce_hd_preview"))
	{
		GdkScreen *ss;
		gint s_w, s_h;
		gint orig_w, orig_h;
		gint factor = 80;

		if (ghb_settings_get_boolean(settings, "preview_fullscreen"))
		{
			factor = 100;
		}
		ss = gdk_screen_get_default();
		s_w = gdk_screen_get_width(ss);
		s_h = gdk_screen_get_height(ss);
		orig_w = dstWidth;
		orig_h = dstHeight;

		if (dstWidth > s_w * factor / 100)
		{
			dstWidth = s_w * factor / 100;
			dstHeight = dstHeight * dstWidth / orig_w;
		}
		if (dstHeight > s_h * factor / 100)
		{
			dstHeight = s_h * factor / 100;
			dstWidth = dstWidth * dstHeight / orig_h;
		}
		xscale *= (gdouble)dstWidth / orig_w;
		yscale *= (gdouble)dstHeight / orig_h;
		w *= (gdouble)dstWidth / orig_w;
		h *= (gdouble)dstHeight / orig_h;
	}
	GdkPixbuf *scaled_preview;
	scaled_preview = gdk_pixbuf_scale_simple(preview, dstWidth, dstHeight, GDK_INTERP_HYPER);
	if (ghb_settings_get_boolean(settings, "show_crop"))
	{
		c0 *= yscale;
		c1 *= yscale;
		c2 *= xscale;
		c3 *= xscale;
		// Top
		hash_pixbuf(scaled_preview, c2, 0, w, c0, 32, 0);
		// Bottom
		hash_pixbuf(scaled_preview, c2, dstHeight-c1, w, c1, 32, 0);
		// Left
		hash_pixbuf(scaled_preview, 0, c0, c2, h, 32, 1);
		// Right
		hash_pixbuf(scaled_preview, dstWidth-c3, c0, c3, h, 32, 1);
	}
	g_object_unref (preview);
	return scaled_preview;
}

static void
sanitize_volname(gchar *name)
{
	gchar *a, *b;

	a = b = name;
	while (*b)
	{
		switch(*b)
		{
		case '<':
			b++;
			break;
		case '>':
			b++;
			break;
		default:
			*a = *b;
			a++; b++;
			break;
		}
	}
	*a = 0;
}

gchar*
ghb_dvd_volname(const gchar *device)
{
	gchar *name;
	name = hb_dvd_name((gchar*)device);
	if (name != NULL && name[0] != 0)
	{
		name = g_strdup(name);
		sanitize_volname(name);
		return name;
	}
	return NULL;
}
