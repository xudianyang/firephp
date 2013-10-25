PHP_ARG_ENABLE(firephp, Whether to enable the "firephp" extension, 
[  --enable-firephp	Enable "firephp" extension support])

if test $PHP_FIREPHP != "no"; then 
	PHP_SUBST(FIREPHP_SHARED_LIBADD) 
	PHP_NEW_EXTENSION(firephp, firephp.c, $ext_shared) 
fi