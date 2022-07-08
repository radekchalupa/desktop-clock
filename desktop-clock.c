#include <gdk/gdk.h>
/*#include <sys/stat.h>*/
#include <time.h>
#include "config.h"

static GMainLoop* _main_loop;
static GdkWindow* _window;
static char _textbuf[256];
static guint _timer_clock;
static gboolean _resized;

static void redraw(gboolean needresize)
{
	_resized = needresize;
	gdk_window_invalidate_rect(_window, NULL, TRUE);
}

static void swap_decorations()
{
	GdkWMDecoration wnd;
	(void)gdk_window_get_decorations(_window, &wnd);
	if (wnd & GDK_DECOR_ALL)
		gdk_window_set_decorations(_window, 0);
	else
		gdk_window_set_decorations(_window, GDK_DECOR_ALL);
	redraw(TRUE);
}

static void resize_font(gboolean up)
{
	if (up)
	{
		if (_font_size < 128.0)
		{
			_font_size += 1.0;
			redraw(TRUE);
		}
	}
	else
	{
		if (_font_size > 8.0)
		{
			_font_size -= 1.0;
			redraw(TRUE);
		}
	}
}

static void format_time()
{
	time_t timenow;
	time(&timenow);
	char time_buff[32];
	strcpy(time_buff, "%H:%M");
	if (_show_time_flags & SHOW_TIME_SECONDS)
		strcat(time_buff, ":%S");
	if (_show_time_flags & SHOW_TIME_DATE)
		strcat(time_buff, " %m-%d");
	strftime(_textbuf, sizeof(_textbuf), time_buff,
		localtime((const time_t*)&timenow));
}

static gboolean on_timer(gpointer data)
{
	format_time();
	redraw(FALSE);
	return G_SOURCE_CONTINUE;
}

static void save_window_state()
{
	gint xpos, ypos;
	gdk_window_get_position(_window, &xpos, &ypos);
	const gchar* dir = g_get_user_cache_dir();
	if (NULL == dir)
		return;
	GKeyFile* keyfile = g_key_file_new();
	g_key_file_set_integer(keyfile, "window", "xpos", xpos);
	g_key_file_set_integer(keyfile, "window", "ypos", ypos);
	
	GdkWMDecoration wnd;
	(void)gdk_window_get_decorations(_window, &wnd);
	g_key_file_set_boolean(keyfile, "window", "decor", (wnd & GDK_DECOR_ALL));

	g_key_file_set_double(keyfile, "window", "fontsize", _font_size);
	g_key_file_set_double_list(keyfile, "window", "bg", (double*)&_color_bk, 4);
	g_key_file_set_double_list(keyfile, "window", "fg", (double*)&_color_text, 4);
	/* save to file*/
	gchar* path = g_build_filename(dir, "radek-chalupa", NULL);
	if (0 != g_mkdir_with_parents(path, 0755))
	{
		g_free(path);
		return;
	}
	gchar* file = g_build_filename(path, STATE_FILE_NAME, NULL);
	(void)g_key_file_save_to_file(keyfile, file, NULL);
	g_key_file_unref(keyfile);
	g_free(file);
	g_free(path);
}

static void on_end_app()
{
	save_window_state();
	gdk_window_destroy(_window);
	g_main_loop_quit(_main_loop);
}

static void on_key(GdkEventKey* ek, gpointer data)
{
	if (ek->keyval == GDK_KEY_Escape)
	{
		on_end_app();
	}
	else if (ek->keyval == GDK_KEY_F12)
	{
		swap_decorations();
	}
	else if (ek->keyval == GDK_KEY_F)
	{
		resize_font(TRUE);
	}
	else if (ek->keyval == GDK_KEY_f)
	{
		resize_font(FALSE);
	}
}

static void on_expose(GdkEventExpose* ev)
{
	if (ev->region == NULL)
		return;
	static cairo_text_extents_t extents;
	GdkDrawingContext* dc = gdk_window_begin_draw_frame(
		ev->window, ev->region);
	/* background */
	cairo_t* cr = gdk_drawing_context_get_cairo_context(dc);
	cairo_set_source_rgba(cr, _color_bk.red, _color_bk.green,
		_color_bk.blue, _color_bk.alpha);
	cairo_paint(cr);
	/* draw time */
	cairo_set_source_rgba(cr, _color_text.red, _color_text.green,
		_color_text.blue, _color_text.alpha);
	cairo_select_font_face(cr, _font_name,
		CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, _font_size);
	if (!_resized)
		cairo_text_extents(cr, _textbuf, &extents);
	cairo_move_to(cr, _padding, _padding + extents.height);
	cairo_show_text(cr, _textbuf);
	gdk_window_end_draw_frame(ev->window, dc);
	if (!_resized)
	{
		gdk_window_resize(_window, extents.width + (2 * _padding),
			extents.height + (2 * _padding));
		_resized = TRUE;
	}
}

