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
#include "main/php_output.h"

#define VIEW_BUFFER_BLOCK_SIZE	4096
#define VIEW_BUFFER_SIZE_MASK 	4095

struct _yaf_view_simple_buffer {
	char *buffer;
	unsigned long size;
	unsigned long len;
	struct _yaf_view_simple_buffer *prev;
};

typedef struct _yaf_view_simple_buffer yaf_view_simple_buffer;

zend_class_entry *yaf_view_simple_ce;

typedef int(*yaf_body_write_func)(const char *str, uint str_length TSRMLS_DC);

/** {{{ MACROS for Yaf_View_Simple
 */
#define YAF_REDIRECT_OUTPUT_BUFFER(seg) \
	do { \
		if (!YAF_G(owrite_handler)) { \
			YAF_G(owrite_handler) = OG(php_body_write); \
		} \
		OG(php_body_write) = yaf_view_simple_render_write; \
		old_scope = EG(scope); \
		EG(scope) = yaf_view_simple_ce; \
		seg = (yaf_view_simple_buffer *)emalloc(sizeof(yaf_view_simple_buffer)); \
		memset(seg, 0, sizeof(yaf_view_simple_buffer)); \
		seg->prev  	 = YAF_G(buffer);\
		YAF_G(buffer) = seg; \
		YAF_G(buf_nesting)++;\
	} while (0)

#define YAF_RESTORE_OUTPUT_BUFFER(seg) \
	do { \
		OG(php_body_write) 	= (yaf_body_write_func)YAF_G(owrite_handler); \
		EG(scope) 			= old_scope; \
		YAF_G(buffer)  		= seg->prev; \
		if (!(--YAF_G(buf_nesting))) { \
			if (YAF_G(buffer)) { \
				yaf_error("Yaf output buffer was collapsed"); \
			} else { \
				YAF_G(owrite_handler) = NULL; \
			} \
		} \
		if (seg->size) { \
			efree(seg->buffer); \
		} \
		efree(seg); \
	} while (0)
/* }}} */

/** {{{ ARG_INFO */
YAF_BEGIN_ARG_INFO_EX(yaf_view_simple_assign_by_ref_arg, 0, 0, 2)
YAF_ARG_INFO(0, property_name)
YAF_ARG_INFO(1, value)
ZEND_END_ARG_INFO();
/* }}} */

/** {{{ static int yaf_view_simple_render_write(const char *str, uint str_length TSRMLS_DC)
*/
static int yaf_view_simple_render_write(const char *str, uint str_length TSRMLS_DC) {
	char *target  = NULL;
	yaf_view_simple_buffer *buffer = YAF_G(buffer);

	if (!buffer->size) {
		buffer->size   = (str_length | VIEW_BUFFER_SIZE_MASK) + 1;
		buffer->len	   = str_length;
		buffer->buffer = emalloc(buffer->size);
		target = buffer->buffer;
	} else {
		unsigned long len = buffer->len + str_length;

		if (buffer->size < len + 1) {
			buffer->size   = (len | VIEW_BUFFER_SIZE_MASK) + 1;
			buffer->buffer = erealloc(buffer->buffer, buffer->size);
			if (!buffer->buffer) {
				yaf_error("Yaf output buffer collapsed");
			}
		}
		
		target 		= buffer->buffer + buffer->len;
		buffer->len = len;
	}

	memcpy(target, str, str_length);
	target[str_length] = '\0';

	return str_length;
}
/* }}} */

static int yaf_view_simple_valid_var_name(char *var_name, int len) /* {{{ */
{
	int i, ch;

	if (!var_name)
		return 0;

	/* These are allowed as first char: [a-zA-Z_\x7f-\xff] */
	ch = (int)((unsigned char *)var_name)[0];
	if (var_name[0] != '_' &&
			(ch < 65  /* A    */ || /* Z    */ ch > 90)  &&
			(ch < 97  /* a    */ || /* z    */ ch > 122) &&
			(ch < 127 /* 0x7f */ || /* 0xff */ ch > 255)
	   ) {
		return 0;
	}

	/* And these as the rest: [a-zA-Z0-9_\x7f-\xff] */
	if (len > 1) {
		for (i = 1; i < len; i++) {
			ch = (int)((unsigned char *)var_name)[i];
			if (var_name[i] != '_' &&
					(ch < 48  /* 0    */ || /* 9    */ ch > 57)  &&
					(ch < 65  /* A    */ || /* Z    */ ch > 90)  &&
					(ch < 97  /* a    */ || /* z    */ ch > 122) &&
					(ch < 127 /* 0x7f */ || /* 0xff */ ch > 255)
			   ) {	
				return 0;
			}
		}
	}
	return 1;
}
/* }}} */

