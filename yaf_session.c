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
   $Id: yaf_session.c 51 2011-05-13 10:06:11Z laruence $ 
   */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "Zend/zend_interfaces.h"
#include "Zend/zend_objects.h"
#include "main/SAPI.h"

#include "php_yaf.h"
#include "yaf_namespace.h"
#include "yaf_session.h"
#include "yaf_exception.h"

zend_class_entry * yaf_session_ce;

/* {{{ YAF_ARG_INFO
 */
YAF_BEGIN_ARG_INFO_EX(yaf_getter_arg, 0, 0, 1)
	YAF_ARG_INFO(0, property_name)
YAF_END_ARG_INFO()

YAF_BEGIN_ARG_INFO_EX(yaf_setter_arg, 0, 0, 2)
	YAF_ARG_INFO(0, property_name)
	YAF_ARG_INFO(0, property_value)
YAF_END_ARG_INFO()
/* }}} */

/** {{{ inline boolean yaf_session_start(yaf_session_t *session TSRMLS_DC) 
 */
inline boolean yaf_session_start(yaf_session_t *session TSRMLS_DC) {
	zval *status = NULL;

	status  = yaf_read_property(session, YAF_SESSION_PROPERTY_NAME_STATUS);
	if (Z_BVAL_P(status)) {
		return TRUE;
	}

	php_session_start(TSRMLS_C);
	zend_update_property_bool(yaf_session_ce, session, YAF_STRL(YAF_SESSION_PROPERTY_NAME_STATUS), TRUE TSRMLS_CC);
	return TRUE;
}
/* }}} */

/** {{{ yaf_session_t *yaf_session_instance(yaf_session_t *this_ptr TSRMLS_DC)
*/
yaf_session_t *yaf_session_instance(yaf_session_t *this_ptr TSRMLS_DC) {
	yaf_session_t *instance = yaf_read_static_property(yaf_session_ce, YAF_SESSION_PROPERTY_NAME_INSTANCE);
	
	if (Z_TYPE_P(instance) != IS_OBJECT
			|| !instanceof_function(Z_OBJCE_P(instance), yaf_session_ce TSRMLS_CC)) {
		zval **sess 	 = NULL;
		zval *member     = NULL;
		zend_object *obj = NULL;
		zend_property_info *property_info = NULL;;

		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, yaf_session_ce);

		yaf_update_static_property(yaf_session_ce, YAF_SESSION_PROPERTY_NAME_INSTANCE, instance);

		yaf_session_start(instance TSRMLS_CC);

		if (zend_hash_find(&EG(symbol_table), YAF_STRS("_SESSION"), (void **)&sess) == FAILURE
				|| Z_TYPE_PP(sess) != IS_ARRAY) {
			yaf_trigger_error(YAF_ERR_STARTUP_FAILED, "start session failed");
			return NULL;
		}

		MAKE_STD_ZVAL(member);

		ZVAL_STRING(member, YAF_SESSION_PROPERTY_NAME_SESSION, 0);

		obj = zend_objects_get_address(instance TSRMLS_CC);

		property_info = zend_get_property_info(obj->ce, member, TRUE TSRMLS_CC);

		Z_ADDREF_P(*sess);
		/** This is ugly , because we can't set a ref property through the stadard APIs */
		zend_hash_quick_update(obj->properties, property_info->name,
			   	property_info->name_length+1, property_info->h, (void **)sess, sizeof(zval *), NULL);
		efree(member);
	}

	return instance;
}
/* }}} */

/** {{{ proto private Yaf_Session::__construct(void)
*/
PHP_METHOD(yaf_session, __construct) {
}
/* }}} */

