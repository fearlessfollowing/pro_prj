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
** 函数名称: PropertyMap::PropertyMap
** 函数功能: PropertyMap构造函数
** 入口参数: 无
** 返 回 值: 无
**
**
*****************************************************************************************/
PropertyMap::PropertyMap()
{
}


/***************************************************************************************
** 函数名称: PropertyMap::~PropertyMap
** 函数功能: PropertyMap析构函数
** 入口参数: 无
** 返 回 值: 无
**
**
*****************************************************************************************/
PropertyMap::~PropertyMap() 
{
}



/***************************************************************************************
** 函数名称: PropertyMap::clear
** 函数功能: 清空容器
** 入口参数: 无
** 返 回 值: 无
**
**
*****************************************************************************************/
void PropertyMap::clear() 
{
    mProperties.clear();
}


/***************************************************************************************
** 函数名称: PropertyMap::addProperty
** 函数功能: 添加键值对
** 入口参数: key - 键字串的引用
**			 value - 值字串的引用
** 返 回 值: 无
**
**
*****************************************************************************************/
void PropertyMap::addProperty(const String8& key, const String8& value) 
{
    mProperties.add(key, value);
}


/***************************************************************************************
** 函数名称: PropertyMap::hasProperty
** 函数功能: 判断指定的键是否已经存在
** 入口参数: key - 键字串引用
** 返 回 值: 存在返回true;否则返回false
**
**
*****************************************************************************************/
bool PropertyMap::hasProperty(const String8& key) const 
{
    return mProperties.indexOfKey(key) >= 0;
}


/***************************************************************************************
** 函数名称: PropertyMap::tryGetProperty
** 函数功能: 尝试获取指定键的值(String类型)
** 入口参数: key - 键字串引用
**			 outValue - 输出值的引用
** 返 回 值: 存在返回true;否则返回false
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
** 函数名称: PropertyMap::tryGetProperty
** 函数功能: 尝试获取指定键的值(bool类型)
** 入口参数: key - 键字串引用
**			 outValue - 输出值的引用
** 返 回 值: 存在返回true;否则返回false
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
** 函数名称: PropertyMap::tryGetProperty
** 函数功能: 尝试获取指定键的值(int类型)
** 入口参数: key - 键字串引用
**			 outValue - 输出值的引用
** 返 回 值: 存在返回true;否则返回false
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
** 函数名称: PropertyMap::tryGetProperty
** 函数功能: 尝试获取指定键的值(float类型)
** 入口参数: key - 键字串引用
**			 outValue - 输出值的引用
** 返 回 值: 存在返回true;否则返回false
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
** 函数名称: PropertyMap::addAll
** 函数功能: 添加map中所有的键值对
** 入口参数: map - 源PropertyMap
** 返 回 值: 无
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
** 函数名称: PropertyMap::load
** 函数功能: 解析配置文件并加载其属性键值对到PropertyMap的内部容器中
** 入口参数: filename - 属性的文件名引用
**			 outMap - 指向PropertyMap*的指针
** 返 回 值: 无
**
**
*****************************************************************************************/
status_t PropertyMap::load(const String8& filename, PropertyMap** outMap) 
{
    *outMap = NULL;

    Tokenizer* tokenizer;

	/* 打开文件,构造一个标记器对象 */
    status_t status = Tokenizer::open(filename, &tokenizer);
    if (status) {
        ALOGE("Error %d opening property file %s.", status, filename.string());
    } else {
        PropertyMap* map = new PropertyMap();	/* 创建一个PropertyMap对象 */
        if (!map) {
            ALOGE("Error allocating property map.");
            status = NO_MEMORY;
        } else {
        
			#if DEBUG_PARSER_PERFORMANCE
            nsecs_t startTime = systemTime(SYSTEM_TIME_MONOTONIC);
			#endif

			Parser parser(map, tokenizer);	/* 构建一个解析器对象 */
            status = parser.parse();		/* 解析配置文件 */
			
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
        delete tokenizer;	/* 释放标记器对象 */
    }
    return status;
}


// --- PropertyMap::Parser ---

/***************************************************************************************
** 函数名称: PropertyMap::Parser::Parser
** 函数功能: 解析器的构造函数
** 入口参数: map - PropertyMap指针
**			 tokenizer - 标记器对象指针
** 返 回 值: 无
**
**
*****************************************************************************************/
PropertyMap::Parser::Parser(PropertyMap* map, Tokenizer* tokenizer) :
        mMap(map), mTokenizer(tokenizer) 
{
}


/***************************************************************************************
** 函数名称: PropertyMap::Parser::~Parser
** 函数功能: 解析器的析构函数
** 入口参数: 无
** 返 回 值: 无
**
**
*****************************************************************************************/
PropertyMap::Parser::~Parser() 
{
}


/***************************************************************************************
** 函数名称: PropertyMap::Parser::parse
** 函数功能:解析配置文件
** 入口参数: 无
** 返 回 值: 成功返回NO_ERROR
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

        mTokenizer->skipDelimiters(WHITESPACE);	/* 跳过空白行 */

		/* 行没有结束,并且非注释行 */
        if (!mTokenizer->isEol() && mTokenizer->peekChar() != '#') {

			/* 得到一行中的属性名字段 */
            String8 keyToken = mTokenizer->nextToken(WHITESPACE_OR_PROPERTY_DELIMITER);
            if (keyToken.isEmpty()) {
                ALOGE("%s: Expected non-empty property key.", mTokenizer->getLocation().string());
                return BAD_VALUE;
            }

			/* 跳过中间的空格(property = value) */
            mTokenizer->skipDelimiters(WHITESPACE);

			/* 属性名后面跟的不是'='号,说明格式不对,停止解析 */
            if (mTokenizer->nextChar() != '=') {
                ALOGE("%s: Expected '=' between property key and value.",
                        mTokenizer->getLocation().string());
                return BAD_VALUE;
            }

			/* 跳过"="号后面的空格 */
            mTokenizer->skipDelimiters(WHITESPACE);

			/* 得到"="后面的属性值 */
            String8 valueToken = mTokenizer->nextToken(WHITESPACE);

			/* 如果属性值包含"\\"或"\""字串,退出解析 */
            if (valueToken.find("\\", 0) >= 0 || valueToken.find("\"", 0) >= 0) {
                ALOGE("%s: Found reserved character '\\' or '\"' in property value.",
                        mTokenizer->getLocation().string());
                return BAD_VALUE;
            }
			
			/* 跳过属性值后面的所有空格 */
            mTokenizer->skipDelimiters(WHITESPACE);

			/* 如果后面不是换行符,说明格式错误,退出解析 */
            if (!mTokenizer->isEol()) {
                ALOGE("%s: Expected end of line, got '%s'.",
                        mTokenizer->getLocation().string(),
                        mTokenizer->peekRemainderOfLine().string());
                return BAD_VALUE;
            }

			/* 判断该属性名是否已经存在,如果有相同的属性名,出错退出 */
            if (mMap->hasProperty(keyToken)) {
                ALOGE("%s: Duplicate property value for key '%s'.",
                        mTokenizer->getLocation().string(), keyToken.string());
                return BAD_VALUE;
            }

			/* 将得到的键值对加入到Map中 */
            mMap->addProperty(keyToken, valueToken);
        }

        mTokenizer->nextLine();		/* 解析下一行 */
    }
	
    return NO_ERROR;
}

} // namespace android
