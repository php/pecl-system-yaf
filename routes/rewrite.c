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
   $Id: rewrite.c 722 2011-03-14 10:51:14Z huixinchen $
 */

zend_class_entry * yaf_route_rewrite_ce;

/** {{{ yaf_route_t * yaf_route_rewrite_instance(yaf_route_t *this_ptr, zval *match, zval *router, zval *verify TSRMLS_DC)
 */
yaf_route_t * yaf_route_rewrite_instance(yaf_route_t *this_ptr, zval *match, zval *route, zval *verify TSRMLS_DC) {
	yaf_route_t	*instance = NULL;

	if (this_ptr) {
		instance = this_ptr;
	} else {
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, yaf_route_rewrite_ce);
	}

	Z_ADDREF_P(match);
	Z_ADDREF_P(route);
	yaf_update_property(instance, YAF_ROUTE_PROPETY_NAME_MATCH, match);
	yaf_update_property(instance, YAF_ROUTE_PROPETY_NAME_ROUTE, route);

	if (!verify) {
		zend_update_property_null(Z_OBJCE_P(instance), instance, YAF_STRL(YAF_ROUTE_PROPETY_NAME_VERIFY) TSRMLS_CC);
	} else {
		yaf_update_property(instance, YAF_ROUTE_PROPETY_NAME_VERIFY, verify);
	}

	return instance;
}
/* }}} */

/** {{{ zval * yaf_route_rewrite_match(yaf_route_t *router, char *uir, int len TSRMLS_DC)
 */
