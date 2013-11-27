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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "php_ad_keyword.h"


ZEND_DECLARE_MODULE_GLOBALS(ad_keyword)

/* True global resources - no need for thread safety here */
static int le_ad_keyword;
static char *ad_substr(const char *str, int start, int end);
static char **ad_splite_str(char *str, int str_len, int pos[], int pos_len, int *len);
static hash_code(const char *str, int len);
static int hash_size(int pattern_size);

/* {{{ ad_keyword_functions[]
 *
 * Every user visible function must have an entry in ad_keyword_functions[].
 */
const zend_function_entry ad_keyword_functions[] = {
	PHP_FE(ad_keywords,	NULL)		/* For testing, remove later. */
  PHP_FE(ad_wrapper, NULL)
	PHP_FE_END	/* Must be the last line in ad_keyword_functions[] */
};
/* }}} */

/* {{{ ad_keyword_module_entry
 */
zend_module_entry ad_keyword_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"ad_keyword",
	ad_keyword_functions,
	PHP_MINIT(ad_keyword),
	PHP_MSHUTDOWN(ad_keyword),
	PHP_RINIT(ad_keyword),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(ad_keyword),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(ad_keyword),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_AD_KEYWORD
ZEND_GET_MODULE(ad_keyword)
#endif


static hash_code(const char *str, int len){
    unsigned int hash = 0;
    while (*str && len>0)
    {
        hash = (*str++) + (hash << 6) + (hash << 16) - hash;
        --len;
    }
    return (hash & 0x7FFFFFFF);
}

/*get the max array size for hashtable by patterns lines num*/
static int hash_size(int pattern_size) {
	int table_size = 0;
	int primes[6] = {1003, 10007, 100003, 1000003, 10000019};
	int threshold = 10 * WU_MMIN;
	int i = 0;
    for (; i < pattern_size; ++i)
    {
        if (primes[i] > pattern_size && primes[i] / pattern_size > threshold)
        {
            table_size = primes[i];
            break;
        }
    }	
    //if size of patternList is huge.
    if (0 == table_size)
    {
        zend_error(E_WARNING, "ad_keyword_module|amount of pattern is very large, will cost a great amount of memory.");
        table_size = primes[4];
    }    
    return table_size;
}

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("ad_keword.pattern", "", PHP_INI_ALL, OnUpdateString, pattern_path, zend_ad_keyword_globals, ad_keyword_globals)
PHP_INI_END()

/* }}} */
static void php_ad_keyword_init_globals(zend_ad_keyword_globals *ad_keyword_globals)
{
  ad_keyword_globals->pattern_path = NULL;
  ad_keyword_globals->patterns = NULL;
  ad_keyword_globals->hash_table = NULL;
  ad_keyword_globals->shift_hash_table = NULL;
  ad_keyword_globals->pattern_size = 0;
  ad_keyword_globals->table_size = 0;
}


