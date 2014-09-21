--TEST--
\Phalcon\Session\Bag destroys global variables - https://github.com/phalcon/cphalcon/issues/2795
--GET--
dummy=1
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php
$id1 = 0;
$id2 = 0;

$di = new \Phalcon\DI\FactoryDefault();
$di->setShared('nmclass', 'Nmclass');

class Nmclass extends \Phalcon\DI\Injectable
{
	public function run()
	{
		global $id1, $id2;
		$id1 = '123';
		$id2 = '456';
		$this->persistent->set('abc', $id1);
		$this->persistent->def = $id2;
	}
}

$x = $di->getShared('nmclass');
$x->run();

var_dump($id1);
var_dump($id2);

?>
--EXPECT--
string(3) "123"
string(3) "456"
