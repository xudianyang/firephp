/*
   +----------------------------------------------------------------------+
   | FirePHP 1.0                                                          |     |
   +----------------------------------------------------------------------+
   | Authors: Dianyang Xu <xudianyang@phpboy.net>                         |
   +----------------------------------------------------------------------+
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "main/SAPI.h"
#include "Zend/zend_alloc.h"
#include "ext/standard/info.h"
#include "ext/standard/head.h"
#include "php_firephp.h"

ZEND_DECLARE_MODULE_GLOBALS(firephp)


/** {{{ zend_bool firephp_detect_client(TSRMLS_D)
*/
zend_bool firephp_detect_client(TSRMLS_D)
{
	zval *http_user_agent;
	http_user_agent = firephp_request_query(TRACK_VARS_SERVER, ZEND_STRL("HTTP_USER_AGENT") TSRMLS_CC);
	if (strstr(Z_STRVAL_P(http_user_agent), FIREPHP_HTTP_USER_AGENT_NAME) != NULL) {
		return true;
	} else {
		return false;
	}
}
/* }}} */

/** {{{ zend_bool firephp_check_headers_send(TSRMLS_D)
*/
zend_bool firephp_check_headers_send(TSRMLS_D)
{
	const char *file = "";
	int line = 0;

	if (SG(headers_sent)) {
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 3)) || (PHP_MAJOR_VERSION > 5)
		line = php_output_get_start_lineno(TSRMLS_C);
		file = php_output_get_start_filename(TSRMLS_C);
#else
		line = php_get_output_start_lineno(TSRMLS_C);
		file = php_get_output_start_filename(TSRMLS_C);
#endif
		php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "You Have Already Outputted In File:%s, Line:%d", file, line);
		return false;
	} else {
		return true;
	}
}
/* }}} */

/** {{{ zval * firephp_request_query(uint type, char * name, uint len TSRMLS_DC)
*/
zval * firephp_request_query(uint type, char * name, uint len TSRMLS_DC) {
	zval 		**carrier = NULL, **ret;

#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4)
	zend_bool 	jit_initialization = (PG(auto_globals_jit) && !PG(register_globals) && !PG(register_long_arrays));
#else
	zend_bool 	jit_initialization = PG(auto_globals_jit);
#endif

	switch (type) {
		case TRACK_VARS_POST:
		case TRACK_VARS_GET:
		case TRACK_VARS_FILES:
		case TRACK_VARS_COOKIE:
			carrier = &PG(http_globals)[type];
			break;
		case TRACK_VARS_ENV:
			if (jit_initialization) {
				zend_is_auto_global(ZEND_STRL("_ENV") TSRMLS_CC);
			}
			carrier = &PG(http_globals)[type];
			break;
		case TRACK_VARS_SERVER:
			if (jit_initialization) {
				zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC);
			}
			carrier = &PG(http_globals)[type];
			break;
		case TRACK_VARS_REQUEST:
			if (jit_initialization) {
				zend_is_auto_global(ZEND_STRL("_REQUEST") TSRMLS_CC);
			}
			(void)zend_hash_find(&EG(symbol_table), ZEND_STRS("_REQUEST"), (void **)&carrier);
			break;
		default:
			break;
	}

	if (!carrier || !(*carrier)) {
		zval *empty;
		MAKE_STD_ZVAL(empty);
		ZVAL_NULL(empty);
		return empty;
	}

	if (!len) {
		Z_ADDREF_P(*carrier);
		return *carrier;
	}

	if (zend_hash_find(Z_ARRVAL_PP(carrier), name, len + 1, (void **)&ret) == FAILURE) {
		zval *empty;
		MAKE_STD_ZVAL(empty);
		ZVAL_NULL(empty);
		return empty;
	}

	Z_ADDREF_P(*ret);
	return *ret;
}
/* }}} */

