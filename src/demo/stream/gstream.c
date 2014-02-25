/* ///////////////////////////////////////////////////////////////////////
 * includes
 */
#include "../demo.h"

/* ///////////////////////////////////////////////////////////////////////
 * types
 */
typedef struct __tb_demo_context_t
{
	// verbose 
	tb_bool_t 			verbose;

	// rate
	tb_size_t 			rate;

}tb_demo_context_t;

/* ///////////////////////////////////////////////////////////////////////
 * func
 */
static tb_bool_t tb_demo_gstream_head_func(tb_handle_t http, tb_char_t const* line, tb_pointer_t priv)
{
	tb_printf("response: %s\n", line);
	return tb_true;
}
static tb_bool_t tb_demo_gstream_save_func(tb_size_t state, tb_hize_t size, tb_size_t rate, tb_pointer_t priv)
{
	// check
	tb_demo_context_t* context = (tb_demo_context_t*)priv;
	tb_assert_and_check_return_val(context, tb_false);

	// print verbose info
	if (context->verbose) tb_printf("size: %llu bytes, rate: %lu bytes/s, state: %s\n", size, rate, tb_stream_state_cstr(state));

	// save rate
	if (state == TB_STREAM_STATE_OK) context->rate = rate;

	// ok
	return tb_true;
}

/* ///////////////////////////////////////////////////////////////////////
 * globals
 */ 
static tb_option_item_t g_options[] = 
{
	{'-', 	"gzip", 		TB_OPTION_MODE_KEY, 		TB_OPTION_TYPE_BOOL, 		"enable gzip" 				}
,	{'-', 	"no-verbose", 	TB_OPTION_MODE_KEY, 		TB_OPTION_TYPE_BOOL, 		"disable verbose info" 	 	}
,	{'d', 	"debug", 		TB_OPTION_MODE_KEY, 		TB_OPTION_TYPE_BOOL, 		"enable debug info" 	 	}
,	{'h', 	"header", 		TB_OPTION_MODE_KEY_VAL, 	TB_OPTION_TYPE_CSTR, 		"the custem http header" 	}
,	{'-', 	"post-data", 	TB_OPTION_MODE_KEY_VAL, 	TB_OPTION_TYPE_CSTR, 		"set the post data" 		}
,	{'-', 	"post-file", 	TB_OPTION_MODE_KEY_VAL, 	TB_OPTION_TYPE_CSTR, 		"set the post file" 		}
,	{'-', 	"timeout", 		TB_OPTION_MODE_KEY_VAL, 	TB_OPTION_TYPE_INTEGER, 	"set the timeout" 			}
,	{'-', 	"limitrate", 	TB_OPTION_MODE_KEY_VAL, 	TB_OPTION_TYPE_INTEGER, 	"set the limitrate" 		}
,	{'h', 	"help", 		TB_OPTION_MODE_KEY, 		TB_OPTION_TYPE_BOOL, 		"display this help and exit"}
,	{'-', 	"url", 			TB_OPTION_MODE_VAL, 		TB_OPTION_TYPE_CSTR, 		"the url" 					}
,	{'-', 	tb_null, 		TB_OPTION_MODE_MORE, 		TB_OPTION_TYPE_NONE, 		tb_null 					}

};

/* ///////////////////////////////////////////////////////////////////////
 * main
 */ 
