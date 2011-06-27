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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_API.h"
#include "Zend/zend_interfaces.h"

#include "php_yaf.h"
#include "yaf_namespace.h"
#include "yaf_request.h"
#include "yaf_response.h"
#include "yaf_dispatcher.h"
#include "yaf_view.h"
#include "yaf_exception.h"
#include "yaf_action.h"
#include "yaf_controller.h"

zend_class_entry * yaf_controller_ce;

/** {{{ zval * yaf_controller_render(yaf_controller_t *controller, char *action_name, int len, zval *var_array TSRMLS_DC)
 */
zval * yaf_controller_render(yaf_controller_t *controller, char *action_name, int len, zval *var_array TSRMLS_DC) {
	char 	*path		= NULL;
	int 	path_len	= 0;
	char 	*view_ext	= NULL;
	char    *self_name  = NULL;
	char 	*tmp		= NULL;
	zval 	*name		= NULL;
	zval 	*param		= NULL;
	zval    *ret		= NULL;
	yaf_view_t *view 	= NULL;

	view   	  = yaf_read_property(controller, YAF_CONTROLLER_PROPERTY_NAME_VIEW);
	name	  = yaf_read_property(controller, YAF_CONTROLLER_PROPERTY_NAME_NAME);
	view_ext  = YAF_G(view_ext);

	self_name = zend_str_tolower_dup(Z_STRVAL_P(name), Z_STRLEN_P(name));

	tmp = self_name;
 	while (*tmp != '\0') {
		if (*tmp == '_') {
			*tmp = DEFAULT_SLASH;
		}
		tmp++;
	}

	action_name = estrndup(action_name, len);

	tmp = action_name;
 	while (*tmp != '\0') {
		if (*tmp == '_') {
			*tmp = DEFAULT_SLASH;
		}
		tmp++;
	}

	path_len  = spprintf(&path, 0, "%s%c%s.%s", self_name, DEFAULT_SLASH, action_name, view_ext);

	efree(self_name);
	efree(action_name);

	MAKE_STD_ZVAL(param);
	ZVAL_STRING(param, path, FALSE);

	if (var_array) {
		zend_call_method_with_2_params(&view, Z_OBJCE_P(view), NULL, "render", &ret, param, var_array);
	} else {
		zend_call_method_with_1_params(&view, Z_OBJCE_P(view), NULL, "render", &ret, param);
	}

	zval_dtor(param);
	efree(param);

	if (!ret || (Z_TYPE_P(ret) == IS_BOOL && !Z_BVAL_P(ret))) {
		return NULL;
	}

	return ret;
}
/* }}} */

/** {{{ boolean yaf_controller_display(yaf_controller_t *controller, char *action_name, int len, zval *var_array TSRMLS_DC)
 */
boolean yaf_controller_display(yaf_controller_t *controller, char *action_name, int len, zval *var_array TSRMLS_DC) {
	char 	*path		= NULL;
	int 	path_len	= 0;
	char 	*view_ext	= NULL;
	char    *self_name  = NULL;
	zval 	*name		= NULL;
	zval 	*param		= NULL;
	zval    *ret		= NULL;

	yaf_view_t		*view 		= NULL;

	view   	  = yaf_read_property(controller, YAF_CONTROLLER_PROPERTY_NAME_VIEW);
	name	  = yaf_read_property(controller, YAF_CONTROLLER_PROPERTY_NAME_NAME);
	view_ext  = YAF_G(view_ext);
		
	self_name = zend_str_tolower_dup(Z_STRVAL_P(name), Z_STRLEN_P(name));

	path_len  = spprintf(&path, 0, "%s%c%s.%s", self_name, DEFAULT_SLASH, action_name, view_ext);

	MAKE_STD_ZVAL(param);
	ZVAL_STRING(param, path, FALSE);

	if (var_array) {
		zend_call_method_with_2_params(&view, Z_OBJCE_P(view), NULL, "display", &ret, param, var_array);
	} else {
		zend_call_method_with_1_params(&view, Z_OBJCE_P(view), NULL, "display", &ret, param);
	}

	zval_dtor(param);
	efree(param);

	if (!ret || (Z_TYPE_P(ret) == IS_BOOL && !Z_BVAL_P(ret))) {
		return FALSE;
	}

	return TRUE;
}
/* }}} */

/** {{{ boolean yaf_controller_construct(yaf_controller_t *self, yaf_request_t *request, yaf_response_t *responseew_t *view, zval *args TSRMLS_DC)
 */