/** {{{ void firephp_encode_array(zval *format_array, zval **carrier TSRMLS_DC)
*/
void firephp_encode_array(zval *format_array, zval **carrier TSRMLS_DC)
{
	zval **arg, *element = NULL;
	HashTable *format_ht;

	MAKE_STD_ZVAL(*carrier);
	array_init(*carrier);
	format_ht = Z_ARRVAL_P(format_array);
	do {
		if (zend_hash_num_elements(format_ht) <= 0) break;
		int key_type;
		char *key;
		uint keylen;
		ulong idx;
		zend_hash_internal_pointer_reset(format_ht);
		for (; zend_hash_has_more_elements(format_ht) == SUCCESS; zend_hash_move_forward(format_ht)) {
			key_type = zend_hash_get_current_key_ex(format_ht, &key, &keylen, &idx, 1, NULL);
			if (zend_hash_get_current_data(format_ht, (void **)&arg) == FAILURE) continue;
			if (key_type == HASH_KEY_IS_STRING) {
				if (strcmp(key, "GLOBALS") == 0 && Z_TYPE_PP(arg) == IS_ARRAY
						&& zend_hash_exists(Z_ARRVAL_PP(arg), "GLOBALS", sizeof("GLOBALS"))) {
					add_assoc_string(*carrier, key, "** Recursion (GLOBALS) **", 1);
				} else {
					element = firephp_encode_data(*arg TSRMLS_CC);
					if (Z_TYPE_P(element) == IS_ARRAY) {
						add_assoc_zval(*carrier, key, element);
					} else if (Z_TYPE_P(element) == IS_STRING){
						add_assoc_string(*carrier, key, Z_STRVAL_P(element), 1);
					}
				}
			} else if (key_type == HASH_KEY_IS_LONG) {
				element = firephp_encode_data(*arg TSRMLS_CC);
				if (Z_TYPE_P(element) == IS_ARRAY) {
					add_index_zval(*carrier, idx, element);
				} else if (Z_TYPE_P(element) == IS_STRING){
					add_index_string(*carrier, idx, Z_STRVAL_P(element), 1);
				}
			}
		}
	} while(0);
}
/* }}} */

