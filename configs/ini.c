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

zend_class_entry * yaf_config_ini_ce;

yaf_config_t * yaf_config_ini_instance(yaf_config_t *this_ptr, zval *filename, zval *section TSRMLS_DC);

/** {{{ static unsigned int numerics[256]
*/
static unsigned int numerics[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3,
	4, 5, 6, 7, 8, 9, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
};
/* }}} */

/** {{{ static int yaf_config_is_numeric(char *str TSRMLS_DC)
*/
static inline int yaf_config_ini_is_numeric(char *str TSRMLS_DC) {
	int i,j;
	int ret = 0;

	for(i=0, j=strlen(str); i<j; i++) {
		if ((numerics[(unsigned char)str[i]])) {
			ret = ret*10 + (numerics[(unsigned char)str[i]] - 1);
		} else {
			return -1;
		}
	}
	return ret;
}
/* }}} */

/** {{{ zval * yaf_config_ini_format(yaf_config_t *instance, zval **ppzval TSRMLS_DC)
*/
zval * yaf_config_ini_format(yaf_config_t *instance, zval **ppzval TSRMLS_DC) {
	zval *readonly = NULL;
	zval *ret	   = NULL;
	readonly = yaf_read_property(instance, YAF_CONFIG_PROPERT_NAME_READONLY);
	ret = yaf_config_ini_instance(NULL, *ppzval, NULL TSRMLS_CC);
	return ret;
}
/* }}} */

/** {{{ static zval * yaf_config_ini_parse_entry(HashTable * ht, char * name, int start, int name_len TSRMLS_DC) 
 * parse string to array
 */
static zval * yaf_config_ini_parse_entry(HashTable *ht, char *name, int start, int name_len TSRMLS_DC) {
	char 		*key		= NULL;
	uint 		keylen		= 0;
	ulong 		idx			= 0;
	zval 		**ppzval 	= NULL;
	zval 		*ret 		= NULL;
	int 		found 		= 0;
	HashPointer position	= {0};

	for(zend_hash_internal_pointer_reset(ht);
			zend_hash_has_more_elements(ht) == SUCCESS;
			zend_hash_move_forward(ht)) {

		zend_hash_get_current_key_ex(ht, &key, &keylen, &idx, 0, NULL);
		if (keylen != name_len || strncasecmp(key, name, name_len)) {
			continue;
		}

		found = 1;
		if (zend_hash_get_current_data(ht, (void**)&ppzval) == FAILURE) {
			continue;
		}

		ret = *ppzval;
	} 

	if (!found) {
		char * record_name = emalloc(name_len + 1);

		memcpy(record_name, name, name_len - 1);
		record_name[name_len - 1] 	= '.';
		record_name[name_len] 		= '\0';
		MAKE_STD_ZVAL(ret);
		array_init(ret);

		for(zend_hash_internal_pointer_reset(ht);
				zend_hash_has_more_elements(ht) == SUCCESS;
				zend_hash_move_forward(ht)) {

			if (zend_hash_get_current_key_ex(ht, &key, &keylen, &idx, 0, NULL) != HASH_KEY_IS_STRING) {
				continue;
			}
			if (strncasecmp(key, record_name, name_len)) {
				continue;
			} else {
				zval *son 		 = NULL;
				int real_key_len = 0;
				int long_key	 = 0;
				char *real_key  = strstr(key + name_len + 1, ".");
				zend_hash_get_pointer(ht, &position);
				if (real_key == NULL) {
					son 	 	 = yaf_config_ini_parse_entry(ht, key, name_len - 1, keylen TSRMLS_CC);
					real_key	 = estrdup(key + name_len);
					real_key_len = strlen(real_key) + 1;
				} else {
					son 		 = yaf_config_ini_parse_entry(ht, key, name_len - 1 , real_key - key + 1 TSRMLS_CC);
					real_key_len = real_key - (key + name_len) + 1;
					real_key 	 = estrdup(key + name_len);
					*(real_key + real_key_len - 1) = '\0';
				}
				zend_hash_set_pointer(ht, &position);
				if((long_key = yaf_config_ini_is_numeric(real_key TSRMLS_CC)) >= 0) {
					zend_hash_index_update(Z_ARRVAL_P(ret), long_key, (void **)&son, sizeof(zval *), NULL);
				} else {
					zend_hash_update(Z_ARRVAL_P(ret), real_key, real_key_len , (void **)&son, sizeof(zval *), NULL);
				}

				efree(real_key);
			}
		}
	}

	zval_copy_ctor(ret);
	return ret;
}
/* }}} */

