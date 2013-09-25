
/*
  +------------------------------------------------------------------------+
  | Phalcon Framework                                                      |
  +------------------------------------------------------------------------+
  | Copyright (c) 2011-2013 Phalcon Team (http://www.phalconphp.com)       |
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_phalcon.h"
#include "phalcon.h"

#include "Zend/zend_operators.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_interfaces.h"

#include "kernel/main.h"
#include "kernel/memory.h"

#include "kernel/object.h"
#include "kernel/exception.h"
#include "kernel/operators.h"
#include "kernel/fcall.h"
#include "kernel/string.h"
#include "kernel/filter.h"
#include "kernel/concat.h"

/**
 * Phalcon\Security
 *
 * This component provides a set of functions to improve the security in Phalcon applications
 *
 *<code>
 *	$login = $this->request->getPost('login');
 *	$password = $this->request->getPost('password');
 *
 *	$user = Users::findFirstByLogin($login);
 *	if ($user) {
 *		if ($this->security->checkHash($password, $user->password)) {
 *			//The password is valid
 *		}
 *	}
 *</code>
 */


/**
 * Phalcon\Security initializer
 */
PHALCON_INIT_CLASS(Phalcon_Security){

	PHALCON_REGISTER_CLASS(Phalcon, Security, security, phalcon_security_method_entry, 0);

	zend_declare_property_null(phalcon_security_ce, SL("_dependencyInjector"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_long(phalcon_security_ce, SL("_workFactor"), 8, ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_long(phalcon_security_ce, SL("_numberBytes"), 16, ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(phalcon_security_ce, SL("_csrf"), ZEND_ACC_PROTECTED TSRMLS_CC);

	zend_class_implements(phalcon_security_ce TSRMLS_CC, 1, phalcon_di_injectionawareinterface_ce);

	return SUCCESS;
}

/**
 * Sets the dependency injector
 *
 * @param Phalcon\DiInterface $dependencyInjector
 */
PHP_METHOD(Phalcon_Security, setDI){

	zval *dependency_injector;

	phalcon_fetch_params(0, 1, 0, &dependency_injector);
	
	phalcon_update_property_this(this_ptr, SL("_dependencyInjector"), dependency_injector TSRMLS_CC);
	
}

/**
 * Returns the internal dependency injector
 *
 * @return Phalcon\DiInterface
 */
PHP_METHOD(Phalcon_Security, getDI){


	RETURN_MEMBER(this_ptr, "_dependencyInjector");
}

/**
 * Sets a number of bytes to be generated by the openssl pseudo random generator
 *
 * @param string $randomBytes
 */
PHP_METHOD(Phalcon_Security, setRandomBytes){

	zval *random_bytes;

	phalcon_fetch_params(0, 1, 0, &random_bytes);
	
	if (Z_TYPE_P(random_bytes) != IS_LONG) {
		PHALCON_THROW_EXCEPTION_STRW(phalcon_security_exception_ce, "Random bytes must be integer");
		return;
	}
	if (PHALCON_LT_LONG(random_bytes, 16)) {
		PHALCON_THROW_EXCEPTION_STRW(phalcon_security_exception_ce, "At least 16 bytes are needed to produce a correct salt");
		return;
	}
	
	phalcon_update_property_this(this_ptr, SL("_numberBytes"), random_bytes TSRMLS_CC);
	
}

/**
 * Returns a number of bytes to be generated by the openssl pseudo random generator
 *
 * @return string
 */
PHP_METHOD(Phalcon_Security, getRandomBytes){


	RETURN_MEMBER(this_ptr, "_numberBytes");
}

/**
 * Sets the default working factor for bcrypts password's salts
 *
 * @param int $workFactor
 */
PHP_METHOD(Phalcon_Security, setWorkFactor){

	zval *work_factor;

	phalcon_fetch_params(0, 1, 0, &work_factor);
	
	if (Z_TYPE_P(work_factor) != IS_LONG) {
		PHALCON_THROW_EXCEPTION_STRW(phalcon_security_exception_ce, "Work factor must be integer");
		return;
	}
	phalcon_update_property_this(this_ptr, SL("_workFactor"), work_factor TSRMLS_CC);
	
}

/**
 * Returns the default working factor for bcrypts password's salts
 *
 * @return int
 */
PHP_METHOD(Phalcon_Security, getWorkFactor){


	RETURN_MEMBER(this_ptr, "_workFactor");
}

/**
 * Generate a >22-length pseudo random string to be used as salt for passwords
 *
 * @return string
 */
PHP_METHOD(Phalcon_Security, getSaltBytes){

	zval *number_bytes, *random_bytes = NULL, *base64bytes = NULL;
	zval *safe_bytes = NULL, *bytes_length = NULL;

	PHALCON_MM_GROW();

	if (phalcon_function_exists_ex(SS("openssl_random_pseudo_bytes") TSRMLS_CC) == FAILURE) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_security_exception_ce, "Openssl extension must be loaded");
		return;
	}
	
	PHALCON_OBS_VAR(number_bytes);
	phalcon_read_property_this(&number_bytes, this_ptr, SL("_numberBytes"), PH_NOISY_CC);
	
	while (1) {
	
		PHALCON_INIT_NVAR(random_bytes);
		phalcon_call_func_p1(random_bytes, "openssl_random_pseudo_bytes", number_bytes);
	
		PHALCON_INIT_NVAR(base64bytes);
		phalcon_base64_encode(base64bytes, random_bytes);
	
		PHALCON_INIT_NVAR(safe_bytes);
		phalcon_filter_alphanum(safe_bytes, base64bytes);
		if (!zend_is_true(safe_bytes)) {
			continue;
		}
	
		PHALCON_INIT_NVAR(bytes_length);
		phalcon_fast_strlen(bytes_length, safe_bytes);
		if (PHALCON_LT_LONG(bytes_length, 22)) {
			continue;
		}
	
		break;
	}
	
	RETURN_CTOR(safe_bytes);
}

/**
 * Creates a password hash using bcrypt with a pseudo random salt
 *
 * @param string $password
 * @param int $workFactor
 * @return string
 */
PHP_METHOD(Phalcon_Security, hash){

	zval *password, *work_factor = NULL, *format, *factor;
	zval *salt_bytes, *salt;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 1, &password, &work_factor);
	
	if (!work_factor) {
		PHALCON_INIT_VAR(work_factor);
	} else {
		PHALCON_SEPARATE_PARAM(work_factor);
	}
	
	if (Z_TYPE_P(work_factor) == IS_NULL) {
		PHALCON_OBS_NVAR(work_factor);
		phalcon_read_property_this(&work_factor, this_ptr, SL("_workFactor"), PH_NOISY_CC);
	}
	
	PHALCON_INIT_VAR(format);
	ZVAL_STRING(format, "%02s", 1);
	
	PHALCON_INIT_VAR(factor);
	phalcon_call_func_p2(factor, "sprintf", format, work_factor);
	
	PHALCON_INIT_VAR(salt_bytes);
	phalcon_call_method(salt_bytes, this_ptr, "getsaltbytes");
	
	PHALCON_INIT_VAR(salt);
	PHALCON_CONCAT_SVSV(salt, "$2a$", factor, "$", salt_bytes);
	phalcon_call_func_p2(return_value, "crypt", password, salt);
	RETURN_MM();
}

/**
 * Checks a plain text password and its hash version to check if the password matches
 *
 * @param string $password
 * @param string $passwordHash
 * @param int $maxPasswordLength
 * @return boolean
 */
PHP_METHOD(Phalcon_Security, checkHash){

	zval *password, *password_hash, *hash, *max_pass_length = NULL;

	phalcon_fetch_params(0, 2, 1, &password, &password_hash, &max_pass_length);
	
	if (Z_TYPE_P(password) != IS_STRING) {
		PHALCON_SEPARATE_PARAM_NMO(password);
		convert_to_string(password);
	}

	if (max_pass_length) {
		if (Z_TYPE_P(max_pass_length) != IS_LONG) {
			PHALCON_SEPARATE_PARAM_NMO(max_pass_length);
			convert_to_long(max_pass_length);
		}

		if (Z_LVAL_P(max_pass_length) > 0 && Z_STRLEN_P(password) > Z_LVAL_P(max_pass_length)) {
			RETURN_FALSE;
		}
	}

	PHALCON_MM_GROW();
	PHALCON_INIT_VAR(hash);
	phalcon_call_func_p2(hash, "crypt", password, password_hash);
	is_equal_function(return_value, hash, password_hash TSRMLS_CC);
	RETURN_MM();
}

/**
 * Checks if a password hash is a valid bcrypt's hash
 *
 * @param string $password
 * @param string $passwordHash
 * @return boolean
 */
PHP_METHOD(Phalcon_Security, isLegacyHash){

	zval *password_hash;

	phalcon_fetch_params(0, 1, 0, &password_hash);
	
	if (phalcon_start_with_str(password_hash, SL("$2a$"))) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}

/**
 * Generates a pseudo random token key to be used as input's name in a CSRF check
 *
 * @param int $numberBytes
 * @return string
 */
PHP_METHOD(Phalcon_Security, getTokenKey){

	zval *number_bytes = NULL, *random_bytes, *base64bytes;
	zval *safe_bytes, *dependency_injector, *service;
	zval *session, *key;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 0, 1, &number_bytes);
	
	if (!number_bytes) {
		PHALCON_INIT_VAR(number_bytes);
		ZVAL_LONG(number_bytes, 12);
	}
	
	if (phalcon_function_exists_ex(SS("openssl_random_pseudo_bytes") TSRMLS_CC) == FAILURE) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_security_exception_ce, "Openssl extension must be loaded");
		return;
	}
	
	PHALCON_INIT_VAR(random_bytes);
	phalcon_call_func_p1(random_bytes, "openssl_random_pseudo_bytes", number_bytes);
	
	PHALCON_INIT_VAR(base64bytes);
	phalcon_base64_encode(base64bytes, random_bytes);
	
	PHALCON_INIT_VAR(safe_bytes);
	phalcon_filter_alphanum(safe_bytes, base64bytes);
	
	PHALCON_OBS_VAR(dependency_injector);
	phalcon_read_property_this(&dependency_injector, this_ptr, SL("_dependencyInjector"), PH_NOISY_CC);
	if (Z_TYPE_P(dependency_injector) != IS_OBJECT) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_flash_exception_ce, "A dependency injection container is required to access the 'session' service");
		return;
	}
	
	PHALCON_INIT_VAR(service);
	ZVAL_STRING(service, "session", 1);
	
	PHALCON_INIT_VAR(session);
	phalcon_call_method_p1(session, dependency_injector, "getshared", service);
	PHALCON_VERIFY_INTERFACE(session, phalcon_session_adapterinterface_ce);
	
	PHALCON_INIT_VAR(key);
	ZVAL_STRING(key, "$PHALCON/CSRF/KEY$", 1);
	phalcon_call_method_p2_noret(session, "set", key, safe_bytes);
	
	RETURN_CTOR(safe_bytes);
}