/** {{{ void firephp_encode_object(zval *format_object, zval **carrier TSRMLS_DC)
*/
void firephp_encode_object(zval *format_object, zval **carrier TSRMLS_DC)
{
	char *class_name, *carrier_key, *recursion_str, *none_value;
	zend_property_info *property_info;
	zval *prop_value, *element;
	zend_object_handle current_obj_h;
	ulong current_num_index;

	MAKE_STD_ZVAL(*carrier);
	do {
		zend_class_entry *ce = Z_OBJCE_P(format_object);
		class_name = (char *)ce->name;
		current_obj_h = Z_OBJ_HANDLE_P(format_object);
		current_num_index = (ulong) current_obj_h;
		if (zend_hash_index_exists(FIREPHP_G(object_handle_ht), current_num_index)) {
			spprintf(&recursion_str, 0, "** Recursion (%s) **", class_name);
			ZVAL_STRING(*carrier, recursion_str, 1);
			efree(recursion_str);
			break;
		} else {
			zend_hash_index_update(FIREPHP_G(object_handle_ht), current_num_index, &current_obj_h, sizeof(zend_object_handle), NULL);
		}
		array_init(*carrier);
		add_assoc_string(*carrier, "__className", class_name, 1);

		HashTable *properties_info = &ce->properties_info;
		if (zend_hash_num_elements(properties_info) <= 0) break;
		int key_type;
		char *key;
		uint keylen;
		ulong idx;
		zend_hash_internal_pointer_reset(properties_info);
		for (; zend_hash_has_more_elements(properties_info) == SUCCESS;
			zend_hash_move_forward(properties_info)) {
			key_type = zend_hash_get_current_key_ex(properties_info, &key, &keylen, &idx, 1, NULL);
			if (zend_hash_get_current_data(properties_info, (void **)&property_info) == FAILURE) continue;
			if (key_type == HASH_KEY_IS_STRING) {
				if (property_info->flags & ZEND_ACC_STATIC) {
					spprintf(&carrier_key, 0, "static:%s", key);
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 3)) || (PHP_MAJOR_VERSION > 5)
					zend_update_class_constants(ce TSRMLS_CC);
					if (CE_STATIC_MEMBERS(ce)[property_info->offset] && (property_info->flags & ZEND_ACC_PUBLIC)) {
						*prop_value = *CE_STATIC_MEMBERS(ce)[property_info->offset];
					} else {
						spprintf(&none_value, 0, "** You Can't Get %s's Value **", carrier_key);
						add_assoc_string(*carrier, carrier_key, none_value, 1);
						efree(none_value);
						continue;
					}
#else
					zval **member;
					zend_hash_find(ce->static_members, key, strlen(key) + 1, (void **) &member);
					prop_value = *member;
#endif
				} else if (property_info->flags & ZEND_ACC_PUBLIC) {
					spprintf(&carrier_key, 0, "public:%s", key);
					prop_value = zend_read_property(ce, format_object, key, strlen(key), 0 TSRMLS_CC);
				} else if (property_info->flags & ZEND_ACC_PROTECTED) {
					spprintf(&carrier_key, 0, "protected:%s", key);
					prop_value = zend_read_property(ce, format_object, key, strlen(key), 0 TSRMLS_CC);
				} else if (property_info->flags & ZEND_ACC_PRIVATE) {
					spprintf(&carrier_key, 0, "private:%s", key);
					prop_value = zend_read_property(ce, format_object, key, strlen(key), 0 TSRMLS_CC);
				}
				if (prop_value == NULL) continue;

				element = firephp_encode_data(prop_value TSRMLS_CC);
				if (Z_TYPE_P(element) == IS_ARRAY) {
					add_assoc_zval(*carrier, carrier_key, element);
				} else if (Z_TYPE_P(element) == IS_STRING) {
					add_assoc_string(*carrier, carrier_key, Z_STRVAL_P(element), 1);
				}
			}
		}
	} while(0);
}
/* }}} */

/** {{{ zval * firephp_encode_data(zval *format_data TSRMLS_DC)
*/
zval * firephp_encode_data(zval *format_data TSRMLS_DC)
{
	zval *carrier, z_tmp;
	char *c_tmp;

	do {
		switch (Z_TYPE_P(format_data)) {
			case IS_BOOL:{
				MAKE_STD_ZVAL(carrier);
				if (!Z_BVAL_P(format_data)) {
					ZVAL_STRING(carrier, "false", 1);
				} else {
					ZVAL_STRING(carrier, "true", 1);
				}
				break;
			}
			case IS_LONG:
			case IS_DOUBLE:{
				MAKE_STD_ZVAL(carrier);
				z_tmp = *format_data;
				zval_copy_ctor(&z_tmp);
				INIT_PZVAL(&z_tmp);
				convert_to_string(&z_tmp);
				ZVAL_STRING(carrier, Z_STRVAL(z_tmp), 1);
				zval_dtor(&z_tmp);
				break;
			}
			case IS_NULL: {
				MAKE_STD_ZVAL(carrier);
				ZVAL_STRING(carrier, "NULL", 1);
				break;
			}
			case IS_RESOURCE: {
				MAKE_STD_ZVAL(carrier);
				spprintf(&c_tmp, 0, "** Resource ID #%ld **", Z_RESVAL_P(format_data));
				ZVAL_STRING(carrier, c_tmp, 1);
				efree(c_tmp);
				break;
			}
			case IS_STRING: {
				MAKE_STD_ZVAL(carrier);
				if (Z_STRLEN_P(format_data) == 0) {
					ZVAL_STRING(carrier, "** Empty String **", 1);
				} else {
					ZVAL_STRING(carrier, Z_STRVAL_P(format_data), 1);
				}
				break;
			}
			case IS_ARRAY: {
				firephp_encode_array(format_data, &carrier TSRMLS_CC);
				break;
			}
			case IS_OBJECT: {
				firephp_encode_object(format_data, &carrier TSRMLS_CC);
				break;
			}
		}
	} while(0);

	return carrier;
}
/* }}} */

