--TEST--
Check for Yaf_Route_Static
--SKIPIF--
<?php if (!extension_loaded("yaf")) print "skip"; ?>
--FILE--
<?php 
$request_uri = "/prefix/controller/action/name/laruence/age/28";
$base_uri	 = "/prefix/";

$request = new Yaf_Request_Http($request_uri, $base_uri);

unset($base_uri);
unset($request_uri);

$route = new Yaf_Route_Static();

var_dump($route->route($request));

var_dump($request);
?>
--EXPECTF--
bool(true)
object(Yaf_Request_Http)#%d (11) {
  ["module"]=>
  NULL
  ["controller"]=>
  string(10) "controller"
  ["action"]=>
  string(6) "action"
  ["method"]=>
  string(3) "Cli"
  ["params:protected"]=>
  array(2) {
    ["name"]=>
    string(8) "laruence"
    ["age"]=>
    string(2) "28"
  }
  ["language:protected"]=>
  NULL
  ["_exception:protected"]=>
  NULL
  ["_base_uri:protected"]=>
  string(8) "/prefix/"
  ["uri:protected"]=>
  string(46) "/prefix/controller/action/name/laruence/age/28"
  ["dispatched:protected"]=>
  bool(false)
  ["routed:protected"]=>
  bool(false)
}
