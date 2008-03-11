
namespace TOPAS {

template <class Storage>
inline DetectFlowSink<Storage>::DetectFlowSink()
{

}

template <class Storage>
inline DetectFlowSink<Storage>::~DetectFlowSink()
{

}


template <class Storage>
inline int DetectFlowSink<Storage>::onTemplate(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::TemplateInfo* templateInfo) 
{
	return 0;
}

template <class Storage>
inline int DetectFlowSink<Storage>::onOptionsTemplate(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo)
{
	return 0;
}

template <class Storage>
inline int DetectFlowSink<Storage>::onDataTemplate(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::DataTemplateInfo* dataTemplateInfo)
{
	return 0;
}

template <class Storage>
inline int DetectFlowSink<Storage>::onDataRecord(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::TemplateInfo* ti, uint16_t length, VERMONT::IpfixRecord::Data* data)
{
        lock.lock();
        Storage* buf = storage();
        if(buf) {
		if (buf->recordStart(*sourceID)) {
			buf->setValid(true);
			for (uint16_t i = 0; i < ti->fieldCount; ++i) {
				if (isIdInList(ti->fieldInfo[i].type.id)) {
					buf->addFieldData(ti->fieldInfo[i].type.id, data + ti->fieldInfo[i].offset,
							ti->fieldInfo[i].type.length, ti->fieldInfo[i].type.eid);
				}
			}
			buf->recordEnd();
		}
	}/* Error message is printed in filepolicy.h with an error counter
	else { 
		msg(MSG_ERROR, "DetectionBase: getBuffer() returned NULL, record dropped!");
	}*/
        lock.unlock();
        return 0;
}

template <class Storage>
inline int DetectFlowSink<Storage>::onOptionsRecord(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo, uint16_t length, VERMONT::IpfixRecord::Data* data)
{
	return 0;
}

template <class Storage>
inline int DetectFlowSink<Storage>::onDataDataRecord(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::DataTemplateInfo* dti, uint16_t length, VERMONT::IpfixRecord::Data* data)
{
        lock.lock();
        Storage* buf = storage();
        if(buf) {
		if (buf->recordStart(*sourceID)) {
			buf->setValid(true);
			for (uint16_t i = 0; i < dti->fieldCount; ++i) {
				if (isIdInList(dti->fieldInfo[i].type.id)) {
					buf->addFieldData(dti->fieldInfo[i].type.id, data + dti->fieldInfo[i].offset,
							dti->fieldInfo[i].type.length, dti->fieldInfo[i].type.eid);
				}
			}

			/* pass fixed fields now */
			for (unsigned i = 0; i < dti->dataCount; ++i) {
				if (isIdInList(dti->dataInfo[i].type.id)) {
					buf->addFieldData(dti->dataInfo[i].type.id, data + dti->dataInfo[i].offset,
							dti->dataInfo[i].type.length, dti->fieldInfo[i].type.eid);
				}
			}
			buf->recordEnd();
		}
	}/* Error message is printed in filepolicy.h with an error counter
	else { 
		msg(MSG_ERROR, "DetectionBase: getBuffer() returned NULL, record dropped!");
	}*/
	lock.unlock();
        return 0;
	
}

template <class Storage>
inline int DetectFlowSink<Storage>::onTemplateDestruction(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::TemplateInfo* templateInfo)
{
	return 0;
}

template <class Storage>
inline int DetectFlowSink<Storage>::onOptionsTemplateDestruction(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo)
{
	return 0;
}

template <class Storage>
inline int DetectFlowSink<Storage>::onDataTemplateDestruction(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::DataTemplateInfo* dataTemplateInfo)
{
	return 0;
}



};
