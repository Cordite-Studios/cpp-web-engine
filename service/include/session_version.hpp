#pragma once

namespace Cordite {
	enum R_SessionVersion {
		RSV_NULL = 0x0000,
		RSV_BSON = 0x0001,
		RSV_MSGPACK = 0x0002,
		RSV_PROTOBUF = 0x0004
	};
};