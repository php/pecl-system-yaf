/*
   +----------------------------------------------------------------------+
   | Yet Another Framework                                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Laruence<laruence@yahoo.com.cn>                              |
   +----------------------------------------------------------------------+
   $Id$
   */

zend_class_entry * yaf_route_map_ce;

#define YAF_ROUTE_MAP_VAR_NAME_DELIMETER		"_delimeter"
#define YAF_ROUTE_MAP_VAR_NAME_CTL_PREFER	"_ctl_router"

/** {{{ boolean yaf_route_map_route(yaf_route_t *route, yaf_request_t *request TSRMLS_DC)
*/
boolean yaf_route_map_route(yaf_route_t *route, yaf_request_t *request TSRMLS_DC) {
	zval    *ctl_prefer    = NULL;
	zval    *delimer	   = NULL;
	zval	*zuri		   = NULL;
	zval 	*base_uri	   = NULL;
	zval 	*params	 	   = NULL;
	char 	*req_uri	   = NULL;
	char 	*query_str	   = NULL;
	char    *tmp		   = NULL;
	char 	*rest		   = NULL;
	char    *ptrptr		   = NULL;
	char    *seg		   = NULL;
	uint    seg_len		   = 0;
	smart_str route_result = {0};

	zuri 	 = yaf_read_property(request, YAF_REQUEST_PROPERTY_NAME_URI);
	base_uri = yaf_read_property(request, YAF_REQUEST_PROPERTY_NAME_BASE);

	ctl_prefer = yaf_read_property(route, YAF_ROUTE_MAP_VAR_NAME_CTL_PREFER);
	delimer	   = yaf_read_property(route, YAF_ROUTE_MAP_VAR_NAME_DELIMETER);

	if (base_uri && IS_STRING == Z_TYPE_P(base_uri)
			&& strstr(Z_STRVAL_P(zuri), Z_STRVAL_P(base_uri)) == Z_STRVAL_P(zuri)) {
		req_uri  = estrdup(Z_STRVAL_P(zuri) + Z_STRLEN_P(base_uri));
	} else {
		req_uri  = estrdup(Z_STRVAL_P(zuri));
	}

	if (Z_TYPE_P(delimer) == IS_STRING 
			&& Z_STRLEN_P(delimer)) { 
		if ((query_str = strstr(req_uri, Z_STRVAL_P(delimer))) != NULL 
			&& *(query_str - 1) == '/') {
			tmp  = req_uri;
			rest = query_str + Z_STRLEN_P(delimer);
			if (*rest == '\0') {
				req_uri 	= estrndup(req_uri, query_str - req_uri);
				query_str 	= NULL;
				efree(tmp);
			} else if (*rest == '/') {
				req_uri 	= estrndup(req_uri, query_str - req_uri);
				query_str   = estrdup(rest);
				efree(tmp);
			} else {
				query_str = NULL;
			}
		}
	}

	seg = php_strtok_r(req_uri, YAF_ROUTER_URL_DELIMIETER, &ptrptr);
	while (seg) {
		seg_len = strlen(seg);
		if (seg_len) {
			smart_str_appendl(&route_result, seg, seg_len);
		}
		smart_str_appendc(&route_result, '_');
		seg = php_strtok_r(NULL, YAF_ROUTER_URL_DELIMIETER, &ptrptr);
	}

	if (route_result.len) {
		if (Z_BVAL_P(ctl_prefer)) {
			zend_update_property_stringl(Z_OBJCE_P(request), request,  YAF_STRL(YAF_REQUEST_PROPERTY_NAME_CONTROLLER), route_result.c, route_result.len - 1 TSRMLS_CC);
		} else {
			zend_update_property_stringl(Z_OBJCE_P(request), request,  YAF_STRL(YAF_REQUEST_PROPERTY_NAME_ACTION), route_result.c, route_result.len - 1 TSRMLS_CC);
		}
		efree(route_result.c);
	}

	if (query_str) {
		params = yaf_router_parse_parameters(query_str TSRMLS_CC);
		(void)yaf_request_set_params_multi(request, params TSRMLS_CC);
		efree(query_str);
	}

	efree(req_uri);

	return TRUE;
}
/* }}} */

/** {{{ proto public Yaf_Route_Simple::route(Yaf_Request $req)
*/
PHP_METHOD(yaf_route_map, route) {
	yaf_request_t *request = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &request) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {
		RETURN_BOOL(yaf_route_map_route(getThis(), request TSRMLS_CC));
	}
}
/* }}} */

/** {{{ proto public Yaf_Route_Simple::__construct(boolean $controller_prefer=FALSE, string $delimer = '#!')
*/
PHP_METHOD(yaf_route_map, __construct) {
	long controller_prefer 	= 0;
	char *delim		   		= NULL;
	uint delim_len	   		= 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ls",
			   	&controller_prefer, &delim, &delim_len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (controller_prefer) {
		zend_update_property_bool(yaf_route_map_ce, getThis(),
				YAF_STRL(YAF_ROUTE_MAP_VAR_NAME_CTL_PREFER), TRUE TSRMLS_CC);
	}

	if (delim && delim_len) {
		zend_update_property_stringl(yaf_route_map_ce, getThis(), 
				YAF_STRL(YAF_ROUTE_MAP_VAR_NAME_DELIMETER), delim, delim_len TSRMLS_CC);
	}
}
/* }}} */

/** {{{ yaf_route_map_methods
*/
zend_function_entry yaf_route_map_methods[] = {
	PHP_ME(yaf_route_map, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(yaf_route_map, route, yaf_getter_arg, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ YAF_STARTUP_FUNCTION
*/
YAF_STARTUP_FUNCTION(route_map) {
	zend_class_entry ce;

	YAF_INIT_CLASS_ENTRY(ce, "Yaf_Route_Map", "Yaf\\Route\\Map", yaf_route_map_methods);
	yaf_route_map_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	zend_class_implements(yaf_route_map_ce TSRMLS_CC, 1, yaf_route_ce);

	yaf_route_map_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_bool(yaf_route_map_ce, YAF_STRL(YAF_ROUTE_MAP_VAR_NAME_CTL_PREFER), FALSE, ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_route_map_ce, YAF_STRL(YAF_ROUTE_MAP_VAR_NAME_DELIMETER),  ZEND_ACC_PROTECTED TSRMLS_CC);

	return SUCCESS;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4 
 */
