// InternetMail is ИнтернетПочта. Without it, New InternetMail is "Constructor not found".
// Logon speaks plain SMTP. The test stands in for the server.

#include <gtest/gtest.h>

#include <atomic>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <wx/init.h>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/compiler/value.h"
#include "backend/system/value/valueInternetMailSession.h"

namespace {

void Insert(ibValue& structure, const wxString& name, const ibValue& value)
{
	ibValue key(name);
	ibValue held(value);
	ibValue* args[] = { &key, &held };
	ibValue ignored;
	ASSERT_TRUE(structure.CallAsFunc(structure.FindMethod(wxT("Insert")), ignored, args, 2));
}

void Add(ibValue& array, const ibValue& value)
{
	ibValue held(value);
	ibValue* args[] = { &held };
	ibValue ignored;
	ASSERT_TRUE(array.CallAsFunc(array.FindMethod(wxT("Add")), ignored, args, 1));
}

ibValue Prop(ibValue& object, const wxString& name)
{
	ibValue value;
	EXPECT_TRUE(object.GetPropVal(object.FindProp(name), value));
	return value;
}

struct FakeSmtp {
	std::atomic<int> port{0};
	std::string text;
	std::mutex gate;
	std::thread worker;
	int listenFd = -1;

	void Start()
	{
		listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
		ASSERT_GE(listenFd, 0);
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		ASSERT_EQ(::bind(listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)), 0);
		ASSERT_EQ(::listen(listenFd, 1), 0);
		socklen_t len = sizeof(addr);
		ASSERT_EQ(::getsockname(listenFd, reinterpret_cast<sockaddr*>(&addr), &len), 0);
		port = ntohs(addr.sin_port);
		worker = std::thread([this] { Serve(); });
	}

	~FakeSmtp()
	{
		if (worker.joinable())
			worker.join();
		if (listenFd >= 0)
			::close(listenFd);
	}

	void Serve()
	{
		pollfd wait{listenFd, POLLIN, 0};
		if (::poll(&wait, 1, 5000) <= 0)
			return;
		const int client = ::accept(listenFd, nullptr, nullptr);
		if (client < 0)
			return;
		timeval tv{5, 0};
		::setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

		auto say = [&](const char* line) {
			const std::string data = std::string(line) + "\r\n";
			::send(client, data.data(), data.size(), 0);
		};
		auto line = [&]() {
			std::string got;
			char ch = 0;
			while (::recv(client, &ch, 1, 0) == 1 && ch != '\n') {
				if (ch != '\r')
					got += ch;
			}
			std::lock_guard<std::mutex> lock(gate);
			text += got;
			text += '\n';
			return got;
		};

		say("220 ready");
		line();
		say("250 ok");
		std::string cmd = line();
		if (cmd.rfind("AUTH", 0) == 0) {
			say("334 user");
			line();
			say("334 pass");
			line();
			say("235 ok");
			cmd = line();
		}
		say("250 ok");
		line();
		say("250 ok");
		line();
		say("354 go");
		for (;;) {
			const std::string row = line();
			if (row == ".")
				break;
		}
		say("250 queued");
		line();
		say("221 bye");
		::close(client);
	}
};

} // namespace

TEST(InternetMail, AMessageNamesItsSenderAndItsText) {
	ibValue message = ibValue::CreateObject(wxT("InternetMailMessage"));
	ASSERT_TRUE(message.SetPropVal(message.FindProp(wxT("Subject")), ibValue(wxT("Hello"))));
	ibValue from = Prop(message, wxT("From"));
	ASSERT_TRUE(from.SetPropVal(from.FindProp(wxT("Address")), ibValue(wxT("ada@example.com"))));
	ibValue to = Prop(message, wxT("To"));
	Add(to, ibValue(wxT("bob@example.com")));
	ibValue texts = Prop(message, wxT("Texts"));
	Add(texts, ibValue(wxT("one line")));

	EXPECT_EQ(Prop(message, wxT("Subject")).GetString(), wxString(wxT("Hello")));
	EXPECT_EQ(Prop(from, wxT("Address")).GetString(), wxString(wxT("ada@example.com")));
	ibValue count;
	ASSERT_TRUE(to.CallAsFunc(to.FindMethod(wxT("Count")), count, nullptr, 0));
	EXPECT_EQ(count.GetInteger(), 1);
}

TEST(InternetMail, LogonSendsTheMessage) {
	wxInitializer wx;
	ASSERT_TRUE(wx.IsOk());

	FakeSmtp server;
	server.Start();
	ASSERT_GT(server.port.load(), 0);

	ibValue profile = ibValue::CreateObject(wxT("Structure"));
	Insert(profile, wxT("SMTPServerAddress"), ibValue(wxT("127.0.0.1")));
	Insert(profile, wxT("SMTPPort"), ibValue(ibNumber(server.port.load())));
	Insert(profile, wxT("SMTPUser"), ibValue(wxT("ada")));
	Insert(profile, wxT("SMTPPassword"), ibValue(wxT("secret")));
	Insert(profile, wxT("Timeout"), ibValue(ibNumber(5)));

	ibValue message = ibValue::CreateObject(wxT("InternetMailMessage"));
	message.SetPropVal(message.FindProp(wxT("Subject")), ibValue(wxT("Hello")));
	ibValue from = Prop(message, wxT("From"));
	from.SetPropVal(from.FindProp(wxT("Address")), ibValue(wxT("ada@example.com")));
	Add(Prop(message, wxT("To")), ibValue(wxT("bob@example.com")));
	Add(Prop(message, wxT("Texts")), ibValue(wxT("one line")));

	ibValue mail = ibValue::CreateObject(wxT("InternetMail"));
	ibValue* logonArg = &profile;
	ASSERT_TRUE(mail.CallAsProc(mail.FindMethod(wxT("Logon")), &logonArg, 1));
	ibValue* sendArg = &message;
	ASSERT_TRUE(mail.CallAsProc(mail.FindMethod(wxT("Send")), &sendArg, 1));
	ASSERT_TRUE(mail.CallAsProc(mail.FindMethod(wxT("Logoff")), nullptr, 0));

	server.worker.join();
	std::string text;
	{
		std::lock_guard<std::mutex> lock(server.gate);
		text = server.text;
	}
	EXPECT_NE(text.find("AUTH LOGIN"), std::string::npos);
	EXPECT_NE(text.find("MAIL FROM:<ada@example.com>"), std::string::npos);
	EXPECT_NE(text.find("RCPT TO:<bob@example.com>"), std::string::npos);
	EXPECT_NE(text.find("Subject: Hello"), std::string::npos);
	EXPECT_NE(text.find("one line"), std::string::npos);
}

TEST(InternetMail, TheConstructorCompiles) {
	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	try {
		ASSERT_TRUE(cc.Compile(
			wxT("Procedure Check()\n")
			wxT("    var mail;\n")
			wxT("    var message;\n")
			wxT("    mail = New InternetMail;\n")
			wxT("    message = New InternetMailMessage;\n")
			wxT("EndProcedure\n")));
	}
	catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}
