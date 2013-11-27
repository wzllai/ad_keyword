/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_AD_KEYWORD_H
#define PHP_AD_KEYWORD_H

extern zend_module_entry ad_keyword_module_entry;
#define phpext_ad_keyword_ptr &ad_keyword_module_entry

#ifdef PHP_WIN32
#	define PHP_AD_KEYWORD_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_AD_KEYWORD_API __attribute__ ((visibility("default")))
#else
#	define PHP_AD_KEYWORD_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define AD_PATTERN_ALL  1
#define AD_PATTERN_UNIQUE 2
#define WU_MMIN 2
#define WU_MBLOCK 2

PHP_MINIT_FUNCTION(ad_keyword);
PHP_MSHUTDOWN_FUNCTION(ad_keyword);
PHP_RINIT_FUNCTION(ad_keyword);
PHP_RSHUTDOWN_FUNCTION(ad_keyword);
PHP_MINFO_FUNCTION(ad_keyword);


PHP_FUNCTION(ad_keywords);	/* For testing, remove later. */
PHP_FUNCTION(ad_wrapper);
struct _ad_hash_pair {
  int prefix_hash;
  int id;
  struct _ad_hash_pair *next;
};
typedef struct _ad_hash_pair ad_hash_pair;  

ZEND_BEGIN_MODULE_GLOBALS(ad_keyword)
  char *pattern_path;
  char **patterns;
  int *shift_hash_table;
  ad_hash_pair **hash_table;
  int pattern_size;
  int table_size;
ZEND_END_MODULE_GLOBALS(ad_keyword)


/* In every utility function you add that needs to use variables 
   in php_ad_keyword_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as AD_KEYWORD_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define AD_KEYWORD_G(v) TSRMG(ad_keyword_globals_id, zend_ad_keyword_globals *, v)
#else
#define AD_KEYWORD_G(v) (ad_keyword_globals.v)
#endif

#endif	/* PHP_AD_KEYWORD_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
