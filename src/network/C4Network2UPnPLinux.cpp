/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2012-2013, The OpenClonk Team and contributors
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
/* Linux implementation of a UPnP port mapper (using libupnp) */

#include <C4Include.h>
#include <C4Application.h>
#include <C4Version.h>

#include <netdb.h>
#include <upnp.h>
#include <upnptools.h>

#include <C4Network2UPnP.h> // must come after upnp.h

namespace
{
	// This attempts to return the local IP address which is used for
	// internet connections. It does so by associating a UDP socket to talk to
	// 8.8.8.8 (A Google nameserver) and then reading its local address. There
	// might be cleverer ways to do this, such as reading the routing table.
	std::string GetOutgoingAddress()
	{
		struct socket_wrapper
		{
			const int sock;
			operator int() const { return sock; }

			socket_wrapper(int domain, int type, int protocol):
				sock(socket(domain, type | SOCK_CLOEXEC, protocol))
			{
				if(sock == -1)
					throw std::runtime_error(std::string("Failed to create a socket: ") + strerror(errno));
			}

			~socket_wrapper()
			{
				if(close(sock) != 0)
					DebugLogF("Failed to close socket: %s\n", strerror(errno));
			}
		};

		const char* const REMOTE_ADDRESS = "8.8.8.8";
		
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(53); // DNS port
		if(inet_pton(AF_INET, REMOTE_ADDRESS, &addr.sin_addr) != 1)
			throw std::runtime_error(std::string("Failed to convert address text to binary: ") + strerror(errno));

		socket_wrapper sock(AF_INET, SOCK_DGRAM, 0);
		if(connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0)
			throw std::runtime_error(std::string("Failed to set target address on UDP socket: ") + strerror(errno));

		struct sockaddr_in local_addr;
		socklen_t local_addr_len = sizeof(local_addr);
		if(getsockname(sock, reinterpret_cast<struct sockaddr*>(&local_addr), &local_addr_len) != 0 || local_addr_len > sizeof(local_addr))
			throw std::runtime_error(std::string("Failed to query peer name of UDP socket: ") + strerror(errno));

		char text_address[INET_ADDRSTRLEN];
		if(inet_ntop(AF_INET, &local_addr.sin_addr, text_address, INET_ADDRSTRLEN) == NULL)
			throw std::runtime_error(std::string("Failed to convert binary address to text: ") + strerror(errno));

		//DebugLogF("Outgoing address: %s", text_address);
		return text_address;
	}
}

class C4Network2UPnPP: public C4InteractiveThread::Callback
{
public:
	C4Network2UPnPP();
	virtual ~C4Network2UPnPP();

	void AddMapping(C4Network2IOProtocol protocol, uint16_t intport, uint16_t extport);
	void ClearMappings();

private:
	struct IGD {
		//std::string DeviceID;
		std::string Location;
		std::string ServiceType;
	};

	struct PortMapping {
		std::string external_hostname;
		uint16_t external_port;

		std::string internal_hostname;
		uint16_t internal_port;

		std::string protocol;
	};

	struct ActionData {
		ActionData(const IGD& igd_, const PortMapping& mapping_):
			igd(igd_), mapping(mapping_) {}

		IGD igd;
		PortMapping mapping;
	};

	// Main thread notification:
	struct Notify {
		virtual ~Notify() {}
	};
	
	struct NotifySearchResult: Notify {
		NotifySearchResult(const std::string& device_id, const IGD& igd_):
			DeviceId(device_id), igd(igd_) {}

		std::string DeviceId;
		IGD igd;
	};
	
	struct NotifyActionComplete: Notify {
		NotifyActionComplete(const ActionData& data, const std::string& action, int err_code):
			igd(data.igd), Mapping(data.mapping), Action(action), ErrCode(err_code) {}

		// Mapping than was added or removed
		IGD igd;
		PortMapping Mapping;

		std::string Action;
		int ErrCode;
	};

	virtual void OnThreadEvent(C4InteractiveEventType eEvent, void *pEventData);

	static int Callback_Static(Upnp_EventType EventType, void* Event, void* Cookie);
	void OnSearchResult(const std::string& DeviceID, const IGD& igd);
	void OnActionComplete(const IGD& igd, const PortMapping& mapping, const std::string& Action, int ErrCode);

	void AddPortMapping(const IGD& igd, const PortMapping& mapping);
	void RemovePortMapping(const IGD& igd, const PortMapping& mapping);

