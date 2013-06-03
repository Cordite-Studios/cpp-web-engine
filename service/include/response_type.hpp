#pragma once

namespace Cordite {
	enum R_ResponseType {
		RRT_NULL = 0x0000,
		RRT_STRING = 0x001,
		RRT_BSON = 0x0002,
		RRT_MSGPACK = 0x0004,
		RRT_PROTOBUF = 0x0008,
		RRT_JSON = 0x0010
	};
};