/**
 * Generates a pseudo random token value to be used as input's value in a CSRF check
 *
 * @param int $numberBytes
 * @return string
 */
PHP_METHOD(Phalcon_Security, getToken){

	zval *number_bytes = NULL, *random_bytes, *token, *dependency_injector;
	zval *service, *session, *key;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 0, 1, &number_bytes);
	
	if (!number_bytes) {
		PHALCON_INIT_VAR(number_bytes);
		ZVAL_LONG(number_bytes, 12);
	}
	
	if (phalcon_function_exists_ex(SS("openssl_random_pseudo_bytes") TSRMLS_CC) == FAILURE) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_security_exception_ce, "Openssl extension must be loaded");
		return;
	}
	
	PHALCON_INIT_VAR(random_bytes);
	phalcon_call_func_p1(random_bytes, "openssl_random_pseudo_bytes", number_bytes);
	
	PHALCON_INIT_VAR(token);
	phalcon_md5(token, random_bytes);
	
	PHALCON_OBS_VAR(dependency_injector);
	phalcon_read_property_this(&dependency_injector, this_ptr, SL("_dependencyInjector"), PH_NOISY_CC);
	if (Z_TYPE_P(dependency_injector) != IS_OBJECT) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_flash_exception_ce, "A dependency injection container is required to access the 'session' service");
		return;
	}
	
	PHALCON_INIT_VAR(service);
	ZVAL_STRING(service, "session", 1);
	
	PHALCON_INIT_VAR(session);
	phalcon_call_method_p1(session, dependency_injector, "getshared", service);
	PHALCON_VERIFY_INTERFACE(session, phalcon_session_adapterinterface_ce);
	
	PHALCON_INIT_VAR(key);
	ZVAL_STRING(key, "$PHALCON/CSRF$", 1);
	phalcon_call_method_p2_noret(session, "set", key, token);
	
	RETURN_CTOR(token);
}