/** {{{ static HashTable * yaf_config_ini_parse_record(HashTable * ht TSRMLS_DC)
 * parse string to array
 */
static HashTable * yaf_config_ini_parse_record(HashTable *ht TSRMLS_DC) {
	char 		*key		= NULL;
	char 		*real_key	= NULL;
	uint 		keylen		= 0;
	ulong		idx			= 0;
	zval 		*tmp		= NULL;
	HashTable 	*ret		= NULL;

	ALLOC_HASHTABLE(ret);
	zend_hash_init(ret, 128, NULL, NULL, 0);

	for(zend_hash_internal_pointer_reset(ht);
			zend_hash_has_more_elements(ht) == SUCCESS;
			zend_hash_move_forward(ht)) {

		zend_hash_get_current_key_ex(ht, &key, &keylen, &idx, 0, NULL);

		if ((real_key = strstr(key, "."))) {
			keylen	 = real_key - key;
			real_key = estrndup(key, keylen);

			if (zend_hash_exists(ret, real_key, keylen + 1)) {
				efree(real_key);
				continue;
			}

			zend_hash_add_empty_element(ret, real_key, keylen + 1);
			efree(real_key);
		} else {
			if (zend_hash_exists(ret, key, keylen)) {
				continue;
			}
			zend_hash_add_empty_element(ret, key, keylen);
		}
	}


	for(zend_hash_internal_pointer_reset(ret);
			zend_hash_has_more_elements(ret) == SUCCESS;
			zend_hash_move_forward(ret)) {

		zend_hash_get_current_key_ex(ret, &key, &keylen, &idx, 0, NULL);
		tmp = yaf_config_ini_parse_entry(ht, key, 0, keylen TSRMLS_CC);
		zend_hash_update(ret, key, keylen , (void **)&tmp, sizeof(zval *), NULL);
	}

	return ret;
}
/* }}} */

/** {{{static zval *yaf_config_ini_parse_section(HashTable *ht, char *name, int len TSRMLS_DC) 
*/
static zval *yaf_config_ini_parse_section(HashTable *ht, char *name, int len TSRMLS_DC)  {
	char 		*key 		= NULL;
	char 		*buf		= NULL;
	ulong 		idx			= 0;
	uint 		keylen 		= 0;
	zval	 	*section	= NULL;
	zval 	 	*parent		= NULL;
	zval 		**ppzval 	= NULL;
	HashPointer position 	= {0};

	for(zend_hash_internal_pointer_reset(ht);
			zend_hash_has_more_elements(ht) == SUCCESS;
			zend_hash_move_forward(ht)) {

		if (zend_hash_get_current_key_ex(ht, &key, &keylen, &idx, 0, NULL) == HASH_KEY_NON_EXISTANT) {
			continue;
		}

		if (strncasecmp(key, name, len)) {
			continue;
		}

		if (zend_hash_get_current_data(ht, (void**)&ppzval) == FAILURE) {
			continue;
		}

		if (Z_TYPE_PP(ppzval) != IS_ARRAY) {
			continue;
		}

		if (!section) {
			MAKE_STD_ZVAL(section);
			array_init(section);
		}

		zend_hash_merge(Z_ARRVAL_P(section), Z_ARRVAL_PP(ppzval), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *), 0);

		if ((buf = strstr(key, ":"))) {
			char *parent_section = NULL;
			char *tmp			 = NULL;
			int	 parent_len		 = 0;
			while((++buf)[0] == ' ');

			parent_section = buf;

			if ((tmp = strstr(parent_section, ":"))) {
				parent_len = tmp - parent_section;
			} else {
				parent_len = strlen(parent_section);
			}

			/* in case of nesting like "section : section" */
			if (strncasecmp(buf, name, len)) {
				zend_hash_get_pointer(ht, &position);
				parent = yaf_config_ini_parse_section(ht, parent_section, parent_len TSRMLS_CC);
				zend_hash_set_pointer(ht, &position);
				if (parent) {
					zend_hash_merge(Z_ARRVAL_P(parent), Z_ARRVAL_P(section), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *), 1);
					section = parent;
					parent  = NULL;
				}
			}
		} 

		break;
	}


	return section;
}
/* }}} */

