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
  | Author: Xinchen Hui  <laruence@php.net>                              |
  +----------------------------------------------------------------------+
   $Id$
 */

zend_class_entry * yaf_route_regex_ce;

/** {{{ yaf_route_t * yaf_route_regex_instance(yaf_route_t *this_ptr, zval *route, zval *def, zval *map, zval *verify TSRMLS_DC)
 */
yaf_route_t * yaf_route_regex_instance(yaf_route_t *this_ptr, zval *route, zval *def, zval *map, zval *verify TSRMLS_DC) {
	yaf_route_t	*instance = NULL;

	if (this_ptr) {
		instance = this_ptr;
	} else {
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, yaf_route_regex_ce);
	}

	Z_ADDREF_P(route);
	Z_ADDREF_P(def);
	Z_ADDREF_P(map);
	yaf_update_property(instance, YAF_ROUTE_PROPETY_NAME_MATCH, route);
	yaf_update_property(instance, YAF_ROUTE_PROPETY_NAME_ROUTE, def);
	yaf_update_property(instance, YAF_ROUTE_PROPETY_NAME_MAP,   map);

	if (!verify) {
		zend_update_property_null(Z_OBJCE_P(instance), instance, YAF_STRL(YAF_ROUTE_PROPETY_NAME_VERIFY) TSRMLS_CC);
	} else {
		yaf_update_property(instance, YAF_ROUTE_PROPETY_NAME_VERIFY, verify);
	}

	return instance;
}
/* }}} */

/** {{{ zval * yaf_route_regex_match(yaf_route_t *router, char *uir, int len TSRMLS_DC)
 */
zval * yaf_route_regex_match(yaf_route_t *route, char *uir, int len TSRMLS_DC) {
	zval *match			= NULL;
	pcre_cache_entry *pce_regexp = NULL;

	if (!len) {
		return 0;
	}
	
	match 	 = yaf_read_property(route, YAF_ROUTE_PROPETY_NAME_MATCH);

	if ((pce_regexp = pcre_get_compiled_regex_cache(Z_STRVAL_P(match), Z_STRLEN_P(match) TSRMLS_CC)) == NULL) {
		return NULL;
	} else {
		zval *matches  = NULL;
		zval *subparts = NULL;
		zval *map	   = NULL;

		MAKE_STD_ZVAL(matches);
		MAKE_STD_ZVAL(subparts);
		ZVAL_NULL(subparts);
		
		map = yaf_read_property(route, YAF_ROUTE_PROPETY_NAME_MAP);

		php_pcre_match_impl(pce_regexp, uir, len, matches, subparts /* subpats */,
				0/* global */, 0/* ZEND_NUM_ARGS() >= 4 */, 0/*flags PREG_OFFSET_CAPTURE*/, 0/* start_offset */ TSRMLS_CC);

		if (!Z_LVAL_P(matches)) {
			return NULL;
		} else {
			HashTable 	*ht  	 = NULL;
			char		*key 	 = NULL;
			zval 		**ppzval = NULL;
			int			len  	 = 0;
			long		idx	 	 = 0;
			zval 		*ret 	 = NULL;
			zval 		**name   = NULL;

			MAKE_STD_ZVAL(ret);
			array_init(ret);

			ht = Z_ARRVAL_P(subparts);

			for(zend_hash_internal_pointer_reset(ht);
					zend_hash_has_more_elements(ht) == SUCCESS;
					zend_hash_move_forward(ht)) {

				if (zend_hash_get_current_data(ht, (void**)&ppzval) == FAILURE) {
					continue;
				}

				if (zend_hash_get_current_key_ex(ht, &key, &len, &idx, 0, NULL) == HASH_KEY_IS_LONG) {
					if (zend_hash_index_find(Z_ARRVAL_P(map), idx, (void **)&name) == SUCCESS) {
						zend_hash_update(Z_ARRVAL_P(ret), Z_STRVAL_PP(name), Z_STRLEN_PP(name) + 1, 
								(void **)ppzval, sizeof(zval *), NULL);
					}
				} else {
					zend_hash_update(Z_ARRVAL_P(ret), key, len, (void **)ppzval, sizeof(zval *), NULL);
				}
			}

			return ret;
		}
	}

	return NULL;
}
/* }}} */

/** {{{ int yaf_route_regex_route(yaf_route_t *router, yaf_request_t *request TSRMLS_DC)
 */