/** {{{ void firephp_set_header(char *header TSRMLS_DC)
*/
void firephp_set_header(char *header TSRMLS_DC)
{
	sapi_header_line ctr = {0};
	ctr.line = header;
	ctr.line_len = strlen(header);
	sapi_header_op(SAPI_HEADER_REPLACE, &ctr TSRMLS_CC);
}
/* }}} */

/** {{{ void firephp_init_object_handle_ht(TSRMLS_D)
*/
void firephp_init_object_handle_ht(TSRMLS_D)
{
    ALLOC_HASHTABLE(FIREPHP_G(object_handle_ht));
    if (zend_hash_init(FIREPHP_G(object_handle_ht), 0, NULL, NULL, 0) == FAILURE) {
    	FREE_HASHTABLE(FIREPHP_G(object_handle_ht));
    }
}
/* }}} */

/** {{{ void firephp_clean_object_handle_ht(TSRMLS_D)
*/
void firephp_clean_object_handle_ht(TSRMLS_D)
{
	zend_hash_destroy(FIREPHP_G(object_handle_ht));
	FREE_HASHTABLE(FIREPHP_G(object_handle_ht));
}
/* }}} */

/** {{{ zval * firephp_split_str_to_array(char *format_str, zend_uint max_len TSRMLS_DC)
*/
zval * firephp_split_str_to_array(char *format_str, zend_uint max_len TSRMLS_DC)
{
	zval *carrier;
	char *elem, *p, *q;
	size_t str_len = strlen(format_str);
	MAKE_STD_ZVAL(carrier);
	array_init(carrier);

	if (max_len <= 0 ) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Function firephp_split_str_to_array Second Parameter Must Be Greater Than 0");
		return carrier;
	}

	if (str_len <= max_len) {
		add_next_index_string(carrier, format_str, 1);
		return carrier;
	}

	elem = (char *)safe_emalloc(max_len + 1, sizeof(char), 0);
	q = elem;
	p = format_str;
	*q = *p;
	q++;
	p++;
	while (*p != '\0') {
		if ((p - format_str) % max_len == 0) {
			*q = '\0';
			add_next_index_string(carrier, elem, 1);
			q -= max_len;
		}
		*q = *p;
		q++;
		p++;
	}
	if (*(q-1)) {
		*q = '\0';
		add_next_index_string(carrier, elem, 1);
	}
	efree(elem);

	return carrier;
}
/* }}} */

/** {{{ char * firephp_json_encode(zval *data TSRMLS_DC)
*/
char * firephp_json_encode(zval *data TSRMLS_DC)
{
	zval *function, *ret = NULL, **params[1];
	char *json_data = "";

	MAKE_STD_ZVAL(function);
	ZVAL_STRING(function, "json_encode", 0);
	params[0] = &data;
	zend_fcall_info fci = {
		sizeof(fci),
		EG(function_table),
		function,
		NULL,
		&ret,
		1,
		(zval ***)params,
		NULL,
		1
	};

	if (zend_call_function(&fci, NULL TSRMLS_CC) == FAILURE) {
		if (ret) {
			zval_ptr_dtor(&ret);
		}
		efree(function);
	} else {
		json_data = Z_STRVAL_P(ret);
	}

	return json_data;
}

