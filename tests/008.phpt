--TEST--
Check for Yaf_Router
--SKIPIF--
<?php if (!extension_loaded("yaf")) print "skip"; ?>
--FILE--
<?php

$router = new Yaf_Router();

$route  = new Yaf_Route_Simple('m', 'c', 'a');
$sroute = new Yaf_Route_Supervar('r');

$router->addRoute("simple", $route)->addRoute("super", $sroute);
var_dump($router);
var_dump($router->getCurrentRoute());
var_dump($router->getRoutes());
var_dump($router->getRoute("simple"));
var_dump($router->getRoute("noexists"));
--EXPECTF--
object(Yaf_Router)#%d (2) {
  ["_routes:protected"]=>
  array(3) {
    ["_default"]=>
    object(Yaf_Route_Static)#%d (0) {
    }
    ["simple"]=>
    object(Yaf_Route_Simple)#%d (3) {
      ["controller:protected"]=>
      string(1) "c"
      ["module:protected"]=>
      string(1) "m"
      ["action:protected"]=>
      string(1) "a"
    }
    ["super"]=>
    object(Yaf_Route_Supervar)#%d (1) {
      ["_var_name:protected"]=>
      string(1) "r"
    }
  }
  ["_current:protected"]=>
  NULL
}
NULL
array(3) {
  ["_default"]=>
  object(Yaf_Route_Static)#%d (0) {
  }
  ["simple"]=>
  object(Yaf_Route_Simple)#%d (3) {
    ["controller:protected"]=>
    string(1) "c"
    ["module:protected"]=>
    string(1) "m"
    ["action:protected"]=>
    string(1) "a"
  }
  ["super"]=>
  object(Yaf_Route_Supervar)#%d (1) {
    ["_var_name:protected"]=>
    string(1) "r"
  }
}
object(Yaf_Route_Simple)#%d (3) {
  ["controller:protected"]=>
  string(1) "c"
  ["module:protected"]=>
  string(1) "m"
  ["action:protected"]=>
  string(1) "a"
}
NULL
