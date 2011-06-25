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
   $Id: yaf_request.c 51 2011-05-13 10:06:11Z laruence $
   */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_interfaces.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_alloc.h"
#include "ext/standard/php_string.h"

#include "php_yaf.h"
#include "yaf_request.h"
#include "yaf_namespace.h"
#include "yaf_exception.h"

#include "requests/simple.c"
#include "requests/http.c"

zend_class_entry * yaf_request_ce;

/** {{{ yaf_request_t * yaf_request_instance(zval *this_ptr, char *other TSRMLS_DC)
*/
yaf_request_t * yaf_request_instance(yaf_request_t *this_ptr, char *other TSRMLS_DC) {
	yaf_request_t * instance = NULL;

	instance = yaf_request_http_instance(this_ptr, NULL, other TSRMLS_CC);

	return instance;
}
/* }}} */

/** {{{ boolean yaf_request_set_base_uri(yaf_request_t *request, char *base_uri, char *request_uri TSRMLS_DC)
*/
boolean yaf_request_set_base_uri(yaf_request_t *request, char *base_uri, char *request_uri TSRMLS_DC) {
	char *basename 	  = NULL;
	int  basename_len = 0;

	if (!base_uri) {
		zval *script_filename 	= NULL;
		char *file_name			= NULL;
		char *ext				= YAF_G(ext);
		int  ext_len			= 0;
		size_t  file_name_len	= 0;

		ext_len	= strlen(ext);

		script_filename = yaf_request_query(YAF_GLOBAL_VARS_SERVER, YAF_STRL("SCRIPT_FILENAME") TSRMLS_CC);

		do {
			if (script_filename && IS_STRING == Z_TYPE_P(script_filename)) {
				zval *script_name 	= NULL;
				zval *phpself_name  = NULL;
				zval *orig_name     = NULL;
				script_name = yaf_request_query(YAF_GLOBAL_VARS_SERVER, YAF_STRL("SCRIPT_NAME") TSRMLS_CC);

				php_basename(Z_STRVAL_P(script_filename), Z_STRLEN_P(script_filename), 
						ext, ext_len, &file_name, &file_name_len TSRMLS_CC);

				if (script_name && IS_STRING == Z_TYPE_P(script_name)) {
					char 	*script 	= NULL;
					size_t 	script_len 	= 0;

					php_basename(Z_STRVAL_P(script_name), Z_STRLEN_P(script_name), 
							NULL, 0, &script, &script_len TSRMLS_CC);

					if (strncmp(file_name, script, file_name_len) == 0) {
						basename 	 = Z_STRVAL_P(script_name);
						basename_len = Z_STRLEN_P(script_name);
						efree(script);
						break;
					}

					efree(script);
				}

				phpself_name = yaf_request_query(YAF_GLOBAL_VARS_SERVER, YAF_STRL("PHP_SELF") TSRMLS_CC);

				if (phpself_name && IS_STRING == Z_TYPE_P(phpself_name)) {
					char 	*phpself 	= NULL;
					size_t	phpself_len = 0;

					php_basename(Z_STRVAL_P(phpself_name), Z_STRLEN_P(phpself_name),
						   	NULL, 0, &phpself, &phpself_len TSRMLS_CC);

					if (strncmp(file_name, phpself, file_name_len) == 0) {
						efree(phpself);
						basename	 = Z_STRVAL_P(phpself_name);
						basename_len = Z_STRLEN_P(phpself_name);
						break;
					}

					efree(phpself);
				}

				orig_name = yaf_request_query(YAF_GLOBAL_VARS_SERVER, YAF_STRL("PHP_SELF") TSRMLS_CC);

				if (orig_name && IS_STRING == Z_TYPE_P(orig_name)) {
					char 	*orig 	= NULL;
					size_t	orig_len= 0;
					php_basename(Z_STRVAL_P(orig_name), Z_STRLEN_P(orig_name), NULL, 0, &orig, &orig_len TSRMLS_CC);
					if (strncmp(file_name, orig, file_name_len) == 0) {
						efree(orig);
						basename 	 = Z_STRVAL_P(orig_name);
						basename_len = Z_STRLEN_P(orig_name);
						break;
					}

					efree(orig);
				}
			}
		} while (0);

		if (basename && strstr(request_uri, basename) == request_uri) {
			if (*(basename + basename_len - 1) == '/') {
				--basename_len;
			}
			zend_update_property_stringl(Z_OBJCE_P(request), request,
				   	YAF_STRL(YAF_REQUEST_PROPERTY_NAME_BASE), basename, basename_len TSRMLS_CC);
			return TRUE;
		} else if (basename) {
			char 	*dir 	= NULL;
			size_t  dir_len = 0;

			dir_len = php_dirname(basename, basename_len);
			if (*(basename + dir_len - 1) == '/') {
				--dir_len;
			} 

			if (dir_len) {
				dir = estrndup(basename, dir_len);
				if (strstr(request_uri, dir) == request_uri) {
					zend_update_property_string(Z_OBJCE_P(request), request,
						   	YAF_STRL(YAF_REQUEST_PROPERTY_NAME_BASE), dir TSRMLS_CC);
					efree(dir);
					return TRUE;
				}
				efree(dir);
			}
		}

		zend_update_property_string(Z_OBJCE_P(request), request,
			   	YAF_STRL(YAF_REQUEST_PROPERTY_NAME_BASE), "" TSRMLS_CC);
		return TRUE;
	} else {
		zend_update_property_string(Z_OBJCE_P(request), request, 
					YAF_STRL(YAF_REQUEST_PROPERTY_NAME_BASE), base_uri TSRMLS_CC);
		return TRUE;
	}
}
/* }}} */

