/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | http://www.opensource.org/licenses/mit-license.php  MIT License      |
  +----------------------------------------------------------------------+
  | Author: Jani Taskinen <jani.taskinen@iki.fi>                         |
  | Author: Patrick Reilly <preilly@php.net>                             |
  | Author: Stefan Siegl <stesie@php.net>                                |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {
#include "php.h"
#include "ext/date/php_date.h"
#include "ext/standard/php_string.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
#include "ext/spl/spl_exceptions.h"
#include "zend_exceptions.h"
}

#include "php_v8js_macros.h"


/* {{{ Class Entries */
zend_class_entry *php_ce_v8js_exception;
zend_class_entry *php_ce_v8js_script_exception;
zend_class_entry *php_ce_v8js_time_limit_exception;
zend_class_entry *php_ce_v8js_memory_limit_exception;
/* }}} */


/* {{{ Class: V8JsScriptException */

void v8js_create_script_exception(zval *return_value, v8::Isolate *isolate, v8::TryCatch *try_catch TSRMLS_DC) /* {{{ */
{
	v8::String::Utf8Value exception(try_catch->Exception());
	const char *exception_string = ToCString(exception);
	v8::Handle<v8::Message> tc_message = try_catch->Message();
	const char *filename_string, *sourceline_string;
	char *message_string;
	int linenum, start_col, end_col, message_len;

	object_init_ex(return_value, php_ce_v8js_script_exception);

#define PHPV8_EXPROP(type, name, value) \
	zend_update_property##type(php_ce_v8js_script_exception, return_value, #name, sizeof(#name) - 1, value TSRMLS_CC);

	if (tc_message.IsEmpty()) {
		message_len = spprintf(&message_string, 0, "%s", exception_string);
	}
	else
	{
		v8::String::Utf8Value filename(tc_message->GetScriptResourceName());
		filename_string = ToCString(filename);
		PHPV8_EXPROP(_string, JsFileName, filename_string);

		v8::String::Utf8Value sourceline(tc_message->GetSourceLine());
		sourceline_string = ToCString(sourceline);
		PHPV8_EXPROP(_string, JsSourceLine, sourceline_string);

		linenum = tc_message->GetLineNumber();
		PHPV8_EXPROP(_long, JsLineNumber, linenum);

		start_col = tc_message->GetStartColumn();
		PHPV8_EXPROP(_long, JsStartColumn, start_col);

		end_col = tc_message->GetEndColumn();
		PHPV8_EXPROP(_long, JsEndColumn, end_col);

		message_len = spprintf(&message_string, 0, "%s:%d: %s", filename_string, linenum, exception_string);

		v8::String::Utf8Value stacktrace(try_catch->StackTrace());
		if (stacktrace.length() > 0) {
			const char* stacktrace_string = ToCString(stacktrace);
			PHPV8_EXPROP(_string, JsTrace, stacktrace_string);
		}

		if(try_catch->Exception()->IsObject()) {
			v8::Local<v8::Value> php_ref = try_catch->Exception()->ToObject()->GetHiddenValue(V8JS_SYM(PHPJS_OBJECT_KEY));

			if(!php_ref.IsEmpty()) {
				assert(php_ref->IsExternal());
				zval *php_exception = reinterpret_cast<zval *>(v8::External::Cast(*php_ref)->Value());

				zend_class_entry *exception_ce = zend_exception_get_default(TSRMLS_C);
				if (Z_TYPE_P(php_exception) == IS_OBJECT && instanceof_function(Z_OBJCE_P(php_exception), exception_ce TSRMLS_CC)) {
					Z_ADDREF_P(php_exception);
					zend_exception_set_previous(return_value, php_exception TSRMLS_CC);
				}
			}
		}

	}

	PHPV8_EXPROP(_string, message, message_string);

	efree(message_string);
}
/* }}} */

void v8js_throw_script_exception(v8::Isolate *isolate, v8::TryCatch *try_catch TSRMLS_DC) /* {{{ */
{
	v8::String::Utf8Value exception(try_catch->Exception());
	const char *exception_string = ToCString(exception);
	zval *zexception = NULL;

	if (try_catch->Message().IsEmpty()) {
		zend_throw_exception(php_ce_v8js_script_exception, (char *) exception_string, 0 TSRMLS_CC);
	} else {
		MAKE_STD_ZVAL(zexception);
		v8js_create_script_exception(zexception, isolate, try_catch TSRMLS_CC);
		zend_throw_exception_object(zexception TSRMLS_CC);
	}
}
/* }}} */

