/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2017, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */
#include "C4Include.h"
#include "network/C4HTTP.h"
#include "network/C4InteractiveThread.h"
#include "C4Version.h"

#include <regex>

#define CURL_STRICTER
#include <curl/curl.h>

// *** C4HTTPClient

C4HTTPClient::C4HTTPClient()
{
}

C4HTTPClient::~C4HTTPClient()
{
	Cancel(nullptr);
	if (MultiHandle)
		curl_multi_cleanup(MultiHandle);
#ifdef STDSCHEDULER_USE_EVENTS
	if (Event != nullptr)
		WSACloseEvent(Event);
#endif
}

bool C4HTTPClient::Init()
{
	MultiHandle = curl_multi_init();
	if (!MultiHandle) return false;
#ifdef STDSCHEDULER_USE_EVENTS
	if ((Event = WSACreateEvent()) == WSA_INVALID_EVENT)
	{
		SetError("could not create socket event");
		curl_multi_cleanup(MultiHandle);
		return false;
	}
#endif
	curl_multi_setopt(MultiHandle, CURLMOPT_SOCKETFUNCTION, &C4HTTPClient::SSocketCallback);
	curl_multi_setopt(MultiHandle, CURLMOPT_SOCKETDATA, this);
	return true;
}

bool C4HTTPClient::Execute(int iMaxTime, pollfd *readyfd)
{
	int running;
#ifdef STDSCHEDULER_USE_EVENTS
	// On Windows, StdScheduler doesn't inform us about which fd triggered the
	// event, so we have to check manually.
	if (WaitForSingleObject(Event, 0) == WAIT_OBJECT_0)
	{
		for (const auto& kv : sockets)
		{
			auto socket = kv.first;
			WSANETWORKEVENTS NetworkEvents;
			if (WSAEnumNetworkEvents(socket, Event, &NetworkEvents) != SOCKET_ERROR)
			{
				int ev_bitmask = 0;
				if (NetworkEvents.lNetworkEvents & (FD_READ|FD_ACCEPT|FD_CLOSE)) ev_bitmask |= CURL_CSELECT_IN;
				if (NetworkEvents.lNetworkEvents & (FD_WRITE|FD_CONNECT))        ev_bitmask |= CURL_CSELECT_OUT;
				curl_multi_socket_action(MultiHandle, socket, ev_bitmask, &running);
			}
		}
	}
#else
	if (readyfd)
	{
		int ev_bitmask = 0;
		if (readyfd->revents & POLLIN)  ev_bitmask |= CURL_CSELECT_IN;
		if (readyfd->revents & POLLOUT) ev_bitmask |= CURL_CSELECT_OUT;
		if (readyfd->revents & POLLERR) ev_bitmask |= CURL_CSELECT_ERR;
		curl_multi_socket_action(MultiHandle, readyfd->fd, ev_bitmask, &running);
	}
#endif
	else
	{
		curl_multi_socket_action(MultiHandle, CURL_SOCKET_TIMEOUT, 0, &running);
	}

	CURLMsg *m;
	do {
		int msgq = 0;
		m = curl_multi_info_read(MultiHandle, &msgq);
		if(m && (m->msg == CURLMSG_DONE)) {
			CURL *e = m->easy_handle;
			assert(e == CurlHandle);
			CurlHandle = nullptr;
			curl_multi_remove_handle(MultiHandle, e);

			// Check for errors and notify listeners. Note that curl fills
			// the Error buffer automatically.
			fSuccess = m->data.result == CURLE_OK;
			if (!fSuccess && !*Error.getData())
				Error.Copy(curl_easy_strerror(m->data.result));
			char *ip;
			curl_easy_getinfo(e, CURLINFO_PRIMARY_IP, &ip);
			ServerAddr.SetHost(StdStrBuf(ip));
			if (pNotify)
				pNotify->PushEvent(Ev_HTTP_Response, this);

			curl_easy_cleanup(e);
		}
	} while(m);

	return true;
}

C4TimeMilliseconds C4HTTPClient::GetNextTick(C4TimeMilliseconds tNow)
{
	long timeout;
	curl_multi_timeout(MultiHandle, &timeout);
	if (timeout < 0)
		timeout = 1000;
	return tNow + timeout;
}

#ifndef STDSCHEDULER_USE_EVENTS
void C4HTTPClient::GetFDs(std::vector<pollfd> &pollfds)
{
	for (const auto& kv : sockets)
	{
		pollfd pfd;
		pfd.fd = kv.first;
		pfd.revents = 0;
		switch (kv.second)
		{
		case CURL_POLL_IN:
			pfd.events = POLLIN; break;
		case CURL_POLL_OUT:
			pfd.events = POLLOUT; break;
		case CURL_POLL_INOUT:
			pfd.events = POLLIN | POLLOUT; break;
		default:
			pfd.events = 0;
		}
		pollfds.push_back(pfd);
	}
}
#endif

