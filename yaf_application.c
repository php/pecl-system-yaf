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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_interfaces.h"
#include "ext/standard/php_var.h"
#include "ext/standard/basic_functions.h"

#include "php_yaf.h"
#include "yaf_namespace.h"
#include "yaf_application.h"
#include "yaf_dispatcher.h"
#include "yaf_config.h"
#include "yaf_loader.h"
#include "yaf_request.h"
#include "yaf_bootstrap.h"
#include "yaf_exception.h"

zend_class_entry * yaf_application_ce;

/** {{{ static int yaf_application_parse_option(zval *options TSRMLS_DC) 
*/
static int yaf_application_parse_option(zval *options TSRMLS_DC) {
	zval 		**ppzval	= NULL;
	zval 		**ppsval	= NULL;
	zval 		*app		= NULL;
	HashTable 	*conf		= NULL; 

	conf = HASH_OF(options);

	if (zend_hash_find(conf, YAF_STRS("application"), (void **)&ppzval) == FAILURE) {
		if (zend_hash_find(conf, YAF_STRS("yaf"), (void **)&ppzval) == FAILURE) {
			yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s", "expected an array of application configure");
			return FAILURE;
		}
	}

	app = *ppzval;

	if (Z_TYPE_P(app) != IS_ARRAY) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s", "expected an array of application configure");
		return FAILURE;
	}

	if (zend_hash_find(Z_ARRVAL_P(app), YAF_STRS("directory"), (void **)&ppzval) == FAILURE
			|| Z_TYPE_PP(ppzval) != IS_STRING) {
		yaf_trigger_error(YAF_ERR_STARTUP_FAILED, "%s", "expected a directory entry in application configures");
		return FAILURE;
	}

	if (*(Z_STRVAL_PP(ppzval) + Z_STRLEN_PP(ppzval) - 1) == DEFAULT_SLASH) {
		YAF_G(directory) = estrndup(Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval) - 1);
	} else {
		YAF_G(directory) = estrndup(Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval));
	}

	if (zend_hash_find(Z_ARRVAL_P(app), YAF_STRS("ext"), (void **)&ppzval) == SUCCESS
			&& Z_TYPE_PP(ppzval) == IS_STRING) {
		YAF_G(ext) = estrndup(Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval));
	} else {
		YAF_G(ext) = YAF_DEFAULT_EXT;
	}

	if (zend_hash_find(Z_ARRVAL_P(app), YAF_STRS("bootstrap"), (void **)&ppzval) == SUCCESS
			&& Z_TYPE_PP(ppzval) == IS_STRING) {
		YAF_G(bootstrap) = estrndup(Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval));
	}

	if (zend_hash_find(Z_ARRVAL_P(app), YAF_STRS("library"), (void **)&ppzval) == SUCCESS
			&& Z_TYPE_PP(ppzval) == IS_STRING) {
		YAF_G(library_directory) = estrndup(Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval));
	} 

	if (zend_hash_find(Z_ARRVAL_P(app), YAF_STRS("view"), (void **)&ppzval) == FAILURE
			|| Z_TYPE_PP(ppzval) != IS_ARRAY) {
		YAF_G(view_ext) = YAF_DEFAULT_VIEW_EXT;
	} else {
		if (zend_hash_find(Z_ARRVAL_PP(ppzval), YAF_STRS("ext"), (void **)&ppsval) == FAILURE
				|| Z_TYPE_PP(ppsval) != IS_STRING) {
			YAF_G(view_ext) = YAF_DEFAULT_VIEW_EXT;
		} else {
			YAF_G(view_ext) = estrndup(Z_STRVAL_PP(ppsval), Z_STRLEN_PP(ppsval));
		}
	}

	if (zend_hash_find(Z_ARRVAL_P(app), YAF_STRS("baseUri"), (void **)&ppzval) == SUCCESS
			&& Z_TYPE_PP(ppzval) == IS_STRING) {
		YAF_G(base_uri) = estrndup(Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval));
	}

	if (zend_hash_find(Z_ARRVAL_P(app), YAF_STRS("dispatcher"), (void **)&ppzval) == FAILURE
			|| Z_TYPE_PP(ppzval) != IS_ARRAY) {
		YAF_G(default_module) = YAF_ROUTER_DEFAULT_MODULE;
		YAF_G(default_controller) = YAF_ROUTER_DEFAULT_CONTROLLER;
		YAF_G(default_action)  = YAF_ROUTER_DEFAULT_ACTION;
	} else {
		if (zend_hash_find(Z_ARRVAL_PP(ppzval), YAF_STRS("defaultModule"), (void **)&ppsval) == FAILURE
				|| Z_TYPE_PP(ppsval) != IS_STRING) {
			YAF_G(default_module) = YAF_ROUTER_DEFAULT_MODULE;
		} else {
			YAF_G(default_module) = zend_str_tolower_dup(Z_STRVAL_PP(ppsval), Z_STRLEN_PP(ppsval));
			*(YAF_G(default_module)) = toupper(*YAF_G(default_module));
		}

		if (zend_hash_find(Z_ARRVAL_PP(ppzval), YAF_STRS("defaultController"), (void **)&ppsval) == FAILURE
				|| Z_TYPE_PP(ppsval) != IS_STRING) {
			YAF_G(default_controller) = YAF_ROUTER_DEFAULT_CONTROLLER;
		} else {
			YAF_G(default_controller) = zend_str_tolower_dup(Z_STRVAL_PP(ppsval), Z_STRLEN_PP(ppsval));
			*(YAF_G(default_controller)) = toupper(*YAF_G(default_controller));
		}

		if (zend_hash_find(Z_ARRVAL_PP(ppzval), YAF_STRS("defaultAction"), (void **)&ppsval) == FAILURE
				|| Z_TYPE_PP(ppsval) != IS_STRING) {
			YAF_G(default_action)	  = YAF_ROUTER_DEFAULT_ACTION;
		} else {
			YAF_G(default_action) = zend_str_tolower_dup(Z_STRVAL_PP(ppsval), Z_STRLEN_PP(ppsval));
		}

		if (zend_hash_find(Z_ARRVAL_PP(ppzval), YAF_STRS("throwException"), (void **)&ppsval) == SUCCESS) {
			zval_add_ref(ppsval);
			convert_to_boolean_ex(ppsval);
			YAF_G(throw_exception) = Z_BVAL_PP(ppsval);
		}

		if (zend_hash_find(Z_ARRVAL_PP(ppzval), YAF_STRS("catchException"), (void **)&ppsval) == SUCCESS) {
			zval_add_ref(ppsval);
			convert_to_boolean_ex(ppsval);
			YAF_G(catch_exception) = Z_BVAL_PP(ppsval);
		}
	}

	do {
		zval *module   = NULL;
		zval *zmodules = NULL;
		char *ptrptr   = NULL;

		MAKE_STD_ZVAL(zmodules);
		array_init(zmodules);
		if (zend_hash_find(Z_ARRVAL_P(app), YAF_STRS("modules"), (void **)&ppzval) == SUCCESS
				&& Z_TYPE_PP(ppzval) == IS_STRING && Z_STRLEN_PP(ppzval)) {
			char *seg	  = NULL;
			char *modules = estrndup(Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval));

			seg = php_strtok_r(modules, ",", &ptrptr);
			while(seg) {
				if (seg && strlen(seg)) {
					MAKE_STD_ZVAL(module);
					ZVAL_STRINGL(module, seg, strlen(seg), 1);
					zend_hash_next_index_insert(Z_ARRVAL_P(zmodules), (void **)&module, sizeof(zval *), NULL);
				}
				seg = php_strtok_r(NULL, ",", &ptrptr);
			}

			efree(modules);
		} else {
			MAKE_STD_ZVAL(module);
			ZVAL_STRING(module, YAF_G(default_module), 1);
			zend_hash_next_index_insert(Z_ARRVAL_P(zmodules), (void **)&module, sizeof(zval *), NULL);
		}

		YAF_G(modules) = zmodules;
	} while (0);

	return SUCCESS;
}
/* }}} */