	std::string outgoing_address;
	std::map<std::string, IGD> igds;
	std::vector<PortMapping> added_mappings;
	UpnpClient_Handle upnp_handle;
};

C4Network2UPnPP::C4Network2UPnPP()
{
	// Query outgoing network address only once at the beginning. We talk to
	// the IGD (if any) via this address.
	try
	{
		outgoing_address = GetOutgoingAddress();

		int res = UpnpInit(outgoing_address.c_str(), 0);
		if(res != UPNP_E_SUCCESS)
			throw std::runtime_error(std::string("Failed to initialize UPnP: ") + UpnpGetErrorMessage(res));

		C4InteractiveThread &Thread = Application.InteractiveThread;
		Thread.SetCallback(Ev_UPNP_Response, this);

		res = UpnpRegisterClient(C4Network2UPnPP::Callback_Static, this, &upnp_handle);
		if(res != UPNP_E_SUCCESS)
			throw std::runtime_error(std::string("Failed to register UPnP client: ") + UpnpGetErrorMessage(res));

		res = UpnpSearchAsync(upnp_handle, 5, "urn:schemas-upnp-org:device:InternetGatewayDevice:1", this);
		if(res != UPNP_E_SUCCESS)
			throw std::runtime_error(std::string("Failed to search for IGDs: ") + UpnpGetErrorMessage(res));
	}
	catch(const std::runtime_error& error)
	{
		LogF("Failed to initialize UPnP: %s", error.what());
		outgoing_address.clear();
	}
}

C4Network2UPnPP::~C4Network2UPnPP()
{
	ClearMappings();
	UpnpUnRegisterClient(upnp_handle);

	C4InteractiveThread &Thread = Application.InteractiveThread;
	Thread.ClearCallback(Ev_UPNP_Response, this);

	UpnpFinish();
}

void C4Network2UPnPP::AddMapping(C4Network2IOProtocol protocol, uint16_t intport, uint16_t extport)
{
	PortMapping mapping;
	mapping.external_hostname = "";
	mapping.external_port = extport;
	mapping.internal_hostname = outgoing_address;
	mapping.internal_port = intport;
	mapping.protocol = (protocol == P_TCP ? "TCP" : "UDP");

	added_mappings.push_back(mapping);

	for(std::map<std::string, IGD>::const_iterator iter = igds.begin(); iter != igds.end(); ++iter)
		AddPortMapping(iter->second, mapping);
}

void C4Network2UPnPP::ClearMappings()
{
	for(std::map<std::string, IGD>::const_iterator igd_iter = igds.begin(); igd_iter != igds.end(); ++igd_iter)
		for(std::vector<PortMapping>::const_iterator mapping_iter = added_mappings.begin(); mapping_iter != added_mappings.end(); ++mapping_iter)
			RemovePortMapping(igd_iter->second, *mapping_iter);

	added_mappings.clear();
}

// This function is called asynchronously from a libupnp thread.
// It is not allowed to call back into the library so we queue a function
// to be called by the main thread. The event data is not guaranteed to
// stay alive past this function call so we need to inspect the event type
// and preserve the event data we want to process later.
int C4Network2UPnPP::Callback_Static(Upnp_EventType EventType, void* Event, void* Cookie)
{
	//C4Network2UPnPP* upnp = static_cast<C4Network2UPnPP*>(Cookie);

	switch(EventType)
	{
	case UPNP_DISCOVERY_SEARCH_RESULT:
		{
			Upnp_Discovery* discovery = static_cast<Upnp_Discovery*>(Event);

			IGD igd;
			igd.Location = discovery->Location;
			igd.ServiceType = discovery->ServiceType;

			Application.InteractiveThread.PushEvent(Ev_UPNP_Response, new NotifySearchResult(discovery->DeviceId, igd));
		}
		break;
	case UPNP_CONTROL_ACTION_COMPLETE:
		{
			std::unique_ptr<ActionData> data(static_cast<ActionData*>(Cookie));
			Upnp_Action_Complete* complete = static_cast<Upnp_Action_Complete*>(Event);
			std::string action = ixmlNode_getNodeName(ixmlNode_getFirstChild(&complete->ActionRequest->n));
			Application.InteractiveThread.PushEvent(Ev_UPNP_Response, new NotifyActionComplete(*data, action, complete->ErrCode));
		}
		break;
	default:
		Application.InteractiveThread.ThreadLogDebug("Unhandled UPNP event: %d", static_cast<int>(EventType));
		break;
	}

	return 0;
}

void C4Network2UPnPP::OnThreadEvent(C4InteractiveEventType eEvent, void *pEventData)
{
	std::unique_ptr<Notify> notify(static_cast<Notify*>(pEventData));

	// TODO: Should call a virtual method instead of dynamic_casting
	NotifySearchResult* notify_search_result = dynamic_cast<NotifySearchResult*>(notify.get());
	NotifyActionComplete* notify_action_complete = dynamic_cast<NotifyActionComplete*>(notify.get());

	if(notify_search_result)
		OnSearchResult(notify_search_result->DeviceId, notify_search_result->igd);
	if(notify_action_complete)
		OnActionComplete(notify_action_complete->igd, notify_action_complete->Mapping, notify_action_complete->Action, notify_action_complete->ErrCode);
}

void C4Network2UPnPP::OnSearchResult(const std::string& DeviceID, const IGD& igd)
{
	// Make sure we don't find the same device twice
	std::map<std::string, IGD>::const_iterator iter = igds.find(DeviceID);
	if(iter != igds.end()) return;

	// Add device
	igds[DeviceID] = igd;

	// Add all port mappings with this device
	for(std::vector<PortMapping>::const_iterator mapping_iter = added_mappings.begin(); mapping_iter != added_mappings.end(); ++mapping_iter)
		AddPortMapping(igd, *mapping_iter);
}

void C4Network2UPnPP::OnActionComplete(const IGD& igd, const PortMapping& mapping, const std::string& Action, int ErrCode)
{
	// If adding failed with an error of 718 this means that this external port
	// already exists in the port mapping table. Probably this was from some
	// previous OpenClonk game. We remove it, and then, after it has been
	// successfully removed, we try to add it again.
	if(Action == "u:AddPortMapping" && ErrCode == 718)
		RemovePortMapping(igd, mapping);
	else if(Action == "u:DeletePortMapping" && ErrCode == 0)
		AddPortMapping(igd, mapping);
	else if(ErrCode != 0)
		LogF("UPnP operation %s failed: %s", Action.c_str(), UpnpGetErrorMessage(ErrCode));
}

void C4Network2UPnPP::AddPortMapping(const IGD& igd, const PortMapping& mapping)
{
	if(igds.empty()) return; // Catches the case that UPnP initialization failed

	StdStrBuf external_port_buf, internal_port_buf;
	external_port_buf.Format("%d", static_cast<int>(mapping.external_port));
	internal_port_buf.Format("%d", static_cast<int>(mapping.internal_port));

	IXML_Document* action = UpnpMakeAction("AddPortMapping",
		igd.ServiceType.c_str(), 8,
		"NewRemoteHost", mapping.external_hostname.c_str(),
		"NewExternalPort", external_port_buf.getData(),
		"NewProtocol", mapping.protocol.c_str(),
		"NewInternalPort", internal_port_buf.getData(),
		"NewInternalClient", mapping.internal_hostname.c_str(),
		"NewEnabled", "1",
		"NewPortMappingDescription", C4ENGINECAPTION,
		"NewLeaseDuration", "0");

	UpnpSendActionAsync(upnp_handle, igd.Location.c_str(), igd.ServiceType.c_str(), NULL, action, Callback_Static, new ActionData(igd, mapping));
	ixmlDocument_free(action);
}

void C4Network2UPnPP::RemovePortMapping(const IGD& igd, const PortMapping& mapping)
{
	if(igds.empty()) return; // Catches the case that UPnP initialization failed

	StdStrBuf external_port_buf;
	external_port_buf.Format("%d", static_cast<int>(mapping.external_port));

	IXML_Document* action = UpnpMakeAction("DeletePortMapping",
		igd.ServiceType.c_str(), 3,
		"NewRemoteHost", mapping.external_hostname.c_str(),
		"NewExternalPort", external_port_buf.getData(),
		"NewProtocol", mapping.protocol.c_str());

	UpnpSendActionAsync(upnp_handle, igd.Location.c_str(), igd.ServiceType.c_str(), NULL, action, Callback_Static, new ActionData(igd, mapping));
	ixmlDocument_free(action);
}

C4Network2UPnP::C4Network2UPnP():
	p(new C4Network2UPnPP)
{
}

C4Network2UPnP::~C4Network2UPnP()
{
	delete p;
}

void C4Network2UPnP::AddMapping(C4Network2IOProtocol protocol, uint16_t intport, uint16_t extport)
{
	p->AddMapping(protocol, intport, extport);
}

void C4Network2UPnP::ClearMappings()
{
	p->ClearMappings();
}