boolean yaf_controller_construct(yaf_controller_t *self, yaf_request_t *request, yaf_response_t *response, yaf_view_t *view, zval *args TSRMLS_DC) {
	zval *module = NULL;

	if (args) {
		yaf_update_property(self, YAF_CONTROLLER_PROPERTY_NAME_ARGS, args);
	}

	module = yaf_read_property(request, YAF_REQUEST_PROPERTY_NAME_MODULE);

	yaf_update_property(self, YAF_CONTROLLER_PROPERTY_NAME_REQUEST,  request);
	yaf_update_property(self, YAF_CONTROLLER_PROPERTY_NAME_RESPONSE, response);
	yaf_update_property(self, YAF_CONTROLLER_PROPERTY_NAME_MODULE,   module);
	yaf_update_property(self, YAF_CONTROLLER_PROPERTY_NAME_VIEW,	   view);

	if (!instanceof_function(Z_OBJCE_P(self), yaf_action_ce TSRMLS_CC) 
			&& zend_hash_exists(&Z_OBJCE_P(self)->function_table, YAF_STRS("init"))) {
		zend_call_method_with_0_params(&self, Z_OBJCE_P(self), NULL, "init", NULL);
	}

	return TRUE;
}
/* }}} */

/** {{{ proto public Yaf_Controller_Abstract::init()
*/
PHP_METHOD(yaf_controller, init) {
}

/* }}} */

/** {{{ proto protected Yaf_Controller_Abstract::__construct(Yaf_Request_Abstract $request, Yaf_Response_abstrct $response, Yaf_View_Interface $view, array $invokeArgs = NULL)
*/
PHP_METHOD(yaf_controller, __construct) {
	yaf_request_t 	*request 	= NULL;
	yaf_response_t	*response	= NULL;
	yaf_view_t		*view		= NULL;
	zval 			*invoke_arg = NULL;
	yaf_controller_t	*self		= getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ooo|z", 
				&request, yaf_request_ce, &response, yaf_response_ce, &view, yaf_view_interface_ce, &invoke_arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else	{
		if(!yaf_controller_construct(self, request, response, view, invoke_arg TSRMLS_CC)) {
			RETURN_FALSE;
		}
	}
}
/* }}} */

