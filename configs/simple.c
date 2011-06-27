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

zend_class_entry * yaf_config_simple_ce;

/** {{{ yaf_config_t * yaf_config_simple_instance(yaf_config_t *this_ptr, zval *values, zval *readonly TSRMLS_DC)
*/
yaf_config_t * yaf_config_simple_instance(yaf_config_t *this_ptr, zval *values, zval *readonly TSRMLS_DC) {
	yaf_config_t *instance 	= NULL;

	switch (Z_TYPE_P(values)) {
		case IS_ARRAY : { 
			if (this_ptr) {
				instance = this_ptr;
			} else {
				MAKE_STD_ZVAL(instance);
				object_init_ex(instance, yaf_config_simple_ce);

			}
			yaf_update_property(instance, YAF_CONFIG_PROPERT_NAME, values);
			if (readonly) {
				convert_to_boolean(readonly);
				yaf_update_property(instance, YAF_CONFIG_PROPERT_NAME_READONLY, readonly);
			}
			return instance;
		}
		break;
		default:
			yaf_trigger_error(YAF_ERR_TYPE_ERROR, "invalid parameters provided, must be an array");
			return NULL;	
			break;
	}

	return instance;
}
/* }}} */

/** {{{ zval * yaf_config_simple_format(yaf_config_t *instance, zval **ppzval TSRMLS_DC)
 */
zval * yaf_config_simple_format(yaf_config_t *instance, zval **ppzval TSRMLS_DC) {
	zval *readonly = NULL;
	zval *ret	   = NULL;
	readonly = yaf_read_property(instance, YAF_CONFIG_PROPERT_NAME_READONLY);
	ret = yaf_config_simple_instance(NULL, *ppzval, readonly TSRMLS_CC);
	return ret;
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::__construct(mixed $array, string $readonly) 
*/
PHP_METHOD(yaf_config_simple, __construct) {
	zval *values 	= NULL;
	zval *readonly 	= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &values, &readonly) == FAILURE) {
		zval *prop = NULL;
		MAKE_STD_ZVAL(prop);
		array_init(prop);
		yaf_update_property(getThis(), YAF_CONFIG_PROPERT_NAME, prop);

		WRONG_PARAM_COUNT;
	}

	(void)yaf_config_simple_instance(getThis(), values, readonly TSRMLS_CC);
}
/** }}} */

