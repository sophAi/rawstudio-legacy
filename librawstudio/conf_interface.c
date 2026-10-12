/*
 * Copyright (C) 2006-2011 Anders Brander <anders@brander.dk>,
 * Anders Kvist <akv@lnxbx.dk> and Klaus Post <klauspost@gmail.com>
 *
 * GKeyFile modernization for rawstudio-legacy
 */

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <string.h>
#include "conf_interface.h"

#define GROUP_NAME "Rawstudio"

static GKeyFile *key_file = NULL;
static gchar *conf_file_path = NULL;
static GMutex lock;

static const gchar *
normalize_key(const gchar *name)
{
	if (!name)
		return "";
	const gchar *p = name;
	if (g_str_has_prefix(p, "/apps/rawstudio-legacy/"))
		p += strlen("/apps/rawstudio-legacy/");
	else if (g_str_has_prefix(p, "/apps/rawstudio/"))
		p += strlen("/apps/rawstudio/");
	else if (g_str_has_prefix(p, "/apps/"))
	{
		const gchar *slash = strchr(p + 6, '/');
		if (slash)
			p = slash + 1;
		else
			p += 6;
	}
	while (*p == '/')
		p++;
	return p;
}

static void
ensure_conf_loaded(void)
{
	if (key_file)
		return;

	key_file = g_key_file_new();
	const gchar *config_dir = g_get_user_config_dir();
	gchar *dir = g_build_filename(config_dir, "rawstudio-legacy", NULL);
	g_mkdir_with_parents(dir, 0700);
	conf_file_path = g_build_filename(dir, "preferences", NULL);
	g_free(dir);

	/* If preferences doesn't exist in rawstudio-legacy, check older rawstudio dir */
	if (!g_file_test(conf_file_path, G_FILE_TEST_EXISTS))
	{
		gchar *legacy_dir = g_build_filename(config_dir, "rawstudio", "preferences", NULL);
		if (g_file_test(legacy_dir, G_FILE_TEST_EXISTS))
		{
			g_key_file_load_from_file(key_file, legacy_dir, G_KEY_FILE_NONE, NULL);
		}
		g_free(legacy_dir);
	}
	else
	{
		g_key_file_load_from_file(key_file, conf_file_path, G_KEY_FILE_NONE, NULL);
	}
}

static void
save_conf_file(void)
{
	if (key_file && conf_file_path)
	{
		gsize length = 0;
		gchar *data = g_key_file_to_data(key_file, &length, NULL);
		if (data)
		{
			g_file_set_contents(conf_file_path, data, length, NULL);
			g_free(data);
		}
	}
}

void
rs_set_last_working_directory(const char *lwd)
{
	if (lwd)
		rs_conf_set_string(CONF_LWD, lwd);
}

gchar *
rs_get_last_working_directory(void)
{
	return rs_conf_get_string(CONF_LWD);
}

gboolean
rs_conf_get_boolean(const gchar *name, gboolean *boolean_value)
{
	gboolean ret = FALSE;
	const gchar *k = normalize_key(name);
	g_mutex_lock(&lock);
	ensure_conf_loaded();
	if (g_key_file_has_key(key_file, GROUP_NAME, k, NULL))
	{
		GError *err = NULL;
		gboolean val = g_key_file_get_boolean(key_file, GROUP_NAME, k, &err);
		if (!err)
		{
			if (boolean_value)
				*boolean_value = val;
			ret = TRUE;
		}
		else
		{
			g_error_free(err);
		}
	}
	g_mutex_unlock(&lock);
	return ret;
}

gboolean
rs_conf_get_boolean_with_default(const gchar *name, gboolean *boolean_value, gboolean default_value)
{
	if (!rs_conf_get_boolean(name, boolean_value))
	{
		if (boolean_value)
			*boolean_value = default_value;
		return FALSE;
	}
	return TRUE;
}

gboolean
rs_conf_set_boolean(const gchar *name, gboolean bool_value)
{
	const gchar *k = normalize_key(name);
	g_mutex_lock(&lock);
	ensure_conf_loaded();
	g_key_file_set_boolean(key_file, GROUP_NAME, k, bool_value);
	save_conf_file();
	g_mutex_unlock(&lock);
	return TRUE;
}

gchar *
rs_conf_get_string(const gchar *path)
{
	gchar *ret = NULL;
	const gchar *k = normalize_key(path);
	g_mutex_lock(&lock);
	ensure_conf_loaded();
	if (g_key_file_has_key(key_file, GROUP_NAME, k, NULL))
	{
		ret = g_key_file_get_string(key_file, GROUP_NAME, k, NULL);
	}
	g_mutex_unlock(&lock);
	return ret;
}

gboolean
rs_conf_set_string(const gchar *path, const gchar *string)
{
	const gchar *k = normalize_key(path);
	g_mutex_lock(&lock);
	ensure_conf_loaded();
	g_key_file_set_string(key_file, GROUP_NAME, k, string ? string : "");
	save_conf_file();
	g_mutex_unlock(&lock);
	return TRUE;
}