bool C4HTTPClient::Query(const StdBuf &Data, bool fBinary)
{
	if (URL.isNull()) return false;
	// Cancel previous request
	if (CurlHandle)
		Cancel("Cancelled");
	// No result known yet
	ResultString.Clear();
	// store mode
	this->fBinary = fBinary;
	// Create request
	CURL *curl = curl_easy_init();
	if (!curl) return false;
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_URL, URL.getData());
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_USERAGENT, C4ENGINENAME "/" C4VERSION );
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, C4HTTPQueryTimeout);
	curl_slist *headers = nullptr;
	headers = curl_slist_append(headers, "Accept-Charset: utf-8");
	headers = curl_slist_append(headers, FormatString("Accept-Language: %s", Config.General.LanguageEx).getData());

	if (Data.getSize())
	{
		RequestData = Data;
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, RequestData.getData());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, RequestData.getSize());
		// Disable the Expect: 100-Continue header which curl automatically
		// adds for POST requests.
		headers = curl_slist_append(headers, "Expect:");
		headers = curl_slist_append(headers, "Content-Type: text/plain; charset=utf-8");
	}

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &C4HTTPClient::SWriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &C4HTTPClient::SProgressCallback);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
	Error.Clear();
	Error.SetLength(CURL_ERROR_SIZE);
	*Error.getMData() = '\0';
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, Error.getMData());

	curl_multi_add_handle(MultiHandle, curl);
	CurlHandle = curl;
	iDownloadedSize = iTotalSize = 0;

	int running;
	curl_multi_socket_action(MultiHandle, CURL_SOCKET_TIMEOUT, 0, &running);

	return true;
}

size_t C4HTTPClient::SWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	C4HTTPClient *client = reinterpret_cast<C4HTTPClient*>(userdata);
	return client->WriteCallback(ptr, size * nmemb);
}

size_t C4HTTPClient::WriteCallback(char *ptr, size_t realsize)
{
	if (fBinary)
		ResultBin.Append(ptr, realsize);
	else
		ResultString.Append(ptr, realsize);
	return realsize;
}

int C4HTTPClient::SProgressCallback(void *clientp, int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow)
{
	C4HTTPClient *client = reinterpret_cast<C4HTTPClient*>(clientp);
	return client->ProgressCallback(dltotal, dlnow, ultotal, ulnow);
}

int C4HTTPClient::ProgressCallback(int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow)
{
	iDownloadedSize = dlnow;
	iTotalSize = dltotal;
	return 0;
}

int C4HTTPClient::SSocketCallback(CURL *easy, curl_socket_t s, int what, void *userp, void *socketp)
{
	C4HTTPClient *client = reinterpret_cast<C4HTTPClient*>(userp);
	return client->SocketCallback(easy, s, what, socketp);
}

int C4HTTPClient::SocketCallback(CURL *easy, curl_socket_t s, int what, void *socketp)
{
#ifdef STDSCHEDULER_USE_EVENTS
	long NetworkEvents;
	switch (what)
	{
	case CURL_POLL_IN:
		NetworkEvents = FD_READ | FD_ACCEPT | FD_CLOSE; break;
	case CURL_POLL_OUT:
		NetworkEvents = FD_WRITE | FD_CONNECT; break;
	case CURL_POLL_INOUT:
		NetworkEvents = FD_READ | FD_ACCEPT | FD_CLOSE | FD_WRITE | FD_CONNECT; break;
	default:
		NetworkEvents = 0;
	}
	if (WSAEventSelect(s, Event, NetworkEvents) == SOCKET_ERROR)
	{
		SetError("could not set event");
		return 1;
	}
#endif
	if (what == CURL_POLL_REMOVE)
		sockets.erase(s);
	else
		sockets[s] = what;
	return 0;
}

void C4HTTPClient::Cancel(const char *szReason)
{
	if (CurlHandle)
	{
		curl_multi_remove_handle(MultiHandle, CurlHandle);
		curl_easy_cleanup(CurlHandle);
		CurlHandle = nullptr;
	}
	fBinary = false;
	iDownloadedSize = iTotalSize = 0;
	Error = szReason;
}

void C4HTTPClient::Clear()
{
	Cancel(nullptr);
	ServerAddr.Clear();
	ResultBin.Clear();
	ResultString.Clear();
}

bool C4HTTPClient::SetServer(const char *szServerAddress)
{
	static std::regex HostnameRegex(R"(^(:?[a-z]+:\/\/)?([^/:]+).*)", std::regex::icase);
	std::cmatch match;
	if (std::regex_match(szServerAddress, match, HostnameRegex))
	{
		// CURL validates URLs only on connect.
		URL.Copy(szServerAddress);
		ServerName.Copy(match[2].str().c_str());
		return true;
	}
	// The HostnameRegex above is pretty stupid, so we will reject only very
	// malformed URLs immediately.
	SetError("Malformed URL");
	return false;
}
