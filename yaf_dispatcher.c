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
#include "yaf_dispatcher.h"
#include "yaf_controller.h"
#include "yaf_action.h"
#include "yaf_application.h"
#include "yaf_view.h"
#include "yaf_response.h"
#include "yaf_loader.h"
#include "yaf_router.h"
#include "yaf_request.h"
#include "yaf_config.h"
#include "yaf_plugin.h"
#include "yaf_exception.h"

zend_class_entry * yaf_dispatcher_ce;

/** {{{ yaf_dispatcher_t * yaf_dispatcher_instance(zval *this_ptr TSRMLS_DC) 
*/
yaf_dispatcher_t * yaf_dispatcher_instance(yaf_dispatcher_t *this_ptr TSRMLS_DC) {
	yaf_dispatcher_t *instance = NULL;
	yaf_response_t	*response = NULL;
	yaf_router_t		*router	  = NULL;
	zval			*plugins  = NULL;

	instance = yaf_read_static_property(yaf_dispatcher_ce, YAF_DISPATCHER_PROPERTY_NAME_INSTANCE);

	if (IS_OBJECT == Z_TYPE_P(instance) 
			&& instanceof_function(Z_OBJCE_P(instance), yaf_dispatcher_ce TSRMLS_CC)) {
		return instance;
	}

	if (this_ptr) {
		instance = this_ptr;
		return this_ptr;
	} else {
		instance = NULL;
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, yaf_dispatcher_ce);
	}

	/** unecessary yet 
	MAKE_STD_ZVAL(args);
	array_init(args);
	yaf_update_property(instance, YAF_DISPATCHER_PROPERTY_NAME_ARGS, 	  args);
	*/

	MAKE_STD_ZVAL(plugins);
	array_init(plugins);
	yaf_update_property(instance, YAF_DISPATCHER_PROPERTY_NAME_PLUGINS, plugins);

	response = yaf_response_instance(NULL, sapi_module.name TSRMLS_CC);
	router	= yaf_router_instance(NULL TSRMLS_CC);

	yaf_update_property(instance, YAF_DISPATCHER_PROPERTY_NAME_RESPONSE, response);
	yaf_update_property(instance, YAF_DISPATCHER_PROPERTY_NAME_ROUTER, router);

	zend_update_property_string(Z_OBJCE_P(instance), instance, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_MODULE), 	YAF_G(default_module) TSRMLS_CC);
	zend_update_property_string(Z_OBJCE_P(instance), instance, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_CONTROLLER), YAF_G(default_controller) TSRMLS_CC);
	zend_update_property_string(Z_OBJCE_P(instance), instance, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_ACTION), 	YAF_G(default_action) TSRMLS_CC);

	yaf_update_static_property(Z_OBJCE_P(instance), YAF_DISPATCHER_PROPERTY_NAME_INSTANCE, instance);

	return instance;
}
/* }}} */

/** {{{ static void yaf_dispatcher_get_call_parmaters(yaf_request_t *request, zend_function *fptr, zval ****params, uint *count TSRMLS_DC)
 */
static void yaf_dispatcher_get_call_parmaters(yaf_request_t *request, zend_function *fptr, zval ****params, uint *count TSRMLS_DC) {
	zval *args 			 	= NULL;
	zval **arg 				= NULL;
	zend_arg_info *arg_info = NULL;
	uint current 			= 0;
	HashTable *params_ht	= NULL;
	char *key				= NULL;
	int  keylen 			= 0;
	long idx				= 0;
	long llen				= 0;

	args = yaf_read_property(request, YAF_REQUEST_PROPERTY_NAME_PARAMS);

	params_ht = Z_ARRVAL_P(args);

	arg_info  = fptr->common.arg_info;
	*params   = safe_emalloc(sizeof(zval **), fptr->common.num_args, 0);

	for (;current < fptr->common.num_args; current++, arg_info++) {
		if (zend_hash_find(params_ht, arg_info->name, arg_info->name_len + 1, (void **)&arg) == SUCCESS) {
			(*params)[current] = arg;
			(*count)++;
		} else {
			arg = NULL;
			llen = arg_info->name_len + 1;
			/* since we need search ignoring case, can't use zend_hash_find */
			for(zend_hash_internal_pointer_reset(params_ht);
					zend_hash_has_more_elements(params_ht) == SUCCESS;
					zend_hash_move_forward(params_ht)) {

				if (zend_hash_get_current_key_ex(params_ht, &key, &keylen, &idx, 0, NULL) == HASH_KEY_IS_STRING) {
					if (keylen == llen && !strncasecmp(key, arg_info->name, keylen)) {
						if (zend_hash_get_current_data(params_ht, (void**)&arg) == SUCCESS) {
							/* return when we find first match, there is a trap 
							 * when multi different parameters in different case presenting in params_ht
							 * only the first take affect
							 */
							(*params)[current] = arg;
							(*count)++;
							break;
						}
					}
				}
			}

			if (NULL == arg) {
				break;
			}
		}	
	}
}
/* }}} */

/** {{{ yaf_view_t * yaf_dispatcher_init_view(yaf_dispatcher_t *dispatcher, zval *tpl_dir, zval *options TSRMLS_DC)
*/
yaf_view_t * yaf_dispatcher_init_view(yaf_dispatcher_t *dispatcher, zval *tpl_dir, zval *options TSRMLS_DC) {
	yaf_view_t *view = yaf_read_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_VIEW);

	if (view && IS_OBJECT == Z_TYPE_P(view)
			&& instanceof_function(Z_OBJCE_P(view), yaf_view_interface_ce TSRMLS_CC)) {
		return view;
	}

	view = yaf_view_instance(NULL, tpl_dir, options TSRMLS_CC);

	yaf_update_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_VIEW, view);

	return view;
}
/* }}} */