/** {{{ zval * yaf_request_query(uint type, char * name, uint len TSRMLS_DC)
*/
zval * yaf_request_query(uint type, char * name, uint len TSRMLS_DC) {
	zval **carrier = NULL;
	zval **ret	   = NULL;

	zend_bool jit_initialization = (PG(auto_globals_jit) && !PG(register_globals) && !PG(register_long_arrays));

	/* for phpunit test requirements */
#if PHP_YAF_DEBUG
	switch (type) {
		case YAF_GLOBAL_VARS_POST:
			(void)zend_hash_find(&EG(symbol_table), YAF_STRS("_POST"), (void **)&carrier);
			break;
		case YAF_GLOBAL_VARS_GET:
			(void)zend_hash_find(&EG(symbol_table), YAF_STRS("_GET"), (void **)&carrier);
			break;
		case YAF_GLOBAL_VARS_COOKIE:
			(void)zend_hash_find(&EG(symbol_table), YAF_STRS("_COOKIE"), (void **)&carrier);
			break;
		case YAF_GLOBAL_VARS_SERVER:
			if (jit_initialization) {
				zend_is_auto_global(YAF_STRL("_SERVER") TSRMLS_CC);
			} 
			(void)zend_hash_find(&EG(symbol_table), YAF_STRS("_SERVER"), (void **)&carrier);
			break;
		case YAF_GLOBAL_VARS_ENV:
			if (jit_initialization) {
				zend_is_auto_global(YAF_STRL("_ENV") TSRMLS_CC);
			} 
			carrier = &PG(http_globals)[YAF_GLOBAL_VARS_ENV];
			break;
		case YAF_GLOBAL_VARS_FILES:
			carrier = &PG(http_globals)[YAF_GLOBAL_VARS_FILES];
			break;
		case YAF_GLOBAL_VARS_REQUEST:
			if (jit_initialization) {
				zend_is_auto_global(YAF_STRL("_REQUEST") TSRMLS_CC);
			} 
			(void)zend_hash_find(&EG(symbol_table), YAF_STRS("_REQUEST"), (void **)&carrier);
			break;
		default:
			break;
	}
#else 
	switch (type) {
		case YAF_GLOBAL_VARS_POST:
		case YAF_GLOBAL_VARS_GET:
		case YAF_GLOBAL_VARS_FILES:
		case YAF_GLOBAL_VARS_COOKIE:
			carrier = &PG(http_globals)[type];
			break;
		case YAF_GLOBAL_VARS_ENV:
			if (jit_initialization) {
				zend_is_auto_global(YAF_STRL("_ENV") TSRMLS_CC);
			} 
			carrier = &PG(http_globals)[type];
			break;
		case YAF_GLOBAL_VARS_SERVER:
			if (jit_initialization) {
				zend_is_auto_global(YAF_STRL("_SERVER") TSRMLS_CC);
			} 
			carrier = &PG(http_globals)[type];
			break;
		case YAF_GLOBAL_VARS_REQUEST:
			if (jit_initialization) {
				zend_is_auto_global(YAF_STRL("_REQUEST") TSRMLS_CC);
			} 
			(void)zend_hash_find(&EG(symbol_table), YAF_STRS("_REQUEST"), (void **)&carrier);
			break;
		default:
			break;
	}
#endif

	if (!carrier || !(*carrier)) {
		zval *empty = NULL;
		MAKE_STD_ZVAL(empty);
		ZVAL_NULL(empty);
		return empty;
	}

	if (!len) {
		return *carrier;
	}

	if (zend_hash_find(Z_ARRVAL_PP(carrier), name, len + 1, (void **)&ret) == FAILURE ){
		zval *empty = NULL;
		MAKE_STD_ZVAL(empty);
		ZVAL_NULL(empty);
		return empty;
	}

	return *ret;
}
/* }}} */