static ad_keyword_globals_ctor(zend_ad_keyword_globals *ad_keyword_globals TSRMLS_DC) {
	FILE *fp;
	int file_exists;
	int pattern_size = 0;
	char buffer[60];
	int table_size;
  char *tmp_buffer = NULL;

	int i = 0;
	struct stat file_stat;
	const char *file_path = INI_STR("ad_keword.pattern");
  file_exists = VCWD_STAT(file_path, &file_stat);
	if(file_exists != 0){
		php_printf("the file:%s from php.ini not exists!\n", file_path);
		return 0;
	}
	fp = VCWD_FOPEN(file_path, "r");
  if (fp == NULL) { 
    return 1;
  } 
	while (fgets(buffer, 60, fp)) {
    tmp_buffer = php_trim(buffer, strlen(buffer), NULL, 0, NULL, 3 TSRMLS_CC);
    if (strlen(tmp_buffer)) {
		  pattern_size++;
    } 
	}
  char **patterns = pemalloc(pattern_size * sizeof(char*), 1);
  rewind(fp);
  while (fgets(buffer, 60, fp)) {
    tmp_buffer = php_trim(buffer, strlen(buffer), NULL, 0, NULL, 3 TSRMLS_CC);
    if (strlen(tmp_buffer)) {
      patterns[i++] = pestrdup(tmp_buffer, 1);
    }
  } 
  efree(tmp_buffer);

  //init hash table
  table_size = hash_size(pattern_size);
  ad_hash_pair **hash_table = pecalloc(table_size, sizeof(ad_hash_pair *), 1);
  int defaultValue = WU_MMIN - WU_MBLOCK + 1;
  int *shift_hash_table = pemalloc(table_size * sizeof(int), 1);
  int j;
  for (j = 0; j < table_size; ++j) {
  	shift_hash_table[j] = defaultValue;
  }

  //loop through patterns
  int id;
  for (id = 0; id < pattern_size; ++id) { 
      // loop through each pattern from right to left
      int index = WU_MMIN;
      for (index; index >= WU_MBLOCK; --index){
          unsigned int hash = hash_code(patterns[id] + index - WU_MBLOCK, WU_MBLOCK) % table_size;
          if (shift_hash_table[hash] > (WU_MMIN - index)){
              shift_hash_table[hash]  = WU_MMIN - index;
          }
          if (index == WU_MMIN){
              unsigned int prefix_hash = hash_code(patterns[id], WU_MBLOCK);
              ad_hash_pair *tmp = hash_table[hash];
              ad_hash_pair *p = pemalloc(sizeof(ad_hash_pair), 1);
              p->prefix_hash = prefix_hash;
              p->id = id;
              p->next = NULL;
              if (tmp) {
              while (tmp->next) {
              	tmp = tmp->next;
              }
              tmp->next = p; 
            } else {
              hash_table[hash] = p;
            }
          }
      }
  }    

  ad_keyword_globals->patterns = patterns;
  ad_keyword_globals->hash_table = hash_table;
  ad_keyword_globals->shift_hash_table = shift_hash_table;
  ad_keyword_globals->pattern_size = pattern_size;
  ad_keyword_globals->table_size = table_size;
}

static ad_keyword_globals_dctor(zend_ad_keyword_globals *ad_keyword_globals TSRMLS_DC) {
  char **patterns = ad_keyword_globals->patterns;;
  int size = ad_keyword_globals->pattern_size;
  int i, j;
  for (i = 0; i < size; ++i){
    pefree(patterns[i], 1);
  }
  pefree(patterns, 1);
  pefree(ad_keyword_globals->shift_hash_table, 1);

  int table_size = ad_keyword_globals->table_size;
  ad_hash_pair **hash_table = ad_keyword_globals->hash_table;
  for (j = 0; j < table_size; ++j){
    ad_hash_pair * tmp, *p;
    p = hash_table[j];
    while(p) {
      tmp = p;
      p = p->next;
      pefree(tmp, 1);
    }
  } 
  pefree(hash_table, 1);
}

static char  **ad_splite_str(char *str, int str_len, int pos_arr[], int pos_len, int *len) {
  int k = 1;
  int i = 1;

  for(;k<pos_len;k++) {
    if (pos_arr[k] == 0) {
      break;
    }
    if (k %2 == 0 && (pos_arr[k] < pos_arr[k-1])) {
      pos_arr[k-1] = pos_arr[k] = 0;
      k++;
    } else {
      i++;
    }
  }  
  i++;
  char **retval = emalloc(sizeof(char *) * i);
  *len = i;
  retval[0] = ad_substr(str, 0, pos_arr[0]-1);

  int j = 1;
  k = 0;
  while(k < pos_len) {
    int start = pos_arr[k]-1;
    int m = 1;
    int end;
    while (pos_arr[k+m] == 0) {
      m++;
    }
    k = k+m;
    if (k >= pos_len) {
      end = str_len;
    } else {
      end = pos_arr[k]-1;
    }
    retval[j] = ad_substr(str, start, end);
    j++;
  }  
  return retval;
}

static char *ad_substr(const char *str, int start, int end) {
  if (end <= start) {
    return NULL;
  }
  int len = end - start;
  int i = 0;
  char sub[len+1];
  while(i < len) {
    sub[i] = str[start+i];
    i++;
  }
  sub[len] = '\0';
  return estrdup(sub);
}

