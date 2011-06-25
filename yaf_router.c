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
   $Id: yaf_router.c 51 2011-05-13 10:06:11Z laruence $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_alloc.h"
#include "Zend/zend_interfaces.h"
#include "ext/pcre/php_pcre.h"

#include "php_yaf.h"
#include "yaf_namespace.h"
#include "yaf_application.h"
#include "yaf_exception.h"
#include "yaf_request.h"
#include "yaf_router.h"
#include "yaf_config.h"
#include "routes/interface.c"

zend_class_entry * yaf_router_ce;

/** {{{ yaf_router_t * yaf_router_instance(yaf_router_t *this_ptr TSRMLS_DC)
 */
yaf_router_t * yaf_router_instance(yaf_router_t *this_ptr TSRMLS_DC) {
	zval 		*routes   = NULL;
	yaf_router_t *instance = NULL;
	yaf_route_t	*route	  = NULL;
	
	if (this_ptr) {
		instance = this_ptr;
	} else {
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, yaf_router_ce);
	}

	MAKE_STD_ZVAL(routes);
	array_init(routes);

	MAKE_STD_ZVAL(route);

	object_init_ex(route, yaf_route_static_ce);

	zend_hash_update(Z_ARRVAL_P(routes), "_default", sizeof("_default"), (void **)&route, sizeof(zval *), NULL);

	yaf_update_property(instance, YAF_ROUTER_PROPERTY_NAME_ROUTERS, routes);

	return instance;
}
/** }}} */

/** {{{ boolean yaf_router_route(yaf_router_t *router, yaf_request_t *request TSRMLS_DC)
*/
boolean yaf_router_route(yaf_router_t *router, yaf_request_t *request TSRMLS_DC) {
	zval 		*routers = NULL;
	zval 		*ret	 = NULL;
	HashTable 	*ht 	 = NULL;
	yaf_route_t 	**route  = NULL;

	routers = yaf_read_property(router, YAF_ROUTER_PROPERTY_NAME_ROUTERS);
	ht = Z_ARRVAL_P(routers);

	for(zend_hash_internal_pointer_end(ht);
			zend_hash_has_more_elements(ht) == SUCCESS;
			zend_hash_move_backwards(ht)) {

		if (zend_hash_get_current_data(ht, (void**)&route) == FAILURE) {
			continue;
		}

		zend_call_method_with_1_params(route, Z_OBJCE_PP(route), NULL, "route", &ret, request);

		if (IS_BOOL != Z_TYPE_P(ret) || !Z_BVAL_P(ret)) {
			continue;
		} else {
			char *key = NULL;
			int  len  = 0;
			long idx  = 0;

			zend_hash_get_current_key_ex(ht, &key, &len, &idx, 0, NULL);

			if (len) {
				zend_update_property_string(Z_OBJCE_P(router), router, YAF_STRL(YAF_ROUTER_PROPERTY_NAME_CURRENT_ROUTE), key TSRMLS_CC);
			}
			yaf_request_set_routed(request, TRUE TSRMLS_CC);
			return TRUE;
		}
	} 

	return FALSE;
}
/* }}} */

/** {{{ boolean yaf_router_add_config(yaf_router_t *router, zval *configs TSRMLS_DC)
*/
boolean yaf_router_add_config(yaf_router_t *router, zval *configs TSRMLS_DC) {
	HashTable 	*ht 	= NULL;
	yaf_route_t 	*route  = NULL;
	zval 		**entry	= NULL;

	if (!configs || IS_ARRAY != Z_TYPE_P(configs)) {
		return FALSE;
	} else {
		char *key = NULL;
		int	 len  = 0;
		long idx  = 0;

		ht = Z_ARRVAL_P(configs);

		for(zend_hash_internal_pointer_reset(ht);
				zend_hash_has_more_elements(ht) == SUCCESS;
				zend_hash_move_forward(ht)) {

			if (zend_hash_get_current_key_ex(ht, &key, &len, &idx, 0, NULL) != HASH_KEY_IS_STRING) {
				continue;
			}

			if (zend_hash_get_current_data(ht, (void**)&entry) == FAILURE) {
				continue;
			}

			if (!entry || Z_TYPE_PP(entry) != IS_ARRAY) {
				continue;
			}

			route = yaf_route_instance(NULL, *entry TSRMLS_CC);

			if (route) {
				zval *routes = yaf_read_property(router, YAF_ROUTER_PROPERTY_NAME_ROUTERS);
				zend_hash_update(Z_ARRVAL_P(routes), key, len, (void **)&route, sizeof(zval *), NULL);
			}
		} 
		return TRUE;
	}
}
/* }}} */

/** {{{ zval * yaf_router_parse_parameters(char *uri TSRMLS_DC)
 */
zval * yaf_router_parse_parameters(char *uri TSRMLS_DC) {
	char *key 		= NULL;
	char *ptrptr	= NULL;
	char *tmp		= NULL;
	char *value		= NULL;
	zval *params    = NULL;
	char *slash     = YAF_ROUTER_URL_DELIMIETER;
	int	 key_len	= 0;
	zval *val		= NULL;

	MAKE_STD_ZVAL(params);
	array_init(params);

	tmp = estrdup(uri);
	key = php_strtok_r(tmp, slash, &ptrptr);
	while (key) {
		key_len = strlen(key);
		if (key_len) {
			MAKE_STD_ZVAL(val);
			value = php_strtok_r(NULL, slash, &ptrptr);
			if (value && strlen(value)) {
				ZVAL_STRING(val, value, 1);
			} else {
				ZVAL_NULL(val);
			}
			zend_hash_update(Z_ARRVAL_P(params), key, key_len + 1, (void **)&val, sizeof(zval *), NULL);
		}

		key = php_strtok_r(NULL, slash, &ptrptr);
	}

	efree(tmp);

	return params;
}
/* }}} */