/** {{{ inline void yaf_dispatcher_fix_default(yaf_dispatcher_t *dispatcher, yaf_request_t *request TSRMLS_DC)
*/
inline void yaf_dispatcher_fix_default(yaf_dispatcher_t *dispatcher, yaf_request_t *request TSRMLS_DC) {
	zval	*module 	= NULL;
	zval 	*controller = NULL;
	zval 	*action		= NULL;

	module 		= yaf_read_property(request, YAF_REQUEST_PROPERTY_NAME_MODULE);
	controller 	= yaf_read_property(request, YAF_REQUEST_PROPERTY_NAME_CONTROLLER);
	action	 	= yaf_read_property(request, YAF_REQUEST_PROPERTY_NAME_ACTION);

	if (!module || Z_TYPE_P(module) != IS_STRING || !Z_STRLEN_P(module)) {
		zval *default_module = yaf_read_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_MODULE);
		yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_MODULE, default_module);
	} else {
		ZVAL_STRINGL(module, zend_str_tolower_dup(Z_STRVAL_P(module), Z_STRLEN_P(module)), Z_STRLEN_P(module), FALSE);
		*(Z_STRVAL_P(module)) = toupper(*(Z_STRVAL_P(module)));
		yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_MODULE, module);
	}

	if (!controller || Z_TYPE_P(controller) != IS_STRING || !Z_STRLEN_P(controller)) {
		zval *default_controller = yaf_read_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_CONTROLLER);
		yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_CONTROLLER, default_controller);
	} else {
		char *p = NULL;
		ZVAL_STRINGL(controller, zend_str_tolower_dup(Z_STRVAL_P(controller), Z_STRLEN_P(controller)), Z_STRLEN_P(controller), FALSE);

		p = Z_STRVAL_P(controller);

		/**
		 * upper contolerr name
		 * eg: Index_sub -> Index_Sub
		 */
		*p = toupper(*p);
		while (*p != '\0') {
			if (*p == '_'
#ifdef YAF_HAVE_NAMESPACE
					|| *p == '\\'	
#endif
			   ) {
				if (*(p+1) != '\0') {
					*(p+1) = toupper(*(p+1));
					p++;
				}
			}
			p++;
		}

		yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_CONTROLLER, controller);
	}


	if (!action || Z_TYPE_P(action) != IS_STRING || !Z_STRLEN_P(action)) {
		zval *default_action = yaf_read_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_ACTION);
		yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_ACTION, default_action);
	} else {
		ZVAL_STRINGL(action, zend_str_tolower_dup(Z_STRVAL_P(action), Z_STRLEN_P(action)), Z_STRLEN_P(action), 0);
		yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_ACTION, action);
	}
}
/* }}} */

/** {{{ boolean yaf_dispatcher_set_request(yaf_dispatcher_t *dispatcher, yaf_request_t *request TSRMLS_DC)
*/
boolean yaf_dispatcher_set_request(yaf_dispatcher_t *dispatcher, yaf_request_t *request TSRMLS_DC) {
	if (request) {
		yaf_update_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_REQUEST, request);
		return TRUE;
	}

	return FALSE;
}
/* }}} */

/** {{{ zend_class_entry * yaf_dispatcher_get_controller(char *app_dir, char *module, char *controller, int len, boolean def_module TSRMLS_DC)
 */
zend_class_entry * yaf_dispatcher_get_controller(char *app_dir, char *module, char *controller, int len, boolean def_module TSRMLS_DC) {
	char *directory 	= NULL;
	int	 directory_len 	= 0;

	if (def_module) {
		directory_len = spprintf(&directory, 0, "%s%c%s", app_dir, DEFAULT_SLASH, "controllers");
	} else {
		directory_len = spprintf(&directory, 0, "%s%c%s%c%s%c%s", app_dir, DEFAULT_SLASH,
				"modules", DEFAULT_SLASH, module, DEFAULT_SLASH, "controllers");
	}

	if (directory_len) {
		char *class			  = NULL;
		char *class_lowercase = NULL;
		int	 class_len		  = 0;
		zend_class_entry **ce = NULL;

		if (YAF_G(name_suffix)) {
			class_len = spprintf(&class, 0, "%s%s%s", controller, YAF_G(name_separator), "Controller");
		} else {
			class_len = spprintf(&class, 0, "%s%s%s", "Controller", YAF_G(name_separator), controller);
		}

		class_lowercase = zend_str_tolower_dup(class, class_len);

		if (zend_hash_find(EG(class_table), class_lowercase, class_len + 1, (void *)&ce) != SUCCESS) {

			if (!yaf_internal_autoload(controller, len, &directory TSRMLS_CC)) {
				yaf_trigger_error(YAF_ERR_NOTFOUND_CONTROLLER, "could not find controller script %s", directory);
				efree(class);
				efree(class_lowercase);
				efree(directory);
				return NULL;
			} else if (zend_hash_find(EG(class_table), class_lowercase, class_len + 1, (void **) &ce) != SUCCESS)  {
				yaf_trigger_error(YAF_ERR_AUTOLOAD_FAILED, "could not find class %s in controller script %s", class, directory);
				efree(class);
				efree(class_lowercase);
				efree(directory);
				return FALSE;
			} else if (!instanceof_function(*ce, yaf_controller_ce TSRMLS_CC)) {
				yaf_trigger_error(YAF_ERR_TYPE_ERROR, "controller must be an instance of %s", yaf_controller_ce->name);
				efree(class);
				efree(class_lowercase);
				efree(directory);
				return FALSE;
			}
		}

		efree(class);
		efree(class_lowercase);
		efree(directory);

		return *ce;
	}

	return NULL;
}
/* }}} */

