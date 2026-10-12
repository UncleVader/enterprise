// =============================================================================
// picture_set on a common Picture (#190).
//
// A configuration Picture stores the image itself (ibPropertyExternalPicture).
// Commands, command groups and sections store a reference (ibPropertyPicture).
// picture_set used to look only for the reference, so a common picture was
// refused with "has no picture of its own" and the bytes never landed.
//
// DB-free: ibMcpApplyPicture is the same apply picture_set runs once the object
// is in hand. No configuration is installed as the active one, and no designer
// session is opened.
// =============================================================================

#include <gtest/gtest.h>

#include "backend/mcp/mcpTool.h"
#include "backend/metadataConfiguration.h"
#include "backend/metaCollection/metaObject.h"
#include "backend/metaCollection/metaPictureObject.h"
#include "core/serialize/dataBuilder.h"

#include <wx/image.h>
#include <wx/init.h>
#include <wx/log.h>

namespace {

// 1x1 PNG. The same kind of base64 a section or a command already accepts.
const char* kOnePixelPng =
	"iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==";

struct PictureHost {
	wxInitializer wx;

	PictureHost() {
		wxInitAllImageHandlers();
		if (wxLog::GetActiveTarget() != nullptr)
			delete wxLog::SetActiveTarget(new wxLogStderr());
	}
};

ibValueMetaObject* Make(ibMetaDataConfigurationFile& cfg, const ibClassID& kind, const wxString& name) {
	ibValueMetaObjectConfiguration* root = cfg.GetCommonMetaObject();
	if (root == nullptr)
		return nullptr;
	ibValueMetaObject* object = cfg.CreateMetaObject(kind, root, /*runObject*/ false);
	if (object != nullptr)
		cfg.RenameMetaObject(object, name);
	return object;
}

} // namespace

// The bytes go into the Picture property and read back as the image, at its own size.
TEST(PictureSet, CommonPicture_StoresTheImageBytes)
{
	PictureHost host;
	ASSERT_TRUE(host.wx.IsOk());

	ibMetaDataConfigurationFile cfg;
	ibValueMetaObject* object = Make(cfg, g_metaPictureCLSID, wxT("P"));
	ASSERT_NE(nullptr, object);

	ibDataNode params;
	params.SetValue(wxT("data"), wxString::FromUTF8(kOnePixelPng));
	params.SetValue(wxT("name"), wxT("mark"));

	ibDataNode result;
	wxString refusal;
	ASSERT_TRUE(ibMcpApplyPicture(object, params, result, refusal)) << std::string(refusal.utf8_str());

	EXPECT_EQ(wxString(wxT("P")), result.GetValue<wxString>(wxT("object")));
	EXPECT_TRUE(result.GetValue<bool>(wxT("set")));
	EXPECT_FALSE(result.GetValue<bool>(wxT("empty")));

	const ibDataValue* shaped = result.FindField(wxT("picture"));
	ASSERT_NE(nullptr, shaped);
	const std::shared_ptr<ibDataNode>& image = shaped->AsChild();
	ASSERT_TRUE(static_cast<bool>(image));
	EXPECT_EQ(wxString(wxT("mark")), image->GetValue<wxString>(wxT("Name")));
	EXPECT_EQ(1, image->GetValue<s32>(wxT("Width")));
	EXPECT_EQ(1, image->GetValue<s32>(wxT("Height")));
	EXPECT_GT(image->GetValue<wxMemoryBuffer>(wxT("Buffer")).GetDataLen(), 0u);

	auto* picture = dynamic_cast<ibValueMetaObjectPicture*>(object);
	ASSERT_NE(nullptr, picture);
	const wxBitmap drawn = picture->GetValueAsBitmap();
	EXPECT_TRUE(drawn.IsOk());
	EXPECT_EQ(1, drawn.GetWidth());
	EXPECT_EQ(1, drawn.GetHeight());
}

// A common picture is the image. A reference to the engine's set does not fit that property.
TEST(PictureSet, CommonPicture_RefusesAnEngineOrConfigurationReference)
{
	PictureHost host;
	ASSERT_TRUE(host.wx.IsOk());

	ibMetaDataConfigurationFile cfg;
	ibValueMetaObject* object = Make(cfg, g_metaPictureCLSID, wxT("P"));
	ASSERT_NE(nullptr, object);

	ibDataNode params;
	params.SetValue(wxT("engine"), wxT("Print"));

	ibDataNode result;
	wxString refusal;
	EXPECT_FALSE(ibMcpApplyPicture(object, params, result, refusal));
	EXPECT_NE(wxNOT_FOUND, refusal.Find(wxT("holds the image itself")));

	params.SetField(wxT("engine"), ibDataValue::String(wxEmptyString));
	params.SetValue(wxT("configuration"), wxT("Other"));
	refusal.clear();
	EXPECT_FALSE(ibMcpApplyPicture(object, params, result, refusal));
	EXPECT_NE(wxNOT_FOUND, refusal.Find(wxT("holds the image itself")));
}

// A catalog has neither kind of picture property. The old refusal still applies.
TEST(PictureSet, AnObjectWithoutAPicture_IsStillRefused)
{
	PictureHost host;
	ASSERT_TRUE(host.wx.IsOk());

	ibMetaDataConfigurationFile cfg;
	ibValueMetaObject* catalog = Make(cfg, g_metaCatalogCLSID, wxT("Firms"));
	ASSERT_NE(nullptr, catalog);

	ibDataNode params;
	params.SetValue(wxT("data"), wxString::FromUTF8(kOnePixelPng));
	params.SetValue(wxT("name"), wxT("mark"));

	ibDataNode result;
	wxString refusal;
	EXPECT_FALSE(ibMcpApplyPicture(catalog, params, result, refusal));
	EXPECT_NE(wxNOT_FOUND, refusal.Find(wxT("no picture of its own")));
}