/** {{{ inline yaf_request_t * yaf_request_get_method(yaf_request_t *instance TSRMLS_DC)
*/ 
inline yaf_request_t * yaf_request_get_method(yaf_request_t *instance TSRMLS_DC) {
	yaf_request_t *method = yaf_read_property(instance, YAF_REQUEST_PROPERTY_NAME_METHOD);
	return method;
}
/* }}} */

/** {{{ inline yaf_request_t * yaf_request_get_language(yaf_request_t *instance TSRMLS_DC)
*/
inline zval * yaf_request_get_language(yaf_request_t *instance TSRMLS_DC) {
	yaf_request_t *lang = yaf_read_property(instance, YAF_REQUEST_PROPERTY_NAME_LANG);

	if (IS_STRING != Z_TYPE_P(lang)) {
		zval * accept_langs = yaf_request_query(YAF_GLOBAL_VARS_SERVER, YAF_STRL("HTTP_ACCEPT_LANGUAGE") TSRMLS_CC);

		if (IS_STRING != Z_TYPE_P(accept_langs) || !Z_STRLEN_P(accept_langs)) {
			return lang;
		} else {
			double	max_qvlaue 	= 0;
			char  	*prefer	 	= NULL;
			char  	*ptrptr		= NULL;
			int	  	prefer_len 	= 0;
			char  	*seg		= NULL;
			char  	*langs 	 	= estrndup(Z_STRVAL_P(accept_langs), Z_STRLEN_P(accept_langs));

			seg = php_strtok_r(langs, ",", &ptrptr);

			while(seg) {
				char *qvalue = NULL;
				while( *(seg) == ' ') seg++ ;
				/* Accept-Language: da, en-gb;q=0.8, en;q=0.7 */
				if ((qvalue = strstr(seg, "q="))) {
					float qval = zend_string_to_double(qvalue + 2, seg - qvalue + 2);
					if (qval > max_qvlaue) {
						max_qvlaue = qval;
						if (prefer) {
							efree(prefer);
						}
						prefer_len = qvalue - seg - 1;
						prefer 	   = estrndup(seg, prefer_len);
					}
				} else {
					if (max_qvlaue < 1) {
						max_qvlaue = 1;
						prefer_len = strlen(seg);
						prefer 	   = estrndup(seg, prefer_len);
					}
				}

				seg = php_strtok_r(NULL, ",", &ptrptr);
			}

			if (prefer) {
				zval *accept_language = NULL;
				MAKE_STD_ZVAL(accept_language);
				ZVAL_STRINGL(accept_language,  prefer, prefer_len, TRUE);
				yaf_update_property(instance, YAF_REQUEST_PROPERTY_NAME_LANG, accept_language);
				efree(prefer);
				efree(langs);
				return accept_language;
			}

			efree(langs);
		}
	}

	return lang;
}
/* }}} */

/** {{{ inline boolean yaf_request_is_routed(yaf_request_t *instance TSRMLS_DC) 
*/
inline boolean yaf_request_is_routed(yaf_request_t *instance TSRMLS_DC) {
	yaf_request_t *routed = yaf_read_property(instance, YAF_REQUEST_PROPERTY_NAME_ROUTED);
	return Z_LVAL_P(routed);
}
/* }}} */

/** {{{ inline boolean yaf_request_is_dispatched(yaf_request_t *instance TSRMLS_DC)
*/
inline boolean yaf_request_is_dispatched(yaf_request_t *instance TSRMLS_DC) {
	yaf_request_t *dispatched = yaf_read_property(instance, YAF_REQUEST_PROPERTY_NAME_STATE);
	return Z_LVAL_P(dispatched);
}
/* }}} */

