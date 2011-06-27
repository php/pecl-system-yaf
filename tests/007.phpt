--TEST--
Check for Yaf_Config_Simple
--SKIPIF--
<?php if (!extension_loaded("yaf")) print "skip"; ?>
--FILE--
<?php 
$config = array(
	'section1' => array(
		'name' => 'value',
		'dummy' =>  'foo',
	),
	'section2' => "laruence",
);

$config1 = new Yaf_Config_Simple($config, 'section2');
var_dump($config1);
$config2 = new Yaf_Config_Simple($config, 'section1');
var_dump($config2->readonly());
$config2->new = "value";
var_dump(isset($config->new));
$config3 = new Yaf_Config_Simple($config);
unset($config);

echo "Isset config3 section:";
var_dump(isset($config3["section2"]));
$config3->new = "value";
echo "Config3 readonly:";
var_dump($config3->readonly());

foreach($config3 as $key => $val) {
	var_dump($key);
	var_dump($val);
}

var_dump($config3->toArray());

$sick = new Yaf_Config_Simple();

var_dump($sick->__isset(1));
var_dump($sick->__get(2));
$sick->total = 1;
var_dump(count($sick));
var_dump($sick->total);
?>
--EXPECTF--
object(Yaf_Config_Simple)#%d (2) {
  ["_config:protected"]=>
  array(2) {
    ["section1"]=>
    array(2) {
      ["name"]=>
      string(5) "value"
      ["dummy"]=>
      string(3) "foo"
    }
    ["section2"]=>
    string(8) "laruence"
  }
  ["_readonly:protected"]=>
  bool(true)
}
bool(true)
bool(false)
Isset config3 section:bool(true)
Config3 readonly:bool(false)
string(8) "section1"
object(Yaf_Config_Simple)#%d (2) {
  ["_config:protected"]=>
  array(2) {
    ["name"]=>
    string(5) "value"
    ["dummy"]=>
    string(3) "foo"
  }
  ["_readonly:protected"]=>
  bool(false)
}
string(8) "section2"
string(8) "laruence"
string(3) "new"
string(5) "value"
array(3) {
  ["section1"]=>
  array(2) {
    ["name"]=>
    string(5) "value"
    ["dummy"]=>
    string(3) "foo"
  }
  ["section2"]=>
  string(8) "laruence"
  ["new"]=>
  string(5) "value"
}

Warning: %s

Warning: %s
bool(false)
bool(false)
int(1)
int(1)
