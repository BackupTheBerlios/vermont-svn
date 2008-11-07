#include "RecordAnonymizer.h"

void RecordAnonymizer::anonRecord(IpfixRecord* record)
{
}

void RecordAnonymizer::onTemplate(IpfixTemplateRecord* record)
{
	send(record);
}

void RecordAnonymizer::onOptionsTemplate(IpfixOptionsTemplateRecord* record)
{
	send(record);
}


void RecordAnonymizer::onDataTemplate(IpfixDataTemplateRecord* record)
{
	send(record);
}

void RecordAnonymizer::onDataRecord(IpfixDataRecord* record)
{
	for (int i = 0; i != record->templateInfo->fieldCount; ++i) {
		IpfixRecord::FieldInfo* field = record->templateInfo->fieldInfo + i;
		anonField(field->type.id, record->data + field->offset, field->type.length);
	}
	send(record);
}


void RecordAnonymizer::onOptionsRecord(IpfixOptionsRecord* record)
{
	send(record);
}

void RecordAnonymizer::onDataDataRecord(IpfixDataDataRecord* record)
{
	for (int i = 0; i != record->dataTemplateInfo->dataCount; ++i) {
		IpfixRecord::FieldInfo* field = record->dataTemplateInfo->dataInfo + i;
		anonField(field->type.id, record->data + field->offset, field->type.length);
	}
	send(record);
}

void RecordAnonymizer::onTemplateDestruction(IpfixTemplateDestructionRecord* record)
{
	send(record);
}

void RecordAnonymizer::onOptionsTemplateDestruction(IpfixOptionsTemplateDestructionRecord* record)
{
	send(record);
}


void RecordAnonymizer::onDataTemplateDestruction(IpfixDataTemplateDestructionRecord* record)
{
	send(record);
}


