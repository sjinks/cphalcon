
/*
  +------------------------------------------------------------------------+
  | Phalcon Framework                                                      |
  +------------------------------------------------------------------------+
  | Copyright (c) 2011-2014 Phalcon Team (http://www.phalconphp.com)       |
  +------------------------------------------------------------------------+
  | This source file is subject to the New BSD License that is bundled     |
  | with this package in the file docs/LICENSE.txt.                        |
  |                                                                        |
  | If you did not receive a copy of the license and are unable to         |
  | obtain it through the world-wide-web, please send an email             |
  | to license@phalconphp.com so we can send you a copy immediately.       |
  +------------------------------------------------------------------------+
  | Authors: Andres Gutierrez <andres@phalconphp.com>                      |
  |          Eduar Carvajal <eduar@phalconphp.com>                         |
  +------------------------------------------------------------------------+
*/

#include "mvc/model/validator/regex.h"
#include "mvc/model/validator.h"
#include "mvc/model/validatorinterface.h"
#include "mvc/model/exception.h"

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/fcall.h"
#include "kernel/exception.h"
#include "kernel/string.h"
#include "kernel/array.h"
#include "kernel/operators.h"
#include "kernel/concat.h"

/**
 * Phalcon\Mvc\Model\Validator\Regex
 *
 * Allows validate if the value of a field matches a regular expression
 *
 *<code>
 *use Phalcon\Mvc\Model\Validator\Regex as RegexValidator;
 *
 *class Subscriptors extends Phalcon\Mvc\Model
 *{
 *
 *  public function validation()
 *  {
 *      $this->validate(new RegexValidator(array(
 *          'field' => 'created_at',
 *          'pattern' => '/^[0-9]{4}[-\/](0[1-9]|1[12])[-\/](0[1-9]|[12][0-9]|3[01])$/'
 *      )));
 *      if ($this->validationHasFailed() == true) {
 *          return false;
 *      }
 *  }
 *
 *}
 *</code>
 *
 */
zend_class_entry *phalcon_mvc_model_validator_regex_ce;

PHP_METHOD(Phalcon_Mvc_Model_Validator_Regex, validate);

static const zend_function_entry phalcon_mvc_model_validator_regex_method_entry[] = {
	PHP_ME(Phalcon_Mvc_Model_Validator_Regex, validate, arginfo_phalcon_mvc_model_validatorinterface_validate, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * Phalcon\Mvc\Model\Validator\Regex initializer
 */
PHALCON_INIT_CLASS(Phalcon_Mvc_Model_Validator_Regex){

	PHALCON_REGISTER_CLASS_EX(Phalcon\\Mvc\\Model\\Validator, Regex, mvc_model_validator_regex, phalcon_mvc_model_validator_ce, phalcon_mvc_model_validator_regex_method_entry, 0);

	zend_class_implements(phalcon_mvc_model_validator_regex_ce TSRMLS_CC, 1, phalcon_mvc_model_validatorinterface_ce);

	return SUCCESS;
}

/**
 * Executes the validator
 *
 * @param Phalcon\Mvc\ModelInterface $record
 * @return boolean
 */
PHP_METHOD(Phalcon_Mvc_Model_Validator_Regex, validate){

	zval *record, *option = NULL, *field_name, *is_set, *value;
	zval *failed = NULL, *matches, *pattern, *match_pattern;
	zval *match_zero, *message = NULL, *type, *is_set_code, *code;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &record);
	
	PHALCON_INIT_VAR(option);
	ZVAL_STRING(option, "field", 1);
	
	PHALCON_INIT_VAR(field_name);
	phalcon_call_method_p1(field_name, this_ptr, "getoption", option);
	if (Z_TYPE_P(field_name) != IS_STRING) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_exception_ce, "Field name must be a string");
		return;
	}
	
	/** 
	 * The 'pattern' option must be a valid regular expression
	 */
	PHALCON_INIT_NVAR(option);
	ZVAL_STRING(option, "pattern", 1);
	
	PHALCON_INIT_VAR(is_set);
	phalcon_call_method_p1(is_set, this_ptr, "issetoption", option);
	if (!zend_is_true(is_set)) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_exception_ce, "Validator requires a perl-compatible regex pattern");
		return;
	}
	
	PHALCON_INIT_VAR(value);
	phalcon_call_method_p1(value, record, "readattribute", field_name);
	
	PHALCON_INIT_VAR(failed);
	ZVAL_BOOL(failed, 0);
	
	PHALCON_INIT_VAR(matches);
	
	/** 
	 * The regular expression is set in the option 'pattern'
	 */
	PHALCON_INIT_VAR(pattern);
	phalcon_call_method_p1(pattern, this_ptr, "getoption", option);
	
	/** 
	 * Check if the value matches using preg_match
	 */
	PHALCON_INIT_VAR(match_pattern);
	phalcon_preg_match(match_pattern, NULL, pattern, value, matches TSRMLS_CC);
	
	if (zend_is_true(match_pattern)) {
		PHALCON_OBS_VAR(match_zero);
		phalcon_array_fetch_long(&match_zero, matches, 0, PH_NOISY);
	
		is_not_equal_function(failed, match_zero, value TSRMLS_CC);
	} else {
		PHALCON_INIT_NVAR(failed);
		ZVAL_BOOL(failed, 1);
	}
	
	if (PHALCON_IS_TRUE(failed)) {
	
		/** 
		 * Check if the developer has defined a custom message
		 */
		PHALCON_INIT_NVAR(option);
		ZVAL_STRING(option, "message", 1);
	
		PHALCON_INIT_VAR(message);
		phalcon_call_method_p1(message, this_ptr, "getoption", option);
		if (!zend_is_true(message)) {
			PHALCON_INIT_NVAR(message);
			PHALCON_CONCAT_SVS(message, "Value of field '", field_name, "' doesn't match regular expression");
		}
	
		PHALCON_INIT_VAR(type);
		ZVAL_STRING(type, "Regex", 1);

		/*
		 * Is code set
		 */
		PHALCON_INIT_NVAR(option);
		ZVAL_STRING(option, "code", 1);

		PHALCON_INIT_VAR(is_set_code);
		phalcon_call_method_p1(is_set_code, this_ptr, "issetoption", option);
		PHALCON_INIT_VAR(code);
		if (zend_is_true(is_set_code)) {
			phalcon_call_method_p1(code, this_ptr, "getoption", option);
		} else {
			ZVAL_LONG(code, 0);
		}

		phalcon_call_method_p4_noret(this_ptr, "appendmessage", message, field_name, type, code);
		RETURN_MM_FALSE;
	}
	
	RETURN_MM_TRUE;
}