/** {{{ static boolean yaf_view_simple_extract(zval *tpl_vars, zval *vars TSRMLS_DC) 
*/
static boolean yaf_view_simple_extract(zval *tpl_vars, zval *vars TSRMLS_DC) {
	zval **entry	= NULL;
	char *var_name  = NULL;
	long num_key	= 0;
	uint var_name_len = 0;
	HashPosition  pos = {0};

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
	if (!EG(active_symbol_table)) {
		/*zend_rebuild_symbol_table(TSRMLS_C);*/
		return TRUE;
	}
#endif

	if (tpl_vars && Z_TYPE_P(tpl_vars) == IS_ARRAY) {
		for(zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(tpl_vars), &pos);
				zend_hash_get_current_data_ex(Z_ARRVAL_P(tpl_vars), (void **)&entry, &pos) == SUCCESS; 
				zend_hash_move_forward_ex(Z_ARRVAL_P(tpl_vars), &pos)) {
			if (zend_hash_get_current_key_ex(Z_ARRVAL_P(tpl_vars), &var_name, &var_name_len, &num_key, 0, &pos) != HASH_KEY_IS_STRING) {
				continue;
			}

			/* GLOBALS protection */
			if (var_name_len == sizeof("GLOBALS") && !strcmp(var_name, "GLOBALS")) {
				continue;
			}

			if (var_name_len == sizeof("this")  && !strcmp(var_name, "this") && EG(scope) && EG(scope)->name_length != 0) {
				continue;
			}


			if (yaf_view_simple_valid_var_name(var_name, var_name_len - 1)) {
				ZEND_SET_SYMBOL_WITH_LENGTH(EG(active_symbol_table), var_name, var_name_len, 
						*entry, Z_REFCOUNT_P(*entry) + 1, FALSE /**PZVAL_IS_REF(*entry)*/);
			}
		}
	}

	if (vars && Z_TYPE_P(vars) == IS_ARRAY) {
		for(zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(vars), &pos);
				zend_hash_get_current_data_ex(Z_ARRVAL_P(vars), (void **)&entry, &pos) == SUCCESS; 
				zend_hash_move_forward_ex(Z_ARRVAL_P(vars), &pos)) {
			if (zend_hash_get_current_key_ex(Z_ARRVAL_P(vars), &var_name, &var_name_len, &num_key, 0, &pos) != HASH_KEY_IS_STRING) {
				continue;
			}

			/* GLOBALS protection */
			if (var_name_len == sizeof("GLOBALS") && !strcmp(var_name, "GLOBALS")) {
				continue;
			}

			if (var_name_len == sizeof("this")  && !strcmp(var_name, "this") && EG(scope) && EG(scope)->name_length != 0) {
				continue;
			}

			if (yaf_view_simple_valid_var_name(var_name, var_name_len - 1)) {
				ZEND_SET_SYMBOL_WITH_LENGTH(EG(active_symbol_table), var_name, var_name_len, 
						*entry, Z_REFCOUNT_P(*entry) + 1, FALSE /**PZVAL_IS_REF(*entry)*/);
			}
		}
	}

	return TRUE;
}
/* }}} */

