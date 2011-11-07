--TEST--
Check for Yaf_Application
--SKIPIF--
<?php if (!extension_loaded("yaf")) print "skip"; ?>
--INI--
--FILE--
<?php 
$config = array(
	"application" => array(
		"directory" => realpath(dirname(__FILE__)),
		"dispatcher" => array(
			"catchException" => 0,
			"throwException" => 0,
		),
	),
);

$app = new Yaf_Application($config);
$app->setAppDirectory('/tmp');
$app->run();
?>
--EXPECTF--
Catchable fatal error: Yaf_Application::run(): Could not find controller script /tmp/controllers/Index.php in %s022.php on line %d