/** {{{ zend_class_entry * yaf_dispatcher_get_action(char *app_dir, yaf_controller_t *controller, char *module, boolean def_module, char *action, int len TSRMLS_DC)
 */
zend_class_entry * yaf_dispatcher_get_action(char *app_dir, yaf_controller_t *controller, char *module, boolean def_module, char *action, int len TSRMLS_DC) {
	zval **ppaction 	= NULL;
	zval *actions_map 	= yaf_read_property(controller, YAF_CONTROLLER_PROPERTY_NAME_ACTIONS);

	if (IS_ARRAY == Z_TYPE_P(actions_map)) {
		if (zend_hash_find(Z_ARRVAL_P(actions_map), action, len + 1, (void **)&ppaction) == SUCCESS) {
			char *action_path 	= NULL;
			int	action_path_len = 0;

			action_path_len = spprintf(&action_path, 0, "%s%c%s", app_dir, DEFAULT_SLASH, Z_STRVAL_PP(ppaction));
			if (yaf_loader_import(action_path, action_path_len, FALSE TSRMLS_CC)) {
				zend_class_entry **ce   = NULL;
				char *class  			= NULL;
				char *class_lowercase 	= NULL;
				int  class_len 			= 0;

				char *action_upper	  	= estrndup(action, len);

				*(action_upper) = toupper(*action_upper);

				if (YAF_G(name_suffix)) {
					class_len = spprintf(&class, 0, "%s%s%s", action_upper, YAF_G(name_separator), "Action");
				} else {
					class_len = spprintf(&class, 0, "%s%s%s", "Action", YAF_G(name_separator), action_upper);
				}

				class_lowercase = zend_str_tolower_dup(class, class_len);

				if (zend_hash_find(EG(class_table), class_lowercase, class_len + 1, (void **) &ce) == SUCCESS) {
					efree(action_path);
					efree(action_upper);
					efree(class_lowercase);

					if (instanceof_function(*ce, yaf_action_ce TSRMLS_CC)) {
						efree(class);
						return *ce;
					} else {
						yaf_trigger_error(YAF_ERR_TYPE_ERROR, "action %s must extends from %s", class, yaf_action_ce->name);
						efree(class);
					}

				} else {
					yaf_trigger_error(YAF_ERR_NOTFOUND_ACTION, "could not find action %s in %s", class, action_path);
				}

				efree(action_path);
				efree(action_upper);
				efree(class);
				efree(class_lowercase);

			} else {
				yaf_trigger_error(YAF_ERR_NOTFOUND_ACTION, "could not find action script %s", action_path);
				efree(action_path);
			}
		} else {
			yaf_trigger_error(YAF_ERR_NOTFOUND_ACTION, "there is no method %s%s in %s::$%s", 
					action, "Action", Z_OBJCE_P(controller)->name, YAF_CONTROLLER_PROPERTY_NAME_ACTIONS);
		}
	} else {
		yaf_trigger_error(YAF_ERR_NOTFOUND_ACTION, "there is no method %s%s in %s", action, "Action", Z_OBJCE_P(controller)->name);
	}
	
	return NULL;
}
/* }}} */

