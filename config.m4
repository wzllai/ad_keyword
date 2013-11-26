dnl $Id$
dnl config.m4 for extension ad_keyword

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(ad_keyword, for ad_keyword support,
dnl Make sure that the comment is aligned:
dnl [  --with-ad_keyword             Include ad_keyword support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(ad_keyword, whether to enable ad_keyword support,
dnl Make sure that the comment is aligned:
[  --enable-ad_keyword           Enable ad_keyword support])

if test "$PHP_AD_KEYWORD" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-ad_keyword -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/ad_keyword.h"  # you most likely want to change this
  dnl if test -r $PHP_AD_KEYWORD/$SEARCH_FOR; then # path given as parameter
  dnl   AD_KEYWORD_DIR=$PHP_AD_KEYWORD
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for ad_keyword files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       AD_KEYWORD_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$AD_KEYWORD_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the ad_keyword distribution])
  dnl fi

  dnl # --with-ad_keyword -> add include path
  dnl PHP_ADD_INCLUDE($AD_KEYWORD_DIR/include)

  dnl # --with-ad_keyword -> check for lib and symbol presence
  dnl LIBNAME=ad_keyword # you may want to change this
  dnl LIBSYMBOL=ad_keyword # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $AD_KEYWORD_DIR/lib, AD_KEYWORD_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_AD_KEYWORDLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong ad_keyword lib version or lib not found])
  dnl ],[
  dnl   -L$AD_KEYWORD_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(AD_KEYWORD_SHARED_LIBADD)

  PHP_NEW_EXTENSION(ad_keyword, ad_keyword.c, $ext_shared)
fi