double firephp_microtime(TSRMLS_D)
{
	zval *function, *ret = NULL, **params[1], *param;
	double microtime;

	MAKE_STD_ZVAL(function);
	MAKE_STD_ZVAL(param);
	ZVAL_TRUE(param);
	params[0] = &param;
	ZVAL_STRING(function, "microtime", 0);

	zend_fcall_info fci = {
		sizeof(fci),
		EG(function_table),
		function,
		NULL,
		&ret,
		1,
		(zval ***)params,
		NULL,
		1
	};

	if (zend_call_function(&fci, NULL TSRMLS_CC) == FAILURE) {
		if (ret) {
			zval_ptr_dtor(&ret);
		}
		efree(function);
	} else {
		microtime = Z_DVAL_P(ret);
	}
	efree(param);

	return microtime;
}
/* }}} */

/** {{{ void firephp_output_headers(zval *data, int header_total_len TSRMLS_DC)
*/
void firephp_output_headers(zval *data, int header_total_len TSRMLS_DC)
{
	zval **elem;
	HashTable *headers;
	int header_total_num, i;
	char *header;

	headers = Z_ARRVAL_P(data);
	if ((header_total_num = zend_hash_num_elements(headers)) <= 0) return;
	zend_hash_internal_pointer_reset(headers);
	for(i = 0; zend_hash_has_more_elements(headers) == SUCCESS; zend_hash_move_forward(headers), i++) {
		if (zend_hash_get_current_data(headers, (void **)&elem) == FAILURE) continue;
		if (header_total_num >= 2) {
			if (i == 0) {
				spprintf(&header, 0, "X-Wf-1-1-1-%d: %d|%s|%s", FIREPHP_G(message_index),
						header_total_len, Z_STRVAL_PP(elem), "\\");
			} else {
				spprintf(&header, 0, "X-Wf-1-1-1-%d: |%s|%s", FIREPHP_G(message_index)
										 , Z_STRVAL_PP(elem), (i < (header_total_num - 1)) ? "\\" : "");
			}
		}else {
			spprintf(&header, 0, "X-Wf-1-1-1-%d: %d|%s|", FIREPHP_G(message_index), Z_STRLEN_PP(elem), Z_STRVAL_PP(elem));
		}
		firephp_set_header(header TSRMLS_CC);
		FIREPHP_G(message_index)++;
		efree(header);
	}

	spprintf(&header, 0, "X-Wf-1-Index: %d", FIREPHP_G(message_index) - 1);
	firephp_set_header(FIREPHP_PLUGIN_HEADER TSRMLS_CC);
	firephp_set_header(FIREBUG_CONSOLE_HEADER TSRMLS_CC);
	firephp_set_header(FIREPHP_PROTOCOL_JSONSTREAM TSRMLS_CC);
	firephp_set_header(FIREPHP_PROTOCOL_JSONSTREAM TSRMLS_CC);
	firephp_set_header(header TSRMLS_CC);
	efree(header);
}
/* }}} */