/**
 * Check if the CSRF token sent in the request is the same that the current in session
 *
 * @param string $tokenKey
 * @param string $tokenValue
 * @return boolean
 */
PHP_METHOD(Phalcon_Security, checkToken){

	zval *token_key = NULL, *token_value = NULL, *dependency_injector;
	zval *service = NULL, *session, *key = NULL, *request, *token = NULL, *session_token;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 0, 2, &token_key, &token_value);
	
	if (!token_key) {
		PHALCON_INIT_VAR(token_key);
	} else {
		PHALCON_SEPARATE_PARAM(token_key);
	}
	
	if (!token_value) {
		PHALCON_INIT_VAR(token_value);
	}
	
	PHALCON_OBS_VAR(dependency_injector);
	phalcon_read_property_this(&dependency_injector, this_ptr, SL("_dependencyInjector"), PH_NOISY_CC);
	if (Z_TYPE_P(dependency_injector) != IS_OBJECT) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_flash_exception_ce, "A dependency injection container is required to access the 'session' service");
		return;
	}
	
	PHALCON_INIT_VAR(service);
	ZVAL_STRING(service, "session", 1);
	
	PHALCON_INIT_VAR(session);
	phalcon_call_method_p1(session, dependency_injector, "getshared", service);
	PHALCON_VERIFY_INTERFACE(session, phalcon_session_adapterinterface_ce);

	if (Z_TYPE_P(token_key) == IS_NULL) {
		PHALCON_INIT_VAR(key);
		ZVAL_STRING(key, "$PHALCON/CSRF/KEY$", 1);
	
		PHALCON_INIT_NVAR(token_key);
		phalcon_call_method_p1(token_key, session, "get", key);
	}
	
	if (Z_TYPE_P(token_value) == IS_NULL) {
		PHALCON_INIT_NVAR(service);
		ZVAL_STRING(service, "request", 1);
	
		PHALCON_INIT_VAR(request);
		phalcon_call_method_p1(request, dependency_injector, "getshared", service);
		PHALCON_VERIFY_INTERFACE(request, phalcon_http_requestinterface_ce);
	
		/** 
		 * We always check if the value is correct in post
		 */
		PHALCON_INIT_VAR(token);
		phalcon_call_method_p1(token, request, "getpost", token_key);
	} else {
		PHALCON_CPY_WRT(token, token_value);
	}
	
	PHALCON_INIT_NVAR(key);
	ZVAL_STRING(key, "$PHALCON/CSRF$", 1);
	
	PHALCON_INIT_VAR(session_token);
	phalcon_call_method_p1(session_token, session, "get", key);
	
	/** 
	 * The value is the same?
	 */
	is_equal_function(return_value, token, session_token TSRMLS_CC);
	
	RETURN_MM();
}

/**
 * Returns the value of the CSRF token in session
 *
 * @return string
 */
PHP_METHOD(Phalcon_Security, getSessionToken){

	zval *dependency_injector, *service, *session;
	zval *key, *session_token;

	PHALCON_MM_GROW();

	PHALCON_OBS_VAR(dependency_injector);
	phalcon_read_property_this(&dependency_injector, this_ptr, SL("_dependencyInjector"), PH_NOISY_CC);
	if (Z_TYPE_P(dependency_injector) != IS_OBJECT) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_flash_exception_ce, "A dependency injection container is required to access the 'session' service");
		return;
	}
	
	PHALCON_INIT_VAR(service);
	ZVAL_STRING(service, "session", 1);
	
	PHALCON_INIT_VAR(session);
	phalcon_call_method_p1(session, dependency_injector, "getshared", service);
	PHALCON_VERIFY_INTERFACE(session, phalcon_session_adapterinterface_ce);
	
	PHALCON_INIT_VAR(key);
	ZVAL_STRING(key, "$PHALCON/CSRF$", 1);
	
	PHALCON_INIT_VAR(session_token);
	phalcon_call_method_p1(session_token, session, "get", key);
	
	RETURN_CCTOR(session_token);
}

