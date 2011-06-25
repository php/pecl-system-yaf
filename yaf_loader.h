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
   $Id: yaf_loader.h 53 2011-06-01 02:57:04Z laruence $
 */

#ifndef YAF_LOADER_H
#define YAF_LOADER_H

#define YAF_SPL_AUTOLOAD_REGISTER_NAME 		"spl_autoload_register"
#define YAF_AUTOLOAD_FUNC_NAME 				"autoload"
#define YAF_LOADER_PROPERTY_NAME_INSTANCE	"_instance"
#define YAF_LOADER_PROPERTY_NAME_NAMESPACE	"_local_ns"

#define YAF_LOADER_CONTROLLER				"Controller"
#define YAF_LOADER_LEN_CONTROLLER			10
#define YAF_LOADER_MODEL						"Model"
#define YAF_LOADER_LEN_MODEL					5
#define YAF_LOADER_PLUGIN					"Plugin"
#define YAF_LOADER_LEN_PLUGIN				6
#define YAF_LOADER_RESERVERD					"Yaf_"
#define YAF_LOADER_LEN_RESERVERD				3

#define	YAF_LOADER_PROPERTY_NAME_LIBRARY		"_library"
#define YAF_LOADER_PROPERTY_NAME_GLOBAL_LIB  "_global_library"

#ifndef YAF_STANDARD
#define YAF_LOADER_DAO						"Dao_"
#define YAF_LOADER_LEN_DAO					4	
#define YAF_LOADER_SERVICE					"Service_"
#define YAF_LOADER_LEN_SERVICE				8
#endif


#ifdef PHP5_3P

#define YAF_STORE_EG_ENVIRON() \
	{ \
		zval ** __old_return_value_pp  		   = EG(return_value_ptr_ptr); \
		zend_op ** __old_opline_ptr 		   = EG(opline_ptr); \
		zend_op_array * __old_op_array 		   = EG(active_op_array);

#define YAF_RESTORE_EG_ENVIRON() \
		EG(return_value_ptr_ptr) = __old_return_value_pp;\
		EG(opline_ptr)			 = __old_opline_ptr; \
		EG(active_op_array)		 = __old_op_array; \
	}

#else

#define YAF_STORE_EG_ENVIRON() \
	{ \
		zval ** __old_return_value_pp  		   = EG(return_value_ptr_ptr); \
		zend_op ** __old_opline_ptr 		   = EG(opline_ptr); \
		zend_op_array * __old_op_array 		   = EG(active_op_array); \
		zend_function_state * __old_func_state = EG(function_state_ptr);

#define YAF_RESTORE_EG_ENVIRON() \
		EG(return_value_ptr_ptr) = __old_return_value_pp;\
		EG(opline_ptr)			 = __old_opline_ptr; \
		EG(active_op_array)		 = __old_op_array; \
		EG(function_state_ptr)	 = __old_func_state; \
	}

#endif

extern zend_class_entry *yaf_loader_ce;

boolean yaf_internal_autoload(char *file_name, uint name_len, char **directory TSRMLS_DC);
boolean yaf_loader_import(char *path, int len, boolean use_path TSRMLS_DC);
boolean yaf_loader_compose(char *path, int len, boolean use_path TSRMLS_DC);
boolean yaf_register_autoloader(yaf_loader_t *loader TSRMLS_DC);
yaf_loader_t * yaf_loader_instance(yaf_loader_t *this_ptr, char *library_path, char *global_path TSRMLS_DC);

extern PHPAPI int php_stream_open_for_zend_ex(const char *filename, zend_file_handle *handle, int mode TSRMLS_DC);

YAF_STARTUP_FUNCTION(loader);

#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
