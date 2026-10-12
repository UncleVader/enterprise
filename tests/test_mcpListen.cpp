// =============================================================================
// The MCP listener's address.
//
// The default is this computer. 0.0.0.0 and any other network address are
// refused. Asking to expose the port does not open it: the server speaks plain
// HTTP, and a network bind would put the designer's session on the wire.
// =============================================================================

#include <gtest/gtest.h>

#include "backend/mcp/mcpServer.h"

namespace {

ibMcpSettings Settings(const wxString& address, bool expose)
{
	ibMcpSettings settings;
	settings.m_address = address;
	settings.m_exposeOnNetwork = expose;
	return settings;
}

} // namespace

TEST(McpListen, LoopbackAddressesAreAccepted)
{
	wxString refusal;
	for (const wxString& address : {
		wxString(wxT("127.0.0.1")),
		wxString(wxT("127.0.0.2")),
		wxString(wxT("localhost")),
		wxString(wxT("LOCALHOST")),
		wxString(wxT("::1")),
		wxString(wxT("[::1]")),
		wxString(wxT("  127.0.0.1  ")) }) {
		EXPECT_TRUE(Settings(address, false).ListenAllowed(refusal)) << address.ToStdString();
	}
}

TEST(McpListen, ANetworkAddressIsRefusedWithoutTheSwitch)
{
	wxString refusal;
	EXPECT_FALSE(Settings(wxT("0.0.0.0"), false).ListenAllowed(refusal));
	EXPECT_NE(wxNOT_FOUND, refusal.Find(wxT("only on 127.0.0.1")));

	refusal.clear();
	EXPECT_FALSE(Settings(wxT("192.168.0.1"), false).ListenAllowed(refusal));
	EXPECT_FALSE(Settings(wxT("::"), false).ListenAllowed(refusal));
	EXPECT_FALSE(Settings(wxEmptyString, false).ListenAllowed(refusal));
}

TEST(McpListen, TheSwitchDoesNotOpenThePortWithoutTls)
{
	wxString refusal;
	EXPECT_FALSE(Settings(wxT("0.0.0.0"), true).ListenAllowed(refusal));
	EXPECT_NE(wxNOT_FOUND, refusal.Find(wxT("no TLS")));
}

TEST(McpListen, ALookalikeHostIsRefused)
{
	wxString refusal;
	EXPECT_FALSE(Settings(wxT("localhost.example.com"), false).ListenAllowed(refusal));
	EXPECT_FALSE(Settings(wxT("127.0.0.1.example.com"), false).ListenAllowed(refusal));
	EXPECT_FALSE(Settings(wxT("127.0.0"), false).ListenAllowed(refusal));
	EXPECT_FALSE(Settings(wxT("::ffff:127.0.0.1"), false).ListenAllowed(refusal));
}
