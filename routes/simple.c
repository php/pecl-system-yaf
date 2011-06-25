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
   $Id: simple.c 575 2011-03-08 11:12:08Z huixinchen $
 */

zend_class_entry * yaf_route_simple_ce;

#define YAF_ROUTE_SIMPLE_VAR_NAME_MODULE			"module"
#define	YAF_ROUTE_SIMPLE_VAR_NAME_CONTROLLER 	"controller"
#define YAF_ROUTE_SIMPLE_VAR_NAME_ACTION			"action"

/** {{{ boolean yaf_route_simple_route(yaf_route_t *route, yaf_request_t *request TSRMLS_DC)
 */
boolean yaf_route_simple_route(yaf_route_t *route, yaf_request_t *request TSRMLS_DC) {
	zval 	*module 	= NULL;
	zval 	*controller	= NULL;
	zval 	*action		= NULL;

	zval 	*nmodule	= NULL;
	zval 	*ncontroller= NULL;
	zval 	*naction	= NULL;

	nmodule = yaf_read_property(route, YAF_ROUTE_SIMPLE_VAR_NAME_MODULE);
	ncontroller = yaf_read_property(route, YAF_ROUTE_SIMPLE_VAR_NAME_CONTROLLER);
	naction = yaf_read_property(route, YAF_ROUTE_SIMPLE_VAR_NAME_ACTION);

	/* if there is no expect parameter in supervars, then null will be return */
	module 		= yaf_request_query(YAF_GLOBAL_VARS_GET, Z_STRVAL_P(nmodule), Z_STRLEN_P(nmodule) TSRMLS_CC);
	controller 	= yaf_request_query(YAF_GLOBAL_VARS_GET, Z_STRVAL_P(ncontroller), Z_STRLEN_P(ncontroller) TSRMLS_CC);
	action 		= yaf_request_query(YAF_GLOBAL_VARS_GET, Z_STRVAL_P(naction), Z_STRLEN_P(naction) TSRMLS_CC);

	if (ZVAL_IS_NULL(module) && ZVAL_IS_NULL(controller) && ZVAL_IS_NULL(action)) {
		return FALSE;
	}

	yaf_update_property(request, YAF_ROUTE_SIMPLE_VAR_NAME_MODULE, 	 module);
	yaf_update_property(request, YAF_ROUTE_SIMPLE_VAR_NAME_CONTROLLER, controller);
	yaf_update_property(request, YAF_ROUTE_SIMPLE_VAR_NAME_ACTION, 	 action);

	return TRUE;
}
/* }}} */

/** {{{ yaf_route_t * yaf_route_simple_instance(yaf_route_t *this_ptr, zval *module, zval *controller, zval *action TSRMLS_DC)
 */
yaf_route_t * yaf_route_simple_instance(yaf_route_t *this_ptr, zval *module, zval *controller, zval *action TSRMLS_DC) {
	yaf_route_t *instance = NULL;

	if (this_ptr) {
		instance  = this_ptr;
	} else {
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, yaf_route_simple_ce);
	}

	yaf_update_property(instance, YAF_ROUTE_SIMPLE_VAR_NAME_MODULE, 		module);
	yaf_update_property(instance, YAF_ROUTE_SIMPLE_VAR_NAME_CONTROLLER, 	controller);
	yaf_update_property(instance, YAF_ROUTE_SIMPLE_VAR_NAME_ACTION,	 	action);

	return instance;
}
/* }}} */

/** {{{ proto public Yaf_Route_Simple::route(Yaf_Request $req)
*/
PHP_METHOD(yaf_route_simple, route) {
	yaf_request_t *request = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &request) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {
		RETURN_BOOL(yaf_route_simple_route(getThis(), request TSRMLS_CC));
	}
}
/* }}} */

/** {{{ proto public Yaf_Route_Simple::__construct(string $module, string $controller, string $action)
 */
PHP_METHOD(yaf_route_simple, __construct) {
	zval	*module 	= NULL;
	zval 	*controller = NULL;
	zval 	*action		= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zzz", &module, &controller, &action) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (IS_STRING != Z_TYPE_P(module)
			|| IS_STRING != Z_TYPE_P(controller)
			|| IS_STRING != Z_TYPE_P(action)) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::__construct expects 3 string paramsters", yaf_route_simple_ce->name);
		RETURN_FALSE;
	} else {
		yaf_router_t *self = getThis();
		yaf_update_property(self, YAF_ROUTE_SIMPLE_VAR_NAME_MODULE, 		module);
		yaf_update_property(self, YAF_ROUTE_SIMPLE_VAR_NAME_CONTROLLER, 	controller);
		yaf_update_property(self, YAF_ROUTE_SIMPLE_VAR_NAME_ACTION,	 	action);
	}
}
/* }}} */

/** {{{ yaf_route_simple_methods
 */
zend_function_entry yaf_route_simple_methods[] = {
	PHP_ME(yaf_route_simple, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(yaf_route_simple, route, yaf_getter_arg, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ YAF_STARTUP_FUNCTION
 */
YAF_STARTUP_FUNCTION(route_simple) {
	zend_class_entry ce;

	YAF_INIT_CLASS_ENTRY(ce, "Yaf_Route_Simple", "Yaf\\Route\\Simple", yaf_route_simple_methods);
	yaf_route_simple_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	zend_class_implements(yaf_route_simple_ce TSRMLS_CC, 1, yaf_route_ce);

	yaf_route_simple_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_null(yaf_route_simple_ce, YAF_STRL(YAF_ROUTE_SIMPLE_VAR_NAME_CONTROLLER), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_route_simple_ce, YAF_STRL(YAF_ROUTE_SIMPLE_VAR_NAME_MODULE), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_route_simple_ce, YAF_STRL(YAF_ROUTE_SIMPLE_VAR_NAME_ACTION), ZEND_ACC_PROTECTED TSRMLS_CC);

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
