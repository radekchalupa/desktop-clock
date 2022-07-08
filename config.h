#define SHOW_TIME_SECONDS 1
#define SHOW_TIME_DATE 2

static const gchar* STATE_FILE_NAME = "desktop-clock.ini";
/*static const gchar* _font_name = "Monospace";*/
static const gchar* _font_name = "Hack Regular";
static GdkRGBA _color_bk = {0.1, 0.1, 0.1, 1.0};
static GdkRGBA _color_text = {0.8, 0.9, 0.8, 1.0};
static const double _padding = 8.0;
static double _font_size = 35.0;
static guint _interval_ms = 1000;
static guint _show_time_flags = 0;


