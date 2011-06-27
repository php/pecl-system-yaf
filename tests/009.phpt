--TEST--
Check for Yaf_View_Simple
--SKIPIF--
<?php if (!extension_loaded("yaf")) print "skip"; ?>
--FILE--
<?php
$view = new Yaf_View_Simple(dirname(__FILE__));
$value = "laruence";
$view->assign("name", $value);
unset($value);
var_dump($view);
var_dump(strlen($view->render(dirname(__FILE__) . "/002.phpt")));
var_dump($view->name);
var_dump($view->noexists);
var_dump($view);
--EXPECTF--
object(Yaf_View_Simple)#1 (2) {
  ["_tpl_vars:protected"]=>
  array(1) {
    ["name"]=>
    string(8) "laruence"
  }
  ["_tpl_dir:protected"]=>
  string(%d) "%s"
}
int(2038)
string(8) "laruence"
NULL
object(Yaf_View_Simple)#1 (2) {
  ["_tpl_vars:protected"]=>
  array(1) {
    ["name"]=>
    string(8) "laruence"
  }
  ["_tpl_dir:protected"]=>
  string(%d) "%s"
}