/** {{{ boolean yaf_dispatcher_handle(yaf_dispatcher_t *dispatcher, yaf_request_t *request,  yaf_response_t *response, yaf_view_t *view TSRMLS_DC)
*/
boolean yaf_dispatcher_handle(yaf_dispatcher_t *dispatcher, yaf_request_t *request,  yaf_response_t *response, yaf_view_t *view TSRMLS_DC) {
	char *app_dir = YAF_G(directory);

	yaf_request_set_dispatched(request, TRUE TSRMLS_CC);

	if (!app_dir) {
		yaf_trigger_error(YAF_ERR_STARTUP_FAILED, "%s requires %s(which set the application.directory) to be initialized first", 
				yaf_dispatcher_ce->name, yaf_application_ce->name);
		return FALSE;
	} else {
		boolean 		is_def_module = FALSE;
		boolean 		is_def_ctr	  = FALSE;
		zval 			*module		  = NULL;
		zval 			*controller   = NULL;
		zend_class_entry *ce  		  = NULL;
		yaf_controller_t	*executor 	  = NULL;
		zend_function   *fptr		  = NULL;

		zval			*dmodule	  = NULL;
		zval 			*dcontroller  = NULL;
		zval 		*return_response  = NULL;
		zval 			*instantly_flush	  = NULL;

		module		= yaf_read_property(request, YAF_REQUEST_PROPERTY_NAME_MODULE);
		controller	= yaf_read_property(request, YAF_REQUEST_PROPERTY_NAME_CONTROLLER);

		dmodule		= yaf_read_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_MODULE);
		dcontroller = yaf_read_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_CONTROLLER);

		if (Z_TYPE_P(module) != IS_STRING
				|| !Z_STRLEN_P(module)) {
			yaf_trigger_error(YAF_ERR_DISPATCH_FAILED, "unexcepted a empty module name");
			return FALSE;
		} else if (!yaf_application_is_module_name(Z_STRVAL_P(module), Z_STRLEN_P(module) TSRMLS_CC)) {
			yaf_trigger_error(YAF_ERR_NOTFOUND_MODULE, "there is no module %s", Z_STRVAL_P(module));
			return FALSE;
		}

		if (Z_TYPE_P(controller) != IS_STRING
				|| !Z_STRLEN_P(controller)) {
			yaf_trigger_error(YAF_ERR_DISPATCH_FAILED, "unexcepted a empty controller name");
			return FALSE;
		}

		if(strncasecmp(Z_STRVAL_P(dmodule), Z_STRVAL_P(module), Z_STRLEN_P(module)) == 0) {
			is_def_module = TRUE;
		} 

		if (strncasecmp(Z_STRVAL_P(dcontroller), Z_STRVAL_P(controller), Z_STRLEN_P(controller)) == 0) {
			is_def_ctr = TRUE;
		} 

		ce = yaf_dispatcher_get_controller(app_dir, Z_STRVAL_P(module), Z_STRVAL_P(controller), Z_STRLEN_P(controller), is_def_module TSRMLS_CC);
		if (!ce) {
			return FALSE;
		} else {
			zval  *action			= NULL;
			char  *action_lower		= NULL;
			char  *func_name 		= NULL;
			int   func_name_len   	= 0;
			zval  *ret				= NULL;
			zval  *view_dir  		= NULL;
			zval  *render 			= NULL;

			yaf_controller_t	 *icontroller 	= NULL;

			MAKE_STD_ZVAL(icontroller);
			object_init_ex(icontroller, ce);

			/* cause controller's constructor is a final method, so it must be a internal function 
			   do { 
			   zend_function *constructor = NULL;
			   constructor = Z_OBJ_HT_P(exec_ctr)->get_constructor(exec_ctr TSRMLS_CC);
			   if (constructor != NULL) {
			   if (zend_call_method_with_2_params(&exec_ctr, *ce
			   , &constructor, NULL, &ret, request, response) == NULL) {
			   yaf_trigger_error(YAF_ERR_CALL_FAILED, "function call for %s::__construct failed", (*ce)->name);
			   return FALSE;
			   }
			   }
			   } while(0);
			*/
			yaf_controller_construct(icontroller, request, response, view, NULL TSRMLS_CC);

			MAKE_STD_ZVAL(view_dir);
			Z_TYPE_P(view_dir) = IS_STRING;

			if (is_def_module) {
				Z_STRLEN_P(view_dir) = spprintf(&(Z_STRVAL_P(view_dir)), 0, "%s/%s", app_dir ,"views");
			} else {
				Z_STRLEN_P(view_dir) = spprintf(&(Z_STRVAL_P(view_dir)), 0, "%s/%s/%s/%s", app_dir,
						"modules", Z_STRVAL_P(module), "views");
			}

			/** tell the view engine where to find templates */
			if (Z_OBJCE_P(view) == yaf_view_simple_ce) {
				yaf_update_property(view, YAF_VIEW_PROPERTY_NAME_TPLDIR, view_dir);
			} else {
				zend_call_method_with_1_params(&view, Z_OBJCE_P(view), NULL, "setscriptpath", NULL, view_dir);
			}

			zval_ptr_dtor(&view_dir);

			yaf_update_property(icontroller, YAF_CONTROLLER_PROPERTY_NAME_NAME,	controller);

			action		 = yaf_read_property(request, YAF_REQUEST_PROPERTY_NAME_ACTION);
			action_lower = zend_str_tolower_dup(Z_STRVAL_P(action), Z_STRLEN_P(action));

			/* cause the action might call the forward to override the old action */
			Z_ADDREF_P(action);

			func_name_len    = spprintf(&func_name,  0, "%s%s", action_lower, "action");
			efree(action_lower);

			if (zend_hash_find(&((ce)->function_table), func_name, func_name_len + 1, (void **)&fptr) == SUCCESS) {
				zval ***call_args   = NULL;
				uint count          = 0;

				executor = icontroller;

				if (fptr->common.num_args) {
					yaf_dispatcher_get_call_parmaters(request, fptr, &call_args, &count TSRMLS_CC);
					zval *method_name = NULL;
					MAKE_STD_ZVAL(method_name);
					ZVAL_STRINGL(method_name, func_name, func_name_len, FALSE);
					call_user_function_ex(&(ce)->function_table, &icontroller, 
							method_name, &ret, count, call_args, TRUE, NULL TSRMLS_CC);
					efree(method_name);
					efree(call_args);
				} else {
					zend_call_method(&icontroller, ce, NULL, func_name, func_name_len, &ret, 0, NULL, NULL TSRMLS_CC);
				}

				efree(func_name);

				if (!ret ||( Z_TYPE_P(ret) == IS_BOOL
						&& !Z_BVAL_P(ret))) {
					Z_DELREF_P(action);

					zval_dtor(icontroller);
					efree(icontroller);

					return FALSE;
				}

			} else {

				ce = yaf_dispatcher_get_action(app_dir, icontroller, Z_STRVAL_P(module), is_def_module, Z_STRVAL_P(action), Z_STRLEN_P(action) TSRMLS_CC);
				zval_dtor(icontroller);
				efree(icontroller);

				if (ce) {
					if (zend_hash_find(&(ce)->function_table, YAF_ACTION_EXECUTOR_NAME,
								sizeof(YAF_ACTION_EXECUTOR_NAME), (void **)&fptr) == SUCCESS) {
						zval ***call_args = NULL;
						uint count       = 0;
						yaf_action_t *iaction = NULL;

						MAKE_STD_ZVAL(iaction);
						object_init_ex(iaction, ce);

						yaf_controller_construct(iaction, request, response, view, NULL TSRMLS_CC);
						executor = iaction;

						yaf_update_property(iaction, YAF_CONTROLLER_PROPERTY_NAME_NAME, controller);

						if (fptr->common.num_args) {
							yaf_dispatcher_get_call_parmaters(request, fptr, &call_args, &count TSRMLS_CC);
							zval *method_name = NULL;
							MAKE_STD_ZVAL(method_name);
							ZVAL_STRINGL(method_name, YAF_ACTION_EXECUTOR_NAME, sizeof(YAF_ACTION_EXECUTOR_NAME) - 1, FALSE);
							call_user_function_ex(&(ce)->function_table, &iaction, method_name,
								   	&ret, count, call_args, TRUE, NULL TSRMLS_CC);
							efree(method_name);
							efree(call_args);
						} else {
							zend_call_method_with_0_params(&iaction, ce, NULL, "execute", &ret);
						}

						if (!ret ||( Z_TYPE_P(ret) == IS_BOOL
									&& !Z_BVAL_P(ret))) {
							Z_DELREF_P(action);

							zval_dtor(iaction);
							efree(iaction);

							return FALSE;
						} 
					} else {
						/** should never reached here */
						yaf_trigger_error(YAF_ERR_NOTFOUND_ACTION, "could not find execute method in %s", ce->name);
						return FALSE;
					}	

				} else {
					Z_DELREF_P(action);
					return FALSE;
				}
			}

			render = yaf_read_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_RENDER);
			return_response = yaf_read_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_RETURN);
			instantly_flush	= yaf_read_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_FLUSH);

			if (executor && Z_BVAL_P(render)) {
				MAKE_STD_ZVAL(ret);
				ZVAL_NULL(ret);

				if (!Z_BVAL_P(instantly_flush)) {
					zend_call_method_with_1_params(&executor, ce, NULL, "render", &ret, action);
					zval_dtor(executor);
					efree(executor);

					if (ret && Z_TYPE_P(ret) == IS_STRING && Z_STRLEN_P(ret)) {
						yaf_response_alter_body(response, NULL, 0, Z_STRVAL_P(ret), Z_STRLEN_P(ret), FALSE TSRMLS_CC);
					} else {
						efree(ret);
						Z_DELREF_P(action);
						return FALSE;
					}
				} else {
					zend_call_method_with_1_params(&executor, ce, NULL, "display", &ret, action);
					zval_dtor(executor);
					efree(executor);

					if (!ret || (Z_TYPE_P(ret) == IS_BOOL && !Z_BVAL_P(ret))) {
						efree(ret);
						Z_DELREF_P(action);
						return FALSE;
					}
				}

			}

			Z_DELREF_P(action);
			efree(ret);

			return TRUE;
		}
	}	

	return FALSE;
}
/* }}} */

