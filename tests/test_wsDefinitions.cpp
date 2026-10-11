// WSDefinitions is WSОпределения. Without it, New WSDefinitions is
// "Constructor not found".

#include <gtest/gtest.h>

#include <cpp-httplib/httplib.h>

#include <chrono>
#include <string>
#include <thread>

#include <wx/file.h>
#include <wx/filename.h>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/compiler/value.h"
#include "backend/system/value/valueArray.h"

namespace {

const char kWsdl[] =
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"<wsdl:definitions xmlns:wsdl=\"http://schemas.xmlsoap.org/wsdl/\""
	" xmlns:soap=\"http://schemas.xmlsoap.org/wsdl/soap/\""
	" targetNamespace=\"http://example/goods\">"
	"<wsdl:service name=\"GoodsReceiptData\">"
	"<wsdl:port name=\"GoodsReceiptDataSoap\" binding=\"tns:GoodsReceiptDataSoap\">"
	"<soap:address location=\"http://example/goods\"/>"
	"</wsdl:port>"
	"</wsdl:service>"
	"</wsdl:definitions>";

wxString Prop(ibValue& value, const wxChar* name)
{
	ibValue found;
	EXPECT_TRUE(value.GetPropVal(value.FindProp(name), found));
	return wxString(found.GetString());
}

void ExpectGoods(ibValue& made)
{
	ibValue services;
	ASSERT_TRUE(made.GetPropVal(made.FindProp(wxT("Services")), services));
	const ibValueArray* const list = dynamic_cast<const ibValueArray*>(services.GetRef());
	ASSERT_NE(list, nullptr);
	ASSERT_EQ(list->Count(), 1u);
	ibValue service = list->Values()[0];
	EXPECT_EQ(Prop(service, wxT("Name")), wxT("GoodsReceiptData"));
	EXPECT_EQ(Prop(service, wxT("NamespaceURI")), wxT("http://example/goods"));

	ibValue ports;
	ASSERT_TRUE(service.GetPropVal(service.FindProp(wxT("Ports")), ports));
	const ibValueArray* const portList = dynamic_cast<const ibValueArray*>(ports.GetRef());
	ASSERT_NE(portList, nullptr);
	ASSERT_EQ(portList->Count(), 1u);
	ibValue port = portList->Values()[0];
	EXPECT_EQ(Prop(port, wxT("Name")), wxT("GoodsReceiptDataSoap"));
	EXPECT_EQ(Prop(port, wxT("Location")), wxT("http://example/goods"));
}

class TempWsdl {
public:
	TempWsdl() : m_path(wxFileName::CreateTempFileName(wxT("wsdl")))
	{
		wxFile file(m_path, wxFile::write);
		file.Write(kWsdl, sizeof(kWsdl) - 1);
	}
	~TempWsdl() { wxRemoveFile(m_path); }
	const wxString& Path() const { return m_path; }
private:
	wxString m_path;
};

} // namespace

TEST(WSDefinitions, AFileIsReadIntoServices) {
	TempWsdl file;
	ibValue location(file.Path());
	ibValue* params = &location;
	ibValue made = ibValue::CreateObject(wxT("WSDefinitions"), &params, 1);
	ASSERT_TRUE(made.IsReference());
	EXPECT_EQ(Prop(made, wxT("Location")), file.Path());
	ExpectGoods(made);

	ibValue missing(wxT("no-such-wsdl.wsdl"));
	params = &missing;
	EXPECT_THROW(ibValue::CreateObject(wxT("WSDefinitions"), &params, 1), ibBackendException);

	ibValue proxy(wxT("proxy"));
	ibValue* refused[] = { &location, &location, &location, &proxy };
	try {
		ibValue::CreateObject(wxT("WSDefinitions"), refused, 4);
		FAIL() << "a proxy is not applied";
	}
	catch (const ibBackendException& err) {
		EXPECT_NE(err.GetErrorDescription().find(wxT("InternetProxy")), wxString::npos)
			<< err.GetErrorDescription().ToStdString();
	}
}

TEST(WSDefinitions, AnHttpLocationIsFetched) {
	httplib::Server server;
	server.Get("/goods.wsdl", [](const httplib::Request&, httplib::Response& res) {
		res.set_content(kWsdl, "text/xml");
	});
	server.Get("/secret.wsdl", [](const httplib::Request& req, httplib::Response& res) {
		if (req.get_header_value("Authorization").empty()) {
			res.status = 401;
			return;
		}
		res.set_content(kWsdl, "text/xml");
	});
	const int port = server.bind_to_any_port("127.0.0.1");
	ASSERT_GT(port, 0);
	std::thread worker([&server] { server.listen_after_bind(); });
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	struct Stop {
		httplib::Server& server;
		std::thread& worker;
		~Stop()
		{
			server.stop();
			if (worker.joinable())
				worker.join();
		}
	} stop{ server, worker };

	const wxString url = wxString::Format(wxT("http://127.0.0.1:%d/goods.wsdl"), port);
	ibValue location(url);
	ibValue* params = &location;
	ibValue made = ibValue::CreateObject(wxT("WSDefinitions"), &params, 1);
	ExpectGoods(made);

	const wxString secret = wxString::Format(wxT("http://127.0.0.1:%d/secret.wsdl"), port);
	ibValue secretLocation(secret);
	ibValue user(wxT("ada"));
	ibValue password(wxT("secret"));
	ibValue* authed[] = { &secretLocation, &user, &password };
	ibValue opened = ibValue::CreateObject(wxT("WSDefinitions"), authed, 3);
	ExpectGoods(opened);

	params = &secretLocation;
	try {
		ibValue::CreateObject(wxT("WSDefinitions"), &params, 1);
		FAIL() << "the secret document requires a user";
	}
	catch (const ibBackendException& err) {
		EXPECT_NE(err.GetErrorDescription().find(wxT("401")), wxString::npos)
			<< err.GetErrorDescription().ToStdString();
	}
}

TEST(WSDefinitions, TheConstructorCompiles) {
	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	try {
		ASSERT_TRUE(cc.Compile(
			wxT("Procedure Check()\n")
			wxT("    var definitions;\n")
			wxT("    definitions = New WSDefinitions(\"goods.wsdl\");\n")
			wxT("EndProcedure\n")));
	}
	catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}