tb_int_t tb_demo_stream_gstream_main(tb_int_t argc, tb_char_t** argv)
{
	// done
	tb_handle_t 	option = tb_null;
	tb_gstream_t* 	ist = tb_null;
	tb_gstream_t* 	ost = tb_null;
	tb_gstream_t* 	pst = tb_null;
	do
	{
		// init option
		option = tb_option_init("gstream", "the gstream demo", g_options);
		tb_assert_and_check_break(option);
	
		// done option
		if (tb_option_done(option, argc - 1, &argv[1]))
		{
			// debug & verbose
			tb_bool_t debug = tb_option_find(option, "debug");
			tb_bool_t verbose = tb_option_find(option, "no-verbose")? tb_false : tb_true;
		
			// done url
			if (tb_option_find(option, "url")) 
			{
				// init ist
				ist = tb_gstream_init_from_url(tb_option_item_cstr(option, "url"));
				tb_assert_and_check_break(ist);
	
				// ctrl http
				if (tb_stream_type(ist) == TB_STREAM_TYPE_HTTP) 
				{
					// enable gzip?
					if (tb_option_find(option, "gzip"))
					{
						// auto unzip
						if (!tb_gstream_ctrl(ist, TB_STREAM_CTRL_HTTP_SET_AUTO_UNZIP, 1)) break;

						// need gzip
						if (!tb_gstream_ctrl(ist, TB_STREAM_CTRL_HTTP_SET_HEAD, "Accept-Encoding", "gzip,deflate")) break;
					}

					// enable debug?
					if (!tb_gstream_ctrl(ist, TB_STREAM_CTRL_HTTP_SET_HEAD_FUNC, debug? tb_demo_gstream_head_func : tb_null)) break;

					// custem header?
					if (tb_option_find(option, "header"))
					{
						// init
						tb_pstring_t key;
						tb_pstring_t val;
						tb_pstring_init(&key);
						tb_pstring_init(&val);

						// done
						tb_bool_t 			k = tb_true;
						tb_char_t const* 	p = tb_option_item_cstr(option, "header");
						while (*p)
						{
							// is key?
							if (k)
							{
								if (*p != ':' && !tb_isspace(*p)) tb_pstring_chrcat(&key, *p++);
								else if (*p == ':') 
								{
									// skip ':'
									p++;

									// skip space
									while (*p && tb_isspace(*p)) p++;

									// is val now
									k = tb_false;
								}
								else p++;
							}
							// is val?
							else
							{
								if (*p != ';') tb_pstring_chrcat(&val, *p++);
								else
								{
									// skip ';'
									p++;

									// skip space
									while (*p && tb_isspace(*p)) p++;

									// set header
									if (tb_pstring_size(&key) && tb_pstring_size(&val))
									{
										if (debug) tb_printf("header: %s: %s\n", tb_pstring_cstr(&key), tb_pstring_cstr(&val));
										if (!tb_gstream_ctrl(ist, TB_STREAM_CTRL_HTTP_SET_HEAD, tb_pstring_cstr(&key), tb_pstring_cstr(&val))) break;
									}

									// is key now
									k = tb_true;

									// clear key & val
									tb_pstring_clear(&key);
									tb_pstring_clear(&val);
								}
							}
						}

						// set header
						if (tb_pstring_size(&key) && tb_pstring_size(&val))
						{
							if (debug) tb_printf("header: %s: %s\n", tb_pstring_cstr(&key), tb_pstring_cstr(&val));
							if (!tb_gstream_ctrl(ist, TB_STREAM_CTRL_HTTP_SET_HEAD, tb_pstring_cstr(&key), tb_pstring_cstr(&val))) break;
						}

						// exit 
						tb_pstring_exit(&key);
						tb_pstring_exit(&val);
					}

					// post-data?
					if (tb_option_find(option, "post-data"))
					{
						tb_hize_t post_size = tb_strlen(tb_option_item_cstr(option, "post-data"));
						if (!tb_gstream_ctrl(ist, TB_STREAM_CTRL_HTTP_SET_METHOD, TB_HTTP_METHOD_POST)) break;
						if (!tb_gstream_ctrl(ist, TB_STREAM_CTRL_HTTP_SET_POST_SIZE, post_size)) break;
						if (debug) tb_printf("post: %llu\n", post_size);
					}
					// post-file?
					else if (tb_option_find(option, "post-file"))
					{
						// exist?
						tb_file_info_t info = {0};
						if (tb_file_info(tb_option_item_cstr(option, "post-file"), &info) && info.type == TB_FILE_TYPE_FILE)
						{
							tb_hize_t post_size = info.size;
							if (!tb_gstream_ctrl(ist, TB_STREAM_CTRL_HTTP_SET_METHOD, TB_HTTP_METHOD_POST)) break;
							if (!tb_gstream_ctrl(ist, TB_STREAM_CTRL_HTTP_SET_POST_SIZE, post_size)) break;
							if (debug) tb_printf("post: %llu\n", post_size);
						}
					}
				}

				// set timeout
				if (tb_option_find(option, "timeout"))
				{
					tb_size_t timeout = tb_option_item_uint32(option, "timeout");
					if (!tb_gstream_ctrl(ist, TB_STREAM_CTRL_SET_TIMEOUT, timeout)) break;
				}

				// print verbose info
				if (verbose) tb_printf("open: %s: ..\n", tb_option_item_cstr(option, "url"));

				// open ist
				if (!tb_gstream_bopen(ist)) 
				{
					// print verbose info
					if (verbose) tb_printf("open: %s\n", tb_stream_state_cstr(tb_gstream_state(ist)));
					break;
				}

				// print verbose info
				if (verbose) tb_printf("open: ok\n");

				// post-data?
				if (tb_option_find(option, "post-data"))
				{
					// the post data
					tb_char_t const* data = tb_option_item_cstr(option, "post-data");

					// flush writing the post data
					if (!tb_gstream_bfwrit(ist, data, tb_strlen(data))) break;
				}
				// post-file?
				else if (tb_option_find(option, "post-file"))
				{
					// init pst
					pst = tb_gstream_init_from_file(tb_option_item_cstr(option, "post-file"), TB_FILE_MODE_RW | TB_FILE_MODE_CREAT | TB_FILE_MODE_BINARY | TB_FILE_MODE_TRUNC);
					tb_assert_and_check_break(pst);

					// writ ist
					if (tb_tstream_save_gg(pst, ist, 0, tb_null, tb_null) < 0) break;
				}

				// init ost
				if (tb_option_find(option, "more0"))
				{
					// the path
					tb_char_t const* path = tb_option_item_cstr(option, "more0");

					// init
					ost = tb_gstream_init_from_file(path, TB_FILE_MODE_RW | TB_FILE_MODE_CREAT | TB_FILE_MODE_BINARY | TB_FILE_MODE_TRUNC);

					// print verbose info
					if (verbose) tb_printf("save: %s\n", path);
				}
				else 
				{
					// the name
					tb_char_t const* name = tb_strrchr(tb_option_item_cstr(option, "url"), '/');
					if (!name) name = tb_strrchr(tb_option_item_cstr(option, "url"), '\\');
					if (!name) name = "/gstream.file";

					// the path
					tb_char_t path[TB_PATH_MAXN] = {0};
					if (tb_directory_curt(path, TB_PATH_MAXN))
						tb_strcat(path, name);
					else break;

					// init file
					ost = tb_gstream_init_from_file(path, TB_FILE_MODE_RW | TB_FILE_MODE_CREAT | TB_FILE_MODE_BINARY | TB_FILE_MODE_TRUNC);

					// print verbose info
					if (verbose) tb_printf("save: %s\n", path);
				}
				tb_assert_and_check_break(ost);

				// the limit rate
				tb_size_t limitrate = 0;
				if (tb_option_find(option, "limitrate"))
					limitrate = tb_option_item_uint32(option, "limitrate");

				// save it
				tb_hong_t 			save = 0;
				tb_hong_t 			base = tb_mclock();
				tb_demo_context_t 	context = {0}; 
				context.verbose 	= verbose;
				if ((save = tb_tstream_save_gg(ist, ost, limitrate, tb_demo_gstream_save_func, &context)) < 0) break;

				// print verbose info
				if (verbose) tb_printf("save: %lld bytes, size: %llu bytes, time: %llu ms, rate: %lu bytes/ s, state: %s\n", save, tb_gstream_size(ist), tb_mclock() - base, context.rate, tb_stream_state_cstr(tb_gstream_state(ist)));
			}
			else tb_option_help(option);
		}
		else tb_option_help(option);

	} while (0);

	// exit pst
	if (pst) tb_gstream_exit(pst);
	pst = tb_null;

	// exit ist
	if (ist) tb_gstream_exit(ist);
	ist = tb_null;

	// exit ost
	if (ost) tb_gstream_exit(ost);
	ost = tb_null;

	// exit option
	if (option) tb_option_exit(option);
	option = tb_null;

	return 0;
}