/** {{{ void yaf_dispatcher_exception_handler(yaf_dispatcher_t *dispatcher, yaf_request_t *request, yaf_response_t *response TSRMLS_DC)
*/
void yaf_dispatcher_exception_handler(yaf_dispatcher_t *dispatcher, yaf_request_t *request, yaf_response_t *response TSRMLS_DC) {
	zval *module 	 = NULL;
	zval *controller = NULL;
	zval *action	 = NULL;
	zval *exception	 = NULL;
	yaf_view_t  *view = NULL;

	if (YAF_G(in_exception) || !EG(exception)) {
		return;
	}

	YAF_G(in_exception) = TRUE;

	MAKE_STD_ZVAL(controller);
	MAKE_STD_ZVAL(action);

	module = yaf_read_property(request, YAF_REQUEST_PROPERTY_NAME_MODULE);

	if (!module || Z_TYPE_P(module) != IS_STRING || !Z_STRLEN_P(module)) {
		module = yaf_read_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_MODULE);
		yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_MODULE, module);
	}

	ZVAL_STRING(controller, YAF_ERROR_CONTROLLER, TRUE);
	ZVAL_STRING(action, 	YAF_ERROR_ACTION, TRUE);

	exception = EG(exception);
	EG(exception) = NULL;

	yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_CONTROLLER, controller);
	yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_ACTION, action);
	yaf_update_property(request, YAF_REQUEST_PROPERTY_NAME_EXCEPTION, exception);

	Z_DELREF_P(controller);
	Z_DELREF_P(action);

	/** use $request->getException() instand of */
	yaf_request_set_params_single(request, YAF_STRL("exception"), exception TSRMLS_CC);
	yaf_request_set_dispatched(request, FALSE TSRMLS_CC);

	view = yaf_dispatcher_init_view(dispatcher, NULL, NULL TSRMLS_CC);

	if (!yaf_dispatcher_handle(dispatcher, request, response, view TSRMLS_CC)) {
		return;
	}

	(void)yaf_response_send(response TSRMLS_CC);

	YAF_EXCEPTION_ERASE_EXCEPTION();
}
/* }}} */

/** {{{ boolean yaf_dispatcher_route(yaf_dispatcher_t *dispatcher, yaf_request_t *request TSRMLS_DC)
 */
boolean yaf_dispatcher_route(yaf_dispatcher_t *dispatcher, yaf_request_t *request TSRMLS_DC) {
	yaf_router_t *router = NULL;
	
	router	= yaf_read_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_ROUTER);

	if (IS_OBJECT == Z_TYPE_P(router) && Z_OBJCE_P(router) == yaf_router_ce) {
		/* use buildin router */
		if (!yaf_router_route(router, request TSRMLS_CC)) {
			return FALSE;
		}
	} else {
		/* user custom router */
		zval *ret = NULL;
		ret = zend_call_method_with_1_params(&router, Z_OBJCE_P(router), NULL, "route", &ret, request);

		if (Z_TYPE_P(ret) == IS_BOOL
				&& Z_BVAL_P(ret) == 0) {
			yaf_trigger_error(YAF_ERR_ROUTE_FAILED, "calling for %s::route failed", Z_OBJCE_P(router)->name);
			return FALSE;
		}
	}

	return TRUE;
}
/* }}} */

