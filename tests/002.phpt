--TEST--
Check for Yaf_Request_Simple
--SKIPIF--
<?php if (!extension_loaded("yaf")) print "skip"; ?>
--FILE--
<?php 
$request  = new Yaf_Request_Simple("CLI", "index", "index", "index");
var_dump($request);
var_dump($request->setParam("name", "Laruence"));
var_dump($request->isCli());
var_dump($request->getParam("name"));
var_dump($request->getParam("notexists"));

?>
--EXPECTF--
object(Yaf_Request_Simple)#%d (11) {
  ["module"]=>
  string(5) "index"
  ["controller"]=>
  string(5) "index"
  ["action"]=>
  string(5) "index"
  ["method"]=>
  string(3) "CLI"
  ["params:protected"]=>
  array(0) {
  }
  ["language:protected"]=>
  NULL
  ["_exception:protected"]=>
  NULL
  ["_base_uri:protected"]=>
  string(0) ""
  ["uri:protected"]=>
  string(0) ""
  ["dispatched:protected"]=>
  bool(false)
  ["routed:protected"]=>
  bool(true)
}
object(Yaf_Request_Simple)#%d (11) {
  ["module"]=>
  string(5) "index"
  ["controller"]=>
  string(5) "index"
  ["action"]=>
  string(5) "index"
  ["method"]=>
  string(3) "CLI"
  ["params:protected"]=>
  array(1) {
    ["name"]=>
    string(8) "Laruence"
  }
  ["language:protected"]=>
  NULL
  ["_exception:protected"]=>
  NULL
  ["_base_uri:protected"]=>
  string(0) ""
  ["uri:protected"]=>
  string(0) ""
  ["dispatched:protected"]=>
  bool(false)
  ["routed:protected"]=>
  bool(true)
}
bool(true)
string(8) "Laruence"
NULL
