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

#ifndef YAF_EXCEPTION_H
#define YAF_EXCEPTION_H

#define YAF_MAX_BUILDIN_EXCEPTION	10

#define YAF_ERR_ERROR    E_USER_ERROR
#define YAF_ERR_WARNING  E_WARNING
#define YAF_ERR_NOTICE   E_NOTICE
#define YAF_ERR_STRICT   E_STRICT

#define YAF_ERR_BASE 				512
#define YAF_UERR_BASE				1024
#define YAF_ERR_MASK					127

#define YAF_ERR_STARTUP_FAILED 		512
#define YAF_ERR_ROUTE_FAILED 		513
#define YAF_ERR_DISPATCH_FAILED 		514
#define YAF_ERR_NOTFOUND_MODULE 		515 
#define YAF_ERR_NOTFOUND_CONTROLLER 	516 
#define YAF_ERR_NOTFOUND_ACTION 		517
#define YAF_ERR_NOTFOUND_VIEW 		518
#define YAF_ERR_CALL_FAILED			519
#define YAF_ERR_AUTOLOAD_FAILED 		520
#define YAF_ERR_TYPE_ERROR			521

#define YAF_EXCEPTION_OFFSET(x) (x & YAF_ERR_MASK)

#define YAF_CORRESPOND_ERROR(x) (x>>9L)

#if defined(PHP_WIN32) && (_MSC_VER > 1400)

#define yaf_trigger_error(type, format, ...) \
	if (YAF_G(throw_exception)) {\
		yaf_throw_exception(type TSRMLS_CC, format, __VA_ARGS__);\
	} else { \
		php_error(YAF_ERR_ERROR, "[%d]"format, type, __VA_ARGS__); \
	}

#define yaf_error(format, ...) \
 		php_error(YAF_ERR_ERROR, format, __VA_ARGS__);

#define yaf_strict(format, ...) \
		php_error(YAF_ERR_STRICT, format,  __VA_ARGS__); 

#define yaf_warn(format, ...) \
		php_error(YAF_ERR_WARNING, format,  __VA_ARGS__); 

#define yaf_notice(format, ...) \
		php_error(YAF_ERR_NOTICE, format,  __VA_ARGS__);

#elif defined(__GNUC__) 

#define yaf_trigger_error(type, format, arg...) \
	if (YAF_G(throw_exception)) {\
		yaf_throw_exception(type TSRMLS_CC, format, ##arg);\
	} else { \
		php_error(YAF_ERR_ERROR, "[%d]"format, type, ##arg); \
	}

#define yaf_error(format, arg...) \
 		php_error(YAF_ERR_ERROR, format, ##arg);

#define yaf_strict(format, arg...) \
		php_error(YAF_ERR_STRICT, format,  ##arg); 

#define yaf_warn(format, arg...) \
		php_error(YAF_ERR_WARNING, format,  ##arg); 

#define yaf_notice(format, arg...) \
		php_error(YAF_ERR_NOTICE, format,  ##arg);

#endif

#define YAF_EXCEPTION_HANDLE(dispatcher, request, response) \
	if (EG(exception)) { \
		if (YAF_G(catch_exception)) { \
			yaf_dispatcher_exception_handler(dispatcher, request, response TSRMLS_CC); \
		} \
		return NULL; \
	}

#define YAF_EXCEPTION_HANDLE_NORET(dispatcher, request, response) \
	if (EG(exception)) { \
		if (YAF_G(catch_exception)) { \
			yaf_dispatcher_exception_handler(dispatcher, request, response TSRMLS_CC); \
		} \
	}

#define YAF_EXCEPTION_ERASE_EXCEPTION() \
	do { \
		EG(current_execute_data)->opline = EG(opline_before_exception); \
	} while(0)


void yaf_throw_exception(long code TSRMLS_DC, char *format, ...);
extern zend_class_entry * yaf_ce_RuntimeException;
extern zend_class_entry * yaf_exception_ce;

YAF_STARTUP_FUNCTION(exception);

#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