/** {{{ yaf_response_t * yaf_dispatcher_dispatch(yaf_dispatcher_t *dispatcher TSRMLS_DC)
*/
yaf_response_t * yaf_dispatcher_dispatch(yaf_dispatcher_t *dispatcher TSRMLS_DC) {
	uint 			nesting	 = YAF_G(forward_limit);
	yaf_response_t  *response = NULL;
	yaf_request_t	*request = NULL;
	zval 	*return_response = NULL;
	zval 			*plugins = NULL;
	zval 			*view	 = NULL;

	response = yaf_response_instance(NULL, sapi_module.name TSRMLS_CC);
	request	 = yaf_read_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_REQUEST);
	plugins	 = yaf_read_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_PLUGINS);

	if (!request
			|| IS_OBJECT != Z_TYPE_P(request)) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::dispatch expects a %s instance", yaf_dispatcher_ce->name, yaf_request_ce->name);
		return NULL;
	}

	/* route request */
	if (!yaf_request_is_routed(request TSRMLS_CC)) {
		YAF_PLUGIN_HANDLE(plugins, YAF_PLUGIN_HOOK_ROUTESTARTUP, request, response);
		YAF_EXCEPTION_HANDLE(dispatcher, request, response);
		if (yaf_dispatcher_route(dispatcher, request TSRMLS_CC) == FAILURE) {
			yaf_trigger_error(YAF_ERR_ROUTE_FAILED, "route request failed");
			YAF_EXCEPTION_HANDLE_NORET(dispatcher, request, response);
			return NULL;
		}
		YAF_PLUGIN_HANDLE(plugins, YAF_PLUGIN_HOOK_ROUTESHUTDOWN, request, response);
		YAF_EXCEPTION_HANDLE(dispatcher, request, response);
		(void)yaf_request_set_routed(request, TRUE TSRMLS_CC);
	}

	yaf_dispatcher_fix_default(dispatcher, request TSRMLS_CC);

	YAF_PLUGIN_HANDLE(plugins, YAF_PLUGIN_HOOK_LOOPSTARTUP, request, response);
	YAF_EXCEPTION_HANDLE(dispatcher, request, response);

	view = yaf_dispatcher_init_view(dispatcher, NULL, NULL TSRMLS_CC);

	do {
		YAF_PLUGIN_HANDLE(plugins, YAF_PLUGIN_HOOK_PREDISPATCH, request, response);
		if (!yaf_dispatcher_handle(dispatcher, request, response, view TSRMLS_CC)) {
			YAF_EXCEPTION_HANDLE(dispatcher, request, response);
			return NULL;
		}
		yaf_dispatcher_fix_default(dispatcher, request TSRMLS_CC);
		YAF_PLUGIN_HANDLE(plugins, YAF_PLUGIN_HOOK_POSTDISPATCH, request, response);
		YAF_EXCEPTION_HANDLE(dispatcher, request, response);
	} while (--nesting > 0 && !yaf_request_is_dispatched(request TSRMLS_CC));

	YAF_PLUGIN_HANDLE(plugins, YAF_PLUGIN_HOOK_LOOPSHUTDOWN, request, response);
	YAF_EXCEPTION_HANDLE(dispatcher, request, response);

	if (0 == nesting
			&& !yaf_request_is_dispatched(request TSRMLS_CC)) {
		yaf_trigger_error(YAF_ERR_DISPATCH_FAILED, "reache the max dispatch nesting %ld", YAF_G(forward_limit));
		YAF_EXCEPTION_HANDLE_NORET(dispatcher, request, response);
		return NULL;
	}

	return_response = yaf_read_property(dispatcher, YAF_DISPATCHER_PROPERTY_NAME_RETURN);

	if (!Z_BVAL_P(return_response)) {
		(void)yaf_response_send(response TSRMLS_CC);
		yaf_response_clear_body(response TSRMLS_CC);
	}

	return response;
}
/* }}} */

/** {{{ proto private Yaf_Dispatcher::__construct(void) 
*/
PHP_METHOD(yaf_dispatcher, __construct) {
}
/* }}} */

/** {{{ proto private Yaf_Dispatcher::__sleep(void)
*/
PHP_METHOD(yaf_dispatcher, __sleep) {
}
/* }}} */

/** {{{ proto private Yaf_Dispatcher::__wakeup(void)
*/
PHP_METHOD(yaf_dispatcher, __wakeup) {
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::setErrorHandler(string $callbacak[, int $error_types = E_ALL | E_STRICT ] )
*/
PHP_METHOD(yaf_dispatcher, setErrorHandler) {
	zval *callback 	 = NULL;
	zval *error_type = NULL;
	zval *params[2]	 = {0};
	zval function	 = {{0}, 0};

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &callback, &error_type) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	params[0] = callback;
	if (error_type) {
		params[1] = error_type;
	}

	ZVAL_STRING(&function, "set_error_handler", 0);
	if (call_user_function(EG(function_table), NULL, &function, return_value, ZEND_NUM_ARGS(), params TSRMLS_CC) == FAILURE) {
		yaf_trigger_error(YAF_ERR_ERROR, "call to set_error_handler failed");
	}
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::disableView() 
*/
PHP_METHOD(yaf_dispatcher, disableView) {
	yaf_dispatcher_t *self = getThis();
	zend_update_property_bool(Z_OBJCE_P(self), self, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_RENDER), 0 TSRMLS_CC);
	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::enableView() 
*/
PHP_METHOD(yaf_dispatcher, enableView) {
	yaf_dispatcher_t *self = getThis();
	zend_update_property_bool(Z_OBJCE_P(self), self, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_RENDER), 1 TSRMLS_CC);
	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::returnResponse() 
*/
PHP_METHOD(yaf_dispatcher, returnResponse) {
	yaf_dispatcher_t *self = getThis();
	long	auto_response = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &auto_response) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	zend_update_property_bool(Z_OBJCE_P(self), self, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_RETURN), (auto_response)? TRUE:FALSE TSRMLS_CC);

	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::flushInstantly() 