/** {{{ inline boolean yaf_request_set_dispatched(yaf_request_t *instance, int flag TSRMLS_DC)
*/
inline boolean yaf_request_set_dispatched(yaf_request_t *instance, int flag TSRMLS_DC) {
	zend_update_property_bool(Z_OBJCE_P(instance), instance, YAF_STRL(YAF_REQUEST_PROPERTY_NAME_STATE), flag TSRMLS_CC);
	return TRUE;
}
/* }}} */

/** {{{ inline boolean yaf_request_set_routed(yaf_request_t *instance, int flag TSRMLS_DC)
*/
inline boolean yaf_request_set_routed(yaf_request_t *instance, int flag TSRMLS_DC) {
	zend_update_property_bool(Z_OBJCE_P(instance), instance, YAF_STRL(YAF_REQUEST_PROPERTY_NAME_ROUTED), flag TSRMLS_CC);
	return TRUE;
}
/* }}} */

/** {{{ inline boolean yaf_request_set_params_single(yaf_request_t *instance, char *key, int len, zval *value TSRMLS_DC)
*/
inline boolean yaf_request_set_params_single(yaf_request_t *instance, char *key, int len, zval *value TSRMLS_DC) {

	zval *params = NULL;

	params	= yaf_read_property(instance, YAF_REQUEST_PROPERTY_NAME_PARAMS);

	if (zend_hash_update(Z_ARRVAL_P(params), key, len+1, &value, sizeof(zval *), NULL) == SUCCESS) {
		Z_ADDREF_P(value);
		return TRUE;
	}

	return FALSE;
}
/* }}} */