/** {{{ yaf_view_t * yaf_view_simple_instance(yaf_view_t *view, zval *tpl_dir, zval *options TSRMLS_DC)
*/
yaf_view_t * yaf_view_simple_instance(yaf_view_t *view, zval *tpl_dir, zval *options TSRMLS_DC) {
	zval * instance = NULL;
	zval * tpl_vars = NULL;

	instance = view;

	if (!instance) {
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, yaf_view_simple_ce);
	}

	MAKE_STD_ZVAL(tpl_vars);
	array_init(tpl_vars);

	yaf_update_property(instance, YAF_VIEW_PROPERTY_NAME_TPLVARS, tpl_vars);

	if (tpl_dir && Z_TYPE_P(tpl_dir) == IS_STRING 
			&& IS_ABSOLUTE_PATH(Z_STRVAL_P(tpl_dir), Z_STRLEN_P(tpl_dir))) {
		yaf_update_property(instance, YAF_VIEW_PROPERTY_NAME_TPLDIR, tpl_dir);
	}

	return instance;
}
/* }}} */

/** {{{ boolean yaf_view_simple_render(yaf_view_t *view, zval *tpl, zval * vars, zval *ret TSRMLS_DC)
*/
boolean yaf_view_simple_render(yaf_view_t *view, zval *tpl, zval * vars, zval *ret TSRMLS_DC) {
	zval *tpl_vars = NULL;
	char *script   = NULL;
	int  len 	   = 0;

	zend_class_entry *old_scope 	= NULL;
	HashTable *calling_symbol_table = NULL;
	yaf_view_simple_buffer *buffer	= NULL;

	ZVAL_NULL(ret);

	tpl_vars = yaf_read_property(view, YAF_VIEW_PROPERTY_NAME_TPLVARS);

	if (EG(active_symbol_table)) {
		calling_symbol_table = EG(active_symbol_table);
	}

	ALLOC_HASHTABLE(EG(active_symbol_table));
	zend_hash_init(EG(active_symbol_table), 0, NULL, ZVAL_PTR_DTOR, 0);

	(void)yaf_view_simple_extract(tpl_vars, vars TSRMLS_CC);

	YAF_REDIRECT_OUTPUT_BUFFER(buffer);

	if (IS_ABSOLUTE_PATH(Z_STRVAL_P(tpl), Z_STRLEN_P(tpl))) {
		script 	= Z_STRVAL_P(tpl);
		len 	= Z_STRLEN_P(tpl);
		if (yaf_loader_compose(script, len + 1, FALSE TSRMLS_CC) == FALSE) {
			YAF_RESTORE_OUTPUT_BUFFER(buffer);
			if (calling_symbol_table) {
				zend_hash_destroy(EG(active_symbol_table));
				FREE_HASHTABLE(EG(active_symbol_table));
				EG(active_symbol_table) = calling_symbol_table;
			}

			yaf_trigger_error(YAF_ERR_NOTFOUND_VIEW, "could not find view script %s", script);
			return FALSE;
		}
	} else {
		zval *tpl_dir = yaf_read_property(view, YAF_VIEW_PROPERTY_NAME_TPLDIR);

		if (ZVAL_IS_NULL(tpl_dir)) {
			YAF_RESTORE_OUTPUT_BUFFER(buffer);

			if (calling_symbol_table) {
				zend_hash_destroy(EG(active_symbol_table));
				FREE_HASHTABLE(EG(active_symbol_table));
				EG(active_symbol_table) = calling_symbol_table;
			}

			yaf_trigger_error(YAF_ERR_NOTFOUND_VIEW,
				   	"could not determine the view script path, you should call %s::setScriptPath to specific it", 
					yaf_view_simple_ce->name);
			return FALSE;
		}

		len = spprintf(&script, 0, "%s%c%s", Z_STRVAL_P(tpl_dir), DEFAULT_SLASH, Z_STRVAL_P(tpl));

		if (yaf_loader_compose(script, len + 1, FALSE TSRMLS_CC) == FALSE) {
			YAF_RESTORE_OUTPUT_BUFFER(buffer);

			if (calling_symbol_table) {
				zend_hash_destroy(EG(active_symbol_table));
				FREE_HASHTABLE(EG(active_symbol_table));
				EG(active_symbol_table) = calling_symbol_table;
			}

			yaf_trigger_error(YAF_ERR_NOTFOUND_VIEW, "could not find view script %s" , script);
			efree(script);
			return FALSE;
		}
		efree(script);
	}

	if (calling_symbol_table) {
		zend_hash_destroy(EG(active_symbol_table));
		FREE_HASHTABLE(EG(active_symbol_table));
		EG(active_symbol_table) = calling_symbol_table;
	}

	if (buffer->len) {
		ZVAL_STRINGL(ret, buffer->buffer, buffer->len, TRUE);
	}

	YAF_RESTORE_OUTPUT_BUFFER(buffer);

	return TRUE;
}
/* }}} */

