#pragma once
#include <functional>
#include <ostream>
namespace Cordite {
	namespace Logging {
		enum LogType {
			LGT_NULL		= 0x0000,
			LGT_INSANE		= 0x0002,
			LGT_SILLY		= 0x0004,
			LGT_VERBOSE		= 0x0008,
			LGT_INFO		= 0x0010,
			LGT_NOTICE		= 0x0020,
			LGT_WARN		= 0x0040,
			LGT_MEGAWARN	= 0x0080,
			LGT_ERROR		= 0x0100,
			LGT_FATAL		= 0x1000,
			LGT_NO_USE		= 0xFFFF
		};
		enum LogOutput {
			LGO_NULL = 0x00,
			LGO_CERR = 0x01,
			LGO_NETWORK = 0x02,
			LGO_FILE = 0x04
		};
		class Logger {
		public:
			static void start();
			static void shutdown();
			static void log(LogType, const char * const);
			static void log(LogType, std::function<void(std::ostream &)>);
			static void output(const LogOutput, const LogType, const char * const = "");
			static void remOutput(const LogOutput, const char * const);
			static void setName(const char * const);
		};
		class ScopedLogger {
		public:
			ScopedLogger()
			{
				Logger::start();
			}
			~ScopedLogger()
			{
				Logger::shutdown();
			}
		};
	};
};