/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(ad_keyword)
{
	REGISTER_INI_ENTRIES();

	REGISTER_LONG_CONSTANT("AD_PATTERN_ALL", AD_PATTERN_ALL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("AD_PATTERN_UNIQUE", AD_PATTERN_UNIQUE, CONST_CS | CONST_PERSISTENT);
	#ifndef ZTS
    ad_keyword_globals_ctor(&ad_keyword_globals TSRMLS_CC);
  #endif
  return SUCCESS;
	
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(ad_keyword)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/

  #ifndef ZTS
  ad_keyword_globals_dctor(&ad_keyword_globals TSRMLS_CC);
  #endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(ad_keyword)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(ad_keyword)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(ad_keyword)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "ad_keyword support", "enabled");
	php_info_print_table_end();
  DISPLAY_INI_ENTRIES();
}
/* }}} */


/* Remove the following function when you have succesfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_ad_keyword_compiled(string arg)
   Return a string to confirm that the module is compiled in */


PHP_FUNCTION(ad_keywords)
{
  char *content = NULL;
  int content_len = 0;
  long mode = AD_PATTERN_UNIQUE; 
  zval *arr = NULL;
  MAKE_STD_ZVAL(arr);
  array_init(arr);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &content, &content_len, &mode) == FAILURE) {
		return;
	}
  if (!content_len) {
    RETURN_ZVAL(arr, 1, 1);
  }

  int size;
  int table_size;
  size = AD_KEYWORD_G(pattern_size);
  table_size = AD_KEYWORD_G(table_size);
  if (size == 0 || table_size == 0) {
     RETURN_ZVAL(arr, 1, 1);
  }

  char **patterns;
  int *shift_hash_table;
  ad_hash_pair **hash_table;
  patterns = AD_KEYWORD_G(patterns);
  shift_hash_table = AD_KEYWORD_G(shift_hash_table);
  hash_table = AD_KEYWORD_G(hash_table);

  int index = WU_MMIN - 1; // start off by matching end of largest common pattern
  int blockMaxIndex = WU_MBLOCK - 1;
  int windowMaxIndex = WU_MMIN - 1;

  //verify unique
  int (*is_equal_func)(zval *, zval *, zval * TSRMLS_DC) = is_equal_function;
  HashPosition pos;
  zval **entry, res, value;

  while (index < content_len) {
    int blockHash = hash_code(content + index - blockMaxIndex, WU_MBLOCK);
    blockHash = blockHash % table_size;
    int shift = shift_hash_table[blockHash];
      if (shift > 0) {
          index = index + shift;
      } else {
        int prefixHash = hash_code(content + index - windowMaxIndex, WU_MBLOCK);
        ad_hash_pair *list = hash_table[blockHash];
        while (list) {
          if (prefixHash == list->prefix_hash) {
            const char* indexTarget = content + index - windowMaxIndex;    //+mBlock
            const char* indexPattern = patterns[list->id]; //+mBlock  
            while (('\0' != *indexTarget) && ('\0' != *indexPattern)){
            // match until we reach end of either string
              if (*indexTarget == *indexPattern){
                  // match against chosen case sensitivity
                  ++indexTarget;
                  ++indexPattern;
              } else {
                  break;
              }
            }
            // match succeed since we reach the end of the pattern.
            if ('\0' == *indexPattern) {
              int last = index + strlen(patterns[list->id]);
              printf("index:%d, %d,%s,%s\n", index, last,indexTarget, indexPattern);
              int is_unique = 1;
              if (mode == AD_PATTERN_UNIQUE) {
                zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(arr), &pos);
                INIT_ZVAL(value);
                ZVAL_STRING(&value, patterns[list->id], 1);
                while (zend_hash_get_current_data_ex(Z_ARRVAL_P(arr), (void **)&entry, &pos) == SUCCESS) {
                    is_equal_func(&res, &value, *entry TSRMLS_CC);
                    if (Z_LVAL(res)) {
                      is_unique = 0;
                      break;
                    }   
                    zend_hash_move_forward_ex(Z_ARRVAL_P(arr), &pos);
                }        
                zval_dtor(&value);          
              }
              if (is_unique == 1) {
                add_next_index_string(arr, patterns[list->id], 1);
              }
            }                               
          }
              list = list->next;        
        }
        index++;
      }    
    }
    RETURN_ZVAL(arr, 1, 1);
}

