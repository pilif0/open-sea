/*
 * Logging implementation
 */
#include <open-sea/Log.h>
#include <open-sea/config.h>

#include <fstream>
#include <iomanip>
#include <iostream>

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
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
namespace logging   = ::boost::log;
namespace keywords  = ::boost::log::keywords;
namespace src       = ::boost::log::sources;
namespace triv      = ::boost::log::trivial;
namespace sinks     = ::boost::log::sinks;
namespace expr      = ::boost::log::expressions;

namespace open_sea::log {

    /**
     * \brief Add a text sink to the file path in \c open_sea::log::file_path
     *
     * Initialize and add a text sink to the file in \c open_sea::log::file_path with auto flush enabled and the following
     * format: <tt><em>0xLineID</em>: (<em>TimeStamp</em>) <<em>Severity</em>> <em>Message</em></tt> where \a TimeStamp
     * is formatted using the datetime format in \c open_sea::log::datetime_format.
     */
    void init_file_sink() {
        // Prepare the sink
        typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink;
        boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();
        boost::shared_ptr<std::ofstream> stream = boost::make_shared<std::ofstream>(file_path);
        sink->locked_backend()->add_stream(stream);

#if !defined(OPEN_SEA_DEBUG)
        // Filter out log records that are not at least warnings
        sink->set_filter(
                logging::trivial::severity >= logging::trivial::warning
        );
#endif

        // Enable auto flush
        sink->locked_backend()->auto_flush(true);

        // Set the formatter
        sink->set_formatter(
                expr::stream
                        << "0x" << std::hex << std::setw(8) << std::setfill('0') << expr::attr<unsigned int>("LineID")
                        << ": (" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", datetime_format)
                        << ") <" << expr::attr<severity_level>("Severity")
                        << "> " << expr::smessage
        );

        // Add the sink
        logging::core::get()->add_sink(sink);
    }

    /**
     * \brief Initialize logging with a single file sink
     *
     * Initialize logging with a single file sink added by \c open_sea::log::init_file_sink() and with common attributes
     * added. Also log a message that the logging has been initialized.
     */
    void init_logging() {
        // Initialize the file sink
        init_file_sink();

        // Add common logging attributes
        logging::add_common_attributes();

        // Note start of logging
        severity_logger lg;
        log(lg, info, "Logging initialized");
    }

    /**
     * \brief Stream the severity level to an output stream
     * @param os Reference to the output stream
     * @param lvl Severity level to stream
     * @return Reference to the output stream
     */
    std::ostream &operator<<(std::ostream &os, severity_level lvl) {
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
     * @param logger Logger to use when logging the message
     * @param lvl Severity level of the message
     * @param message Message itself
     */
    void log(severity_logger logger, severity_level lvl, std::string message) {
        logging::record rec = logger.open_record(keywords::severity = lvl);

        if (rec) {
            logging::record_ostream stream(rec);
            stream << message;
            stream.flush();
            logger.push_record(boost::move(rec));
        }
    }
}
