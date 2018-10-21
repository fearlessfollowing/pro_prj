/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "PropertyMap"

#include <stdlib.h>
#include <string.h>

#include <utils/PropertyMap.h>
#include <utils/Log.h>

// Enables debug output for the parser.
#define DEBUG_PARSER 0

// Enables debug output for parser performance.
#define DEBUG_PARSER_PERFORMANCE 0


namespace android {

static const char* WHITESPACE = " \t\r";
static const char* WHITESPACE_OR_PROPERTY_DELIMITER = " \t\r=";


// --- PropertyMap ---

/***************************************************************************************
** ��������: PropertyMap::PropertyMap
** ��������: PropertyMap���캯��
** ��ڲ���: ��
** �� �� ֵ: ��
**
**
*****************************************************************************************/
PropertyMap::PropertyMap()
{
}


/***************************************************************************************
** ��������: PropertyMap::~PropertyMap
** ��������: PropertyMap��������
** ��ڲ���: ��
** �� �� ֵ: ��
**
**
*****************************************************************************************/
PropertyMap::~PropertyMap() 
{
}



/***************************************************************************************
** ��������: PropertyMap::clear
** ��������: �������
** ��ڲ���: ��
** �� �� ֵ: ��
**
**
*****************************************************************************************/
void PropertyMap::clear() 
{
    mProperties.clear();
}


/***************************************************************************************
** ��������: PropertyMap::addProperty
** ��������: ��Ӽ�ֵ��
** ��ڲ���: key - ���ִ�������
**			 value - ֵ�ִ�������
** �� �� ֵ: ��
**
**
*****************************************************************************************/
void PropertyMap::addProperty(const String8& key, const String8& value) 
{
    mProperties.add(key, value);
}


/***************************************************************************************
** ��������: PropertyMap::hasProperty
** ��������: �ж�ָ���ļ��Ƿ��Ѿ�����
** ��ڲ���: key - ���ִ�����
** �� �� ֵ: ���ڷ���true;���򷵻�false
**
**
*****************************************************************************************/
bool PropertyMap::hasProperty(const String8& key) const 
{
    return mProperties.indexOfKey(key) >= 0;
}


/***************************************************************************************
** ��������: PropertyMap::tryGetProperty
** ��������: ���Ի�ȡָ������ֵ(String����)
** ��ڲ���: key - ���ִ�����
**			 outValue - ���ֵ������
** �� �� ֵ: ���ڷ���true;���򷵻�false
**
**
*****************************************************************************************/
bool PropertyMap::tryGetProperty(const String8& key, String8& outValue) const 
{
    ssize_t index = mProperties.indexOfKey(key);
    if (index < 0) {
        return false;
    }

    outValue = mProperties.valueAt(index);
    return true;
}


/***************************************************************************************
** ��������: PropertyMap::tryGetProperty
** ��������: ���Ի�ȡָ������ֵ(bool����)
** ��ڲ���: key - ���ִ�����
**			 outValue - ���ֵ������
** �� �� ֵ: ���ڷ���true;���򷵻�false
**
**
*****************************************************************************************/
bool PropertyMap::tryGetProperty(const String8& key, bool& outValue) const 
{
    int32_t intValue;
    if (!tryGetProperty(key, intValue)) {
        return false;
    }

    outValue = intValue;
    return true;
}


/***************************************************************************************
** ��������: PropertyMap::tryGetProperty
** ��������: ���Ի�ȡָ������ֵ(int����)
** ��ڲ���: key - ���ִ�����
**			 outValue - ���ֵ������
** �� �� ֵ: ���ڷ���true;���򷵻�false
**
**
*****************************************************************************************/
bool PropertyMap::tryGetProperty(const String8& key, int32_t& outValue) const 
{
    String8 stringValue;
    if (! tryGetProperty(key, stringValue) || stringValue.length() == 0) {
        return false;
    }

    char* end;
    int value = strtol(stringValue.string(), & end, 10);
    if (*end != '\0') {
        ALOGW("Property key '%s' has invalid value '%s'.  Expected an integer.",
                key.string(), stringValue.string());
        return false;
    }
    outValue = value;
    return true;
}


/***************************************************************************************
** ��������: PropertyMap::tryGetProperty
** ��������: ���Ի�ȡָ������ֵ(float����)
** ��ڲ���: key - ���ִ�����
**			 outValue - ���ֵ������
** �� �� ֵ: ���ڷ���true;���򷵻�false
**
**
*****************************************************************************************/
bool PropertyMap::tryGetProperty(const String8& key, float& outValue) const 
{
    String8 stringValue;
    if (! tryGetProperty(key, stringValue) || stringValue.length() == 0) {
        return false;
    }

    char* end;
    float value = strtof(stringValue.string(), & end);
    if (*end != '\0') {
        ALOGW("Property key '%s' has invalid value '%s'.  Expected a float.",
                key.string(), stringValue.string());
        return false;
    }
    outValue = value;
    return true;
}



/***************************************************************************************
** ��������: PropertyMap::addAll
** ��������: ���map�����еļ�ֵ��
** ��ڲ���: map - ԴPropertyMap
** �� �� ֵ: ��
**
**
*****************************************************************************************/
void PropertyMap::addAll(const PropertyMap* map) 
{
    for (size_t i = 0; i < map->mProperties.size(); i++) {
        mProperties.add(map->mProperties.keyAt(i), map->mProperties.valueAt(i));
    }
}


/***************************************************************************************
** ��������: PropertyMap::load
** ��������: ���������ļ������������Լ�ֵ�Ե�PropertyMap���ڲ�������
** ��ڲ���: filename - ���Ե��ļ�������
**			 outMap - ָ��PropertyMap*��ָ��
** �� �� ֵ: ��
**
**
*****************************************************************************************/
status_t PropertyMap::load(const String8& filename, PropertyMap** outMap) 
{
    *outMap = NULL;

    Tokenizer* tokenizer;

	/* ���ļ�,����һ����������� */
    status_t status = Tokenizer::open(filename, &tokenizer);
    if (status) {
        ALOGE("Error %d opening property file %s.", status, filename.string());
    } else {
        PropertyMap* map = new PropertyMap();	/* ����һ��PropertyMap���� */
        if (!map) {
            ALOGE("Error allocating property map.");
            status = NO_MEMORY;
        } else {
        
			#if DEBUG_PARSER_PERFORMANCE
            nsecs_t startTime = systemTime(SYSTEM_TIME_MONOTONIC);
			#endif

			Parser parser(map, tokenizer);	/* ����һ������������ */
            status = parser.parse();		/* ���������ļ� */
			
			#if DEBUG_PARSER_PERFORMANCE
            nsecs_t elapsedTime = systemTime(SYSTEM_TIME_MONOTONIC) - startTime;
            ALOGD("Parsed property file '%s' %d lines in %0.3fms.",
                    tokenizer->getFilename().string(), tokenizer->getLineNumber(),
                    elapsedTime / 1000000.0);
			#endif

			if (status) {
                delete map;
            } else {
                *outMap = map;
            }
        }
        delete tokenizer;	/* �ͷű�������� */
    }
    return status;
}


// --- PropertyMap::Parser ---

/***************************************************************************************
** ��������: PropertyMap::Parser::Parser
** ��������: �������Ĺ��캯��
** ��ڲ���: map - PropertyMapָ��
**			 tokenizer - ���������ָ��
** �� �� ֵ: ��
**
**
*****************************************************************************************/
PropertyMap::Parser::Parser(PropertyMap* map, Tokenizer* tokenizer) :
        mMap(map), mTokenizer(tokenizer) 
{
}


/***************************************************************************************
** ��������: PropertyMap::Parser::~Parser
** ��������: ����������������
** ��ڲ���: ��
** �� �� ֵ: ��
**
**
*****************************************************************************************/
PropertyMap::Parser::~Parser() 
{
}


/***************************************************************************************
** ��������: PropertyMap::Parser::parse
** ��������:���������ļ�
** ��ڲ���: ��
** �� �� ֵ: �ɹ�����NO_ERROR
**
**
*****************************************************************************************/
status_t PropertyMap::Parser::parse() 
{

    while (!mTokenizer->isEof()) {
		
		#if DEBUG_PARSER
        ALOGD("Parsing %s: '%s'.", mTokenizer->getLocation().string(),
                mTokenizer->peekRemainderOfLine().string());
		#endif

        mTokenizer->skipDelimiters(WHITESPACE);	/* �����հ��� */

		/* ��û�н���,���ҷ�ע���� */
        if (!mTokenizer->isEol() && mTokenizer->peekChar() != '#') {

			/* �õ�һ���е��������ֶ� */
            String8 keyToken = mTokenizer->nextToken(WHITESPACE_OR_PROPERTY_DELIMITER);
            if (keyToken.isEmpty()) {
                ALOGE("%s: Expected non-empty property key.", mTokenizer->getLocation().string());
                return BAD_VALUE;
            }

			/* �����м�Ŀո�(property = value) */
            mTokenizer->skipDelimiters(WHITESPACE);

			/* ������������Ĳ���'='��,˵����ʽ����,ֹͣ���� */
            if (mTokenizer->nextChar() != '=') {
                ALOGE("%s: Expected '=' between property key and value.",
                        mTokenizer->getLocation().string());
                return BAD_VALUE;
            }

			/* ����"="�ź���Ŀո� */
            mTokenizer->skipDelimiters(WHITESPACE);

			/* �õ�"="���������ֵ */
            String8 valueToken = mTokenizer->nextToken(WHITESPACE);

			/* �������ֵ����"\\"��"\""�ִ�,�˳����� */
            if (valueToken.find("\\", 0) >= 0 || valueToken.find("\"", 0) >= 0) {
                ALOGE("%s: Found reserved character '\\' or '\"' in property value.",
                        mTokenizer->getLocation().string());
                return BAD_VALUE;
            }
			
			/* ��������ֵ��������пո� */
            mTokenizer->skipDelimiters(WHITESPACE);

			/* ������治�ǻ��з�,˵����ʽ����,�˳����� */
            if (!mTokenizer->isEol()) {
                ALOGE("%s: Expected end of line, got '%s'.",
                        mTokenizer->getLocation().string(),
                        mTokenizer->peekRemainderOfLine().string());
                return BAD_VALUE;
            }

			/* �жϸ��������Ƿ��Ѿ�����,�������ͬ��������,�����˳� */
            if (mMap->hasProperty(keyToken)) {
                ALOGE("%s: Duplicate property value for key '%s'.",
                        mTokenizer->getLocation().string(), keyToken.string());
                return BAD_VALUE;
            }

			/* ���õ��ļ�ֵ�Լ��뵽Map�� */
            mMap->addProperty(keyToken, valueToken);
        }

        mTokenizer->nextLine();		/* ������һ�� */
    }
	
    return NO_ERROR;
}

} // namespace android