/** {{{ boolean yaf_view_simple_display(yaf_view_t *view, zval *tpl, zval * vars, zval *ret TSRMLS_DC)
*/
boolean yaf_view_simple_display(yaf_view_t *view, zval *tpl, zval *vars, zval *ret TSRMLS_DC) {
	zval *tpl_vars = NULL;
	char *script   = NULL;
	int  len 	   = 0;

	zend_class_entry *old_scope 	= NULL;
	HashTable *calling_symbol_table = NULL;

	ZVAL_NULL(ret);

	tpl_vars = yaf_read_property(view, YAF_VIEW_PROPERTY_NAME_TPLVARS);

	if (EG(active_symbol_table)) {
		calling_symbol_table = EG(active_symbol_table);
	}

	ALLOC_HASHTABLE(EG(active_symbol_table));
	zend_hash_init(EG(active_symbol_table), 0, NULL, ZVAL_PTR_DTOR, 0);

	(void)yaf_view_simple_extract(tpl_vars, vars TSRMLS_CC);

	old_scope = EG(scope);
	EG(scope) = yaf_view_simple_ce;

	if (IS_ABSOLUTE_PATH(Z_STRVAL_P(tpl), Z_STRLEN_P(tpl))) {
		script 	= Z_STRVAL_P(tpl);
		len 	= Z_STRLEN_P(tpl);
		if (yaf_loader_compose(script, len + 1, FALSE TSRMLS_CC) == FALSE) {
			yaf_trigger_error(YAF_ERR_NOTFOUND_VIEW, "could not find view script %s" , script);
			EG(scope) = old_scope;
			if (calling_symbol_table) {
				zend_hash_destroy(EG(active_symbol_table));
				FREE_HASHTABLE(EG(active_symbol_table));
				EG(active_symbol_table) = calling_symbol_table;
			}
			return FALSE;
		}
	} else {
		zval *tpl_dir = yaf_read_property(view, YAF_VIEW_PROPERTY_NAME_TPLDIR);

		if (ZVAL_IS_NULL(tpl_dir)) {
			yaf_trigger_error(YAF_ERR_NOTFOUND_VIEW, "could not determine the view script path, you should call %s::setScriptPath to specific it", yaf_view_simple_ce->name);
			EG(scope) = old_scope;
			if (calling_symbol_table) {
				zend_hash_destroy(EG(active_symbol_table));
				FREE_HASHTABLE(EG(active_symbol_table));
				EG(active_symbol_table) = calling_symbol_table;
			}
			return FALSE;
		}

		len = spprintf(&script, 0, "%s%c%s", Z_STRVAL_P(tpl_dir), DEFAULT_SLASH, Z_STRVAL_P(tpl));
		if (yaf_loader_compose(script, len + 1, FALSE TSRMLS_CC) == FALSE) {
			yaf_trigger_error(YAF_ERR_NOTFOUND_VIEW, "could not find view script %s" , script);
			efree(script);
			EG(scope) = old_scope;
			if (calling_symbol_table) {
				zend_hash_destroy(EG(active_symbol_table));
				FREE_HASHTABLE(EG(active_symbol_table));
				EG(active_symbol_table) = calling_symbol_table;
			}
			return FALSE;
		}
		efree(script);
	}

	EG(scope) = old_scope;
	if (calling_symbol_table) {
		zend_hash_destroy(EG(active_symbol_table));
		FREE_HASHTABLE(EG(active_symbol_table));
		EG(active_symbol_table) = calling_symbol_table;
	}

	return TRUE;
}
/* }}} */