#define V8JS_EXCEPTION_METHOD(property) \
	static PHP_METHOD(V8JsScriptException, get##property) \
	{ \
		zval *value; \
		\
		if (zend_parse_parameters_none() == FAILURE) { \
			return; \
		} \
		value = zend_read_property(php_ce_v8js_script_exception, getThis(), #property, sizeof(#property) - 1, 0 TSRMLS_CC); \
		*return_value = *value; \
		zval_copy_ctor(return_value); \
		INIT_PZVAL(return_value); \
	}

/* {{{ proto string V8JsEScriptxception::getJsFileName()
 */
V8JS_EXCEPTION_METHOD(JsFileName);
/* }}} */

/* {{{ proto string V8JsScriptException::getJsLineNumber()
 */
V8JS_EXCEPTION_METHOD(JsLineNumber);
/* }}} */

/* {{{ proto string V8JsScriptException::getJsStartColumn()
 */
V8JS_EXCEPTION_METHOD(JsStartColumn);
/* }}} */

/* {{{ proto string V8JsScriptException::getJsEndColumn()
 */
V8JS_EXCEPTION_METHOD(JsEndColumn);
/* }}} */

/* {{{ proto string V8JsScriptException::getJsSourceLine()
 */
V8JS_EXCEPTION_METHOD(JsSourceLine);
/* }}} */

/* {{{ proto string V8JsScriptException::getJsTrace()
 */
V8JS_EXCEPTION_METHOD(JsTrace);	
/* }}} */


ZEND_BEGIN_ARG_INFO(arginfo_v8jsscriptexception_no_args, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry v8js_script_exception_methods[] = { /* {{{ */
	PHP_ME(V8JsScriptException,	getJsFileName,		arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsScriptException,	getJsLineNumber,	arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsScriptException,	getJsStartColumn,	arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsScriptException,	getJsEndColumn,		arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsScriptException,	getJsSourceLine,	arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(V8JsScriptException,	getJsTrace,			arginfo_v8jsscriptexception_no_args,	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	{NULL, NULL, NULL}
};
/* }}} */

/* }}} V8JsScriptException */



/* {{{ Class: V8JsException */

static const zend_function_entry v8js_exception_methods[] = { /* {{{ */
	{NULL, NULL, NULL}
};
/* }}} */

/* }}} V8JsException */



/* {{{ Class: V8JsTimeLimitException */

static const zend_function_entry v8js_time_limit_exception_methods[] = { /* {{{ */
	{NULL, NULL, NULL}
};
/* }}} */

/* }}} V8JsTimeLimitException */



/* {{{ Class: V8JsMemoryLimitException */

static const zend_function_entry v8js_memory_limit_exception_methods[] = { /* {{{ */
	{NULL, NULL, NULL}
};
/* }}} */

/* }}} V8JsMemoryLimitException */



PHP_MINIT_FUNCTION(v8js_exceptions) /* {{{ */
{
	zend_class_entry ce;

	/* V8JsException Class */
	INIT_CLASS_ENTRY(ce, "V8JsException", v8js_exception_methods);
	php_ce_v8js_exception = zend_register_internal_class_ex(&ce, spl_ce_RuntimeException, NULL TSRMLS_CC);

	/* V8JsScriptException Class */
	INIT_CLASS_ENTRY(ce, "V8JsScriptException", v8js_script_exception_methods);
	php_ce_v8js_script_exception = zend_register_internal_class_ex(&ce, php_ce_v8js_exception, NULL TSRMLS_CC);
	php_ce_v8js_script_exception->ce_flags |= ZEND_ACC_FINAL;

	/* Add custom JS specific properties */
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsFileName"),		ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsLineNumber"),		ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsStartColumn"),	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsEndColumn"),		ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsSourceLine"),		ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(php_ce_v8js_script_exception, ZEND_STRL("JsTrace"),			ZEND_ACC_PROTECTED TSRMLS_CC);

	/* V8JsTimeLimitException Class */
	INIT_CLASS_ENTRY(ce, "V8JsTimeLimitException", v8js_time_limit_exception_methods);
	php_ce_v8js_time_limit_exception = zend_register_internal_class_ex(&ce, php_ce_v8js_exception, NULL TSRMLS_CC);
	php_ce_v8js_time_limit_exception->ce_flags |= ZEND_ACC_FINAL;

	/* V8JsMemoryLimitException Class */
	INIT_CLASS_ENTRY(ce, "V8JsMemoryLimitException", v8js_memory_limit_exception_methods);
	php_ce_v8js_memory_limit_exception = zend_register_internal_class_ex(&ce, php_ce_v8js_exception, NULL TSRMLS_CC);
	php_ce_v8js_memory_limit_exception->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
} /* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
