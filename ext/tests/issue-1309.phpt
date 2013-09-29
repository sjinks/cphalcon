--TEST--
Segmentation fault in \Phalcon\Tag::tagHtml - https://github.com/phalcon/cphalcon/issues/1309
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php
echo \Phalcon\Tag::tagHtml('aside');
?>
--EXPECT--
<aside></aside>
