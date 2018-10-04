/** \file Log.cpp
 * Logging implementation
 *
 * \author Filip Smola
 */
#include <open-sea/Log.h>
#include <open-sea/config.h>

#include <fstream>
#include <iomanip>
#include <iostream>

// Boost file system
#include <boost/filesystem.hpp>

// Boost date time formatting
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/support/date_time.hpp>

// Boost shared pointer
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>

// Boost logging
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/utility/setup/console.hpp>
namespace logging   = ::boost::log;
namespace keywords  = ::boost::log::keywords;
namespace src       = ::boost::log::sources;
namespace triv      = ::boost::log::trivial;
namespace sinks     = ::boost::log::sinks;
namespace expr      = ::boost::log::expressions;
namespace attr      = ::boost::log::attributes;

namespace open_sea::log {

    /**
     * \brief Add a text sink to the file path in \c open_sea::log::file_path
     *
     * Initialize and add a text sink to the file in \c open_sea::log::file_path with auto flush enabled and the following
     * format: <tt><em>0xLineID</em>: (<em>TimeStamp</em>) [<em>Module</em>] <<em>Severity</em>> <em>Message</em></tt>
     * where \a TimeStamp is formatted using the datetime format in \c open_sea::log::datetime_format. Fails when the
     * parent directory of the file does not exist and cannot be created. Only logs messages with severity less than
     * \warning if \c OPEN_SEA_DEBUG is defined.
     *
     * \return \c true when successful, \c false otherwise
     */
    bool add_file_sink() {
        // Make sure the directory exits
        boost::filesystem::path path(file_path);
        if(!boost::filesystem::exists(path.parent_path())) {
            // Directory doesn't exist -> create it
            if(!boost::filesystem::create_directories(path.parent_path())){
                // Directory couldn't be created -> abort
                return false;
            }
        }

        // Prepare the sink
        typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink;
        boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();
        boost::shared_ptr<std::ofstream> stream = boost::make_shared<std::ofstream>(file_path);
        sink->locked_backend()->add_stream(stream);

        if (!open_sea::debug_log) {
            // Filter out log records that are not at least warnings
            sink->set_filter(
                    logging::trivial::severity >= logging::trivial::warning
            );
        }

        // Enable auto flush
        sink->locked_backend()->auto_flush(true);

        // Set the formatter
        sink->set_formatter(
                expr::stream
                        << "0x" << std::hex << std::setw(8) << std::setfill('0') << expr::attr<unsigned int>("LineID")
                        << ": (" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", datetime_format)
                        << ") [" << expr::attr<std::string>("Module")
                        << "] <" << expr::attr<severity_level>("Severity")
                        << "> " << expr::smessage
        );

        // Add the sink
        logging::core::get()->add_sink(sink);
        return true;
    }

    /**
     * \brief Add a console sink
     *
     * Initialize and add a console sink with the following format:
     * <tt><em>0xLineID</em>: (<em>TimeStamp</em>) <<em>Severity</em>> <em>Message</em></tt> where \a TimeStamp is
     * formatted using the datetime format in \c open_sea::log::datetime_format.
     */
    void add_console_sink(){
        // Add the sink
        typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink;
        boost::shared_ptr<text_sink> sink = logging::add_console_log();

        // Set the formatter
        sink->set_formatter(
                expr::stream
                        << "0x" << std::hex << std::setw(8) << std::setfill('0') << expr::attr<unsigned int>("LineID")
                        << ": (" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", datetime_format)
                        << ") [" << expr::attr<std::string>("Module")
                        << "] <" << expr::attr<severity_level>("Severity")
                        << "> " << expr::smessage
        );
    }

    /**
     * \brief Initialize logging with a single file sink
     *
     * Initialize logging with a single file sink added by \c open_sea::log::init_file_sink() and with common attributes
     * added. Also log a message that the logging has been initialized.
     */
    void init_logging() {
        // Add common logging attributes
        logging::add_common_attributes();

        // Initialize the file sink
        severity_logger lg = get_logger("Logging");
        if(!add_file_sink()) {
            // Sink initialization failed
            add_console_sink();
            log(lg, warning, "File sink initialization failed, using console log.");
        }

        // Note start of logging
        log(lg, info, "Logging initialized");
    }

    /**
     * \brief Clean up after logging
     *
     * Remove all sinks to prevent problems at process termination
     * (see <a href="http://www.boost.org/doc/libs/1_58_0/libs/log/doc/html/log/rationale/why_crash_on_term.html">Boost.Log FAQ</a>)
     */
    void clean_up() {
        // Note the clean up
        severity_logger lg = get_logger("Logging");
        log(lg, info, "Cleaning up after logging");

        // Remove all sinks
        logging::core::get()->remove_all_sinks();
    }

    /**
     * \brief Stream the severity level to an output stream
     * \param os Reference to the output stream
     * \param lvl Severity level to stream
     * \return Reference to the output stream
     */
    std::ostream &operator<<(std::ostream& os, severity_level lvl) {
        static const char* strings[] = {
                "trace",
                "debug",
                "info",
                "warning",
                "error",
                "fatal"
        };

        if (static_cast<std::size_t>(lvl) < sizeof(strings) / sizeof(*strings)) {
            os << strings[lvl];
        } else {
            os << static_cast<int>(lvl);
        }

        return os;
    }

    /**
     * \brief Log a message
     *
     * \param logger Logger to use when logging the message
     * \param lvl Severity level of the message
     * \param message Message itself
     */
    void log(severity_logger& logger, severity_level lvl, const std::string& message) {
        logging::record rec = logger.open_record(keywords::severity = lvl);

        if (rec) {
            logging::record_ostream stream(rec);
            stream << message;
            stream.flush();
            logger.push_record(boost::move(rec));
        }
    }

    /**
     * \brief Get a logger
     *
     * Get a logger for the provided module
     *
     * @param module Module name
     * @return Logger instance
     */
    severity_logger get_logger(const std::string& module) {
        severity_logger lg;
        lg.add_attribute("Module", attr::constant<std::string>(module));
        return lg;
    }

    /**
     * \brief Get a logger
     *
     * Get a logger with no module provided
     *
     * @return Logger instance
     */
    severity_logger get_logger() {
        return get_logger("No Module");
    }
}
