--TEST--
Check for Yaf_Router and Config Routes
--SKIPIF--
<?php if (!extension_loaded("yaf")) print "skip"; ?>
--FILE--
<?php 
$file = dirname(__FILE__) . "/simple.ini";

$config = new Yaf_Config_Ini($file, 'extra');

$routes = $config->routes;

$router = new Yaf_Router();
$router->addConfig($routes);

print_r($router->getRoutes());
?>
--EXPECTF--
Array
(
    [_default] => Yaf_Route_Static Object
        (
        )

    [regex] => Yaf_Route_Regex Object
        (
            [_route:protected] => ^/ap/(.*)
            [_default:protected] => Array
                (
                    [controller] => Index
                    [action] => action
                )

            [_maps:protected] => Array
                (
                    [0] => name
                    [1] => name
                    [2] => value
                )

            [_verify:protected] => 
        )

    [simple] => Yaf_Route_Simple Object
        (
            [controller:protected] => c
            [module:protected] => m
            [action:protected] => a
        )

    [supervar] => Yaf_Route_Supervar Object
        (
            [_var_name:protected] => c
        )

    [rewrite] => Yaf_Route_Rewrite Object
        (
            [_route:protected] => /ap/:name/:value
            [_default:protected] => Array
                (
                    [controller] => Index
                    [action] => action
                )

            [_verify:protected] => 
        )

)
