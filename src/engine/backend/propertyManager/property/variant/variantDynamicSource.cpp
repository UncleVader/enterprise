#include "variantDynamicSource.h"

#include "backend/appData.h"                      // appData -> GetQueryableFactory (global fallback)
#include "backend/metaData.h"                     // ibMetaData::GetSourceFactory — resolve THROUGH the owner's config
#include "backend/query/queryableFactory.h"       // ResolveById (live re-resolve)
#include "backend/query/queryable.h"              // GetQueryTableId / GetQueryName
#include "backend/propertyManager/propertyObject.h"   // ibPropertyObject::GetMetaData

// The configuration this cell re-resolves through. A metadata-backed source names its own — that is where
// RegisterSource put it. The property owner's GetMetaData() is the form, or, for a list that is not attached
// yet, the ACTIVE configuration. Those are different objects whenever the list was built against a
// configuration that is not the process-wide one, and ResolveById on the wrong factory answers null.
// RunComposerPage then returns an empty page without sending SQL.
static const ibMetaData* ibVariantConfig(const ibBackendQueryable* queryable, const ibPropertyObject* owner)
{
	if (queryable != nullptr) {
		if (const ibMetaData* md = queryable->GetMetaData())
			return md;
	}
	return owner != nullptr ? owner->GetMetaData() : nullptr;
}

ibVariantDataDynamicSource::ibVariantDataDynamicSource(const ibBackendQueryable* queryable, const ibPropertyObject* owner)
	: wxVariantData(), m_tableId(queryable != nullptr ? (ibMetaID)queryable->GetQueryTableId() : wxNOT_FOUND),
	  m_metaData(ibVariantConfig(queryable, owner))
{
}

// The factory this cell re-resolves through: the SPECIFIC config's own factory (where the queryable lives), which
// descends to the global factory INTERNALLY on a miss — so a copied / other-config list resolves ITS source, never
// whatever registered globally last. Null metadata → the global base factory (common/plugin; empty today).
static ibQueryableFactory* ibVariantSourceFactory(const ibMetaData* md)
{
	ibQueryableFactory* factory = md != nullptr ? md->GetSourceFactory() : nullptr;
	return factory != nullptr ? factory : ibApplicationData::GetQueryableFactory();
}

const ibBackendQueryable* ibVariantDataDynamicSource::GetQueryable() const
{
	// Re-resolve LIVE every time — never a cached pointer. Goes THROUGH the owner's metadata factory (per-config →
	// global), which hands back the descriptor's own `&m_queryable`, or null if the source was deleted.
	if (m_tableId == wxNOT_FOUND)
		return nullptr;
	ibQueryableFactory* factory = ibVariantSourceFactory(m_metaData);
	return factory != nullptr ? factory->ResolveById(m_tableId) : nullptr;
}

const ibQueryableSourceDescriptor* ibVariantDataDynamicSource::GetDescriptor() const
{
	// Re-resolve LIVE, same id as GetQueryable, THROUGH the owner's metadata factory — the descriptor (its command +
	// column surface, FillSourceExplorer) is the copy's OWN, so the columns are the copy's. Null when deleted.
	if (m_tableId == wxNOT_FOUND)
		return nullptr;
	ibQueryableFactory* factory = ibVariantSourceFactory(m_metaData);
	return factory != nullptr ? factory->ResolveDescriptorById(m_tableId) : nullptr;
}

void ibVariantDataDynamicSource::SetQueryable(const ibBackendQueryable* queryable)
{
	m_tableId = queryable != nullptr ? (ibMetaID)queryable->GetQueryTableId() : wxNOT_FOUND;
}

wxString ibVariantDataDynamicSource::MakeString() const
{
	const ibBackendQueryable* q = GetQueryable();
	return q != nullptr ? q->GetQueryName() : wxString();
}