/** {{{ proto console(mixed $data);
*/
ZEND_FUNCTION(console)
{
	zval *arg, *format_data, *format_meta, *split_array;
	char *format_str, *format_data_json, *format_meta_json, *lable_str;
	int header_total_len;
	double microtime, pass_time = 0;
	zend_bool lable = false;

	firephp_init_object_handle_ht(TSRMLS_C);
	do {
		if (!firephp_detect_client(TSRMLS_C)) break;
		if (!firephp_check_headers_send(TSRMLS_C)) break;
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|b", &arg, &lable) == FAILURE) break;

		microtime = firephp_microtime(TSRMLS_C);
		if (lable) {
			if (FIREPHP_G(microtime) > 0) {
				pass_time = microtime - FIREPHP_G(microtime);
				spprintf(&lable_str, 0, "%09.5fs", pass_time);
			} else {
				spprintf(&lable_str, 0, "%09.5fs", microtime);
			}
		}
		FIREPHP_G(microtime) = microtime;

		MAKE_STD_ZVAL(format_meta);
		array_init(format_meta);
		add_assoc_string(format_meta, "Type", "INFO", 1);
		if (lable) {
			add_assoc_string(format_meta, "Label", lable_str, 1);
			efree(lable_str);
		}
		format_data = firephp_encode_data(arg TSRMLS_CC);
		format_data_json = firephp_json_encode(format_data TSRMLS_CC);
		format_meta_json = firephp_json_encode(format_meta TSRMLS_CC);
		spprintf(&format_str, 0, "[%s,%s]", format_meta_json, format_data_json);
		efree(format_meta_json);
		efree(format_data_json);
		zval_ptr_dtor(&format_meta);
		zval_ptr_dtor(&format_data);
		split_array = firephp_split_str_to_array(format_str, FIREPHP_HEADER_MAX_LEN TSRMLS_CC);
		header_total_len = strlen(format_str);
		efree(format_str);
		firephp_output_headers(split_array , header_total_len TSRMLS_CC);
		zval_ptr_dtor(&split_array);
		firephp_clean_object_handle_ht(TSRMLS_C);
		RETURN_TRUE;
	} while(0);
	firephp_clean_object_handle_ht(TSRMLS_C);
	RETURN_FALSE;
}
/* }}} */

/* {{{ firephp_functions[]
*/
zend_function_entry firephp_functions[] = {
    ZEND_FE(console, NULL)
    {NULL, NULL, NULL}
};
/* }}} */

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(firephp)
{
    return SUCCESS;
}
/* }}} */

/** {{{ PHP_RINIT_FUNCTION
*/
PHP_RINIT_FUNCTION(firephp)
{
    return SUCCESS;
}
/* }}} */

/** {{{ PHP_RSHUTDOWN_FUNCTION
*/
PHP_RSHUTDOWN_FUNCTION(firephp)
{
	FIREPHP_G(message_index) = 1;
	FIREPHP_G(microtime)	 = 0;
    return SUCCESS;
}
/* }}} */

/** {{{ PHP_MSHUTDOWN_FUNCTION
*/
PHP_MSHUTDOWN_FUNCTION(firephp)
{
    return SUCCESS;
}
/* }}} */

/** {{{ PHP_MINFO_FUNCTION
*/
PHP_MINFO_FUNCTION(firephp)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "FirePHP support", "enabled");
    php_info_print_table_row(2, "Version", FIREPHP_VERSION);
    php_info_print_table_row(2, "Supports", FIREPHP_SUPPORT_URL);
    php_info_print_table_end();
}
/* }}} */

/** {{{ PHP_GINIT_FUNCTION
*/
PHP_GINIT_FUNCTION(firephp)
{
	firephp_globals->object_handle_ht = NULL;
	firephp_globals->message_index    = 1;
	firephp_globals->microtime		  = 0;
}
/* }}} */

/** {{{ DL support
 */
#ifdef COMPILE_DL_FIREPHP
ZEND_GET_MODULE(firephp)
#endif
/* }}} */

/** {{{ module depends
 */
#if ZEND_MODULE_API_NO >= 20050922
zend_module_dep firephp_deps[] = {
	ZEND_MOD_REQUIRED("json")
    {NULL, NULL, NULL}
};
#endif
/* }}} */

/** {{{ firephp_module_entry
*/
zend_module_entry firephp_module_entry = {
#if ZEND_MODULE_API_NO >= 20050922
    STANDARD_MODULE_HEADER_EX, NULL,
    firephp_deps,
#else
    STANDARD_MODULE_HEADER,
#endif
    "firephp",
    firephp_functions,
    PHP_MINIT(firephp),
    PHP_MSHUTDOWN(firephp),
    PHP_RINIT(firephp),
    PHP_RSHUTDOWN(firephp),
    PHP_MINFO(firephp),
    FIREPHP_VERSION,
    PHP_MODULE_GLOBALS(firephp),
    PHP_GINIT(firephp),
    NULL,
    NULL,
    STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */
