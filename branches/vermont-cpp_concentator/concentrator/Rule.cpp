/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

/*
 * Internal representation of an IP address: up to 5 bytes: 192.168.0.201/24 ==> 192 168 0 201 8(!)
 * Internal representation of a Port range: 2n bytes: 80,443 ==> 80 80 443 443
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "Rule.hpp"
#include "IpfixParser.hpp"
#include "ipfix.hpp"

#include "msg.h"

#define MAX_LINE_LEN 256

/* --- constants ------------*/

/* --- functions ------------*/

Rule::Rule() {
	fieldCount = 0;
}

/**
 * De-allocates memory used by the given rule.
 * This will NOT destroy hashtables associated with the rule
 */
Rule::~Rule() {
	int i;
	for (i = 0; i < fieldCount; i++) {
		delete field[i];
	}
}

char* modifier2string(Rule::Field::Modifier i) {
	static char s[16];
	if (i == Rule::Field::DISCARD) return "discard";
	if (i == Rule::Field::KEEP) return "keep";
	if (i == Rule::Field::AGGREGATE) return "aggregate";
	if ((i >= Rule::Field::MASK_START) && (i <= Rule::Field::MASK_END)) {
		sprintf(s, "mask/%d", i - Rule::Field::MASK_START);
		return s;
	}
	return "unknownModifier";
}

/**
 * Prints a textual representation of the rule to stdout
 */
void Rule::print() {
	int i;

	printf("Aggregate %d %d\n",id,preceding);
	for (i=0; i < fieldCount; i++) {
		printf("\t");
		char* modifier = modifier2string(field[i]->modifier);
		if (modifier != NULL) {
			printf("%s ", modifier);
		} else {
			printf("unknownModifier ");
		}

		char* type = typeid2string(field[i]->type.id);
		if (type != NULL) {
			printf("%s ", type);
		} else {
			printf("unknownType ");
		}

		if (field[i]->pattern != NULL) {
			printf("in ");
			printFieldData(field[i]->type, field[i]->pattern);
		}
		printf("\n");
	}
	printf("\n");
}

uint8_t getIPv4IMask(FieldType* type, FieldData* data) {
	if (type->length > 4) return data[4];
	if (type->length == 4) return 0;
	if (type->length == 3) return 8;
	if (type->length == 2) return 16;
	if (type->length == 1) return 24;
	if (type->length == 0) return 32;

	msg(MSG_FATAL, "Invalid IPv4 address length: %d", type->length);
	return 0;
}

uint32_t getIPv4Address(FieldType* type, FieldData* data) {
	uint32_t addr = 0;
	if (type->length >= 1) addr |= data[0] << 24;
	if (type->length >= 2) addr |= data[1] << 16;
	if (type->length >= 3) addr |= data[2] << 8;
	if (type->length >= 4) addr |= data[3] << 0;
	return addr;
}

/**
 * Checks if a given "PortRanges" Field matches a "PortRanges" Pattern
 * @c return 1 if field matches
 */
int matchesPortPattern(FieldType* dataType, FieldData* data, FieldType* patternType, FieldData* pattern) {
	int i;
	int j;

	if ((dataType->length == 2) && (patternType->length == 2)) {
		return ((data[0] == pattern[0]) && (data[1] == pattern[1]));
	}
	if ((dataType->length == 2) && ((patternType->length % 4) == 0)) {
		int dport = (data[0] << 8) + data[1];
		int foundMatch = 0;
		for (j = 0; j < patternType->length; j+=4) {
			int pports = (pattern[j+0] << 8) + pattern[j+1];
			int pporte = (pattern[j+2] << 8) + pattern[j+3];
			if ((dport >= pports) && (dport <= pporte)) {
				foundMatch = 1;
				break;
			}
		}
		return foundMatch;
	}
	if (((dataType->length % 4) == 0) && (patternType->length == 2)) {
		for (i = 0; i < dataType->length; i+=4) {
			int dports = (data[i+0] << 8) + data[i+1];
			int dporte = (data[i+2] << 8) + data[i+3];
			int pport = (pattern[0] << 8) + pattern[1];
			if ((dports < pport) && (dporte > pport)) return 0;
		}
		return 1;
	}
	if (((dataType->length % 4) == 0) && ((patternType->length % 4) == 0)) {
		for (i = 0; i < dataType->length; i+=4) {
			int dports = (data[i+0] << 8) + data[i+1];
			int dporte = (data[i+2] << 8) + data[i+3];
			int foundMatch = 0;
			for (j = 0; j < patternType->length; j+=4) {
				int pports = (pattern[j+0] << 8) + pattern[j+1];
				int pporte = (pattern[j+2] << 8) + pattern[j+3];
				if ((dports >= pports) && (dporte <= pporte)) {
					foundMatch = 1;
					break;
				}
			}
			if (!foundMatch) return 0;
		}
		return 1;
	}

	msg(MSG_FATAL, "matching port of length %d with pattern of length %d not supported",
	    dataType->length, patternType->length);
	return 0;
}

