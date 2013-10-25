PHP_ARG_ENABLE(firephp, whether to enable firephp support,
[  --enable-firephp           Enable firephp support])

if test "$PHP_FIREPHP" != "no"; then

  AC_MSG_CHECKING([PHP version])

  tmp_version=$PHP_VERSION
  if test -z "$tmp_version"; then
    if test -z "$PHP_CONFIG"; then
      AC_MSG_ERROR([php-config not found])
    fi
    php_version=`$PHP_CONFIG --version 2>/dev/null|head -n 1|sed -e 's#\([0-9]\.[0-9]*\.[0-9]*\)\(.*\)#\1#'`
  else
    php_version=`echo "$tmp_version"|sed -e 's#\([0-9]\.[0-9]*\.[0-9]*\)\(.*\)#\1#'`
  fi

  if test -z "$php_version"; then
    AC_MSG_ERROR([failed to detect PHP version, please report])
  fi

  ac_IFS=$IFS
  IFS="."
  set $php_version
  IFS=$ac_IFS
  firephp_php_version=`expr [$]1 \* 1000000 + [$]2 \* 1000 + [$]3`

  if test "$firephp_php_version" -le "5002000"; then
    AC_MSG_ERROR([You need at least PHP 5.2.0 to be able to use this version of FirePHP. PHP $php_version found])
  else
    AC_MSG_RESULT([$php_version, ok])
  fi
  PHP_SUBST(FIREPHP_SHARED_LIBADD) 
  PHP_NEW_EXTENSION(firephp, firephp.c, $ext_shared) 
fi
