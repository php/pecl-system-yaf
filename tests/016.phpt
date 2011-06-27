--TEST--
Check for Yaf_Session
--SKIPIF--
<?php if (!extension_loaded("yaf")) print "skip"; ?>
--FILE--
<?php 
$session = Yaf_Session::getInstance();

$_SESSION["name"] = "Laruence";

$age = 28;
$session->age = $age;
unset($age);

$session["company"] = "Baidu";

var_dump(isset($session->age));
var_dump($session->has("name"));
var_dump(count($session));
foreach ($session as $key => $value) {
	echo $key , "=>", $value, "\n";
}

$session->del("name");
unset($session["company"]);
unset($session->age);

var_dump(count($session));
?>
--EXPECTF--
bool(true)
bool(true)
int(3)
name=>Laruence
age=>28
company=>Baidu
int(0)
