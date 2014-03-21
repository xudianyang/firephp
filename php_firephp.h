/*
   +----------------------------------------------------------------------+
   | FirePHP 1.0                                                          |     |
   +----------------------------------------------------------------------+
   | Authors: Dianyang Xu <xudianyang@phpboy.net>                         |
   +----------------------------------------------------------------------+
 */
/* $Id$ */

#ifndef FIREPHP_H
#define FIREPHP_H

extern zend_module_entry firephp_module_entry;
#define phpext_firephp_ptr &firephp_module_entry

#ifdef ZTS
#include "TSRM.h"
#endif

#ifdef ZTS
#define FIREPHP_G(v) TSRMG(firephp_globals_id, zend_firephp_globals *, v)
#else
#define FIREPHP_G(v) (firephp_globals.v)
#endif

#ifndef true
#define true  1
#endif
#ifndef false
#define false 0
#endif

#define FIREPHP_VERSION					"1.0.5"
#define FIREPHP_HEADER_MAX_LEN			5000
#define FIREPHP_SUPPORT_URL				"http://www.phpboy.net/"
#define FIREPHP_PLUGIN_HEADER			"X-Wf-1-Plugin-1: http://meta.firephp.org/Wildfire/Plugin/FirePHP/Library-FirePHPCore/0.2.1"
#define FIREBUG_CONSOLE_HEADER			"X-Wf-1-Structure-1: http://meta.firephp.org/Wildfire/Structure/FirePHP/FirebugConsole/0.1"
#define FIREPHP_PROTOCOL_JSONSTREAM		"X-Wf-Protocol-1: http://meta.wildfirehq.org/Protocol/JsonStream/0.2"
#define FIREPHP_HTTP_USER_AGENT_NAME	"FirePHP"

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION <= 2))
#define Z_SET_REFCOUNT_P(pz, rc)      (pz)->refcount = rc
#define Z_SET_REFCOUNT_PP(ppz, rc)    Z_SET_REFCOUNT_P(*(ppz), rc)
#define Z_ADDREF_P 	 ZVAL_ADDREF
#define Z_REFCOUNT_P ZVAL_REFCOUNT
#define Z_DELREF_P 	 ZVAL_DELREF
#endif

zval * firephp_request_query(uint type, char * name, uint len TSRMLS_DC);
zend_bool firephp_detect_client(TSRMLS_D);
zend_bool firephp_check_headers_send(TSRMLS_D);
void firephp_encode_array(zval *format_array, zval **carrier TSRMLS_DC);
zval * firephp_encode_data(zval *format_data TSRMLS_DC);
zval * firephp_split_str_to_array(char *format_str, zend_uint max_len TSRMLS_DC);
void firephp_set_header(char *header_str TSRMLS_DC);
char * firephp_json_encode(zval *data TSRMLS_DC);
void firephp_output_headers(zval *data, int header_total_len TSRMLS_DC);
void firephp_init_object_handle_ht(TSRMLS_D);
void firephp_clean_object_handle_ht(TSRMLS_D);

ZEND_BEGIN_MODULE_GLOBALS(firephp)

   HashTable *object_handle_ht;
   int message_index;
   double microtime;

ZEND_END_MODULE_GLOBALS(firephp)

PHP_MINIT_FUNCTION(firephp);
PHP_MSHUTDOWN_FUNCTION(firephp);
PHP_RINIT_FUNCTION(firephp);
PHP_RSHUTDOWN_FUNCTION(firephp);
PHP_MINFO_FUNCTION(firephp);

ZEND_EXTERN_MODULE_GLOBALS(firephp)

#endif
