--TEST--
Bug #63438 (Strange behavior with nested rendering)
--SKIPIF--
<?php if (!extension_loaded("yaf")) print "skip"; ?>
--INI--
yaf.use_spl_autoload=1
yaf.lowcase_path=0
--FILE--
<?php
function view($file){
    static $view;
   
    $view = new Yaf_View_Simple(dirname(__FILE__));
    return $view->render($file);
}

file_put_contents(dirname(__FILE__) . '/outer.phtml', "1 <?php print view('inner.phtml');?> 3");
file_put_contents(dirname(__FILE__) . '/inner', "2");
print (view('/tmp/outer.phtml'));

file_put_contents(dirname(__FILE__) . '/outer.phtml', "1 <?php \$this->display('inner.phtml');?> 3");
print (view('/tmp/outer.phtml'));

file_put_contents(dirname(__FILE__) . '/outer.phtml', "1 <?php echo \$this->eval('inner.phtml');?> 3");
print (view('/tmp/outer.phtml'));
?>
--CLEAN--
<?php
@unlink(dirname(__FILE__) . '/outer.phtml');
@unlink(dirname(__FILE__) . '/inner.phtml');
?>
--EXPECTF--
1 2
 3
1 2
 3
1 2
 3