/** {{{ proto public Yaf_View_Simple::__construct(string $tpl_dir, array $options = NULL) 
*/
PHP_METHOD(yaf_view_simple, __construct) {
	zval *tpl_dir = NULL;
	zval *options = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &tpl_dir, &options) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	yaf_view_simple_instance(getThis(), tpl_dir, options TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yaf_View_Simple::__isset($name)
*/
PHP_METHOD(yaf_view_simple, __isset) {
	char 	*name 	= NULL;
	int 	len		= 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {
		zval *tpl_vars = yaf_read_property(getThis(), YAF_VIEW_PROPERTY_NAME_TPLVARS);
		RETURN_BOOL(zend_hash_exists(Z_ARRVAL_P(tpl_vars), name, len + 1)); 
	}
}
/* }}} */

/** {{{ proto public Yaf_View_Simple::setScriptPath(string $tpl_dir)
*/
PHP_METHOD(yaf_view_simple, setScriptPath) {
	zval *tpl_dir = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &tpl_dir) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (tpl_dir && Z_TYPE_P(tpl_dir) == IS_STRING
			&& IS_ABSOLUTE_PATH(Z_STRVAL_P(tpl_dir), Z_STRLEN_P(tpl_dir))) {

		yaf_update_property(getThis(), YAF_VIEW_PROPERTY_NAME_TPLDIR, tpl_dir);
		RETURN_ZVAL(getThis(), 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_View_Simple::getScriptPath(void)
*/
PHP_METHOD(yaf_view_simple, getScriptPath) {
	zval *tpl_dir = yaf_read_property(getThis(), YAF_VIEW_PROPERTY_NAME_TPLDIR);
	RETURN_ZVAL(tpl_dir, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_View_Simple::compose(string $script, zval *args)
*/
PHP_METHOD(yaf_view_simple, compose) {
	char *script = NULL;
	int  len	 = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &script, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (!len) {
		RETURN_FALSE;
	}

}
/* }}} */

/** {{{ proto public Yaf_View_Simple::assign(mixed $value, mixed $value = null)
*/
PHP_METHOD(yaf_view_simple, assign) {
	uint argc		= ZEND_NUM_ARGS();
	zval *tpl_vars 	= yaf_read_property(getThis(), YAF_VIEW_PROPERTY_NAME_TPLVARS);

	if (argc == 1) {
		zval *value = NULL;
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &value) == FAILURE) {
			WRONG_PARAM_COUNT;
		}     
		if (value && Z_TYPE_P(value) == IS_ARRAY) {
			zend_hash_copy(Z_ARRVAL_P(tpl_vars), Z_ARRVAL_P(value), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
			RETURN_TRUE;
		} 
		RETURN_FALSE;
	} else if (argc == 2) {
		zval 	*value	= NULL;
		char 	*name	= NULL;
		int 	len		= 0;
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &len, &value) == FAILURE) {
			WRONG_PARAM_COUNT;
		}     

		Z_ADDREF_P(value);
		if (zend_hash_update(Z_ARRVAL_P(tpl_vars), name, len + 1, &value, sizeof(zval *), NULL) == SUCCESS) {
			RETURN_TRUE;
		}
	} else {
		WRONG_PARAM_COUNT;
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_View_Simple::assignRef(mixed $value, mixed $value)
*/
PHP_METHOD(yaf_view_simple, assignRef) {
	char * name; int len;
	zval * value, * tpl_vars;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &len, &value) == FAILURE) {
		WRONG_PARAM_COUNT;
	} 

	tpl_vars = yaf_read_property(getThis(), YAF_VIEW_PROPERTY_NAME_TPLVARS);

	Z_ADDREF_P(value);
	if (zend_hash_update(Z_ARRVAL_P(tpl_vars), name, len + 1, &value, sizeof(zval *), NULL) == SUCCESS) {
		RETURN_TRUE;
	}		
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_View_Simple::get($name)
*/
PHP_METHOD(yaf_view_simple, get) {
	char	*name		= NULL;
	int		len			= 0;
	zval 	*tpl_vars	= NULL;
	zval 	**ret		= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}    

	tpl_vars = yaf_read_property(getThis(), YAF_VIEW_PROPERTY_NAME_TPLVARS);

	if (tpl_vars && Z_TYPE_P(tpl_vars) == IS_ARRAY) {
		if (zend_hash_find(Z_ARRVAL_P(tpl_vars), name, len + 1, (void **) &ret) == SUCCESS) {
			RETURN_ZVAL(*ret, 1, 0);
		}
	} 

	RETURN_NULL();
}
/* }}} */

/** {{{ proto public Yaf_View_Simple::__get($name)
*/
PHP_METHOD(yaf_view_simple, __get) {
	ZEND_MN(yaf_view_simple_get)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_View_Simple::__set(mixed $value, mixed $value)
*/
PHP_METHOD(yaf_view_simple, __set) {
	ZEND_MN(yaf_view_simple_assign)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_View_Simple::render(string $tpl, array $vars = NULL)
*/
PHP_METHOD(yaf_view_simple, render) {
	zval *tpl  		= NULL;
	zval *vars 		= NULL;
	zval *tpl_vars	= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z"
				,&tpl, &vars) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	tpl_vars = yaf_read_property(getThis(), YAF_VIEW_PROPERTY_NAME_TPLVARS);

	if (!yaf_view_simple_render(getThis(), tpl, vars, return_value TSRMLS_CC)) {
		RETURN_FALSE;
	}
}
/* }}} */

/** {{{ proto public Yaf_View_Simple::display(string $tpl, array $vars = NULL)
*/
PHP_METHOD(yaf_view_simple, display) {
	zval *tpl  		= NULL;
	zval *vars 		= NULL;
	zval *tpl_vars	= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z"
				,&tpl, &vars) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	tpl_vars = yaf_read_property(getThis(), YAF_VIEW_PROPERTY_NAME_TPLVARS);

	if (!yaf_view_simple_display(getThis(), tpl, vars, return_value TSRMLS_CC)) {
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/** {{{ yaf_view_simple_methods 
*/
zend_function_entry yaf_view_simple_methods[] = {
	PHP_ME(yaf_view_simple, __construct, NULL, 	ZEND_ACC_CTOR | ZEND_ACC_PUBLIC)
	PHP_ME(yaf_view_simple, __set, 		yaf_setter_arg,	ZEND_ACC_PUBLIC)
	PHP_ME(yaf_view_simple, __get,		yaf_getter_arg, 	ZEND_ACC_PUBLIC)
	PHP_ME(yaf_view_simple, __isset,		yaf_getter_arg,	ZEND_ACC_PUBLIC)
	PHP_ME(yaf_view_simple, get,			yaf_getter_arg, 	ZEND_ACC_PUBLIC)
	PHP_ME(yaf_view_simple, assign,		yaf_2op1_arg, 	ZEND_ACC_PUBLIC)
	PHP_ME(yaf_view_simple, render, 		yaf_2op1_arg, 	ZEND_ACC_PUBLIC)
	PHP_ME(yaf_view_simple, display, 	yaf_2op1_arg, 	ZEND_ACC_PUBLIC)
	PHP_ME(yaf_view_simple, assignRef, 	yaf_view_simple_assign_by_ref_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_view_simple, setScriptPath,yaf_getter_arg,	ZEND_ACC_PUBLIC)
	PHP_ME(yaf_view_simple, getScriptPath,NULL,			ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ YAF_STARTUP_FUNCTION 
*/
YAF_STARTUP_FUNCTION(view_simple) {
	zend_class_entry ce;

	YAF_INIT_CLASS_ENTRY(ce, "Yaf_View_Simple", "Yaf\\View\\Simple", yaf_view_simple_methods);
	yaf_view_simple_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);

	zend_declare_property_null(yaf_view_simple_ce, YAF_STRL(YAF_VIEW_PROPERTY_NAME_TPLVARS), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yaf_view_simple_ce, YAF_STRL(YAF_VIEW_PROPERTY_NAME_TPLDIR),  ZEND_ACC_PROTECTED TSRMLS_CC);

	yaf_view_simple_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;
	zend_class_implements(yaf_view_simple_ce TSRMLS_CC, 1, yaf_view_interface_ce);

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

