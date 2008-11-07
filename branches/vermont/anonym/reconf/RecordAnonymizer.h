#ifndef _RECORD_ANONYMIZER_H_
#define _RECORD_ANONYMIZER_H_

#include "Source.h"
#include <concentrator/IpfixRecordDestination.h>
#include <anon/AnonModule.h>

class RecordAnonymizer : public Source<IpfixRecord*>, public IpfixRecordDestination, public AnonModule {
public:
	virtual ~RecordAnonymizer() {}

protected:
	virtual void onTemplate(IpfixTemplateRecord* record);
	virtual void onOptionsTemplate(IpfixOptionsTemplateRecord* record);
	virtual void onDataTemplate(IpfixDataTemplateRecord* record);
	virtual void onDataRecord(IpfixDataRecord* record);
	virtual void onOptionsRecord(IpfixOptionsRecord* record);
	virtual void onDataDataRecord(IpfixDataDataRecord* record);
	virtual void onTemplateDestruction(IpfixTemplateDestructionRecord* record);
	virtual void onOptionsTemplateDestruction(IpfixOptionsTemplateDestructionRecord* record);
	virtual void onDataTemplateDestruction(IpfixDataTemplateDestructionRecord* record);
private:
	void anonRecord(IpfixRecord* record);
};

#endif