/** {{{ proto private Yaf_Session::__destruct(void)
*/
PHP_METHOD(yaf_session, __destruct) {
	zval **osess = NULL;
	zval *sess   = yaf_read_property(getThis(), YAF_SESSION_PROPERTY_NAME_SESSION);

	if (zend_hash_find(&EG(symbol_table), YAF_STRS("_SESSION"), (void **)&osess) == FAILURE
			|| Z_TYPE_PP(osess) != IS_ARRAY) {
		yaf_warn("could not find $_SESSION at %s:%s", Z_OBJCE_P(getThis())->name, "__destruct");
		RETURN_FALSE;
	}

	zend_hash_copy(Z_ARRVAL_PP(osess), Z_ARRVAL_P(sess), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
}
/* }}} */

/** {{{ proto private Yaf_Session::__sleep(void)
*/
PHP_METHOD(yaf_session, __sleep) {
}
/* }}} */

/** {{{ proto private Yaf_Session::__wakeup(void)
*/
PHP_METHOD(yaf_session, __wakeup) {
}
/* }}} */

/** {{{ proto private Yaf_Session::__clone(void)
*/
PHP_METHOD(yaf_session, __clone) {
}
/* }}} */

/** {{{ proto public Yaf_Session::getInstance(void)
*/
PHP_METHOD(yaf_session, getInstance) {
	yaf_session_t	*session = NULL;

	session = yaf_session_instance(NULL TSRMLS_CC);
	RETURN_ZVAL(session, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Session::count(void)
*/
PHP_METHOD(yaf_session, count) {
	zval *sess = yaf_read_property(getThis(), YAF_SESSION_PROPERTY_NAME_SESSION);
	RETURN_LONG(zend_hash_num_elements(Z_ARRVAL_P(sess)));
}
/* }}} */

/** {{{ proto public static Yaf_Session::start()
*/
PHP_METHOD(yaf_session, start) {
	yaf_session_start(getThis() TSRMLS_CC);
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/** {{{ proto public static Yaf_Session::get($name)
*/
PHP_METHOD(yaf_session, get) {
	char *name  = NULL;
	int  len	= 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &name, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {  
		zval **ret = NULL;
		zval *sess = NULL;

		sess = yaf_read_property(getThis(), YAF_SESSION_PROPERTY_NAME_SESSION);

		if (!len) {
			RETURN_ZVAL(sess, 1, 0);
		}

		if (zend_hash_find(Z_ARRVAL_P(sess), name, len + 1, (void **)&ret) == FAILURE ){
			RETURN_NULL();
		}

		RETURN_ZVAL(*ret, 1, 0);
	} 
}
/* }}} */

/** {{{ proto public static Yaf_Session::__get($name)
*/
PHP_METHOD(yaf_session, __get) {
	PHP_MN(yaf_session_get)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public staitc Yaf_Session::set($name, $value)
*/
PHP_METHOD(yaf_session, set) {
	zval	*value	= NULL;
	char 	*name	= NULL;
	int  	len		= 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &len, &value) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {
		zval *sess = NULL;

		sess = yaf_read_property(getThis(), YAF_SESSION_PROPERTY_NAME_SESSION);

		Z_ADDREF_P(value);
		if (zend_hash_update(Z_ARRVAL_P(sess), name, len + 1, &value, sizeof(zval *), NULL) == FAILURE) {
			Z_DELREF_P(value);
			RETURN_FALSE;
		}
	}

	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/** {{{ proto public staitc Yaf_Session::__set($name, $value)
*/
PHP_METHOD(yaf_session, __set) {
	PHP_MN(yaf_session_set)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public staitc Yaf_Session::del($name)
*/
PHP_METHOD(yaf_session, del) {
	char 	*name	= NULL;
	int  	len		= 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {
		zval *sess = NULL;

		sess = yaf_read_property(getThis(), YAF_SESSION_PROPERTY_NAME_SESSION);

		if (zend_hash_del(Z_ARRVAL_P(sess), name, len + 1) == SUCCESS) {
			RETURN_ZVAL(getThis(), 1, 0);
		}
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Session::has($name)
*/
PHP_METHOD(yaf_session, has) {
	char *name  = NULL;
	int  len    = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {
		zval *sess = NULL;

		sess = yaf_read_property(getThis(), YAF_SESSION_PROPERTY_NAME_SESSION);

		RETURN_BOOL(zend_hash_exists(Z_ARRVAL_P(sess), name, len + 1));
	}

}
/* }}} */

/** {{{ proto public staitc Yaf_Session::__isset($name)
*/
PHP_METHOD(yaf_session, __isset) {
	PHP_MN(yaf_session_has)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Session::__unset($name)
 */
PHP_METHOD(yaf_session, __unset) {
	PHP_MN(yaf_session_del)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Session::offsetGet($index) 
*/
PHP_METHOD(yaf_session, offsetGet) {
	PHP_MN(yaf_session_get)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Session::offsetSet($index, $value) 
*/
PHP_METHOD(yaf_session, offsetSet) {
	PHP_MN(yaf_session_set)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Session::offsetUnset($index) 
*/
PHP_METHOD(yaf_session, offsetUnset) {
	PHP_MN(yaf_session_del)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Session::offsetExists($index) 
*/
PHP_METHOD(yaf_session, offsetExists) {
	PHP_MN(yaf_session_has)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ proto public Yaf_Session::rewind(void)
*/
PHP_METHOD(yaf_session, rewind) {
	zval *sess = yaf_read_property(getThis(), YAF_SESSION_PROPERTY_NAME_SESSION);
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(sess));
}
/* }}} */

/** {{{ proto public Yaf_Session::current(void)
*/
PHP_METHOD(yaf_session, current) {
	zval *sess, **ppzval;
	sess = yaf_read_property(getThis(), YAF_SESSION_PROPERTY_NAME_SESSION);
	if (zend_hash_get_current_data(Z_ARRVAL_P(sess), (void **)&ppzval) == FAILURE) {
		RETURN_FALSE;
	}

	RETURN_ZVAL(*ppzval, 1, 0);
}
/* }}} */

/** {{{ proto public Yaf_Session::key(void)
*/
PHP_METHOD(yaf_session, key) {
	zval *sess = NULL;
	char *key  = NULL;
	long index = 0; 

	sess = yaf_read_property(getThis(), YAF_SESSION_PROPERTY_NAME_SESSION);

	if (zend_hash_get_current_key(Z_ARRVAL_P(sess), &key, &index, 0) == HASH_KEY_IS_LONG) {
		RETURN_LONG(index);
	} else {
		RETURN_STRING(key, 1);
	}
}
/* }}} */

/** {{{ proto public Yaf_Session::next(void)
*/
PHP_METHOD(yaf_session, next) {
	zval *sess = yaf_read_property(getThis(), YAF_SESSION_PROPERTY_NAME_SESSION);
	zend_hash_move_forward(Z_ARRVAL_P(sess));
}
/* }}} */

/** {{{ proto public Yaf_Session::valid(void)
*/
PHP_METHOD(yaf_session, valid) {
	zval *sess = yaf_read_property(getThis(), YAF_SESSION_PROPERTY_NAME_SESSION);
	RETURN_LONG(zend_hash_has_more_elements(Z_ARRVAL_P(sess)) == SUCCESS);
}
/* }}} */

/** {{{ yaf_session_methods 
*/
zend_function_entry yaf_session_methods[] = {
	PHP_ME(yaf_session, __construct, 	NULL, ZEND_ACC_CTOR|ZEND_ACC_PRIVATE)
	PHP_ME(yaf_session, __clone, 		NULL, ZEND_ACC_CLONE|ZEND_ACC_PRIVATE)
	PHP_ME(yaf_session, __sleep, 		NULL, ZEND_ACC_PRIVATE)
	PHP_ME(yaf_session, __wakeup, 		NULL, ZEND_ACC_PRIVATE)
	PHP_ME(yaf_session, getInstance,		NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yaf_session, start,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, get,				yaf_getter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, has,				yaf_getter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, set,				yaf_setter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, del,				yaf_getter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, offsetGet,		yaf_getter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, offsetSet,		yaf_setter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, offsetExists,	yaf_getter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, offsetUnset,		yaf_getter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, count,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, rewind,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, next,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, current,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, key,				NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, valid,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, __get,			yaf_getter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, __isset,			yaf_getter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, __set,			yaf_setter_arg, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_session, __unset,			yaf_getter_arg, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ YAF_STARTUP_FUNCTION
*/
YAF_STARTUP_FUNCTION(session) {
	zend_class_entry ce;

	YAF_INIT_CLASS_ENTRY(ce, "Yaf_Session", "Yaf\\Session", yaf_session_methods);

	yaf_session_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	yaf_session_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

#ifdef HAVE_SPL
	zend_class_implements(yaf_session_ce TSRMLS_CC, 3, zend_ce_iterator, zend_ce_arrayaccess, spl_ce_Countable);
#else
	zend_class_implements(yaf_session_ce TSRMLS_CC, 2, zend_ce_iterator, zend_ce_arrayaccess);
#endif

	zend_declare_property_null(yaf_session_ce, YAF_STRL(YAF_SESSION_PROPERTY_NAME_INSTANCE), ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_null(yaf_session_ce, YAF_STRL(YAF_SESSION_PROPERTY_NAME_SESSION),  ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_bool(yaf_session_ce, YAF_STRL(YAF_SESSION_PROPERTY_NAME_STATUS),   FALSE, ZEND_ACC_PROTECTED TSRMLS_CC);
	
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