/** {{{ proto public Yaf_Config_Simple::get(string $name = NULL)
*/
PHP_METHOD(yaf_config_simple, get) {
	char * name;
	int len = 0;
	zval * ret, **ppzval;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &name, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (!len) {
		RETURN_ZVAL(getThis(), 1, 0);
	} else {
		zval * properties;
		HashTable * hash;

		properties = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
		hash  = Z_ARRVAL_P(properties);

		if (zend_hash_find(hash, name, len + 1, (void **) &ppzval) == FAILURE) {
			RETURN_FALSE;
		}
		
		if (Z_TYPE_PP(ppzval) == IS_ARRAY) {
			ret = yaf_config_simple_format(getThis(), ppzval TSRMLS_CC);
			RETURN_ZVAL(ret, 0, 0);
		} else {
			RETURN_ZVAL(*ppzval, 1, 0);
		}
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::toArray(void)
*/
PHP_METHOD(yaf_config_simple, toArray) {
	zval * properties = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
	RETURN_ZVAL(properties, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::__get($name = NULL)
*/
PHP_METHOD(yaf_config_simple, __get) {
	ZEND_MN(yaf_config_simple_get)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::set($name, $value)
*/
PHP_METHOD(yaf_config_simple, set) {
	zval *readonly = NULL;
	readonly = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME_READONLY);

	if (!Z_BVAL_P(readonly)) {
		zval *name 	= NULL;
		zval *value = NULL;
		zval *props = NULL;
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &name, &value) == FAILURE) {
			WRONG_PARAM_COUNT;
		}

		if (!name || Z_TYPE_P(name) != IS_STRING || Z_TYPE_P(name) != IS_STRING) {
			yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::set expects a string key name", Z_OBJCE_P(getThis())->name);
			RETURN_FALSE;
		}
		
		Z_ADDREF_P(value);
		props = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
		if (zend_hash_update(Z_ARRVAL_P(props), Z_STRVAL_P(name), Z_STRLEN_P(name) + 1, (void **)&value, sizeof(zval*), NULL) == SUCCESS) {
			RETURN_TRUE;
		} else {
			Z_DELREF_P(value);
		}
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::__isset($name)
*/
PHP_METHOD(yaf_config_simple, __isset) {
	char * name;
	int len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {
		zval * prop = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
		RETURN_BOOL(zend_hash_exists(Z_ARRVAL_P(prop), name, len + 1)); 
	}
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::__set($name, $value)
*/
PHP_METHOD(yaf_config_simple, __set) {
	PHP_MN(yaf_config_simple_set)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::count($name)
*/
PHP_METHOD(yaf_config_simple, count) {
	zval * prop = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
	RETURN_LONG(zend_hash_num_elements(Z_ARRVAL_P(prop)));
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::offsetGet($index) 
*/
PHP_METHOD(yaf_config_simple, offsetGet) {
	PHP_MN(yaf_config_simple_get)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::offsetSet($index, $value) 
*/
PHP_METHOD(yaf_config_simple, offsetSet) {
	PHP_MN(yaf_config_simple_set)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::offsetUnset($index) 
*/
PHP_METHOD(yaf_config_simple, offsetUnset) {
	zval *readonly = NULL;
	readonly = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME_READONLY);

	if (Z_BVAL_P(readonly)) {
		zval *name 	= NULL;
		zval *props = NULL;
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &name) == FAILURE) {
			WRONG_PARAM_COUNT;
		}

		if (!name || Z_TYPE_P(name) != IS_STRING || Z_TYPE_P(name) != IS_STRING) {
			yaf_trigger_error(YAF_ERR_TYPE_ERROR, "%s::offsetSet expects a string key name", Z_OBJCE_P(getThis())->name);
			RETURN_FALSE;
		}
		
		props = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
		if (zend_hash_del(Z_ARRVAL_P(props), Z_STRVAL_P(name), Z_STRLEN_P(name)) == SUCCESS) {
			RETURN_TRUE;
		}
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::offsetExists($index) 
*/
PHP_METHOD(yaf_config_simple, offsetExists) {
	PHP_MN(yaf_config_simple___isset)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::rewind(void)
*/
PHP_METHOD(yaf_config_simple, rewind) {
	zval * prop = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(prop));
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::current(void)
*/
PHP_METHOD(yaf_config_simple, current) {
	zval * prop, ** ppzval;
	zval *ret = NULL;
	prop = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
	if (zend_hash_get_current_data(Z_ARRVAL_P(prop), (void **)&ppzval) == FAILURE) {
		RETURN_FALSE;
	}

	if (Z_TYPE_PP(ppzval) == IS_ARRAY) {
		ret = yaf_config_simple_format(getThis(), ppzval TSRMLS_CC);
		RETURN_ZVAL(ret, 0, 0);
	} else {
		RETURN_ZVAL(*ppzval, 1, 0);
	}
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::key(void)
*/
PHP_METHOD(yaf_config_simple, key) {
	zval * prop;
	char * string;
	long index;

	prop = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);

	zend_hash_get_current_key(Z_ARRVAL_P(prop), &string, &index, 0);
	if (zend_hash_get_current_key_type(Z_ARRVAL_P(prop)) == HASH_KEY_IS_LONG) {
		RETURN_LONG(index);
	} else {
		RETURN_STRING(string, 1);
	}
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::next(void)
*/
PHP_METHOD(yaf_config_simple, next) {
	zval *prop;
	prop = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
	zend_hash_move_forward(Z_ARRVAL_P(prop));
	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::valid(void)
*/
PHP_METHOD(yaf_config_simple, valid) {
	zval * prop = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
	RETURN_LONG(zend_hash_has_more_elements(Z_ARRVAL_P(prop)) == SUCCESS);
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::readonly(void)
*/
PHP_METHOD(yaf_config_simple, readonly) {
	zval * prop = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME_READONLY);
	RETURN_ZVAL(prop, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Config_Simple::__destruct
*/
PHP_METHOD(yaf_config_simple, __destruct) {
}
/* }}} */

/** {{{ proto private Yaf_Config_Simple::__clone
*/
PHP_METHOD(yaf_config_simple, __clone) {
}
/* }}} */

/** {{{ yaf_config_simple_methods
*/
zend_function_entry yaf_config_simple_methods[] = {
	PHP_ME(yaf_config_simple, __construct,	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	/* PHP_ME(yaf_config_simple, __destruct,	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DTOR) */
	PHP_ME(yaf_config_simple, __isset,		yaf_getter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, __set,			yaf_setter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, __get,			yaf_getter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, get,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, set,			yaf_setter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, count,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, offsetGet,		yaf_getter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, offsetExists, 	yaf_getter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, offsetUnset,	yaf_getter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, offsetSet,		yaf_setter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, rewind,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, current,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, next,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, valid,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, key,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, readonly,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_config_simple, toArray,		NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ YAF_STARTUP_FUNCTION
*/
YAF_STARTUP_FUNCTION(config_simple) {
	zend_class_entry ce;

	YAF_INIT_CLASS_ENTRY(ce, "Yaf_Config_Simple", "Yaf\\Config\\Simple", yaf_config_simple_methods);
	yaf_config_simple_ce = zend_register_internal_class_ex(&ce, yaf_config_ce, NULL TSRMLS_CC);

#ifdef HAVE_SPL
	zend_class_implements(yaf_config_simple_ce TSRMLS_CC, 3, zend_ce_iterator, zend_ce_arrayaccess, spl_ce_Countable);
#else
	zend_class_implements(yaf_config_simple_ce TSRMLS_CC, 2, zend_ce_iterator, zend_ce_arrayaccess);
#endif
	zend_declare_property_bool(yaf_config_simple_ce, YAF_STRL(YAF_CONFIG_PROPERT_NAME_READONLY), 0, ZEND_ACC_PROTECTED TSRMLS_CC);

	yaf_config_simple_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

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