zval * yaf_route_rewrite_match(yaf_route_t *router, char *uir, int len TSRMLS_DC) {
	char *seg 			= NULL;
	char *pmatch		= NULL;
	char *ptrptr		= NULL;
	int  seg_len		= 0;
	zval *match			= NULL;
	smart_str pattern 	= {0};
	pcre_cache_entry *pce_regexp = NULL;

	if (!len) {
		return FALSE;
	}
	
	match  = yaf_read_property(router, YAF_ROUTE_PROPETY_NAME_MATCH);
	pmatch = estrndup(Z_STRVAL_P(match), Z_STRLEN_P(match));
	
	smart_str_appendc(&pattern, YAF_ROUTE_REGEX_DILIMITER);
	smart_str_appendc(&pattern, '^');

	seg = php_strtok_r(pmatch, YAF_ROUTER_URL_DELIMIETER, &ptrptr);
	while (seg) {
		seg_len = strlen(seg);
		if (seg_len) {
			smart_str_appendl(&pattern, YAF_ROUTER_URL_DELIMIETER, 1);

			if(*(seg) == '*') {
				smart_str_appendl(&pattern, "(?P<__yaf_route_rest>.*)", sizeof("(?P<__yaf_route_rest>.*)") -1);
				break;
			}		

			if(*(seg) == ':') {
				smart_str_appendl(&pattern, "(?P<", sizeof("(?P<") -1 );
				smart_str_appendl(&pattern, seg + 1, seg_len - 1);
				smart_str_appendl(&pattern, ">[^"YAF_ROUTER_URL_DELIMIETER"]+)", sizeof(">[^"YAF_ROUTER_URL_DELIMIETER"]+)") - 1);
			} else {
				smart_str_appendl(&pattern, seg, seg_len);
			}

		}
		seg = php_strtok_r(NULL, YAF_ROUTER_URL_DELIMIETER, &ptrptr);
	}

	efree(pmatch);
	smart_str_appendc(&pattern, YAF_ROUTE_REGEX_DILIMITER);
	smart_str_appendc(&pattern, 'i');
	smart_str_0(&pattern);
	
	if ((pce_regexp = pcre_get_compiled_regex_cache(pattern.c, pattern.len TSRMLS_CC)) == NULL) {
		smart_str_free(&pattern);
		return NULL;
	} else {
		zval *matches  = NULL;
		zval *subparts = NULL;

		smart_str_free(&pattern);

		MAKE_STD_ZVAL(matches);
		MAKE_STD_ZVAL(subparts);
		ZVAL_LONG(matches, 0);
		ZVAL_NULL(subparts);

		php_pcre_match_impl(pce_regexp, uir, len, matches, subparts /* subpats */,
				0/* global */, 0/* ZEND_NUM_ARGS() >= 4 */, 0/*flags PREG_OFFSET_CAPTURE*/, 0/* start_offset */ TSRMLS_CC);

		if (!matches || !Z_LVAL_P(matches)) {
			return NULL;
		} else {
			HashTable 	*ht  	 = NULL;
			char		*key 	 = NULL;
			zval 		**ppzval = NULL;
			int			len  	 = 0;
			long		idx	 	 = 0;
			zval 		*ret 	 = NULL;

			MAKE_STD_ZVAL(ret);
			array_init(ret);

			ht = Z_ARRVAL_P(subparts);

			for(zend_hash_internal_pointer_reset(ht);
					zend_hash_has_more_elements(ht) == SUCCESS;
					zend_hash_move_forward(ht)) {

				if (zend_hash_get_current_key_type(ht) != HASH_KEY_IS_STRING) {
					continue;
				}

				zend_hash_get_current_key_ex(ht, &key, &len, &idx, 0, NULL);
				if (zend_hash_get_current_data(ht, (void**)&ppzval) == FAILURE) {
					continue;
				}
				
				if (!strncmp(key, "__yaf_route_rest", len)) {
					zval *args = yaf_router_parse_parameters(Z_STRVAL_PP(ppzval) TSRMLS_CC);
					if (args) {
						zend_hash_copy(Z_ARRVAL_P(ret), Z_ARRVAL_P(args), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
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

/** {{{ boolean yaf_route_rewrite_route(yaf_route_t *router, yaf_request_t *request TSRMLS_DC)
 */
boolean yaf_route_rewrite_route(yaf_route_t *router, yaf_request_t *request TSRMLS_DC) {
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

	if (!(args = yaf_route_rewrite_match(router, request_uri, strlen(request_uri) TSRMLS_CC))) {
		return FALSE;
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
		return TRUE;
	}

}
/* }}} */

/** {{{ proto public Yaf_Route_Rewrite::route(Yaf_Request_Abstarct $request)
 */
PHP_METHOD(yaf_route_rewrite, route) {
	yaf_route_t 		*route   = NULL;
	yaf_request_t 	*request = NULL;

	route = getThis();

	RETVAL_FALSE;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &request) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (!request
			|| IS_OBJECT != Z_TYPE_P(request)
			|| !instanceof_function(Z_OBJCE_P(request), yaf_request_ce TSRMLS_CC)) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::route expects a %s instance",
				yaf_route_rewrite_ce->name, yaf_request_ce->name);
		RETURN_FALSE;
	}

	RETURN_BOOL(yaf_route_rewrite_route(route, request TSRMLS_CC));
}
/** }}} */

/** {{{ proto public Yaf_Route_Rewrite::match(string $uri)
 */
PHP_METHOD(yaf_route_rewrite, match) {
	char *uri		= NULL;
	int	 len		= 0;
	zval *matches 	= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &uri, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (!len) RETURN_FALSE;

	if ((matches = yaf_route_rewrite_match(getThis(), uri, len TSRMLS_CC))) {
		RETURN_ZVAL(matches, 1, 0);
	}

	RETURN_FALSE;
}
/** }}} */

/** {{{ proto public Yaf_Route_Rewrite::__construct(string $match, array $route, array $verify = NULL)
 */
PHP_METHOD(yaf_route_rewrite, __construct) {
	zval 		*match 		= NULL;
	zval 		*route		= NULL;
	zval 		*verify		= NULL;
	yaf_route_t	*self 		= getThis();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "za|a", &match, &route, &verify) ==  FAILURE) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::__construct expects an array as third paramter",  yaf_route_rewrite_ce->name);
		WRONG_PARAM_COUNT;
	}
	
	if (!match || IS_STRING != Z_TYPE_P(match) || !Z_STRLEN_P(match)) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::__construct expects a string as the first parameter", yaf_route_rewrite_ce->name);
		RETURN_FALSE;
	}

	if (verify && IS_ARRAY != Z_TYPE_P(verify)) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::__construct expects an array as third parameter",  yaf_route_rewrite_ce->name);
		RETURN_FALSE;
	}

	(void)yaf_route_rewrite_instance(self, match, route, verify TSRMLS_CC);

	if (self) {
		RETURN_ZVAL(self, 1, 0);
	}

	RETURN_FALSE;
}
/** }}} */
 
/** {{{ yaf_route_rewrite_methods
 */
zend_function_entry yaf_route_rewrite_methods[] = {
	PHP_ME(yaf_route_rewrite, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(yaf_route_rewrite, route,		  yaf_getter_arg, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};
/* }}} */

/** {{{ YAF_STARTUP_FUNCTION
 */
YAF_STARTUP_FUNCTION(route_rewrite) {
	zend_class_entry ce;
	YAF_INIT_CLASS_ENTRY(ce, "Yaf_Route_Rewrite", "Yaf\\Route\\Rewrite", yaf_route_rewrite_methods);
	yaf_route_rewrite_ce = zend_register_internal_class_ex(&ce, yaf_route_ce, NULL TSRMLS_CC);
	zend_class_implements(yaf_route_rewrite_ce TSRMLS_CC, 1, yaf_route_ce);
	yaf_route_rewrite_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_null(yaf_route_rewrite_ce, YAF_STRL(YAF_ROUTE_PROPETY_NAME_MATCH),  ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_route_rewrite_ce, YAF_STRL(YAF_ROUTE_PROPETY_NAME_ROUTE),  ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_route_rewrite_ce, YAF_STRL(YAF_ROUTE_PROPETY_NAME_VERIFY), ZEND_ACC_PROTECTED TSRMLS_CC);

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