*/
PHP_METHOD(yaf_dispatcher, flushInstantly) {
	yaf_dispatcher_t *self = getThis();
	long	instantly_flush = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &instantly_flush) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	zend_update_property_bool(Z_OBJCE_P(self), self, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_FLUSH), (instantly_flush)? TRUE:FALSE TSRMLS_CC);

	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::registerPlugin(Yaf_Plugin_Abstract $plugin)
*/
PHP_METHOD(yaf_dispatcher, registerPlugin) {
	zval 			* plugin  = NULL;
	zval 			* plugins = NULL;
	yaf_dispatcher_t *self 	  = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &plugin) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (Z_TYPE_P(plugin) != IS_OBJECT
			|| !instanceof_function(Z_OBJCE_P(plugin), yaf_plugin_ce TSRMLS_CC)) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::registerPlugin expects a %s instance", yaf_dispatcher_ce->name, yaf_plugin_ce->name);
		RETURN_FALSE;
	}

	plugins = yaf_read_property(self, YAF_DISPATCHER_PROPERTY_NAME_PLUGINS);

	Z_ADDREF_P(plugin);
	add_next_index_zval(plugins, plugin);

	RETVAL_ZVAL(self, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::setRequest(Yaf_Request_Abstract $request) 
*/
PHP_METHOD(yaf_dispatcher, setRequest) {
	yaf_request_t	*request = NULL;
	yaf_dispatcher_t	*self	 = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &request) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (!request
			|| IS_OBJECT != Z_TYPE_P(request)) {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::dispatch expects a %s instance", yaf_dispatcher_ce->name, yaf_request_ce->name);
		RETURN_FALSE;
	} 
	
	self = getThis();

	if (yaf_dispatcher_set_request(self, request TSRMLS_CC)) {
		RETURN_ZVAL(self, 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::getInstance(void) 
*/
PHP_METHOD(yaf_dispatcher, getInstance) {
	yaf_dispatcher_t *dispatcher = NULL;
	dispatcher = yaf_dispatcher_instance(NULL TSRMLS_CC);
	RETURN_ZVAL(dispatcher, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::getRouter(void) 
*/
PHP_METHOD(yaf_dispatcher, getRouter) {
	yaf_router_t *router = yaf_read_property(getThis(), YAF_DISPATCHER_PROPERTY_NAME_ROUTER);
	RETURN_ZVAL(router, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::getRequest(void) 
*/
PHP_METHOD(yaf_dispatcher, getRequest) {
	yaf_request_t *request = yaf_read_property(getThis(), YAF_DISPATCHER_PROPERTY_NAME_REQUEST);
	RETURN_ZVAL(request, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::getApplication(void) 
*/
PHP_METHOD(yaf_dispatcher, getApplication) {
	PHP_MN(yaf_application_app)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::dispatch(yaf_request_t $request)
*/
PHP_METHOD(yaf_dispatcher, dispatch) {
	yaf_request_t 	*request 	= NULL;
	yaf_response_t 	*response 	= NULL;
	yaf_dispatcher_t *self		= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &request) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	self = getThis();
	yaf_update_property(self, YAF_DISPATCHER_PROPERTY_NAME_REQUEST, request);
	if ((response = yaf_dispatcher_dispatch(self TSRMLS_CC))) {
		RETURN_ZVAL(response, 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::throwException(bool $flag=0) 
*/
PHP_METHOD(yaf_dispatcher, throwException) {
	int flag = -1;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &flag) == FAILURE) {
		WRONG_PARAM_COUNT;
	}	

	if (flag == -1)  {
		RETURN_FALSE;
	}

	YAF_G(throw_exception) = flag? 1: 0;
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::catchException(bool $flag=0) 
*/
PHP_METHOD(yaf_dispatcher, catchException) {
	int flag = -1;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &flag) == FAILURE) {
		WRONG_PARAM_COUNT;
	}	

	if (flag == -1)  {
		RETURN_FALSE;
	}

	YAF_G(catch_exception) = flag? 1: 0;
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::autoRender(boolean $flag)                             
 */                                                                                        
PHP_METHOD(yaf_dispatcher, autoRender) {                                                   
    long flag = 0;                                                                        
    yaf_dispatcher_t *self = getThis();                                                    
                                                                                          
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &flag) == FAILURE) {        
        WRONG_PARAM_COUNT;                                                                
    }                                                                                     
                                                                                          
    zend_update_property_bool(Z_OBJCE_P(self), self, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_RENDER), flag? 1 : 0 TSRMLS_CC);
                                                                                          
    RETURN_ZVAL(self, 1, 0);                                                              
}                                                                                         
/* }}} */     

/** {{{ proto public Yaf_Dispatcher::initView(string $tpl_dir, array $options = NULL)
*/
PHP_METHOD(yaf_dispatcher, initView) {
	yaf_view_t 	*view 	 = NULL;
	zval 		*tpl_dir = NULL;
	zval 		*options = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &tpl_dir, &options) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	view = yaf_dispatcher_init_view(getThis(), tpl_dir, options TSRMLS_CC);

	if (view) {
		RETURN_ZVAL(view, 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::setView(void)
*/
PHP_METHOD(yaf_dispatcher, setView) {
	yaf_view_t		*view 	= NULL;
	yaf_dispatcher_t *self	= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &view) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	self = getThis();
	if (view && IS_OBJECT == Z_TYPE_P(view)
			&& instanceof_function(Z_OBJCE_P(view), yaf_view_interface_ce TSRMLS_CC)) {

		yaf_update_property(self, YAF_DISPATCHER_PROPERTY_NAME_VIEW,  view);
		RETURN_ZVAL(self, 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::setAppDirectory(string $directory) 
*/
PHP_METHOD(yaf_dispatcher, setAppDirectory) {
	char *directory = NULL;
	int	 len		= 0;

	yaf_dispatcher_t *self = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &directory, &len) == FAILURE || !len) {
		WRONG_PARAM_COUNT;
	}

	if (!len || !IS_ABSOLUTE_PATH(directory, len)) {
		RETURN_FALSE;
	}

	efree(YAF_G(directory));
	
	YAF_G(directory) = estrndup(directory, len);

	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::setDefaultModule(string $name)
*/
PHP_METHOD(yaf_dispatcher, setDefaultModule) {
	zval *module = NULL;
	zval *self   = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &module) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (module && IS_STRING == Z_TYPE_P(module) && Z_STRLEN_P(module) 
			&& yaf_application_is_module_name(Z_STRVAL_P(module), Z_STRLEN_P(module) TSRMLS_CC)) {
		zval *module_std = NULL;
		MAKE_STD_ZVAL(module_std);
		ZVAL_STRING(module_std, zend_str_tolower_dup(Z_STRVAL_P(module), Z_STRLEN_P(module)), FALSE);
		*Z_STRVAL_P(module_std) = toupper(*Z_STRVAL_P(module_std));
		yaf_update_property(self, YAF_DISPATCHER_PROPERTY_NAME_MODULE, module_std);
		
		RETURN_ZVAL(self, 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::setDefaultController(string $name)
*/
PHP_METHOD(yaf_dispatcher, setDefaultController) {
	zval *controller = NULL;
	zval *self   	 = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &controller) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (controller && IS_STRING == Z_TYPE_P(controller) && Z_STRLEN_P(controller)) {
		zval *controller_std = NULL;
		MAKE_STD_ZVAL(controller_std);
		ZVAL_STRING(controller_std, zend_str_tolower_dup(Z_STRVAL_P(controller), Z_STRLEN_P(controller)), FALSE);
		*Z_STRVAL_P(controller_std) = toupper(*Z_STRVAL_P(controller_std));
		yaf_update_property(self, YAF_DISPATCHER_PROPERTY_NAME_CONTROLLER, controller_std);
		
		RETURN_ZVAL(self, 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::setDefaultAction(string $name)
*/
PHP_METHOD(yaf_dispatcher, setDefaultAction) {
	zval *action = NULL;
	zval *self   = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &action) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (action && IS_STRING == Z_TYPE_P(action) && Z_STRLEN_P(action)) {
		zval *action_lower = NULL;
		MAKE_STD_ZVAL(action_lower);
		ZVAL_STRING(action_lower, zend_str_tolower_dup(Z_STRVAL_P(action), Z_STRLEN_P(action)), FALSE);
		yaf_update_property(self, YAF_DISPATCHER_PROPERTY_NAME_ACTION, action_lower);
		
		RETURN_ZVAL(self, 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Dispatcher::__desctruct(void)
*/
PHP_METHOD(yaf_dispatcher, __destruct) {
}
/* }}} */

/** {{{ proto private Yaf_Dispatcher::__clone(void)
*/
PHP_METHOD(yaf_dispatcher, __clone) {
}
/* }}} */

/** {{{ yaf_dispatcher_methods 
*/
zend_function_entry yaf_dispatcher_methods[] = {
	PHP_ME(yaf_dispatcher, __construct, 			NULL, ZEND_ACC_PRIVATE|ZEND_ACC_CTOR) 
	PHP_ME(yaf_dispatcher, __clone,				NULL, ZEND_ACC_PRIVATE|ZEND_ACC_CLONE)
	PHP_ME(yaf_dispatcher, __sleep,				NULL, ZEND_ACC_PRIVATE)
	PHP_ME(yaf_dispatcher, __wakeup,				NULL, ZEND_ACC_PRIVATE)
	PHP_ME(yaf_dispatcher, enableView,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, disableView,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, initView,				NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, setView,				NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, setRequest,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, getApplication,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, getRouter,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, getRequest,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, setErrorHandler,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, setAppDirectory,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, setDefaultModule,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, setDefaultController,	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, setDefaultAction,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, returnResponse,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, autoRender,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, flushInstantly,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, getInstance,			NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yaf_dispatcher, dispatch,				NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, throwException,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, catchException,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_dispatcher, registerPlugin,		NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ YAF_STARTUP_FUNCTION
*/
YAF_STARTUP_FUNCTION(dispatcher) {
	zend_class_entry ce;

	YAF_INIT_CLASS_ENTRY(ce, "Yaf_Dispatcher", "Yaf\\Dispatcher", yaf_dispatcher_methods);

	yaf_dispatcher_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	yaf_dispatcher_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_null(yaf_dispatcher_ce, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_ROUTER), 	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_dispatcher_ce, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_VIEW), 	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_dispatcher_ce, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_REQUEST), 	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_dispatcher_ce, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_PLUGINS), 	ZEND_ACC_PROTECTED TSRMLS_CC);

	zend_declare_property_null(yaf_dispatcher_ce, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_INSTANCE), ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);

	zend_declare_property_bool(yaf_dispatcher_ce, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_RENDER),	1,  ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_bool(yaf_dispatcher_ce, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_RETURN),   0, ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_bool(yaf_dispatcher_ce, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_FLUSH), 	0, ZEND_ACC_PROTECTED TSRMLS_CC);

	zend_declare_property_null(yaf_dispatcher_ce, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_MODULE), 		ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_dispatcher_ce, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_CONTROLLER), 	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_dispatcher_ce, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_ACTION), 	 	ZEND_ACC_PROTECTED TSRMLS_CC);

	/*zend_declare_property_null(yaf_dispatcher_ce, YAF_STRL(YAF_DISPATCHER_PROPERTY_NAME_ARGS), 	 	ZEND_ACC_PROTECTED TSRMLS_CC);*/

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
