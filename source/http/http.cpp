// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "http/http.h"
#include "tools/asstring.h"
#include <ostream>

namespace http {

char const * reason_phrase(http_error_code ec) 
{
	switch (ec)
	{
	case http_continue:
		return "Continue";
	case http_switching_protocols:
		return "Switsching Protocols";	
	case http_ok:
		return "OK";
	case http_created:
		return "Created";
	case http_accepted:
		return "Accepted";
	case http_non_authoritative_information:
		return "Non-Authoritative Information";
	case http_no_content:
		return "No Content";
	case http_reset_content:
		return "Reset Content";
	case http_partial_content:
		return "Partial Content";
	case http_multiple_choices:
		return "Multiple Choices";
	case http_moved_permanently:
		return "Moved Permanently";
	case http_found:
		return "Found";
	case http_see_other:
		return "See Other";
	case http_not_modified:
		return "Not Modified";
	case http_use_proxy:
		return "Use Proxy";
	case http_temporary_redirect:
		return "Temporary Redirect";
	case http_bad_request:
		return "Bad Request";
	case http_unauthorized:
		return "Unauthorized";
	case http_payment_required:
		return "Payment Required";
	case http_forbidden:
		return "Forbidden";
	case http_not_found:
		return "Not Found";
	case http_method_not_allowed:
		return "Method Not Allowed";
	case http_not_acceptable:
		return "Not Acceptable";
	case http_proxy_authentication_required:
		return "Proxy Authentication Required";
	case http_request_timeout:
		return "Request Time-out";
	case http_conflict:
		return "Conflict";
	case http_gone:
		return "Gone";
	case http_length_required:
		return "Length Required";
	case http_precondition_failed:
		return "Precondition Failed";
	case http_request_entity_too_large:
		return "Request Entity Too Large";
	case http_request_uri_too_long:
		return "Request-URI Too Large";
	case http_unsupported_media_type:
		return "Unsupported Media Type";
	case http_request_range_not_satisfiable:
		return "Requested range not satisfiable";
	case http_expectation_failed:
		return "Expectation Failed";
	case http_internal_server_error:
		return "Internal Server Error";
	case http_not_implemented:
		return "Not Implemented";
	case http_bad_gateway:
		return "Bad Gateway";
	case http_service_unavailable:
		return "Service Unavailable";
	case http_gateway_timeout:
		return "Gateway Time-out";
	case http_http_version_not_supported:
		return "HTTP Version not supported";
	}
	
	return "unknown Error Code";
}

std::ostream& operator<<(std::ostream& out, http_error_code ec)
{
    return out << reason_phrase(ec);
}

std::string status_line(const std::string& version, http_error_code ec)
{
	return "HTTP/" + version + " " + tools::as_string( static_cast< int >( ec ) ) + " " + reason_phrase(ec) + "\r\n";
}

std::string status_line(const char* version, http_error_code ec)
{
	return status_line(std::string(version), ec);
}

std::ostream& operator<<(std::ostream& out, http_method_code code)
{
	switch (code)
	{
	case http_options:
		return out << "OPTIONS";
	case http_get:
		return out << "GET";
	case http_head:
		return out << "HEAD";
	case http_post:
		return out << "POST";
	case http_put:
		return out << "PUT";
	case http_delete:
		return out << "DELETE";
	case http_trace:
		return out << "TRACE";
	case http_connect:
		return out << "CONNECT";
	default:
		return out << "Unknown(" << static_cast<int>(code) << ")";
	}
}


bool entity_expected(http_error_code ec, http_method_code method)
{
	// accoding to 4.3 of rfc 2616
	if ( method == http_head )
		return false;
	
	if ( ec >= 100 && ec < 200 || ec == http_no_content || ec == http_not_modified )
		return false;
	
	return true;
}

} // namespace http