/** {{{ yaf_config_t * yaf_config_ini_instance(yaf_config_t *this_ptr, zval *filename, zval *section_name TSRMLS_DC)
*/
yaf_config_t * yaf_config_ini_instance(yaf_config_t *this_ptr, zval *filename, zval *section_name TSRMLS_DC) {
	yaf_config_t *instance 	= NULL;
	zval		*configs	= NULL;

	if (filename && Z_TYPE_P(filename) == IS_ARRAY) {
		if (this_ptr) {
			instance = this_ptr;
		} else {
			MAKE_STD_ZVAL(instance);
			object_init_ex(instance, yaf_config_ini_ce);
		}

		yaf_update_property(instance, YAF_CONFIG_PROPERT_NAME, filename);

		return instance;
	} else if (filename && Z_TYPE_P(filename) == IS_STRING) {
		zval *ini_entries		= NULL;
		zval *process_section 	= NULL;
		zval function			= {{0}, 0}; /* missing braces around initializer */
		zval *params[2] 		= {0};

		MAKE_STD_ZVAL(process_section);
		ZVAL_1(process_section);

		params[0] = filename;
		params[1] = process_section;

		ZVAL_STRING(&function, "parse_ini_file", 0);
		MAKE_STD_ZVAL(ini_entries);
		ZVAL_0(ini_entries);
		/**
		 * cause config need section parse
		 * so it's difficult to call zend_parse_ini_file directly
		 */
		if (call_user_function(EG(function_table), NULL, &function, ini_entries, 2, params TSRMLS_CC) == FAILURE) {
			yaf_trigger_error(YAF_ERR_ERROR, "call to parse_ini_file failed");
			return NULL;
		}

		if (Z_TYPE_P(ini_entries) != IS_ARRAY) {
			efree(ini_entries);
			yaf_trigger_error(YAF_ERR_ERROR, "couldn't find config file %s or it is not in INI file format", Z_STRVAL_P(filename));
			return NULL;
		}

		if (section_name && Z_STRLEN_P(section_name)) {
			configs = yaf_config_ini_parse_section(Z_ARRVAL_P(ini_entries), Z_STRVAL_P(section_name), Z_STRLEN_P(section_name) TSRMLS_CC);
			if (!configs) {
				zval_dtor(ini_entries);
				efree(ini_entries);
				yaf_trigger_error(YAF_ERR_ERROR, "there is no section %s in %s", Z_STRVAL_P(section_name), Z_STRVAL_P(filename));
				return NULL;
			}

			Z_ARRVAL_P(configs) = yaf_config_ini_parse_record(Z_ARRVAL_P(configs) TSRMLS_CC);
			zval_dtor(ini_entries);
			efree(ini_entries);
		} else {
			int			len	 = 0;
			int		section  = 0;
			long		idx	 = 0;
			char		*key = NULL;
			char 		*tmp = NULL;
			char 		*ex	 = NULL;
			zval		*sec = NULL;
			zval 	  **ppzv = NULL;
			HashTable	*ht  = NULL;
			HashPointer pot	 = {0};

			ex = ":";
			MAKE_STD_ZVAL(configs);
			array_init(configs);

			ht = Z_ARRVAL_P(ini_entries);
			for(zend_hash_internal_pointer_reset(ht);
					zend_hash_has_more_elements(ht) == SUCCESS;
					zend_hash_move_forward(ht)) {

				if (zend_hash_get_current_key_ex(ht, &key, &len, &idx, 0, NULL) 
						== HASH_KEY_NON_EXISTANT) {
					continue;
				}

				if ((tmp = strstr(key, ex))) {
					while(strlen(tmp) && (*(tmp - 1) == ' ' || *(tmp - 1) == '	')) tmp--;
					len = tmp - key;
				} else {
					len -= 1;
				}

				if (zend_hash_get_current_data(ht, (void **)&ppzv) == FAILURE) {
					continue;
				}

				if (Z_TYPE_PP(ppzv) == IS_ARRAY) {
					section = 1;
					zend_hash_get_pointer(ht, &pot);
					sec = yaf_config_ini_parse_section(ht, key, len TSRMLS_CC);
					zend_hash_set_pointer(ht, &pot);
					if (sec) {
						Z_ARRVAL_P(sec) = yaf_config_ini_parse_record(Z_ARRVAL_P(sec) TSRMLS_CC);
						if (tmp) {
							key = estrndup(key, len);
							zend_hash_update(Z_ARRVAL_P(configs), key, len + 1, (void **)&sec, sizeof(zval *), NULL);
							efree(key);
						} else {
							zend_hash_update(Z_ARRVAL_P(configs), key, len + 1, (void **)&sec, sizeof(zval *), NULL);
						}
					}
				}
			}

			if (!section) {
				zend_hash_copy(Z_ARRVAL_P(configs), ht, (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
				Z_ARRVAL_P(configs) = yaf_config_ini_parse_record(Z_ARRVAL_P(configs) TSRMLS_CC);
			}

			zval_dtor(ini_entries);
			efree(ini_entries);
		}

		if (this_ptr) {
			instance = this_ptr;
		} else {
			MAKE_STD_ZVAL(instance);
			object_init_ex(instance, yaf_config_ini_ce);

		}

		if (!configs) {
			MAKE_STD_ZVAL(configs);
			array_init(configs);
		}

		yaf_update_property(instance, YAF_CONFIG_PROPERT_NAME, configs);

		return instance;
	} else {
		yaf_trigger_error(YAF_ERR_TYPE_ERROR, "invalid parameters provided, must be location of ini file");
		return NULL;	
	}
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::__construct(mixed $config_path, string $section_name) 
*/
PHP_METHOD(yaf_config_ini, __construct) {
	zval *filename	= NULL;
	zval *section	= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &filename, &section) == FAILURE) {
		zval *prop = NULL;
		MAKE_STD_ZVAL(prop);
		array_init(prop);
		yaf_update_property(getThis(), YAF_CONFIG_PROPERT_NAME, prop);

		WRONG_PARAM_COUNT;
	}

	(void)yaf_config_ini_instance(getThis(), filename, section TSRMLS_CC);
}
/** }}} */

/** {{{ proto public Yaf_Config_Ini::get(string $name = NULL)
*/
PHP_METHOD(yaf_config_ini, get) {
	char * name;
	int len = 0;
	zval * ret, **ppzval;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &name, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (!len) {
		RETURN_ZVAL(getThis(), 1, 0);
	} else {
		zval *properties = NULL;
		char *entry, *seg, *pptr;

		properties = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
		entry = estrndup(name, len);

		seg = php_strtok_r(entry, ".", &pptr);
		while (seg) {

			if (Z_TYPE_P(properties) != IS_ARRAY) {
				efree(entry);
				RETURN_NULL();
			}

			if (zend_hash_find(Z_ARRVAL_P(properties), seg, strlen(seg) + 1, (void **) &ppzval) == FAILURE) {
				efree(entry);
				RETURN_NULL();
			}

			properties = *ppzval;
			seg = php_strtok_r(NULL, ".", &pptr);
		}
		efree(entry);

		if (Z_TYPE_PP(ppzval) == IS_ARRAY) {
			ret = yaf_config_ini_format(getThis(), ppzval TSRMLS_CC);
			RETURN_ZVAL(ret, 0, 0);
		} else {
			RETURN_ZVAL(*ppzval, 1, 0);
		}
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::toArray(void)
*/
PHP_METHOD(yaf_config_ini, toArray) {
	zval * properties = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
	RETURN_ZVAL(properties, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::__get($name = NULL)
*/
PHP_METHOD(yaf_config_ini, __get) {
	PHP_MN(yaf_config_ini_get)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::set($name, $value)
*/
PHP_METHOD(yaf_config_ini, set) {
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::__isset($name)
*/
PHP_METHOD(yaf_config_ini, __isset) {
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

/** {{{ proto public Yaf_Config_Ini::__set($name, $value)
*/
PHP_METHOD(yaf_config_ini, __set) {
	PHP_MN(yaf_config_ini_set)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::count($name)
*/
PHP_METHOD(yaf_config_ini, count) {
	zval * prop = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
	RETURN_LONG(zend_hash_num_elements(Z_ARRVAL_P(prop)));
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::offsetGet($index) 
*/
PHP_METHOD(yaf_config_ini, offsetGet) {
	PHP_MN(yaf_config_ini_get)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::offsetSet($index, $value) 
*/
PHP_METHOD(yaf_config_ini, offsetSet) {
	PHP_MN(yaf_config_ini_set)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::offsetUnset($index) 
*/
PHP_METHOD(yaf_config_ini, offsetUnset) {
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::offsetExists($index) 
*/
PHP_METHOD(yaf_config_ini, offsetExists) {
	PHP_MN(yaf_config_ini___isset)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::rewind(void)
*/
PHP_METHOD(yaf_config_ini, rewind) {
	zval * prop = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(prop));
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::current(void)
*/
PHP_METHOD(yaf_config_ini, current) {
	zval * prop, ** ppzval;
	zval *ret = NULL;
	prop = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
	if (zend_hash_get_current_data(Z_ARRVAL_P(prop), (void **)&ppzval) == FAILURE) {
		RETURN_FALSE;
	}

	if (Z_TYPE_PP(ppzval) == IS_ARRAY) {
		ret = yaf_config_ini_format(getThis(),  ppzval TSRMLS_CC);
		RETURN_ZVAL(ret, 0, 0);
	} else {
		RETURN_ZVAL(*ppzval, 1, 0);
	}
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::key(void)
*/
PHP_METHOD(yaf_config_ini, key) {
	zval * prop;
	char * string;
	long index;

	prop = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);

	if (zend_hash_get_current_key(Z_ARRVAL_P(prop), &string, &index, 0) == HASH_KEY_IS_LONG) {
		RETURN_LONG(index);
	} else {
		RETURN_STRING(string, 1);
	}
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::next(void)
*/
PHP_METHOD(yaf_config_ini, next) {
	zval *prop = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
	zend_hash_move_forward(Z_ARRVAL_P(prop));
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::valid(void)
*/
PHP_METHOD(yaf_config_ini, valid) {
	zval * prop = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
	RETURN_LONG(zend_hash_has_more_elements(Z_ARRVAL_P(prop)) == SUCCESS);
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::readonly(void)
*/
PHP_METHOD(yaf_config_ini, readonly) {
	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Yaf_Config_Ini::__destruct
*/
PHP_METHOD(yaf_config_ini, __destruct) {
	zval *properties = yaf_read_property(getThis(), YAF_CONFIG_PROPERT_NAME);
	zval_dtor(properties);
}
/* }}} */

/** {{{ proto private Yaf_Config_Ini::__clone
*/
PHP_METHOD(yaf_config_ini, __clone) {
}
/* }}} */

/** {{{ yaf_config_ini_methods
*/
zend_function_entry yaf_config_ini_methods[] = {
	PHP_ME(yaf_config_ini, __construct,	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
		/* PHP_ME(yaf_config_ini, __destruct,	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DTOR) */
		PHP_ME(yaf_config_ini, __get,		yaf_getter_arg, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, __set,		yaf_setter_arg, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, __isset,		yaf_getter_arg, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, get,			NULL, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, set,			NULL, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, count,		NULL, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, offsetGet,	yaf_getter_arg, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, offsetExists, yaf_getter_arg, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, offsetUnset,	yaf_getter_arg, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, offsetSet,	yaf_setter_arg, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, rewind,		NULL, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, current,		NULL, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, next,			NULL, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, valid,		NULL, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, key,			NULL, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, toArray,		NULL, ZEND_ACC_PUBLIC)
		PHP_ME(yaf_config_ini, readonly,		NULL, ZEND_ACC_PUBLIC)
		{NULL, NULL, NULL}
};

/* }}} */

/** {{{ YAF_STARTUP_FUNCTION
*/
YAF_STARTUP_FUNCTION(config_ini) {
	zend_class_entry ce;

	YAF_INIT_CLASS_ENTRY(ce, "Yaf_Config_Ini", "Yaf\\Config\\Ini", yaf_config_ini_methods);
	yaf_config_ini_ce = zend_register_internal_class_ex(&ce, yaf_config_ce, NULL TSRMLS_CC);

#ifdef HAVE_SPL
	zend_class_implements(yaf_config_ini_ce TSRMLS_CC, 3, zend_ce_iterator, zend_ce_arrayaccess, spl_ce_Countable);
#else
	zend_class_implements(yaf_config_ini_ce TSRMLS_CC, 2, zend_ce_iterator, zend_ce_arrayaccess);
#endif

	yaf_config_ini_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

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
