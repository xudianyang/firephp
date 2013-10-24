# firephp - PHP console


## Requirement


* PHP 5.2 +


## Install firephp


### Compile firephp in Linux/Unix


	$/path/to/phpize
	$./configure --with-php-config=/path/to/php-config
	$make && make install


### For windows

暂时未提供

## Usage
	<?php
		$arr = array(
    		'xu' => '徐',
  			'dian' => '典',
		);
		$e1 = new Exception("测试php的firephp C扩展", 110);
		console($GLOBALS);
		console($e1);
	?>
	