/** {{{ boolean yaf_application_is_module_name(char *name, int len TSRMLS_DC)
*/
boolean yaf_application_is_module_name(char *name, int len TSRMLS_DC) {
	yaf_application_t *app 		= NULL;
	zval 			 *modules 	= NULL;
	HashTable		 *ht 		= NULL;
	zval			 **ppzval 	= NULL;

	app = yaf_read_static_property(yaf_application_ce, YAF_APPLICATION_PROPERTY_NAME_APP);

	if (!app || Z_TYPE_P(app) != IS_OBJECT) {
		return FALSE;
	}

	modules = yaf_read_property(app, YAF_APPLICATION_PROPERTY_NAME_MODULES);

	if (!modules || Z_TYPE_P(modules) != IS_ARRAY) {
		return FALSE;
	}

	ht = Z_ARRVAL_P(modules);

	zend_hash_internal_pointer_reset(ht);
	while (zend_hash_get_current_data(ht, (void **)&ppzval) == SUCCESS) {
		if (Z_TYPE_PP(ppzval) == IS_STRING
				&& strncasecmp(Z_STRVAL_PP(ppzval), name, len) == 0) {
			return TRUE;
		}
		zend_hash_move_forward(ht);
	}
	return FALSE;
}
/* }}} */

/** {{{ proto Yaf_Application::__construct(mixed $config) 
*/
PHP_METHOD(yaf_application, __construct) {
	zval 				*config 		= NULL;
	yaf_config_t 	 	*zconfig       	= NULL;
	yaf_request_t 	 	*request		= NULL;
	yaf_dispatcher_t		*zdispatcher	= NULL;
	yaf_application_t	*app			= NULL;
	yaf_loader_t			*loader			= NULL;

	zval *self 			= getThis();
	zval *section 		= NULL;
	app					= yaf_read_static_property(yaf_application_ce, YAF_APPLICATION_PROPERTY_NAME_APP);

#if PHP_YAF_DEBUG
	yaf_strict("you are running yaf in debug mode");
#endif

	if (!ZVAL_IS_NULL(app)) {
		yaf_trigger_error(YAF_ERR_STARTUP_FAILED, "only one application can be initialized");
		RETURN_FALSE;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &config, &section) == FAILURE) {
		yaf_trigger_error(YAF_ERR_STARTUP_FAILED, "%s::__construct expects at least 1 parameter, 0 give", yaf_application_ce->name);
		WRONG_PARAM_COUNT;
	} 

	if (!section || Z_TYPE_P(section) != IS_STRING || !Z_STRLEN_P(section)) {
		MAKE_STD_ZVAL(section);
		ZVAL_STRING(section, YAF_G(environ), TRUE);
		zconfig = yaf_config_instance(NULL, config, section TSRMLS_CC);
		zval_dtor(section);
		efree(section);
	} else {
		zconfig = yaf_config_instance(NULL, config, section TSRMLS_CC);
	}

	if  (zconfig == NULL
			|| Z_TYPE_P(zconfig) != IS_OBJECT
			|| !instanceof_function(Z_OBJCE_P(zconfig), yaf_config_ce TSRMLS_CC)
			|| yaf_application_parse_option(yaf_read_property(zconfig, YAF_CONFIG_PROPERT_NAME) TSRMLS_CC) == FAILURE) {
		yaf_trigger_error(YAF_ERR_STARTUP_FAILED, "initialization of application config failed");
		RETURN_FALSE;
	}

	request = yaf_request_instance(NULL, YAF_G(base_uri) TSRMLS_CC);

	if (YAF_G(base_uri)) {
		efree(YAF_G(base_uri));
		YAF_G(base_uri) = NULL;
	}

	if (!request) {
		yaf_trigger_error(YAF_ERR_STARTUP_FAILED, "initialization of request failed");
		RETURN_FALSE;
	}

	zdispatcher = yaf_dispatcher_instance(NULL TSRMLS_CC);
	yaf_dispatcher_set_request(zdispatcher, request TSRMLS_CC);

	if (NULL == zdispatcher
			|| Z_TYPE_P(zdispatcher) != IS_OBJECT
			|| !instanceof_function(Z_OBJCE_P(zdispatcher), yaf_dispatcher_ce TSRMLS_CC)) {
		yaf_trigger_error(YAF_ERR_STARTUP_FAILED, "initialization of application dispatcher failed");
		RETURN_FALSE;
	}

	yaf_update_property(self, YAF_APPLICATION_PROPERTY_NAME_CONFIG, zconfig);
	yaf_update_property(self, YAF_APPLICATION_PROPERTY_NAME_DISPATCHER, zdispatcher);

	if (YAF_G(library_directory)) {
		loader = yaf_loader_instance(NULL, YAF_G(library_directory), strlen(YAF_G(global_library))? YAF_G(global_library):NULL TSRMLS_CC);
		efree(YAF_G(library_directory));
		YAF_G(library_directory) = NULL;
	} else {
		char *library_directory = NULL;
		spprintf(&library_directory, 0, "%s%c%s", YAF_G(directory), DEFAULT_SLASH, YAF_LIBRARY_DIRECTORY_NAME);
		loader = yaf_loader_instance(NULL, library_directory, strlen(YAF_G(global_library))? YAF_G(global_library):NULL TSRMLS_CC);
		efree(library_directory);
	}

	if (!loader) {
		yaf_trigger_error(YAF_ERR_STARTUP_FAILED, "initialization of application auto loader failed");
		RETURN_FALSE;
	}

	zend_update_property_bool(Z_OBJCE_P(self), self, YAF_STRL(YAF_APPLICATION_PROPERTY_NAME_RUN), 0 TSRMLS_CC);
	zend_update_property_string(Z_OBJCE_P(self), self, YAF_STRL(YAF_APPLICATION_PROPERTY_NAME_ENV), YAF_G(environ) TSRMLS_CC);

	if (YAF_G(modules)) {
		yaf_update_property(self, YAF_APPLICATION_PROPERTY_NAME_MODULES, YAF_G(modules));
		Z_DELREF_P(YAF_G(modules));
		YAF_G(modules) = NULL;
	} else {
		zend_update_property_null(Z_OBJCE_P(self), self, YAF_STRL(YAF_APPLICATION_PROPERTY_NAME_MODULES) TSRMLS_CC);
	}
 
	zend_update_static_property(yaf_application_ce, YAF_STRL(YAF_APPLICATION_PROPERTY_NAME_APP), self TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yaf_Application::__desctruct(void)
*/
PHP_METHOD(yaf_application, __destruct) {
   zend_update_static_property_null(yaf_application_ce, ZEND_STRL(YAF_APPLICATION_PROPERTY_NAME_APP) TSRMLS_CC);
}
/* }}} */

/** {{{ proto private Yaf_Application::__sleep(void)
*/
PHP_METHOD(yaf_application, __sleep) {
}
/* }}} */

/** {{{ proto private Yaf_Application::__wakeup(void)
*/
PHP_METHOD(yaf_application, __wakeup) {
}
/* }}} */

/** {{{ proto private Yaf_Application::__clone(void)
*/
PHP_METHOD(yaf_application, __clone) {
}
/* }}} */

/** {{{ proto public Yaf_Application::run(void)
*/
PHP_METHOD(yaf_application, run) {
	zval *running				= NULL;
	yaf_dispatcher_t	 *dispatcher= NULL;
	yaf_application_t *self 		= getThis();
	yaf_response_t	 *response	= NULL;

	running = yaf_read_property(self, YAF_APPLICATION_PROPERTY_NAME_RUN);

	if (IS_BOOL == Z_TYPE_P(running)
			&& Z_BVAL_P(running)) {
		yaf_trigger_error(YAF_ERR_STARTUP_FAILED, "application already run");
		RETURN_TRUE;
	}

	ZVAL_BOOL(running, TRUE);
	yaf_update_property(self, YAF_APPLICATION_PROPERTY_NAME_RUN, running);

	dispatcher = yaf_read_property(self, YAF_APPLICATION_PROPERTY_NAME_DISPATCHER);

	if ((response = yaf_dispatcher_dispatch(dispatcher TSRMLS_CC))) {
		RETURN_ZVAL(response, 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Application::execute(callback $func)
 * We can not call to zif_call_user_func on windows, since it was not declared with dllexport
*/
PHP_METHOD(yaf_application, execute) {
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
    zval *retval_ptr = NULL;
    zend_fcall_info fci;
    zend_fcall_info_cache fci_cache;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f*", &fci, &fci_cache, &fci.params, &fci.param_count) == FAILURE) {
        return;
    }

    fci.retval_ptr_ptr = &retval_ptr;

    if (zend_call_function(&fci, &fci_cache TSRMLS_CC) == SUCCESS && fci.retval_ptr_ptr && *fci.retval_ptr_ptr) {
        COPY_PZVAL_TO_ZVAL(*return_value, *fci.retval_ptr_ptr);
    }

    if (fci.params) {
        efree(fci.params);
    }
#else
    zval ***params;
    zval *retval_ptr;
    char *name;
    int argc = ZEND_NUM_ARGS();

    if (argc < 1) {
        WRONG_PARAM_COUNT;
    }

    params = safe_emalloc(sizeof(zval **), argc, 0);

    if (zend_get_parameters_array_ex(1, params) == FAILURE) {
        efree(params);
        RETURN_FALSE;
    }

    if (Z_TYPE_PP(params[0]) != IS_STRING && Z_TYPE_PP(params[0]) != IS_ARRAY) {
        SEPARATE_ZVAL(params[0]);
        convert_to_string_ex(params[0]);
    }

    if (!zend_is_callable(*params[0], 0, &name)) {
        php_error_docref1(NULL TSRMLS_CC, name, E_WARNING, "First argument is expected to be a valid callback");
        efree(name);
        efree(params);
        RETURN_NULL();
    }

    if (zend_get_parameters_array_ex(argc, params) == FAILURE) {
        efree(params);
        RETURN_FALSE;
    }

    if (call_user_function_ex(EG(function_table), NULL, *params[0], &retval_ptr, argc-1, params+1, 0, NULL TSRMLS_CC) == SUCCESS) {
        if (retval_ptr) {
            COPY_PZVAL_TO_ZVAL(*return_value, retval_ptr);
        }
    } else {
        if (argc > 1) {
            SEPARATE_ZVAL(params[1]);
            convert_to_string_ex(params[1]);
            if (argc > 2) {
                SEPARATE_ZVAL(params[2]);
                convert_to_string_ex(params[2]);
                php_error_docref1(NULL TSRMLS_CC, name, E_WARNING, "Unable to call %s(%s,%s)", name, Z_STRVAL_PP(params[1]), Z_STRVAL_PP(params[2]));
            } else {
                php_error_docref1(NULL TSRMLS_CC, name, E_WARNING, "Unable to call %s(%s)", name, Z_STRVAL_PP(params[1]));
            }
        } else {
            php_error_docref1(NULL TSRMLS_CC, name, E_WARNING, "Unable to call %s()", name);
        }
    }

    efree(name);
    efree(params);
#endif
}
/* }}} */

/** {{{ proto public Yaf_Application::app(void) 
*/
PHP_METHOD(yaf_application, app) {
	yaf_application_t * app = NULL;
	app = zend_read_static_property(yaf_application_ce, YAF_STRL(YAF_APPLICATION_PROPERTY_NAME_APP), FALSE TSRMLS_CC);

	RETVAL_ZVAL(app, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Application::getConfig(void) 
*/
PHP_METHOD(yaf_application, getConfig) {
	yaf_config_t *config = NULL;
	config = yaf_read_property(getThis(), YAF_APPLICATION_PROPERTY_NAME_CONFIG);

	RETVAL_ZVAL(config, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Application::getDispatcher(void) 
*/
PHP_METHOD(yaf_application, getDispatcher) {
	yaf_dispatcher_t *dispatcher = NULL;
	dispatcher = yaf_read_property(getThis(), YAF_APPLICATION_PROPERTY_NAME_DISPATCHER);

	RETVAL_ZVAL(dispatcher, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Application::getModules(void)
*/
PHP_METHOD(yaf_application, getModules) {
	zval *modules = NULL;
	modules = yaf_read_property(getThis(), YAF_APPLICATION_PROPERTY_NAME_MODULES);

	RETVAL_ZVAL(modules, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Application::environ(void) 
*/
PHP_METHOD(yaf_application, environ) {
	zval * env = NULL;
	env = yaf_read_property(getThis(), YAF_APPLICATION_PROPERTY_NAME_ENV);

	RETVAL_ZVAL(env, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Application::bootstrap(void) 
*/
PHP_METHOD(yaf_application, bootstrap) {
	zend_class_entry 	**ce		= NULL;
	uint 				len			= 0;
	zval 				*self 		= getThis();
	char 			*bootstrap_path	= NULL;
	uint				retval   	= TRUE;

	if (zend_hash_find(EG(class_table), YAF_DEFAULT_BOOTSTRAP_LOWER, YAF_DEFAULT_BOOTSTRAP_LEN, (void **) &ce) != SUCCESS) {
		if (YAF_G(bootstrap)) {
			bootstrap_path  = YAF_G(bootstrap);
			len = strlen(YAF_G(bootstrap));
			/** bootstrap can't call twice, YAF_G(bootstrap) is able to be freed */
			YAF_G(bootstrap) = NULL;
		} else {
			len = spprintf(&bootstrap_path, 0, "%s%c%s.%s", YAF_G(directory), DEFAULT_SLASH, YAF_DEFAULT_BOOTSTRAP, YAF_G(ext));
		}

		if (!yaf_loader_import(bootstrap_path, len + 1, FALSE TSRMLS_CC)) {
			yaf_trigger_error(YAF_ERR_AUTOLOAD_FAILED, "could not find bootstrap file %s", bootstrap_path);
			retval = FALSE;
		} else if (zend_hash_find(EG(class_table), YAF_DEFAULT_BOOTSTRAP_LOWER, YAF_DEFAULT_BOOTSTRAP_LEN, (void **) &ce) != SUCCESS)  {
			yaf_trigger_error(YAF_ERR_AUTOLOAD_FAILED, "could not find class %s in %s", YAF_DEFAULT_BOOTSTRAP, bootstrap_path);
			retval = FALSE;
		} else if (!instanceof_function(*ce, yaf_bootstrap_ce TSRMLS_CC)) {
			yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::bootstrap expect a %s instance, %s give", yaf_application_ce->name, yaf_bootstrap_ce->name, (*ce)->name);
			retval = FALSE;
		}
	}	

	efree(bootstrap_path);

	if (!retval) {
		RETURN_FALSE;
	}

	do {
		int 		len	 		= 0;
		long		idx  		= 0;
		char  		*func 		= NULL;
		zval 		*bootstrap	= NULL;
		HashTable 	*methods	= NULL;
		zval 		*dispatcher = NULL;


		MAKE_STD_ZVAL(bootstrap);
		object_init_ex(bootstrap, *ce);
		methods 	= &((*ce)->function_table);
		dispatcher	= yaf_read_property(self, YAF_APPLICATION_PROPERTY_NAME_DISPATCHER);

		for(zend_hash_internal_pointer_reset(methods);
				zend_hash_has_more_elements(methods) == SUCCESS;
				zend_hash_move_forward(methods)) {

			zend_hash_get_current_key_ex(methods, &func, &len, &idx, 0, NULL);
			if (strncasecmp(func, YAF_BOOTSTRAP_INITFUNC_PREFIX, sizeof(YAF_BOOTSTRAP_INITFUNC_PREFIX)-1)) {
				continue;
			}

			zend_call_method(&bootstrap, *ce, NULL, func, len - 1, NULL, 1, dispatcher, NULL TSRMLS_CC);

			/** an uncaught exception threw in function call */
			if (EG(exception)) {
				zval_dtor(bootstrap);
				efree(bootstrap);
				RETURN_FALSE;
			}
		}

		zval_dtor(bootstrap);
		efree(bootstrap);

	}  while (0);

	RETVAL_ZVAL(self, 1, 0);
}
/* }}} */

/** {{{ yaf_application_methods 
*/
zend_function_entry yaf_application_methods[] = {
	PHP_ME(yaf_application, __construct, 	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR) 
	PHP_ME(yaf_application, __destruct,		NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(yaf_application, __clone,			NULL, ZEND_ACC_PRIVATE|ZEND_ACC_CLONE)
	PHP_ME(yaf_application, __sleep,			NULL, ZEND_ACC_PRIVATE)
	PHP_ME(yaf_application, __wakeup,		NULL, ZEND_ACC_PRIVATE)
	PHP_ME(yaf_application, run, 	 	 	NULL, ZEND_ACC_PUBLIC) 
	PHP_ME(yaf_application, execute,	 	 	NULL, ZEND_ACC_PUBLIC) 
	PHP_ME(yaf_application, app, 	 	 	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC) 
	PHP_ME(yaf_application, environ, 	  	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_application, bootstrap,   	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_application, getConfig,   	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_application, getModules,   	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_application, getDispatcher,  	NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ YAF_STARTUP_FUNCTION
*/
YAF_STARTUP_FUNCTION(application) {
	zend_class_entry ce;

	YAF_INIT_CLASS_ENTRY(ce, "Yaf_Application", "Yaf\\Application", yaf_application_methods);

	yaf_application_ce 			= zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	yaf_application_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_null(yaf_application_ce, YAF_STRL(YAF_APPLICATION_PROPERTY_NAME_CONFIG), 	ZEND_ACC_PROTECTED 	TSRMLS_CC);
	zend_declare_property_null(yaf_application_ce, YAF_STRL(YAF_APPLICATION_PROPERTY_NAME_DISPATCHER), ZEND_ACC_PROTECTED 	TSRMLS_CC);
	zend_declare_property_null(yaf_application_ce, YAF_STRL(YAF_APPLICATION_PROPERTY_NAME_APP), 	 	ZEND_ACC_STATIC|ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_application_ce, YAF_STRL(YAF_APPLICATION_PROPERTY_NAME_MODULES), 	ZEND_ACC_PROTECTED TSRMLS_CC);

	zend_declare_property_bool(yaf_application_ce, YAF_STRL(YAF_APPLICATION_PROPERTY_NAME_RUN),	 0, ZEND_ACC_PROTECTED 	TSRMLS_CC);
	zend_declare_property_string(yaf_application_ce, YAF_STRL(YAF_APPLICATION_PROPERTY_NAME_ENV), YAF_G(environ), ZEND_ACC_PROTECTED TSRMLS_CC);

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