/**
 * Checks if a given IPv4 Field matches a IPv4 Pattern
 * @c return 1 if field matches
 */
int matchesIPv4Pattern(FieldType* dataType, FieldData* data, FieldType* patternType, FieldData* pattern) {
	/* Get (inverse!) Network Masks */
	int dmaski = getIPv4IMask(dataType, data);
	int pmaski = getIPv4IMask(patternType, pattern);

	/* If the pattern is more specific than our data we return 0 */
	if (dmaski > pmaski) return 0;

	uint32_t daddr = getIPv4Address(dataType, data);
	uint32_t paddr = getIPv4Address(patternType, pattern);

	return ((daddr >> pmaski) == (paddr >> pmaski));
}

/**
 * Checks if a given Field matches a Pattern when compared byte for byte
 * @c return 1 if field matches
 */
int matchesRawPattern(FieldType* dataType, FieldData* data, FieldType* patternType, FieldData* pattern) {
	int i;

	/* Byte-wise comparison, so lengths must be equal */
	if (dataType->length != patternType->length) return 0;

	for (i = 0; i < dataType->length; i++) if (data[i] != pattern[i]) return 0;

	return 1;
}

/**
 * Checks if a data block matches a given pattern
 * @return 1 if pattern is matched
 */
int matchesPattern(FieldType* dataType, FieldData* data, FieldType* patternType, FieldData* pattern) {
	/* an inexistent pattern is always matched */
	if (pattern == NULL) return 1;

	if ((dataType->id != patternType->id) || (dataType->eid != patternType->eid)) return 0;

	switch (patternType->id) {
	case IPFIX_TYPEID_sourceIPv4Address:
	case IPFIX_TYPEID_destinationIPv4Address: {
		return matchesIPv4Pattern(dataType, data, patternType, pattern);
		break;
	}
	case IPFIX_TYPEID_sourceTransportPort:
	case IPFIX_TYPEID_destinationTransportPort:
		return matchesPortPattern(dataType, data, patternType, pattern);
		break;
	default:
		return matchesRawPattern(dataType, data, patternType, pattern);
		break;
	}
}

/**
 * templateDataMatchesRule helper.
 * If a flow's IP matched a ruleField's IP address + mask, 
 * we will also have to check if the flow's mask is no broader than the ruleField's
 * @return 0 if the field had an associated mask that did not match
 */
int checkAssociatedMask(TemplateInfo* info, FieldData* data, Rule::Field* ruleField) {
	if ((ruleField->type.id == IPFIX_TYPEID_sourceIPv4Address) && (ruleField->pattern) && (ruleField->type.length == 5)) {
		FieldInfo* maskInfo = getTemplateFieldInfo(info, IPFIX_TYPEID_sourceIPv4Mask, 0);
		if (!maskInfo) return 1;

		uint8_t pmask = 32 - getIPv4IMask(&ruleField->type, ruleField->pattern);
		uint8_t dmask = *(data + maskInfo->offset);
		return (dmask >= pmask);
	}
	if ((ruleField->type.id == IPFIX_TYPEID_destinationIPv4Address) && (ruleField->pattern) && (ruleField->type.length == 5)) {
		FieldInfo* maskInfo = getTemplateFieldInfo(info, IPFIX_TYPEID_destinationIPv4Mask, 0);
		if (!maskInfo) return 1;

		uint8_t pmask = 32 - getIPv4IMask(&ruleField->type, ruleField->pattern);
		uint8_t dmask = *(data + maskInfo->offset);
		return (dmask >= pmask);
	}
	return 1;
}

/**
 * templateDataMatchesRule helper.
 * If a flow's IP matched a ruleField's IP address + mask, 
 * we will also have to check if the flow's mask is no broader than the ruleField's
 * @return 0 if the field had an associated mask that did not match
 */
int checkAssociatedMask2(DataTemplateInfo* info, FieldData* data, Rule::Field* ruleField) {
	if ((ruleField->type.id == IPFIX_TYPEID_sourceIPv4Address) && (ruleField->pattern) && (ruleField->type.length == 5)) {
		FieldInfo* maskInfo = getDataTemplateFieldInfo(info, IPFIX_TYPEID_sourceIPv4Mask, 0);
		if (!maskInfo) return 1;

		uint8_t pmask = 32 - getIPv4IMask(&ruleField->type, ruleField->pattern);
		uint8_t dmask = *(data + maskInfo->offset);
		return (dmask >= pmask);
	}
	if ((ruleField->type.id == IPFIX_TYPEID_destinationIPv4Address) && (ruleField->pattern) && (ruleField->type.length == 5)) {
		FieldInfo* maskInfo = getDataTemplateFieldInfo(info, IPFIX_TYPEID_destinationIPv4Mask, 0);
		if (!maskInfo) return 1;

		uint8_t pmask = 32 - getIPv4IMask(&ruleField->type, ruleField->pattern);
		uint8_t dmask = *(data + maskInfo->offset);
		return (dmask >= pmask);
	}
	return 1;
}

