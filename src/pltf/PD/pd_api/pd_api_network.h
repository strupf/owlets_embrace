//
//  pd_api_http.h
//  Playdate Simulator
//
//  Created by Dave Hayden on 7/18/24.
//  Copyright © 2024 Panic, Inc. All rights reserved.
//

#ifndef pd_api_http_h
#define pd_api_http_h

#include <stdio.h>
#include <stdbool.h>

#include "pd_api_sys.h"

#if TARGET_EXTENSION

typedef struct HTTPConnection HTTPConnection;
typedef struct TCPConnection TCPConnection;

typedef enum
{
	NET_OK = 0,
	NET_NO_DEVICE = -1,
	NET_BUSY = -2,
	NET_WRITE_ERROR = -3,
	NET_WRITE_BUSY = -4,
	NET_WRITE_TIMEOUT = -5,
	NET_READ_ERROR = -6,
	NET_READ_BUSY = -7,
	NET_READ_TIMEOUT = -8,
	NET_READ_OVERFLOW = -9,
	NET_FRAME_ERROR = -10,
	NET_BAD_RESPONSE = -11,
	NET_ERROR_RESPONSE = -12,
	NET_RESET_TIMEOUT = -13,
	NET_BUFFER_TOO_SMALL = -14,
	NET_UNEXPECTED_RESPONSE = -15,
	NET_NOT_CONNECTED_TO_AP = -16,
	NET_NOT_IMPLEMENTED = -17,
	NET_CONNECTION_CLOSED = -18,
} PDNetErr;

typedef enum {
	kWifiNotConnected = 0,  //!< Not connected to an AP
	kWifiConnected,     //!< Device is connected to an AP
	kWifiNotAvailable,  //!< A connection has been attempted and no configured AP was available
} WifiStatus;

#endif

typedef void AccessRequestCallback(bool allowed, void* userdata);

typedef void HTTPConnectionCallback(HTTPConnection* connection);
typedef void HTTPHeaderCallback(HTTPConnection* conn, const char* key, const char* value);

struct playdate_http
{
	enum accessReply (*requestAccess)(const char* server, int port, bool usessl, const char* purpose, AccessRequestCallback* requestCallback, void* userdata);
	
	HTTPConnection* (*newConnection)(const char* server, int port, bool usessl);
	HTTPConnection* (*retain)(HTTPConnection* http);
	void (*release)(HTTPConnection* http);

	void (*setConnectTimeout)(HTTPConnection* connection, int ms);
	void (*setKeepAlive)(HTTPConnection* connection, bool keepalive);
	void (*setByteRange)(HTTPConnection* connection, int start, int end);
	void (*setUserdata)(HTTPConnection* connection, void* userdata);
	void* (*getUserdata)(HTTPConnection* connection);

	PDNetErr (*get)(HTTPConnection* conn, const char* path, const char* headers, size_t headerlen);
	PDNetErr (*post)(HTTPConnection* conn, const char* path, const char* headers, size_t headerlen, const char* body, size_t bodylen);
	PDNetErr (*query)(HTTPConnection* conn, const char* method, const char* path, const char* headers, size_t headerlen, const char* body, size_t bodylen);
	PDNetErr (*getError)(HTTPConnection* connection);
	void (*getProgress)(HTTPConnection* conn, int* read, int* total);
	int (*getResponseStatus)(HTTPConnection* connection);
	size_t (*getBytesAvailable)(HTTPConnection* conn);
	void (*setReadTimeout)(HTTPConnection* conn, int ms);
	void (*setReadBufferSize)(HTTPConnection* conn, int bytes);
	int (*read)(HTTPConnection* conn, void* buf, unsigned int buflen);
	void (*close)(HTTPConnection* connection);

	void (*setHeaderReceivedCallback)(HTTPConnection* connection, HTTPHeaderCallback* headercb);
	void (*setHeadersReadCallback)(HTTPConnection* connection, HTTPConnectionCallback* callback);
	void (*setResponseCallback)(HTTPConnection* connection, HTTPConnectionCallback* callback);
	void (*setRequestCompleteCallback)(HTTPConnection* connection, HTTPConnectionCallback* callback);
	void (*setConnectionClosedCallback)(HTTPConnection* connection, HTTPConnectionCallback* callback);
};

typedef void TCPConnectionCallback(TCPConnection* connection, PDNetErr err);
typedef void TCPOpenCallback(TCPConnection* conn, PDNetErr err, void* ud);

struct playdate_tcp
{
	enum accessReply (*requestAccess)(const char* server, int port, bool usessl, const char* purpose, AccessRequestCallback* requestCallback, void* userdata);
	TCPConnection* (*newConnection)(const char* server, int port, bool usessl);
	TCPConnection* (*retain)(TCPConnection* http);
	void (*release)(TCPConnection* http);
	PDNetErr (*getError)(TCPConnection* connection);

	void (*setConnectTimeout)(TCPConnection* connection, int ms);
	void (*setUserdata)(TCPConnection* connection, void* userdata);
	void* (*getUserdata)(TCPConnection* connection);

	PDNetErr (*open)(TCPConnection* conn, TCPOpenCallback cb, void* ud);
	PDNetErr (*close)(TCPConnection* conn);
	
	void (*setConnectionClosedCallback)(TCPConnection* conn, TCPConnectionCallback* callback);

	void (*setReadTimeout)(TCPConnection* conn, int ms);
	void (*setReadBufferSize)(TCPConnection* conn, int bytes);
	size_t (*getBytesAvailable)(TCPConnection* conn);

	int (*read)(TCPConnection* conn, void *buffer, size_t length); // returns # of bytes read, or PDNetErr on error
	int (*write)(TCPConnection* conn, const void *buffer, size_t length); // returns # of bytes sent, or PDNetErr on error
};

struct playdate_network
{
	const struct playdate_http* http;
	const struct playdate_tcp* tcp;
	
	WifiStatus (*getStatus)(void);
	void (*setEnabled)(bool flag, void (*callback)(PDNetErr err));

	uintptr_t reserved[3];
};

#endif /* pd_api_http_h */
