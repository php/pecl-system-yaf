/*
   +----------------------------------------------------------------------+
   | Yet Another Framework project                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010 laruence		                                  |
   | http://www.laruence.com/                                             |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   $Id$ 
   */

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_interfaces.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_alloc.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "zend_objects.h"

#include "php_yaf.h"
#include "yaf_exception.h"
#include "yaf_autoloader.h"

/** {{{ yaf_import_file
*/
int yaf_import_file(char * file_path, int file_path_len, int use_path TSRMLS_DC) {

	zend_file_handle file_handle;

	if (php_stream_open_for_zend_ex(file_path, &file_handle, ENFORCE_SAFE_MODE|IGNORE_URL_WIN|STREAM_OPEN_FOR_INCLUDE TSRMLS_CC) == SUCCESS) {
		//if (zend_stream_open(file_path, &file_handle TSRMLS_CC) == SUCCESS) {
		zend_op_array *new_op_array;
		unsigned int laruence = 1;
		if (!file_handle.opened_path) {
			file_handle.opened_path = estrndup(file_path, file_path_len);
		}

		if (zend_hash_add(&EG(included_files), file_handle.opened_path, strlen(file_handle.opened_path)+1, (void *)&laruence, sizeof(int), NULL)==SUCCESS) {
			new_op_array = zend_compile_file(&file_handle, ZEND_REQUIRE TSRMLS_CC);
			zend_destroy_file_handle(&file_handle TSRMLS_CC);
		} else {
			new_op_array = NULL;
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
			zend_file_handle_dtor(&file_handle TSRMLS_CC);
#else
			zend_file_handle_dtor(&file_handle);
#endif
		}
		if (new_op_array) {
			zval *result = NULL;
			YAF_STORE_EG_ENVIRON();
			EG(return_value_ptr_ptr) = &result;
			EG(active_op_array) = new_op_array;

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
			if (!EG(active_symbol_table)) {
				zend_rebuild_symbol_table(TSRMLS_C);
			}
#endif
			zend_execute(new_op_array TSRMLS_CC);

			destroy_op_array(new_op_array TSRMLS_CC);
			efree(new_op_array);
			if (!EG(exception)) {
				if (EG(return_value_ptr_ptr)) {
					zval_ptr_dtor(EG(return_value_ptr_ptr));
				}
			}
			YAF_RESTORE_EG_ENVIRON();
		}
	} else {
		return FAILURE;
	}
	return SUCCESS;
	}
	/* }}} */

/** {{{ yaf_internal_autoload
 * autolaod internal 
 */
YAF_FUNC int yaf_internal_autoload(char * lcname, char *class_name, char * directory TSRMLS_DC) {

	char * file_path, * p, * q;
	int use_path 		= 0;
	int	file_path_len 	= 0;
	char * library_path = YAF_G(library_directory);
	char * ext 			= YAF_G(ext);

	if (directory == NULL) {
		file_path_len  = spprintf(&file_path, 0, "%s", library_path);
	} else {
		file_path_len  = spprintf(&file_path, 0, "%s", "");
	}

	p = lcname;
	q = p;

loop:
	while(++q && *q != '_' && *q != '\0');

	if (*q != '\0') {
		char * seg = estrndup(p, q - p);
		file_path_len = spprintf(&file_path, 0, "%s/%s", file_path, seg);
		p = q + 1;
		goto loop;
	} else {
		file_path_len = spprintf(&file_path, 0, "%s/%s.%s", file_path, p, ext);
	}

	if (directory != NULL) {
		file_path_len = spprintf(&file_path, 0, "%s%s", directory, file_path);
	}

	if (yaf_import_file(file_path, file_path_len + 1, use_path TSRMLS_CC) == FAILURE) {
		return 0; 
	}

	return zend_hash_exists(EG(class_table), zend_str_tolower_dup(class_name, strlen(class_name)), strlen(class_name));
}
/* }}} */

/** {{{ proto yaf_autoloader($class_name)
*/
PHP_FUNCTION(yaf_autoload) {
	char * lcname;
	int	   lcname_len;

	char * base_dir;
	char * controller_suffix ;
	char * model_suffix;
	char * plugin_suffix;
	char * reserved;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s"
				,&lcname, &lcname_len) == FAILURE) {
		WRONG_PARAM_COUNT;
	} 

#ifdef YAF_HAVE_NAMESPACE
	do {
		char * origin_lcname = estrndup_rel(lcname, lcname_len);
		int pos = 0;
		lcname 	= origin_lcname;
		while (pos++ < lcname_len) {
			if (*(origin_lcname + pos) == '\\') {
				*(origin_lcname + pos) = '_';
			}
		}
	} while (0);
