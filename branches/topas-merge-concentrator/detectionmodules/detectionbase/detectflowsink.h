#ifndef _DETECT_FLOWSINK_H_
#define _DETECT_FLOWSINK_H_

#include <concentrator/FlowSink.hpp>
#include <concentrator/IpfixRecord.hpp>

#include <vector>

namespace TOPAS {

template <class Storage>
class DetectFlowSink : public VERMONT::FlowSink {
public:
	DetectFlowSink();
	~DetectFlowSink();

	virtual int onTemplate(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::TemplateInfo* templateInfo); 
	virtual int onOptionsTemplate(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo);
	virtual int onDataTemplate(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::DataTemplateInfo* dataTemplateInfo);
	virtual int onDataRecord(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::TemplateInfo* templateInfo, uint16_t length, VERMONT::IpfixRecord::Data* data);
	virtual int onOptionsRecord(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo, uint16_t length, VERMONT::IpfixRecord::Data* data);
	virtual int onDataDataRecord(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, VERMONT::IpfixRecord::Data* data);
	virtual int onTemplateDestruction(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::TemplateInfo* templateInfo);
	virtual int onOptionsTemplateDestruction(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo);
	virtual int onDataTemplateDestruction(VERMONT::IpfixRecord::SourceID* sourceID, VERMONT::IpfixRecord::DataTemplateInfo* dataTemplateInfo);

	virtual Storage* getStorage() = 0;
	
        void subscribeId(uint16_t id) 
        {
                idList.push_back(id);
        }
protected:
	virtual Storage* storage() = 0;

private:
	Mutex lock;
        std::vector<uint16_t> idList;

        bool isIdInList(uint16_t id) const
        {
		if (idList.empty()) {
			return true;
		}
                /* TODO: think about hashing */
                for (std::vector<uint16_t>::const_iterator i = idList.begin(); i != idList.end(); ++i) {
                        if ((*i) == id)
                                return true;
                }
                
                return false;
        }

};

template <class Storage>
class BufferedDetectSink : public DetectFlowSink<Storage>
{
public:
	virtual Storage* getStorage() 
	{
		return NULL;
	}

protected:
	virtual Storage* storage() {
		return NULL;
	}


};

template <class Storage>
class UnBufferedDetectSink : public DetectFlowSink<Storage>
{
public:
	virtual Storage* getStorage() 
	{
		return NULL;
	}

protected:
	virtual Storage* storage() {
		return NULL;
	}


};


}; // namespace TOPAS

#include "detectflowsink.cpp"

#endif
