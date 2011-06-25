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
   $Id: yaf_router.h 53 2011-06-01 02:57:04Z laruence $
 */

#ifndef YAF_ROUTER_H
#define YAF_ROUTER_H

#define YAF_ROUTER_PROPERTY_NAME_ROUTERS 		"_routes"
#define YAF_ROUTER_PROPERTY_NAME_CURRENT_ROUTE	"_current"

extern zend_class_entry * yaf_router_ce;

yaf_router_t * yaf_router_instance(yaf_router_t *this_ptr TSRMLS_DC);
boolean yaf_router_route(yaf_router_t *router, yaf_request_t *request TSRMLS_DC);
zval * yaf_router_parse_parameters(char *uri TSRMLS_DC);

YAF_STARTUP_FUNCTION(router);
#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