#endif

	base_dir		  = "";
	controller_suffix = "Controller";
	model_suffix	  = "Model";
	plugin_suffix	  = "Plugin";
	reserved		  = "Yaf_";

	if (strncmp(lcname, reserved, strlen(reserved)) == 0) {
		yaf_strict("You should not use '%s' as class name prefix", reserved);
	}

	if (strncmp(lcname + lcname_len - strlen(controller_suffix)
				,controller_suffix, strlen(controller_suffix)) == 0) {
		/* this is a controller Class
		 * but controller should not be auto laoded */
		yaf_warn("Controller %s should not be autoloaded", lcname);
	} else if (strncmp(lcname + lcname_len - strlen(model_suffix)
				, model_suffix, strlen(model_suffix)) == 0){
		/* this is a Model Class */
		char * model_path, * file;
		zval * request, * module;

		request = yaf_read_property(YAF_G(instance), "_request");
		module  = yaf_read_property(request, "module");

		if (strncasecmp(Z_STRVAL_P(module), YAF_G(default_module), Z_STRLEN_P(module)) == 0) {
			spprintf(&model_path, 0, "%s/%s", YAF_G(directory), YAF_MODEL_DIRECTORY_NAME);
		} else {
			spprintf(&model_path, 0, "%s/%s/%s/%s", YAF_G(directory), YAF_MODEL_DIRECTORY_NAME, Z_STRVAL_P(module), YAF_MODEL_DIRECTORY_NAME);
		}

		file = estrndup(lcname, lcname_len - strlen(model_suffix));
		RETURN_BOOL(yaf_internal_autoload(file, lcname, model_path TSRMLS_CC)); 
	} else if (strncmp(lcname + lcname_len - strlen(plugin_suffix)
				,plugin_suffix, strlen(plugin_suffix)) == 0) {
		char * plugin_path, * file;
		spprintf(&plugin_path, 0, "%s/%s", YAF_G(directory), YAF_PLUGIN_DIRECTORY_NAME);

		file = estrndup(lcname, lcname_len - strlen(plugin_suffix));
		RETURN_BOOL(yaf_internal_autoload(file, lcname, plugin_path TSRMLS_CC));
	} else {
		RETURN_BOOL(yaf_internal_autoload(lcname, lcname, NULL TSRMLS_CC));
	}

	if (YAF_G(use_spl_autoload)) {
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}
/* }}} */

/** {{{ proto yaf_override_spl_autoload($class_name)
*/
PHP_FUNCTION(yaf_override_spl_autoload) {
	yaf_trigger_error(YAF_ERR_WARNING, "%s is disabled by yaf.use_spl_autoload", YAF_SPL_AUTOLOAD_REGISTER_NAME);
	RETURN_BOOL(0);
}
/* }}} */

/** {{{ yaf_register_autoloader 
*/
int yaf_register_autoloader(TSRMLS_D) {
	if (YAF_G(autoload_started)) {
		return SUCCESS;
	} else {
		zval * ret = NULL, * autoload, * function;
		zval ** params[1] = {&autoload};

		MAKE_STD_ZVAL(autoload);
		MAKE_STD_ZVAL(function);
		ZVAL_STRING(function, YAF_SPL_AUTOLOAD_REGISTER_NAME, 0);
		ZVAL_STRING(autoload, YAF_AUTOLOAD_FUNC_NAME, 0);

		do {
			zend_fcall_info fci = {
				sizeof(fci),
				EG(function_table),
				function,
				NULL,
				&ret,
				1,
				(zval ***)params,
				NULL,
				1
			};

			if (zend_call_function(&fci, NULL TSRMLS_CC) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "Cant not register autoload function %s", YAF_AUTOLOAD_FUNC_NAME);
				return FAILURE;
			}

			if (0 && !YAF_G(use_spl_autoload)) {
				zend_function *reg_function;
				zend_internal_function override_function = {
					ZEND_INTERNAL_FUNCTION,
					YAF_AUTOLOAD_FUNC_NAME,
					NULL,
					ZEND_ACC_PUBLIC,
					NULL,
					1,
					0,
					NULL,
					0,
					0,
					ZEND_FN(yaf_override_spl_autoload),
					NULL /* struct _zend_module_entry *module*/
				};
				/* {{{
				   zend_internal_function *internal_function = (zend_internal_function *)&override_function;
				   internal_function->type 	= ZEND_INTERNAL_FUNCTION;
				   internal_function->module 	= NULL;
				   internal_function->handler 	= ZEND_FN(yaf_override_spl_autoload);
				   internal_function->function_name = YAF_AUTOLOAD_FUNC_NAME;
				   internal_function->scope 	=  NULL;
				   internal_function->prototype = NULL;
				   internal_function->arg_info  = NULL;
				   internal_function->num_args  = 1;
				   internal_function->required_num_args = 0;
				   internal_function->pass_rest_by_reference = 0;
				   internal_function->return_reference = 0;
				   internal_function->fn_flags = ZEND_ACC_PUBLIC;
				   }}} */
				function_add_ref((zend_function*)&override_function);
				/* zend_register_functions */
				if (zend_hash_update(EG(function_table), YAF_SPL_AUTOLOAD_REGISTER_NAME, 
							sizeof(YAF_SPL_AUTOLOAD_REGISTER_NAME), &override_function, sizeof(zend_function), (void **)&reg_function) == FAILURE) {
					YAF_DEBUG("Register autoload failed");
					/* no big deal */
				}
			}
		} while (0);
		YAF_G(autoload_started) = 1;
	}
	return SUCCESS;
}
/* }}} */

/** {{{ YAF_STARTUP_FUNCTION
*/
YAF_STARTUP_FUNCTION(autoloader) {
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
