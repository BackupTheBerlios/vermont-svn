#ifndef EXP_RULES_H
#define EXP_RULES_H

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "exp_rcvIpfix.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FIELD_MODIFIER_DISCARD     0
#define FIELD_MODIFIER_KEEP        1
#define FIELD_MODIFIER_AGGREGATE   2
#define FIELD_MODIFIER_MASK_START  126
#define FIELD_MODIFIER_MASK_END    254

#define MAX_RULE_FIELDS  255
#define MAX_RULES        255

typedef uint8_t ExpressFieldModifier;

/**
 * Single field of an aggregation rule.
 */
typedef struct {
	ExpressFieldType type;         /**< field type this RuleField refers to */
	FieldData* pattern;     /**< pattern to match fields against to determine applicability of a rule. A pattern of NULL means no pattern needs to be matched for this field */
	ExpressFieldModifier modifier; /**< modifier to apply to the corresponding field if this rule is matched */
} ExpressRuleField;

/**
 * Single aggregation rule
 */
typedef struct {
	uint16_t id;
	uint16_t preceding;
	int fieldCount;
	ExpressRuleField* field[MAX_RULE_FIELDS];
	
	void* hashtable;
} ExpressRule;

/**
 * Set of aggregation rules
 */
typedef struct {
	int count;
	ExpressRule* rule[MAX_RULES];
} ExpressRules;

ExpressRules* ExpressparseRulesFromFile(char* fname);

ExpressRuleField* ExpressmallocRuleField();
void ExpressfreeRuleField(ExpressRuleField* ruleField);

ExpressRule* ExpressmallocRule();
void ExpressfreeRule(ExpressRule* rule);

int ExpressparseTcpFlags(char* s, FieldData** fdata, FieldLength* length);
int ExpressparsePortPattern(char* s, FieldData** fdata, FieldLength* length);
int ExpressparseIPv4Pattern(char* s, FieldData** fdata, FieldLength* length);
int ExpressparseProtoPattern(char* s, FieldData** fdata, FieldLength* length);

void ExpressdestroyRules(ExpressRules* rules);
void ExpressprintRule(ExpressRule* rule);
int ExpresstemplateDataMatchesRule(FieldData* data, ExpressRule* rule, int transport_offset);
int ExpressdataTemplateDataMatchesRule(ExpressDataTemplateInfo* info, FieldData* data, ExpressRule* rule);


#ifdef __cplusplus
}
#endif


#endif