/** {{{ proto public Yaf_Router::__construct(void)
 */
PHP_METHOD(yaf_router, __construct) {
	yaf_router_instance(getThis() TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yaf_Router::route(Yaf_Request $req)
*/
PHP_METHOD(yaf_router, route) {
	yaf_request_t *request = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &request) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {
		RETURN_BOOL(yaf_router_route(getThis(), request TSRMLS_CC));
	}
}
/* }}} */

/** {{{  proto public Yaf_Router::addRoute(string $name, Yaf_Route_Interface $route)
 */
PHP_METHOD(yaf_router, addRoute) {
	char 	   *name	= NULL;
	int		   len		= 0;
	yaf_route_t *route	= NULL;
	zval 	   *routes 	= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &len, &route) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (!len) {
		RETURN_FALSE;
	}

	if (IS_OBJECT != Z_TYPE_P(route)
			|| !instanceof_function(Z_OBJCE_P(route), yaf_route_ce TSRMLS_CC)) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::addRoute expects a %s instance", yaf_router_ce->name, yaf_route_ce->name);
		RETURN_FALSE;
	}

	routes = yaf_read_property(getThis(), YAF_ROUTER_PROPERTY_NAME_ROUTERS);

	Z_ADDREF_P(route);	
	zend_hash_update(Z_ARRVAL_P(routes), name, len + 1, (void **)&route, sizeof(zval *), NULL);

	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/** {{{  proto public Yaf_Router::addConfig(Yaf_Config_Abstract $config)
 */
PHP_METHOD(yaf_router, addConfig) {
	yaf_config_t *config = NULL;
	zval		*routes = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &config) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (IS_OBJECT != Z_TYPE_P(config)
			|| !instanceof_function(Z_OBJCE_P(config), yaf_config_ce TSRMLS_CC)) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::addConfig expects a %s instance", yaf_router_ce->name, yaf_config_ce->name);
		RETURN_FALSE;
	}


	Z_ADDREF_P(config);
	routes = yaf_read_property(config, YAF_CONFIG_PROPERT_NAME);

	if (yaf_router_add_config(getThis(), routes TSRMLS_CC)) {
		RETURN_ZVAL(getThis(), 1, 0);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/** {{{  proto public Yaf_Router::getRoute(string $name)
 */
PHP_METHOD(yaf_router, getRoute) {
	char 	   *name	= NULL;
	int		   len		= 0;
	yaf_route_t **route	= NULL;
	zval 	   *routes 	= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (!len) {
		RETURN_FALSE;
	}

	routes = yaf_read_property(getThis(), YAF_ROUTER_PROPERTY_NAME_ROUTERS);
	
	if (zend_hash_find(Z_ARRVAL_P(routes), name, len + 1, (void **)&route) == SUCCESS) {
		RETURN_ZVAL(*route, 1, 0);
	}

	RETURN_NULL();
}
/* }}} */

/** {{{  proto public Yaf_Router::getRoutes(void)
 */
PHP_METHOD(yaf_router, getRoutes) {
	zval *routes = NULL;

	routes = yaf_read_property(getThis(), YAF_ROUTER_PROPERTY_NAME_ROUTERS);

	RETURN_ZVAL(routes, 1, 0);	
}
/* }}} */

/** {{{ proto public Yaf_Router::isModuleName(string $name) 
 */
PHP_METHOD(yaf_router, isModuleName) {
	char *name = NULL;
	int  len   = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	RETURN_BOOL(yaf_application_is_module_name(name, len TSRMLS_CC));
}
/* }}} */

/** {{{  proto public Yaf_Router::getCurrentRoute(void)
 */
PHP_METHOD(yaf_router, getCurrentRoute) {
	zval *route = NULL;

	route = yaf_read_property(getThis(), YAF_ROUTER_PROPERTY_NAME_CURRENT_ROUTE);

	RETURN_ZVAL(route, 1, 0);	
}
/* }}} */

/** {{{ yaf_router_methods
 */
zend_function_entry yaf_router_methods[] = {
	PHP_ME(yaf_router, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(yaf_router, addRoute,  NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_router, addConfig, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_router, route,	 NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_router, getRoute,  NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_router, getRoutes, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_router, getCurrentRoute, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ YAF_STARTUP_FUNCTION
 */
YAF_STARTUP_FUNCTION(router) {
	zend_class_entry ce;

	YAF_INIT_CLASS_ENTRY(ce, "Yaf_Router", "Yaf\\Router", yaf_router_methods);
	yaf_router_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);

	yaf_router_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_null(yaf_router_ce, YAF_STRL(YAF_ROUTER_PROPERTY_NAME_ROUTERS), 		 ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_router_ce, YAF_STRL(YAF_ROUTER_PROPERTY_NAME_CURRENT_ROUTE), ZEND_ACC_PROTECTED TSRMLS_CC);

	YAF_STARTUP(route);

	return SUCCESS;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
