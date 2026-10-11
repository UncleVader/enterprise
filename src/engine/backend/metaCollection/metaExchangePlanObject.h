#ifndef __META_EXCHANGE_PLAN_OBJECT_H__
#define __META_EXCHANGE_PLAN_OBJECT_H__

#include "metaObject.h"

class ibMetaData;

// ExchangePlan is ПланОбмена. The configuration can hold one. ExchangePlans.<Name>
// is its manager. Nodes are not stored: ThisNode, EmptyRef, FindByCode and Select
// are empty. MasterNode is remembered for this configuration in this process.

class BACKEND_API ibValueMetaObjectExchangePlan : public ibValueMetaObject {
public:
	static constexpr unsigned s_features = ibMetaFeature_Manager;

	ibValueMetaObjectExchangePlan() : ibValueMetaObject() {}
	virtual ~ibValueMetaObjectExchangePlan() {}

	virtual ibClassID ResolveChild(const ibClassID&) const override { return 0; }

	virtual bool OnBeforeRunMetaObject(int flags) override;
	virtual bool OnDeleteMetaObject() override;
};

// The ExchangePlans global: one property per plan, plus MasterNode / SetMasterNode.
ibValue ibExchangePlans(ibMetaData* metaData);

#endif
