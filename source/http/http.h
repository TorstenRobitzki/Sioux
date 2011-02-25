// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_HTTP_HTTP_H_
#define SOURCE_HTTP_HTTP_H_

#include <string>
#include <iosfwd>

/** @namespace http */
namespace http {

/**
 * @brief http 1.1 status codes 
 *
 * see rfc 2616 for more details
 */
enum http_error_code {
	http_continue						= 100,
	http_switching_protocols			= 101,
	http_ok								= 200,
	http_created						= 201,
	http_accepted						= 202,
	http_non_authoritative_information	= 203,
	http_no_content						= 204,
	http_reset_content					= 205,
	http_partial_content				= 206,
	http_multiple_choices				= 300,
	http_moved_permanently				= 301,
	http_found							= 302,
	http_see_other						= 303,
	http_not_modified					= 304,
	http_use_proxy						= 305,
	http_temporary_redirect				= 307,
	http_bad_request					= 400,
	http_unauthorized					= 401,
	http_payment_required				= 402,
	http_forbidden						= 403,
	http_not_found						= 404,
	http_method_not_allowed				= 405,
	http_not_acceptable					= 406,
	http_proxy_authentication_required	= 407,
	http_request_timeout				= 408,
	http_conflict						= 409,
	http_gone							= 410,
	http_length_required				= 411,
	http_precondition_failed			= 412,
	http_request_entity_too_large		= 413,
	http_request_uri_too_long			= 414,
	http_unsupported_media_type			= 415,
	http_request_range_not_satisfiable	= 416,
	http_expectation_failed				= 417,
	http_internal_server_error			= 500,
	http_not_implemented				= 501,
	http_bad_gateway					= 502,
	http_service_unavailable			= 503,
	http_gateway_timeout				= 504,
	http_http_version_not_supported		= 505
};

/**
 * @brief enum that can store all http methods
 */
enum http_method_code {
	http_options,
	http_get,
	http_head,
	http_post,
	http_put,
	http_delete,
	http_trace,
	http_connect
};

/**
 * @brief prints the http method text onto the given stream
 */
std::ostream& operator<<(std::ostream&, http_method_code);

char const * reason_phrase(http_error_code);

std::ostream& operator<<(std::ostream&, http_error_code);

// Status line including \r\n
std::string status_line(const std::string& version, http_error_code);
std::string status_line(const char* version, http_error_code);

bool entity_expected(http_error_code, http_method_code);

} // namespace http

#endif // include guard
