# Firephp - PHP console

A PHP extension for php-console.This is a C-implement of FirePHP.But now only implement tranditional INFO format information output. 


> **FirePHP is an advanced logging system that can display PHP variables in the browser as an application is navigated.**
> All communication is out of band to the application meaning that the logging data will not interfere with the normal functioning of the application.

	


## Requirement


* PHP 5.2 +


## Install firephp



### Compile firephp in Linux/Unix

	$/path/to/phpize
	$./configure --with-php-config=/path/to/php-config
	$make && make install
### For windows


Temporarily not available

## Author

This project is authored and maintained by [xudianyang](http://www.phpboy.net/).

##Documentation

Chinese Simplified documentation [PHP扩展之firephp console](http://www.phpboy.net/web/php/733.html).


## Usage

```php
<?php
	$arr = array(
		'key1' => 'php',
  		'key2' => 'boy',
	);
	$e1 = new Exception("test firephp php extension", 110);
	console($GLOBALS);
	sleep(1);
	console('xdy', true);
?>
```