int yaf_route_regex_route(yaf_route_t *router, yaf_request_t *request TSRMLS_DC) {
	char *request_uri = NULL;
	zval *args		  = NULL;
	zval *base_uri	  = NULL;
	zval *zuri		  = NULL;

	zuri 	 = yaf_read_property(request, YAF_REQUEST_PROPERTY_NAME_URI);

	base_uri = yaf_read_property(request, YAF_REQUEST_PROPERTY_NAME_BASE);

	if (base_uri && IS_STRING == Z_TYPE_P(base_uri)
			&& strstr(Z_STRVAL_P(zuri), Z_STRVAL_P(base_uri)) == Z_STRVAL_P(zuri)) {
		request_uri  = estrdup(Z_STRVAL_P(zuri) + Z_STRLEN_P(base_uri));
	} else {
		request_uri  = estrdup(Z_STRVAL_P(zuri));
	}

	if (!(args = yaf_route_regex_match(router, request_uri, strlen(request_uri) TSRMLS_CC))) {
		return 0;
	} else {
		zval **module 		= NULL;
		zval **controller 	= NULL;
		zval **action		= NULL;
		zval *routes		= NULL;

		routes = yaf_read_property(router, YAF_ROUTE_PROPETY_NAME_ROUTE);
		if (zend_hash_find(Z_ARRVAL_P(routes), YAF_STRS("module"), (void **)&module) == SUCCESS) {
			yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_MODULE, *module);
		}

		if (zend_hash_find(Z_ARRVAL_P(routes), YAF_STRS("controller"), (void **)&controller) == SUCCESS) {
			yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_CONTROLLER, *controller);
		}

		if (zend_hash_find(Z_ARRVAL_P(routes), YAF_STRS("action"), (void **)&action) == SUCCESS) {
			yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_ACTION, *action);
		}

		(void)yaf_request_set_params_multi(request, args TSRMLS_CC);
	}

	return 1;
}
/* }}} */

/** {{{ proto public Yaf_Route_Regex::route(string $uri)
 */
PHP_METHOD(yaf_route_regex, route) {
	yaf_route_t 		*route   = NULL;
	yaf_request_t 	*request = NULL;

	route = getThis();

	RETVAL_0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &request) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (!request
			|| IS_OBJECT != Z_TYPE_P(request)
			|| !instanceof_function(Z_OBJCE_P(request), yaf_request_ce TSRMLS_CC)) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::route expects a %s instance",
				yaf_route_regex_ce->name, yaf_request_ce->name);
		RETURN_FALSE;
	}

	RETURN_BOOL(yaf_route_regex_route(route, request TSRMLS_CC));
}
/** }}} */

/** {{{ proto public Yaf_Route_Regex::__construct(string $route, $default, $map, array $verify = NULL)
 */
PHP_METHOD(yaf_route_regex, __construct) {
	zval 		*match 		= NULL;
	zval 		*route		= NULL;
	zval 		*map		= NULL;
	zval 		*verify		= NULL;
	yaf_route_t	*self 		= getThis();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zaa|a", &match, &route, &map, &verify) ==  FAILURE) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::__construct expects an array as third paramter",  yaf_route_regex_ce->name);
		WRONG_PARAM_COUNT;
	}
	
	if (!match || IS_STRING != Z_TYPE_P(match) || !Z_STRLEN_P(match)) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::__construct expects a string as the first parameter", yaf_route_regex_ce->name);
		RETURN_FALSE;
	}

	if (map && IS_ARRAY != Z_TYPE_P(map)) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::__construct expects an array as third parameter",  yaf_route_regex_ce->name);
		RETURN_FALSE;
	}

	if (verify && IS_ARRAY != Z_TYPE_P(verify)) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::__construct expects an array as verify parmater",  yaf_route_regex_ce->name);
		RETURN_FALSE;
	}

	(void)yaf_route_regex_instance(self, match, route, map, verify TSRMLS_CC);

	if (self) {
		RETURN_ZVAL(self, 1, 0);
	}

	RETURN_FALSE;
}
/** }}} */
 
/** {{{ yaf_route_regex_methods
 */
zend_function_entry yaf_route_regex_methods[] = {
	PHP_ME(yaf_route_regex, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(yaf_route_regex, route,		  yaf_getter_arg, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};
/* }}} */

/** {{{ YAF_STARTUP_FUNCTION
 */
YAF_STARTUP_FUNCTION(route_regex) {
	zend_class_entry ce;
	YAF_INIT_CLASS_ENTRY(ce, "Yaf_Route_Regex", "Yaf\\Route\\Regex", yaf_route_regex_methods);
	yaf_route_regex_ce = zend_register_internal_class_ex(&ce, yaf_route_ce, NULL TSRMLS_CC);
	zend_class_implements(yaf_route_regex_ce TSRMLS_CC, 1, yaf_route_ce);
	yaf_route_regex_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_null(yaf_route_regex_ce, YAF_STRL(YAF_ROUTE_PROPETY_NAME_MATCH),  ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_route_regex_ce, YAF_STRL(YAF_ROUTE_PROPETY_NAME_ROUTE),  ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_route_regex_ce, YAF_STRL(YAF_ROUTE_PROPETY_NAME_MAP),    ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_route_regex_ce, YAF_STRL(YAF_ROUTE_PROPETY_NAME_VERIFY), ZEND_ACC_PROTECTED TSRMLS_CC);

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