PHP_FUNCTION(ad_wrapper)
{
  char *content = NULL;
  int content_len = 0;
  char *lef_delimiter = NULL;
  char *right_delimiter = NULL;  
  int lef_delimiter_len;
  int right_delimiter_len;
  long mode = AD_PATTERN_UNIQUE; 
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss", &content, &content_len, &lef_delimiter, &lef_delimiter_len, &right_delimiter, &right_delimiter_len) == FAILURE) {
    return;
  }
  if (!content_len) {
    RETURN_NULL();
  }

  int size;
  int table_size;
  size = AD_KEYWORD_G(pattern_size);
  table_size = AD_KEYWORD_G(table_size);
  if (size == 0 || table_size == 0) {
      RETURN_NULL();
  }

  char **patterns;
  int *shift_hash_table;
  ad_hash_pair **hash_table;
  patterns = AD_KEYWORD_G(patterns);
  shift_hash_table = AD_KEYWORD_G(shift_hash_table);
  hash_table = AD_KEYWORD_G(hash_table);

  int index = WU_MMIN - 1; // start off by matching end of largest common pattern
  int blockMaxIndex = WU_MBLOCK - 1;
  int windowMaxIndex = WU_MMIN - 1;

  int pos_arr[60] = {0};//todo:resize pos_arr
  int pos_len = 60;
  int pos = 0;
  while (index < content_len) {
    int blockHash = hash_code(content + index - blockMaxIndex, WU_MBLOCK);
    blockHash = blockHash % table_size;

    int shift = shift_hash_table[blockHash];
      if (shift > 0) {
          index = index + shift;
      } else {
        int prefixHash = hash_code(content + index - windowMaxIndex, WU_MBLOCK);
        ad_hash_pair *list = hash_table[blockHash];
        while (list) {
          if (prefixHash == list->prefix_hash) {
            const char* indexTarget = content + index - windowMaxIndex;    //+mBlock
            const char* indexPattern = patterns[list->id]; //+mBlock  
            while (('\0' != *indexTarget) && ('\0' != *indexPattern)){
            // match until we reach end of either string
              if (*indexTarget == *indexPattern){
                  // match against chosen case sensitivity
                  ++indexTarget;
                  ++indexPattern;
              } else {
                  break;
              }
            }
            // match succeed since we reach the end of the pattern.
            if ('\0' == *indexPattern) {
              //mark target postion
              pos_arr[pos++] = index;
              pos_arr[pos++] = index + strlen(patterns[list->id]);
              //printf("%d, %d\n", index, index + strlen(patterns[list->id]));

            }                               
          }
              list = list->next;        
        }
        index++;
      }    
    }

    if (pos_arr[0] == 0) {
      RETURN_STRING(content, 1);
    }

    char **splite_arr = NULL;
    int splitel_len = 0;
    int i = 0;

    splite_arr = ad_splite_str(content, content_len, pos_arr, pos_len,  &splitel_len);
    int ret_len = content_len + (lef_delimiter_len + right_delimiter_len + 2 ) * ceil(splitel_len/2) + splitel_len;
    char ret[ret_len];
    memset(ret, 0, ret_len);
    for(i; i<splitel_len; i++){
      if (splite_arr[i] == NULL) {
        continue;
      }
      if (i % 2 == 0) { 
        strcat(ret, splite_arr[i]);
      } else {
        strcat(ret, lef_delimiter);
        strcat(ret, splite_arr[i]);
        strcat(ret, right_delimiter);
      }
      efree(splite_arr[i]);
    }
    efree(splite_arr);
    RETURN_STRING(ret, 1);
}


/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */