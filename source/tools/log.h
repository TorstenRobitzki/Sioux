#ifndef SOURCE_TOOLS_LOG_H
#define SOURCE_TOOLS_LOG_H

#include <boost/shared_ptr.hpp>
#include <iosfwd>

/**
 * @file tools\log.h
 * Logging facility, that keeps a list of log lines, a list of outputs for that lines and a list  
 * that associates a log context with a log level. The list of log lines simply decouples the 
 * logging of a line from the actual writing of that line, so that the logging thread won't block,
 * if some of the output stream buffers blocks. The list of outputs is a list of std stream buffers.
 * If a log message is logged, that line is decorated and than written to every output buffer. If an
 * output buffer causes an error, that buffer will be removed from the output list. The last list stores 
 * a log_levels for log contexts. If the function add_message() is called with a log context, the actual
 * log level (which can be set with set_level()) is looked up and compared with the level given to the 
 * add_message() function. If that level is equal or higher to that stored to the context, the message 
 * will be logged to the output. For smaler applications that do not have a need for different contexts
 * there are overloads for the functions, that take a context as parameter, without this parameter. In
 * this cases, a default context is used.
 * 
 * All functions in this header are save to be called from different threads.
 */

/** 
 * @namespace log
 * logging facilities 
 */
namespace log {
	
/**
 * @brief Type to express the severity of the message to be logged
 * 
 * The log_level is used to indicate the severity level of a message that 
 * should be logged. The level are given with the highest severities first.
 */
enum log_level {
	/** fatal error occured, used when a crash is very likely. Log message with this level are not maskable */
	fatal,
	/** an error occured, the application can recover from this error */  
	error,
	/** there might be something wrong */
	warning,
	/** interesting thing to know. Like starting the application. info, is the default level */
	info,
	/** level, that can be used for tracing main program flow */
	main,
	/** level used to trace detailed program flow */
	detail,
	/** level used for debugging */
	debug,
	/** level for silly verbose absolutly performance harming logging */
	all
};

/** 
 * @brief prints the given log_level in a human readable manner onto the given stream
 * 
 * The output is the name of the enum. "fatal" for fatal, "detail" for detail an so on.
 */ 
std::ostream& operator<<(std::ostream& output, log_level level);

/**
 * @brief reads a log_level from the given input stream
 * 
 * Extracts a string from the input and then compares this string to the name
 * of the enumeration members. If that string compares equal to one and only one
 * of the members name, that enumeration member will return. If no member fits, or
 * more than one member fits, the fail bit of input will be set and level will not
 * be altered. 
 */
std::istream& operator>>(std::istream& input, log_level& level);

/**
 * @brief used to define a custom log context 
 * @headerfile log.h "tools/log.h" 
 *  
 * To establish a new logging context, simply derive from context and use 
 * an instance of the derived type as first part when concatenating a message
 * using std::ostream operators.
 * 
 * Example:
 * @code
 * 
 * struct test_context_t : log::context {} const test_context;
 * struct new_context_t  : log::context {} const new_context;
 *
 * void f(int i)
 * {
 *     // make sure, the following message will go to the output 
 *     log::set_level(test_context, log::main);
 *     LOG_MAIN(test_context << "entering f() i = " << i);
 * 
 *     // make sure, the next message won't not go to the output
 *     log::set_level(new_context, log::error);
 *     LOG_WARNING(new_context << "this function is pointless.");
 *
 *     // the loglevel for test_context is still log::main, so the next message will apear in the logs 
 *     LOG_MAIN(test_context << "but" << " still" << " running.");
 * 
 *     // a context doesn't have to be used. The next message will go to the output, 
 *     // because the default level for every context is info
 *     LOG_INFO("end of f()"); 
 * }  
 * @endcode
 */
struct context
{
	virtual ~context() {}
};

/**
 * @brief adds a log context to the given stream
 * 
 * The given log context is attached to the given stream. If more than one context 
 * is attached to the same stream, the later will be used to examine, whether a message
 * is actual logged or not.
 */
std::ostream& operator<<(std::ostream& output, const context& c);

/**
 * @brief adds a message to the logs
 * 
 * If the stored log level for the given context c is lower or equal to severity, message will
 * be written to the output.
 * 
 * @param c The context where the log_level will be looked devide whether message will be logged or not. 
 *          If no level was given for that context (set_level()), log::info will be assumed.
 * @param severity The log level of the message. This level will be compared to the level stored for the context c
 * @param message The message to be logged. The message will be decorated and than written to all outputs, if severity is
 *                higher than the log_level stored for c.
 */ 
void add_message(const context& c, log_level severity, const std::string& message);

/**
 * @brief adds a message to the logs, using the default log context
 * @sa add_message(const context&, log_level, const std::string&);
 */
void add_message(log_level severity, const std::string& message);

/**
 * @brief adds a message to the log, that was assembles with a string stream before
 * 
 * If a log context was bind to the string stream by using operator<<(std::ostream&, const context&) before, that
 * context will be extracted and the given log_level will compared to the level stored with this context. If no
 * context was used, severity will be compared to the default context and buffer.str() will be logged if severity
 * is higher or equal to the contexts log level.
 */
void add_message(std::ostringstream& buffer, log_level severity);

/**
 * @brief sets the log level for the given log context
 * 
 * Inside the library the log level will be stored for the given context. When ever a message will be logged, the
 * log level stored to the context will be compared to the log level given to the call to add_message(). If the given
 * level is equal or higher to the stored level, the message will be written to the output. If not, the message will
 * be discharged.
 */
void set_level(const context& c, log_level level);

/**
 * @brief sets the log level for the default log context
 * @sa set_level(const context&, log_level)
 */
void set_level(log_level level);

/**
 * @brief adds a stream buffer to the list of outputs
 * 
 * The library will store a shared pointer to the stream buffer until the output 
 * will cause an error or reports eof. The reference is droped and the buffer deleted
 * if no other reference outside the library exist.
 * *
 * @attention the thread that writes to the output will be different to the thread that calls this function.
 */
void add_output(const boost::shared_ptr<std::streambuf>& output);

/**
 * @brief adds a stream buffer to the list of outputs
 * 
 * The libray will add the output to the list of outputs. The caller is responsable to remove the output 
 * before the output gets destroyed. If the output causes an error of reports eof while writing to the 
 * output, the output will be removed.
 * *
 * @attention the thread that writes to the output will be different to the thread that calls this function.
 */
void add_output(std::streambuf& output);

/**
 * @brief adds a stream buffer to the list of outputs
 * 
 * Convenience overload that calls add_output(std::streambuf&) with the stream buffer of the given stream.
 * 
 * @attention the thread that writes to the output will be different to the thread that calls this function.
 */
void add_output(std::ostream& output);

/**
 * @brief removes the given output from the list of outputs
 * @pre the output was added before by call to the add_output() overload, that takes a shared_ptr to a streambuf
 * @post no further output will be writen to the output
 */
void remove_output(const boost::shared_ptr<std::streambuf>& output);

/**
 * @brief remove the given output from the list of outputs
 * @pre the output was added before by call to the add_output() overload, that takes ostream or a stream buffer by reference
 * @post no further output will be writen to the output
 */
void remove_output(std::streambuf& output);

/**
 * @brief remove the given output from the list of outputs
 * @pre the output was added before by call to the add_output() overload, that takes ostream or a stream buffer by reference
 * @post no further output will be writen to the output
 */
void remove_output(std::ostream& output);

} // namespace log

/**
 * @brief convenience macro to log a line
 *  
 * The macro constructs a stringstream, assembles a single string unsing expression and then handles
 * the line to add_message
 */  
#define LOG_MESSAGE(expression, level) \
	try { \
		std::ostringstream tmp_log_output_stream;\
		tmp_log_output_stream << expression;\
		log::add_message(tmp_log_output_stream, level);\
	} catch (...) {}
	

/**
 * @brief convenience marco to log a line with log::fatal severity
 */
#define LOG_FATAL(e) LOG_MESSAGE(e, log::fatal)

/**
 * @brief convenience marco to log a line with log::error severity
 */
#define LOG_ERROR(e) LOG_MESSAGE(e, log::error)

/**
 * @brief convenience marco to log a line with log::warning severity
 */
#define LOG_WARNING(e) LOG_MESSAGE(e, log::warning)

/**
 * @brief convenience marco to log a line with log::info severity
 */
#define LOG_INFO(e) LOG_MESSAGE(e, log::info)

/**
 * @brief convenience marco to log a line with log::main severity
 */
#define LOG_MAIN(e) LOG_MESSAGE(e, log::main)

/**
 * @brief convenience marco to log a line with log::detail severity
 */
#define LOG_DETAIL(e) LOG_MESSAGE(e, log::detail)

/**
 * @brief convenience marco to log a line with log::debug severity
 * 
 * @attention the macro will only be active if NDEBUG is not set
 * 
 * If NDEBUG is set the expression e is not evaluated.
 */
#ifndef NDEBUG
#	define LOG_DEBUG(e) LOG_MESSAGE(e, log::debug)
#else
#	define LOG_DEBUG(e)
#endif

/**
 * @brief convenience marco to log a line with log::fatal severity
 *
 * @attention the macro will only be active if NDEBUG is not set
 * 
 * If NDEBUG is set the expression e is not evaluated.
 */
#ifndef NDEBUG
#	define LOG_ALL(e) LOG_MESSAGE(e, log::all)
#else
#	define LOG_ALL(e)
#endif

#endif // include guard