/** {{{ proto public Yaf_Controller_Abstract::getView(void)
*/
PHP_METHOD(yaf_controller, getView) {
	yaf_view_t	*view = NULL;
	
	view = yaf_read_property(getThis(), YAF_CONTROLLER_PROPERTY_NAME_VIEW);

	RETURN_ZVAL(view, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Controller_Abstract::getRequest(void)
*/
PHP_METHOD(yaf_controller, getRequest) {
	yaf_request_t *request = yaf_read_property(getThis(), YAF_CONTROLLER_PROPERTY_NAME_REQUEST);
	RETURN_ZVAL(request, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Controller_Abstract::getResponse(void)
*/
PHP_METHOD(yaf_controller, getResponse) {
	yaf_view_t *response = yaf_read_property(getThis(), YAF_CONTROLLER_PROPERTY_NAME_RESPONSE);
	RETURN_ZVAL(response, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Controller_Abstract::initView(array $options = NULL)
*/
PHP_METHOD(yaf_controller, initView) {
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Controller_Abstract::getInvokeArg(string $name)
 */
PHP_METHOD(yaf_controller, getInvokeArg) {
	char *name	= NULL;
	int	 len	= 0;
	zval *args	= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",  &name, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (len) {
		zval **ppzval = NULL;
		args = yaf_read_property(getThis(), YAF_CONTROLLER_PROPERTY_NAME_ARGS);

		if (ZVAL_IS_NULL(args)) {
			RETURN_NULL();
		}

		if (zend_hash_find(Z_ARRVAL_P(args), name, len + 1, (void **)&ppzval) == SUCCESS) {
			RETURN_ZVAL(*ppzval, 1, 0);
		}
	}
	RETURN_NULL();
}
/* }}} */

/** {{{ proto public Yaf_Controller_Abstract::getInvokeArgs(void)
 */
PHP_METHOD(yaf_controller, getInvokeArgs) {
	zval *args	= NULL;

	args = yaf_read_property(getThis(), YAF_CONTROLLER_PROPERTY_NAME_ARGS);

	RETURN_ZVAL(args, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Controller_Abstract::getModuleName(void)
 */
PHP_METHOD(yaf_controller, getModuleName) {
	zval *module = yaf_read_property(getThis(), YAF_CONTROLLER_PROPERTY_NAME_MODULE);

	RETURN_ZVAL(module, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Controller_Abstract::setViewpath
*/
PHP_METHOD(yaf_controller, setViewpath) {
	zval *path 		= NULL;
	yaf_view_t *view = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &path) == FAILURE 
			|| Z_TYPE_P(path) != IS_STRING) {
		WRONG_PARAM_COUNT;
	}

	view = yaf_read_property(getThis(), YAF_CONTROLLER_PROPERTY_NAME_VIEW);
	if (Z_OBJCE_P(view) == yaf_view_simple_ce) {
		yaf_update_property(view, YAF_VIEW_PROPERTY_NAME_TPLDIR, path);
	} else {
		zend_call_method_with_1_params(&view, Z_OBJCE_P(view), NULL, "setscriptpath", NULL, path);
	}

	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Yaf_Controller_Abstract::getViewpath
*/
PHP_METHOD(yaf_controller, getViewpath) {
	zval *view = yaf_read_property(getThis(), YAF_CONTROLLER_PROPERTY_NAME_VIEW);
	if (Z_OBJCE_P(view) == yaf_view_simple_ce) {
		zval *tpl_dir = yaf_read_property(view, YAF_VIEW_PROPERTY_NAME_TPLDIR);
		RETURN_ZVAL(tpl_dir, 1, 0);
	} else {
		zval *ret = NULL;
		zend_call_method_with_0_params(&view, Z_OBJCE_P(view), NULL, "getscriptpath", &ret);
		RETURN_ZVAL(ret, 1, 0);
	}
}
/* }}} */

/** {{{ proto public Yaf_Controller_Abstract::forward
*/
PHP_METHOD(yaf_controller, forward) {
	zval  			*controller = NULL;
	zval			*module		= NULL;
	zval 			*action		= NULL;
	zval 			*args		= NULL;
	zval 			*parameters = NULL;
	yaf_request_t 	*request	= NULL;
	yaf_controller_t	*self		= getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|zzz", &module, &controller, &action, &args) == FAILURE) {
		WRONG_PARAM_COUNT;
	}	

	request    = yaf_read_property(self, YAF_CONTROLLER_PROPERTY_NAME_REQUEST);
	parameters = yaf_read_property(self, YAF_CONTROLLER_PROPERTY_NAME_ARGS);

	if (ZVAL_IS_NULL(parameters)) {
		MAKE_STD_ZVAL(parameters);
		array_init(parameters);
	}

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (Z_TYPE_P(module) != IS_STRING) {
				yaf_trigger_error(YAF_ERR_ERROR, "%s:forward expects a string action name", Z_OBJCE_P(self)->name);
				zval_dtor(parameters);
				efree(parameters);
				RETURN_FALSE;
			}
			Z_ADDREF_P(module);
			yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_ACTION, module);
			break;
		case 2:
			if (Z_TYPE_P(controller) ==  IS_STRING) {
				Z_ADDREF_P(module);
				Z_ADDREF_P(controller);
				yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_CONTROLLER, module);
				yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_ACTION, controller);
			} else if (Z_TYPE_P(controller) == IS_ARRAY) {
				Z_ADDREF_P(module);
				Z_ADDREF_P(controller);
				zend_hash_copy(Z_ARRVAL_P(parameters), Z_ARRVAL_P(controller), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
				yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_ACTION, module);
				yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_PARAMS, parameters);
			} else {
				zval_dtor(parameters);
				efree(parameters);
				RETURN_FALSE;
			}
			break;
		case 3:
			if (Z_TYPE_P(action) == IS_STRING) {
				Z_ADDREF_P(module);
				Z_ADDREF_P(controller);
				Z_ADDREF_P(action);
				yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_MODULE, module);
				yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_CONTROLLER, controller);
				yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_ACTION, action);
			} else if (Z_TYPE_P(action) == IS_ARRAY) {
				Z_ADDREF_P(module);
				Z_ADDREF_P(controller);
				Z_ADDREF_P(action);
				zend_hash_copy(Z_ARRVAL_P(parameters), Z_ARRVAL_P(action), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
				yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_CONTROLLER, module);
				yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_ACTION, controller);
				yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_PARAMS, parameters);
			} else {
				zval_dtor(parameters);
				efree(parameters);
				RETURN_FALSE;
			}
			break;
		case 4:
			if (Z_TYPE_P(args) != IS_ARRAY) {
				yaf_trigger_error(YAF_ERR_ERROR, "parameters must be an array");
				zval_dtor(parameters);
				efree(parameters);
				RETURN_FALSE;
			}
			Z_ADDREF_P(module);
			Z_ADDREF_P(controller);
			Z_ADDREF_P(action);
			Z_ADDREF_P(args);
			zend_hash_copy(Z_ARRVAL_P(parameters), Z_ARRVAL_P(args), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
			yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_MODULE, module);
			yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_CONTROLLER, controller);
			yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_ACTION, action);
			yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_PARAMS, parameters);
			break;
	}

	(void)yaf_request_set_dispatched(request, FALSE TSRMLS_CC);
	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Yaf_Controller_Abstract::redirect(string $url)