/**
 * templateDataMatchesRule helper.
 * If a flow's IP matched a ruleField's IP address + mask, 
 * we will also have to check if the flow's mask is no broader than the ruleField's
 * @return 0 if the field had an associated mask that did not match
 */
int checkAssociatedMask3(DataTemplateInfo* info, FieldData* data, Rule::Field* ruleField) {
	if ((ruleField->type.id == IPFIX_TYPEID_sourceIPv4Address) && (ruleField->pattern) && (ruleField->type.length == 5)) {
		FieldInfo* maskInfo = getDataTemplateDataInfo(info, IPFIX_TYPEID_sourceIPv4Mask, 0);
		if (!maskInfo) return 1;

		uint8_t pmask = 32 - getIPv4IMask(&ruleField->type, ruleField->pattern);
		uint8_t dmask = *(data + maskInfo->offset);
		return (dmask >= pmask);
	}
	if ((ruleField->type.id == IPFIX_TYPEID_destinationIPv4Address) && (ruleField->pattern) && (ruleField->type.length == 5)) {
		FieldInfo* maskInfo = getDataTemplateDataInfo(info, IPFIX_TYPEID_destinationIPv4Mask, 0);
		if (!maskInfo) return 1;

		uint8_t pmask = 32 - getIPv4IMask(&ruleField->type, ruleField->pattern);
		uint8_t dmask = *(data + maskInfo->offset);
		return (dmask >= pmask);
	}
	return 1;
}

/**
 * Checks if a given flow matches a rule
 * @return 1 if rule is matched, 0 otherwise
 */
int Rule::templateDataMatches(TemplateInfo* info, FieldData* data) {
	int i;
	FieldInfo* fieldInfo;

	for (i = 0; i < fieldCount; i++) {
		Rule::Field* ruleField = field[i];

		/* for all patterns of this rule, check if they are matched */
		if (field[i]->pattern) {
			fieldInfo = getTemplateFieldInfo(info, &ruleField->type);
			if (fieldInfo) {
				/* corresponding data field found, check if it matches. If it doesn't the whole rule cannot be matched */
				if (!matchesPattern(&fieldInfo->type, (data + fieldInfo->offset), &ruleField->type, ruleField->pattern)) return 0;
				if (!checkAssociatedMask(info, data, ruleField)) return 0;
				continue;
			}

			/* no corresponding data field found, this flow cannot match */
			msg(MSG_DEBUG, "No corresponding DataRecord field for RuleField of type %s", typeid2string(ruleField->type.id));
			return 0;
		}
		/* if a non-discarding rule field specifies no pattern, check at least if the data field exists */
		else if (field[i]->modifier != Rule::Field::DISCARD) {
			fieldInfo = getTemplateFieldInfo(info, &ruleField->type);
			if (fieldInfo) continue;
			msg(MSG_DEBUG, "No corresponding DataRecord field for RuleField of type %s", typeid2string(ruleField->type.id));
			return 0;
		}
	}

	/* all rule fields were matched */
	return 1;
}

/**
 * Checks if a given flow matches a rule
 * @return 1 if rule is matched, 0 otherwise
 */
int Rule::dataTemplateDataMatches(DataTemplateInfo* info, FieldData* data) {
	int i;
	FieldInfo* fieldInfo;

	/* for all patterns of this rule, check if they are matched */
        for(i = 0; i < fieldCount; i++) {

		if(field[i]->pattern) {
			Rule::Field* ruleField = field[i];

			fieldInfo = getDataTemplateFieldInfo(info, &ruleField->type);
			if (fieldInfo) {
				/* corresponding data field found, check if it matches. If it doesn't the whole rule cannot be matched */
				if (!matchesPattern(&fieldInfo->type, (data + fieldInfo->offset), &ruleField->type, ruleField->pattern)) return 0;
				if (!checkAssociatedMask2(info, data, ruleField)) return 0;
				continue;
			}

			/*
			  no corresponding data field found
			  see if we find a corresponding fixed data field
			*/

			fieldInfo = getDataTemplateDataInfo(info, &ruleField->type);
			if (fieldInfo) {
				/* corresponding fixed data field found, check if it matches. If it doesn't the whole rule cannot be matched */
				if (!matchesPattern(&fieldInfo->type, (info->data + fieldInfo->offset), &ruleField->type, ruleField->pattern)) return 0;
				if (!checkAssociatedMask3(info, info->data, ruleField)) return 0;
				continue;
			}

			// FIXME: if a non-discarding rule field specifies no pattern check at least if the data field exists?

			/* no corresponding data field or fixed data field found, this flow cannot match */
			msg(MSG_DEBUG, "No corresponding DataDataRecord field for RuleField of type %s", typeid2string(ruleField->type.id));
			return 0;
		}
	}

	/* all rule fields were matched */
	return 1;
}
