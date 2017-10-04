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
#ifndef C4HTTP_H
#define C4HTTP_H

#include "platform/StdScheduler.h"
#include "network/C4NetIO.h"

#include <map>

const int C4HTTPQueryTimeout = 10; // (s)

typedef struct Curl_multi CURLM;
typedef struct Curl_easy CURL;

// mini HTTP client
class C4HTTPClient : public StdSchedulerProc
{
public:
	C4HTTPClient();
	~C4HTTPClient() override;
	bool Init();

private:

	CURLM *MultiHandle{nullptr};
	CURL *CurlHandle{nullptr};

#ifdef STDSCHEDULER_USE_EVENTS
	// event indicating network activity
	HANDLE Event{nullptr};
#endif
	std::map<SOCKET, int> sockets;

	// Address information
	StdCopyStrBuf URL, ServerName;
	C4NetIO::addr_t ServerAddr;

	StdCopyBuf RequestData;

	bool fBinary{false};
	bool fSuccess{false};

	// Response header data
	size_t iDownloadedSize{0}, iTotalSize{0};
	bool fCompressed;

	// Event queue to use for notify when something happens
	class C4InteractiveThread *pNotify{nullptr};

	// CURL callbacks
	static size_t SWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
	size_t WriteCallback(char *ptr, size_t realsize);
	static int SProgressCallback(void *clientp, int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow);
	int ProgressCallback(int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow);
	static int SSocketCallback(CURL *easy, SOCKET s, int what, void *userp, void *socketp);
	int SocketCallback(CURL *easy, SOCKET s, int what, void *socketp);

protected:
	StdCopyBuf ResultBin; // set if fBinary
	StdCopyStrBuf ResultString; // set if !fBinary
	StdCopyStrBuf Error;
	void SetError(const char *strnError) { Error = strnError; }

public:
	bool Query(const StdBuf &Data, bool fBinary);
	bool Query(const char *szData, bool fBinary) { return Query(StdBuf(szData, SLen(szData)), fBinary); }

	bool isBusy() const { return !!CurlHandle; }
	bool isConnected() const { return iDownloadedSize + iTotalSize != 0; }
	bool isSuccess() const { return fSuccess; }
	size_t getTotalSize() const { return iTotalSize; }
	size_t getDownloadedSize() const { return iDownloadedSize; }
	const StdBuf &getResultBin() const { assert(fBinary); return ResultBin; }
	const char *getResultString() const { assert(!fBinary); return ResultString.getData(); }
	const char *getURL() const { return URL.getData(); }
	const char *getServerName() const { return ServerName.getData(); }
	const C4NetIO::addr_t &getServerAddress() const { return ServerAddr; }
	virtual const char *GetError() const { return Error.getData(); }
	void ResetError() { Error.Clear(); }

	void Cancel(const char *szReason);
	void Clear();

	bool SetServer(const char *szServerAddress);

	void SetNotify(class C4InteractiveThread *pnNotify) { pNotify = pnNotify; }

	// StdScheduler interface
	bool Execute(int iMaxTime = -1, pollfd * readyfds = nullptr) override;
	C4TimeMilliseconds GetNextTick(C4TimeMilliseconds tNow) override;
#ifdef STDSCHEDULER_USE_EVENTS
	HANDLE GetEvent() override { return Event; }
#else
	void GetFDs(std::vector<struct pollfd> &) override;
#endif

};

#endif