*/
PHP_METHOD(yaf_controller, redirect) {
	yaf_response_t *response = NULL;
	char *location; int location_len;
	yaf_controller_t *self	= getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &location, &location_len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}	

	response = yaf_read_property(self, YAF_CONTROLLER_PROPERTY_NAME_RESPONSE);

	yaf_response_set_redirect(response, location, location_len TSRMLS_CC);

	RETURN_TRUE;
}
/* }}} */

/** {{{ proto protected Yaf_Controller_Abstract::render(string $action, array $var_array = NULL)
*/
PHP_METHOD(yaf_controller, render) {
	char *action_name		= NULL;
	int   action_name_len	= 0;
	zval *var_array			= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|z", &action_name, &action_name_len, &var_array) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {
		zval *output = yaf_controller_render(getThis(), action_name, action_name_len, var_array TSRMLS_CC);
		if (output) {
			RETURN_ZVAL(output, 1, 0);
		} else {
			RETURN_FALSE;
		}
	}
}
/* }}} */

/** {{{ proto protected Yaf_Controller_Abstract::display(string $action, array $var_array = NULL)
*/
PHP_METHOD(yaf_controller, display) {
	char *action_name		= NULL;
	int   action_name_len	= 0;
	zval *var_array			= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|z", &action_name, &action_name_len, &var_array) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {
		RETURN_BOOL(yaf_controller_display(getThis(), action_name, action_name_len, var_array TSRMLS_CC));
	}
}
/* }}} */

/** {{{ proto private Yaf_Controller_Abstract::__clone()
*/
PHP_METHOD(yaf_controller, __clone) {
}
/* }}} */

/** {{{ yaf_controller_methods 
*/
zend_function_entry yaf_controller_methods[] = {
	PHP_ME(yaf_controller, __construct, 	NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PUBLIC)
	/* PHP_ME(yaf_controller, init, 		NULL, ZEND_ACC_PUBLIC) */
	PHP_ME(yaf_controller, __clone, 		NULL, ZEND_ACC_PRIVATE|ZEND_ACC_FINAL)
	PHP_ME(yaf_controller, render,	    NULL, ZEND_ACC_PROTECTED|ZEND_ACC_FINAL)
	PHP_ME(yaf_controller, display,	    NULL, ZEND_ACC_PROTECTED|ZEND_ACC_FINAL)
	PHP_ME(yaf_controller, getRequest,	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(yaf_controller, getResponse,	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(yaf_controller, getModuleName,NULL, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(yaf_controller, getView,		NULL, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(yaf_controller, initView,		NULL, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(yaf_controller, setViewpath,	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(yaf_controller, getViewpath,	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(yaf_controller, forward,	   	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(yaf_controller, redirect,    	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(yaf_controller, getInvokeArgs,NULL, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(yaf_controller, getInvokeArg, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ YAF_STARTUP_FUNCTION
*/
YAF_STARTUP_FUNCTION(controller) {
	zend_class_entry ce;
	YAF_INIT_CLASS_ENTRY(ce, "Yaf_Controller_Abstract", "Yaf\\Controller_Abstract", yaf_controller_methods);
	yaf_controller_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	yaf_controller_ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

	zend_declare_property_null(yaf_controller_ce, YAF_STRL(YAF_CONTROLLER_PROPERTY_NAME_ACTIONS),	ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(yaf_controller_ce, YAF_STRL(YAF_CONTROLLER_PROPERTY_NAME_MODULE), 	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_controller_ce, YAF_STRL(YAF_CONTROLLER_PROPERTY_NAME_NAME), 	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_controller_ce, YAF_STRL(YAF_CONTROLLER_PROPERTY_NAME_REQUEST),	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_controller_ce, YAF_STRL(YAF_CONTROLLER_PROPERTY_NAME_RESPONSE),	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_controller_ce, YAF_STRL(YAF_CONTROLLER_PROPERTY_NAME_ARGS),		ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_controller_ce, YAF_STRL(YAF_CONTROLLER_PROPERTY_NAME_VIEW),		ZEND_ACC_PROTECTED TSRMLS_CC);

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