gboolean
rs_conf_get_integer(const gchar *name, gint *integer_value)
{
	gboolean ret = FALSE;
	const gchar *k = normalize_key(name);
	g_mutex_lock(&lock);
	ensure_conf_loaded();
	if (g_key_file_has_key(key_file, GROUP_NAME, k, NULL))
	{
		GError *err = NULL;
		gint val = g_key_file_get_integer(key_file, GROUP_NAME, k, &err);
		if (!err)
		{
			if (integer_value)
				*integer_value = val;
			ret = TRUE;
		}
		else
		{
			g_error_free(err);
		}
	}
	g_mutex_unlock(&lock);
	return ret;
}

gboolean
rs_conf_set_integer(const gchar *name, const gint integer_value)
{
	const gchar *k = normalize_key(name);
	g_mutex_lock(&lock);
	ensure_conf_loaded();
	g_key_file_set_integer(key_file, GROUP_NAME, k, integer_value);
	save_conf_file();
	g_mutex_unlock(&lock);
	return TRUE;
}

gboolean
rs_conf_get_color(const gchar *name, GdkColor *color)
{
	gchar *str = rs_conf_get_string(name);
	gboolean ret = FALSE;
	if (str)
	{
		ret = gdk_color_parse(str, color);
		g_free(str);
	}
	return ret;
}

gboolean
rs_conf_set_color(const gchar *name, GdkColor *color)
{
	gchar *str = g_strdup_printf("#%02x%02x%02x", color->red >> 8, color->green >> 8, color->blue >> 8);
	gboolean ret = FALSE;
	if (str)
	{
		ret = rs_conf_set_string(name, str);
		g_free(str);
	}
	return ret;
}

gboolean
rs_conf_get_double(const gchar *name, gdouble *float_value)
{
	gboolean ret = FALSE;
	const gchar *k = normalize_key(name);
	g_mutex_lock(&lock);
	ensure_conf_loaded();
	if (g_key_file_has_key(key_file, GROUP_NAME, k, NULL))
	{
		GError *err = NULL;
		gdouble val = g_key_file_get_double(key_file, GROUP_NAME, k, &err);
		if (!err)
		{
			if (float_value)
				*float_value = val;
			ret = TRUE;
		}
		else
		{
			g_error_free(err);
		}
	}
	g_mutex_unlock(&lock);
	return ret;
}

gboolean
rs_conf_set_double(const gchar *name, const gdouble float_value)
{
	const gchar *k = normalize_key(name);
	g_mutex_lock(&lock);
	ensure_conf_loaded();
	g_key_file_set_double(key_file, GROUP_NAME, k, float_value);
	save_conf_file();
	g_mutex_unlock(&lock);
	return TRUE;
}

GSList *
rs_conf_get_list_string(const gchar *name)
{
	GSList *list = NULL;
	const gchar *k = normalize_key(name);
	g_mutex_lock(&lock);
	ensure_conf_loaded();
	if (g_key_file_has_key(key_file, GROUP_NAME, k, NULL))
	{
		gsize len = 0;
		gchar **strv = g_key_file_get_string_list(key_file, GROUP_NAME, k, &len, NULL);
		if (strv)
		{
			for (gsize i = 0; i < len; i++)
			{
				list = g_slist_append(list, g_strdup(strv[i]));
			}
			g_strfreev(strv);
		}
	}
	g_mutex_unlock(&lock);
	return list;
}

gboolean
rs_conf_set_list_string(const gchar *name, GSList *list)
{
	const gchar *k = normalize_key(name);
	guint len = g_slist_length(list);
	const gchar **strv = g_new0(const gchar *, len + 1);
	GSList *l = list;
	guint i = 0;
	while (l)
	{
		strv[i++] = (const gchar *) l->data;
		l = l->next;
	}
	g_mutex_lock(&lock);
	ensure_conf_loaded();
	g_key_file_set_string_list(key_file, GROUP_NAME, k, strv, len);
	save_conf_file();
	g_mutex_unlock(&lock);
	g_free(strv);
	return TRUE;
}

gboolean
rs_conf_add_string_to_list_string(const gchar *name, gchar *value)
{
	GSList *oldlist = rs_conf_get_list_string(name);
	GSList *newlist = g_slist_append(oldlist, g_strdup(value));
	gboolean ret = rs_conf_set_list_string(name, newlist);
	g_slist_free_full(newlist, g_free);
	return ret;
}

gchar *
rs_conf_get_nth_string_from_list_string(const gchar *name, gint num)
{
	GSList *list = rs_conf_get_list_string(name);
	GSList *cur = list;
	gint i = 0;
	gchar *value = NULL;
	while (cur)
	{
		if (i == num)
		{
			value = g_strdup((gchar *) cur->data);
			break;
		}
		cur = cur->next;
		i++;
	}
	g_slist_free_full(list, g_free);
	return value;
}

gboolean
rs_conf_unset(const gchar *name)
{
	const gchar *k = normalize_key(name);
	g_mutex_lock(&lock);
	ensure_conf_loaded();
	gboolean ret = g_key_file_remove_key(key_file, GROUP_NAME, k, NULL);
	if (ret)
		save_conf_file();
	g_mutex_unlock(&lock);
	return ret;
}