static void on_gdk_event(GdkEvent* event, gpointer data)
{
	if (event->type == GDK_EXPOSE)
	{
		on_expose((GdkEventExpose*)event);
	}
	else if (event->type == GDK_KEY_PRESS)
	{
		on_key((GdkEventKey*)event, data);
	}
	else if (event->type == GDK_CONFIGURE)
	{
		redraw(FALSE);
	}
	else if (event->type == GDK_DELETE)
	{
		on_end_app();
	}
}

static void load_window_state()
{
	const gchar* dir = g_get_user_cache_dir();
	if (NULL == dir)
		return;
	char* path = g_build_filename(dir, "radek-chalupa", NULL);
	if (NULL == path)
		return;
	char* file = g_build_filename(path, STATE_FILE_NAME, NULL);
	if (NULL == file)
		return;
	GKeyFile* keyfile = g_key_file_new();	
	gboolean loadedok = TRUE;
	GError* err = NULL;
	gint xpos, ypos;
	gboolean decor;
	gdouble* dlist;
	gsize glen;
	double dval;
	for (;;)
	{
		if (!g_key_file_load_from_file(keyfile, file, G_KEY_FILE_NONE, NULL))
		{
			loadedok = FALSE;
			break;
		}
		xpos = g_key_file_get_integer(keyfile, "window", "xpos", &err);
		if (NULL != err)
		{
			g_clear_error(&err);
			loadedok = FALSE;
			break;
		}
		ypos = g_key_file_get_integer(keyfile, "window", "ypos", &err);
		if (NULL != err)
		{
			g_clear_error(&err);
			loadedok = FALSE;
			break;
		}
		decor = g_key_file_get_boolean(keyfile, "window", "decor", &err);
		if (NULL != err)
		{
			g_clear_error(&err);
			loadedok = FALSE;
			break;
		}
		dval = g_key_file_get_double(keyfile, "window", "fontsize", &err);
		if (NULL != err)
		{
			g_clear_error(&err);
			loadedok = FALSE;
			break;
		}
		_font_size = dval;
		dlist = g_key_file_get_double_list(keyfile, "window", "bg", &glen, &err);
		if (NULL != err)
		{
			g_clear_error(&err);
			loadedok = FALSE;
			break;
		}
		if (dlist)
		{
			memcpy(&_color_bk, dlist, sizeof(double) * 4);
			g_free(dlist);
		}
		dlist = g_key_file_get_double_list(keyfile, "window", "fg", &glen, &err);
		if (NULL != err)
		{
			g_clear_error(&err);
			loadedok = FALSE;
			break;
		}
		if (dlist)
		{
			memcpy(&_color_text, dlist, sizeof(double) * 4);
			g_free(dlist);
		}
		break;
	}
	g_key_file_unref(keyfile);
	g_free(file);
	g_free(path);
	if (loadedok)
	{
		if (decor)
			gdk_window_set_decorations(_window, GDK_DECOR_ALL);
		else
			gdk_window_set_decorations(_window, 0);
		gdk_window_move(_window, xpos, ypos);
		redraw(TRUE);
	}
}

int main(int argc, char** argv)
{
		GdkWindowAttr attributes;
	if (!gdk_init_check(&argc, &argv))
		return EXIT_FAILURE;
	_resized = FALSE;
	format_time();
	gint attr_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_TITLE | GDK_WA_WMCLASS;
	memset(&attributes, 0, sizeof(attributes));
	attributes.window_type = GDK_WINDOW_TOPLEVEL;
	attributes.x = 64;
	attributes.y = 64;
	attributes.event_mask = GDK_ALL_EVENTS_MASK;
	attributes.width = 440;
	attributes.height = 480;
	attributes.title = "Clock";
	attributes.wclass = GDK_INPUT_OUTPUT;
	gdk_event_handler_set(on_gdk_event, NULL, NULL);
	_window = gdk_window_new(NULL, &attributes, attr_mask);
	gdk_window_set_skip_taskbar_hint(_window, TRUE);	
	gdk_window_set_keep_below(_window, TRUE);
	gdk_window_show(_window);
	load_window_state();
	_timer_clock = g_timeout_add(_interval_ms, on_timer, (gpointer)1);
	_main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(_main_loop);
	g_main_loop_unref(_main_loop);
	return EXIT_SUCCESS;
}
