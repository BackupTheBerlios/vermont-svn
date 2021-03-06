#ifndef _RECORD_ANONYMIZER_H_
#define _RECORD_ANONYMIZER_H_

#include "core/Source.h"
#include <modules/ipfix//IpfixRecordDestination.h>
#include <common/anon/AnonModule.h>

class IpfixRecordAnonymizer : public Source<IpfixRecord*>, public IpfixRecordDestination, public AnonModule, public Module  {
public:
	IpfixRecordAnonymizer() : copyMode(false) {}
	virtual ~IpfixRecordAnonymizer() {}

	void setCopyMode(bool mode);

protected:
	bool copyMode;	// if true, the anomymization is applied to a copy of the record

	static InstanceManager<IpfixDataRecord> dataRecordIM;

	virtual void onTemplate(IpfixTemplateRecord* record);
	virtual void onOptionsTemplate(IpfixOptionsTemplateRecord* record);
	virtual void onDataRecord(IpfixDataRecord* record);
	virtual void onOptionsRecord(IpfixOptionsRecord* record);
	virtual void onTemplateDestruction(IpfixTemplateDestructionRecord* record);
	virtual void onOptionsTemplateDestruction(IpfixOptionsTemplateDestructionRecord* record);
};

#endif