/** {{{ inline boolean yaf_request_set_params_multi(yaf_request_t *instance, zval *values TSRMLS_DC)
*/
inline boolean yaf_request_set_params_multi(yaf_request_t *instance, zval *values TSRMLS_DC) {
	zval *params = NULL;

	params	= yaf_read_property(instance, YAF_REQUEST_PROPERTY_NAME_PARAMS);
	if (values && Z_TYPE_P(values) == IS_ARRAY) {
		zend_hash_copy(Z_ARRVAL_P(params), Z_ARRVAL_P(values), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
		return TRUE;
	}

	return FALSE;
}
/* }}} */

/** {{{ inline zval * yaf_request_get_param(yaf_request_t * instance, char *key, int len TSRMLS_DC)
*/
inline zval * yaf_request_get_param(zval *instance, char *key, int len TSRMLS_DC) {
	zval **ppzval = NULL;
	zval *params  = yaf_read_property(instance, YAF_REQUEST_PROPERTY_NAME_PARAMS);

	if (zend_hash_find(Z_ARRVAL_P(params), key, len + 1, (void **) &ppzval) == SUCCESS) {
		return *ppzval;
	}

	return NULL;
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::isGet(void)
*/
YAF_REQUEST_IS_METHOD(Get);
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::isPost(void)
*/
YAF_REQUEST_IS_METHOD(Post);
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::isPut(void)
*/
YAF_REQUEST_IS_METHOD(Put);
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::isHead(void)
*/
YAF_REQUEST_IS_METHOD(Head);
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::isOptions(void)
*/
YAF_REQUEST_IS_METHOD(Options);
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::isCli(void)
*/
YAF_REQUEST_IS_METHOD(Cli);
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::isXmlHttpRequest(void) 
*/
PHP_METHOD(yaf_request, isXmlHttpRequest) {
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::getEnv(mixed $name, mixed $default = NULL)
*/
YAF_REQUEST_METHOD(yaf_request, Env, 	YAF_GLOBAL_VARS_ENV);
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::getServer(mixed $name, mixed $default = NULL)
*/
YAF_REQUEST_METHOD(yaf_request, Server, YAF_GLOBAL_VARS_SERVER);
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::getModuleName(void) 
*/
PHP_METHOD(yaf_request, getModuleName) {
	zval *module = yaf_read_property(getThis(), YAF_REQUEST_PROPERTY_NAME_MODULE);
	RETVAL_ZVAL(module, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::getControllerName(void) 
*/
PHP_METHOD(yaf_request, getControllerName) {
	zval *controller = yaf_read_property(getThis(), YAF_REQUEST_PROPERTY_NAME_CONTROLLER);
	RETVAL_ZVAL(controller, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::getActionName(void) 
*/
PHP_METHOD(yaf_request, getActionName) {
	zval *action = yaf_read_property(getThis(), YAF_REQUEST_PROPERTY_NAME_ACTION);
	RETVAL_ZVAL(action, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::setModuleName(string $module) 
*/
PHP_METHOD(yaf_request, setModuleName) {
	zval *module = NULL;
	zval *self	 = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &module) == FAILURE) {
		WRONG_PARAM_COUNT;
	}     

	if (Z_TYPE_P(module) != IS_STRING) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::setModuleName expect a string module name", Z_OBJCE_P(self)->name);
		RETURN_FALSE;
	}

	yaf_update_property(self, YAF_REQUEST_PROPERTY_NAME_MODULE, module);

	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::setControllerName(string $controller) 
*/
PHP_METHOD(yaf_request, setControllerName) {
	zval *controller = NULL;
	zval *self		 = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &controller) == FAILURE) {
		WRONG_PARAM_COUNT;
	}     

	if (Z_TYPE_P(controller) != IS_STRING) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::setControllerName expect a string controller name", Z_OBJCE_P(self)->name);
		RETURN_FALSE;
	}

	yaf_update_property(getThis(), YAF_REQUEST_PROPERTY_NAME_CONTROLLER, controller);

	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::setActionName(string $action) 
*/
PHP_METHOD(yaf_request, setActionName) {
	zval *action = NULL;
	zval *self	 = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &action) == FAILURE) {
		WRONG_PARAM_COUNT;
	}     

	if (Z_TYPE_P(action) != IS_STRING) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::setActionName expect a string action name", Z_OBJCE_P(self)->name);
		RETURN_FALSE;
	}

	yaf_update_property(getThis(), YAF_REQUEST_PROPERTY_NAME_ACTION, action);

	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::setParam(mixed $value)
*/
PHP_METHOD(yaf_request, setParam) {
	int	 			argc 	= 0;
	yaf_request_t 	*self 	= getThis();

	argc = ZEND_NUM_ARGS();

	if (1 == argc) {
		zval *value = NULL;
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &value) == FAILURE) {
			WRONG_PARAM_COUNT;
		}     
		if (value && Z_TYPE_P(value) == IS_ARRAY) {
			if (yaf_request_set_params_multi(self, value TSRMLS_CC)) {
				RETURN_ZVAL(self, 1, 0);
			}
		} 
	} else if (2 == argc) {
		zval *value = NULL;
		char *name	= NULL;
		int  len	= 0;

		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &len, &value) == FAILURE) {
			WRONG_PARAM_COUNT;
		}     

		if (yaf_request_set_params_single(getThis(), name, len, value TSRMLS_CC)) {
			RETURN_ZVAL(self, 1, 0);
		}
	} else {
		WRONG_PARAM_COUNT;
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::getParam(string $name, $mixed $default = NULL)
*/
PHP_METHOD(yaf_request, getParam) {
	char *name	= NULL;
	int  len	= 0;
	zval *def   = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|z", &name, &len, &def) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {
		zval *value = yaf_request_get_param(getThis(), name, len TSRMLS_CC);

		if (value) {
			RETURN_ZVAL(value, 1, 0);
		}
		
		if (def) {
			RETURN_ZVAL(def, 1, 0);
		}

	}
	RETURN_NULL();
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::getException(void)
*/
PHP_METHOD(yaf_request, getException) {
	zval *exception = yaf_read_property(getThis(), YAF_REQUEST_PROPERTY_NAME_EXCEPTION);
	if (IS_OBJECT == Z_TYPE_P(exception)
			&& instanceof_function(Z_OBJCE_P(exception), 
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 2)
				zend_exception_get_default()
#else
				zend_exception_get_default(TSRMLS_C)
#endif
				TSRMLS_CC)) {
		RETURN_ZVAL(exception, 1, 0);
	}

	RETURN_NULL();
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::getParams(void)
*/
PHP_METHOD(yaf_request, getParams) {
	zval *params = yaf_read_property(getThis(), YAF_REQUEST_PROPERTY_NAME_PARAMS);
	RETURN_ZVAL(params, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::getLanguage(void)
*/
PHP_METHOD(yaf_request, getLanguage) {
	zval * lang = yaf_request_get_language(getThis() TSRMLS_CC);
	RETURN_ZVAL(lang, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::getMethod(void)
*/
PHP_METHOD(yaf_request, getMethod) {
	zval *method = yaf_request_get_method(getThis() TSRMLS_CC);
	RETURN_ZVAL(method, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::isDispatched(void)
*/
PHP_METHOD(yaf_request, isDispatched) {
	RETURN_BOOL(yaf_request_is_dispatched(getThis() TSRMLS_CC));
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::setDispatched(void)
*/
PHP_METHOD(yaf_request, setDispatched) {
	RETURN_BOOL(yaf_request_set_dispatched(getThis(), 1 TSRMLS_CC));
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::setBaseUri(string $name)
*/
PHP_METHOD(yaf_request, setBaseUri) {
	zval *uri = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &uri) == FAILURE) {
		WRONG_PARAM_COUNT;
	} 

	if (!uri 
			|| Z_TYPE_P(uri) !=  IS_STRING
			|| !Z_STRLEN_P(uri)) {
		RETURN_FALSE;
	}
	if (yaf_request_set_base_uri(getThis(), Z_STRVAL_P(uri), NULL TSRMLS_CC)) {
		RETURN_ZVAL(getThis(), 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::getBaseUri(string $name)
*/
PHP_METHOD(yaf_request, getBaseUri) {
	zval 		 *uri 	= NULL;
	yaf_request_t *self 	= getThis();

	uri = yaf_read_property(self, YAF_REQUEST_PROPERTY_NAME_BASE);

	RETURN_ZVAL(uri, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::getRequestUri(string $name)
*/
PHP_METHOD(yaf_request, getRequestUri) {
	zval 		 *uri 	= NULL;
	yaf_request_t *self 	= getThis();

	uri = yaf_read_property(self, YAF_REQUEST_PROPERTY_NAME_URI);

	RETURN_ZVAL(uri, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::isRouted(void) 
*/
PHP_METHOD(yaf_request, isRouted) {
	RETURN_BOOL(yaf_request_is_routed(getThis() TSRMLS_CC));
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::setRouted(void)
*/
PHP_METHOD(yaf_request, setRouted) {
	yaf_request_t *self = getThis();

	if (yaf_request_set_routed(self, 1 TSRMLS_CC)) {
		RETURN_ZVAL(self, 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ yaf_request_methods
*/
zend_function_entry yaf_request_methods[] = {
	PHP_ME(yaf_request, isGet,				NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, isPost,				NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, isPut,				NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, isHead,				NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, isOptions,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, isCli,				NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, isXmlHttpRequest,	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, getServer,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, getEnv,				NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, setParam,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, getParam,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, getParams,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, getException, 		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, getModuleName,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, getControllerName,	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, getActionName,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, setModuleName,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, setControllerName,	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, setActionName,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, getMethod,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, getLanguage,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, setBaseUri,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, getBaseUri,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, getRequestUri,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, isDispatched,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, setDispatched,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, isRouted,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request, setRouted,			NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ YAF_STARTUP_FUNCTION
*/
YAF_STARTUP_FUNCTION(request){
	zend_class_entry ce;

	YAF_INIT_CLASS_ENTRY(ce, "Yaf_Request_Abstract", "Yaf\\Request_Abstract", yaf_request_methods);
	yaf_request_ce 			= zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	yaf_request_ce->ce_flags = ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

	zend_declare_property_null(yaf_request_ce, YAF_STRL(YAF_REQUEST_PROPERTY_NAME_MODULE),     ZEND_ACC_PUBLIC	TSRMLS_CC);
	zend_declare_property_null(yaf_request_ce, YAF_STRL(YAF_REQUEST_PROPERTY_NAME_CONTROLLER), ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(yaf_request_ce, YAF_STRL(YAF_REQUEST_PROPERTY_NAME_ACTION),     ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(yaf_request_ce, YAF_STRL(YAF_REQUEST_PROPERTY_NAME_METHOD),     ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(yaf_request_ce, YAF_STRL(YAF_REQUEST_PROPERTY_NAME_PARAMS),  	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_request_ce, YAF_STRL(YAF_REQUEST_PROPERTY_NAME_LANG), 		ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_request_ce, YAF_STRL(YAF_REQUEST_PROPERTY_NAME_EXCEPTION),  ZEND_ACC_PROTECTED TSRMLS_CC);

	zend_declare_property_string(yaf_request_ce, YAF_STRL(YAF_REQUEST_PROPERTY_NAME_BASE), "", ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_string(yaf_request_ce, YAF_STRL(YAF_REQUEST_PROPERTY_NAME_URI),  "", ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_bool(yaf_request_ce, YAF_STRL(YAF_REQUEST_PROPERTY_NAME_STATE),	0,	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_bool(yaf_request_ce, YAF_STRL(YAF_REQUEST_PROPERTY_NAME_ROUTED), 0, 	ZEND_ACC_PROTECTED TSRMLS_CC);

	YAF_STARTUP(request_http);
	YAF_STARTUP(request_simple);

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
