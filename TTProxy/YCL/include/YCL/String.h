/*
 * $Id: String.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_STRING_H_
#define _YCL_STRING_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/common.h>

#include <string.h>
#include <tchar.h>

namespace yebisuya {

// ������̊Ǘ��E������s���N���X�B
class String {
private:
	// ��������i�[����o�b�t�@�B
	// ������̑O�ɂ͎Q�ƃJ�E���^�������A
	// �����j���̍ۂɂ͂�����ύX����B
	const TCHAR* string;

	// utilities
	// ��������i�[����o�b�t�@���쐬����B
	// ������ƎQ�ƃJ�E���^�̕��̗̈���m�ۂ���B
	// �Q�ƃJ�E���^��0�ɂȂ��Ă���B
	// ����:
	//	length ������̒����B
	// �Ԓl:
	//	�쐬�����o�b�t�@�̕����񕔂̃A�h���X�B
	static TCHAR* createBuffer(size_t length) {
		size_t* count = (size_t*) new TCHAR[sizeof (size_t) + sizeof (TCHAR) * (length + 1)];
		*count = 0;
		return (TCHAR*) (count + 1);
	}
	// ��������i�[�����o�b�t�@���쐬����B
	// ����:
	//	source �i�[���镶����B
	// �Ԓl:
	//	�쐬�����o�b�t�@�̕����񕔂̃A�h���X�B
	static const TCHAR* create(const TCHAR* source) {
		return source != NULL ? create(source, _tcslen(source)) : NULL;
	}
	// ��������i�[�����o�b�t�@���쐬����B
	// ����:
	//	source �i�[���镶����B
	//	length ������̒����B
	// �Ԓl:
	//	�쐬�����o�b�t�@�̕����񕔂̃A�h���X�B
	static const TCHAR* create(const TCHAR* source, size_t length) {
		if (source != NULL) {
			TCHAR* buffer = createBuffer(length);
			memcpy(buffer, source, sizeof(TCHAR) * length);
			buffer[length] = '\0';
			return buffer;
		}
		return NULL;
	}
	// ��̕������A�����i�[�����o�b�t�@���쐬����B
	// ����:
	//	str1 �A�����镶����(�O)�B
	//	str2 �A�����镶����(��)�B
	// �Ԓl:
	//	�쐬�����o�b�t�@�̕����񕔂̃A�h���X�B
	static const TCHAR* concat(const TCHAR* str1, const TCHAR* str2) {
		size_t len1 = _tcslen(str1);
		size_t len2 = _tcslen(str2);
		TCHAR* buffer = createBuffer(len1 + len2);
		memcpy(buffer, str1, sizeof(TCHAR) * len1);
		memcpy(buffer + len1, str2, sizeof(TCHAR) * len2);
		buffer[len1 + len2] = '\0';
		return buffer;
	}
	// private methods
	// �Q�ƃJ�E���^�����炵�A0�ɂȂ�����o�b�t�@��j������B
	void release() {
		if (string != NULL) {
			size_t* count = (size_t*) string - 1;
			if (--*count == 0)
				delete[] (TCHAR*) count;
		}
	}
	// �Q�ƃJ�E���^�𑝂₷�B
	void add() {
		if (string != NULL) {
			size_t* count = (size_t*) string - 1;
			++*count;
		}
	}
	// �ʂ̃o�b�t�@�ƒu��������B
	// ���̃o�b�t�@�̎Q�ƃJ�E���^�����炵�A
	// �V�����o�b�t�@�̎Q�ƃJ�E���^�𑝂₷�B
	// ����:
	//	source �u��������V�����o�b�t�@�B
	void set(const TCHAR* source) {
		if (string != source) {
			release();
			string = source;
			add();
		}
	}
public:
	// constructor
	// �f�t�H���g�R���X�g���N�^�B
	// NULL�������Ă���̂ŁA���̂܂܂ŕ����񑀍삷��ƃA�N�Z�X�ᔽ�ɂȂ�̂Œ��ӁB
	String():string(NULL) {
	}
	// ���̕�������w�肷��R���X�g���N�^�B
	// ����:
	//	source ���̕�����B
	String(const TCHAR* source):string(NULL) {
		set(create(source));
	}
	// ���̕�����𒷂��t���Ŏw�肷��R���X�g���N�^�B
	// ����:
	//	source ���̕�����B
	//	length ������̒����B
	String(const TCHAR* source, size_t length):string(NULL) {
		set(create(source, length));
	}
	// �R�s�[�R���X�g���N�^�B
	// ����:
	//	source ���̕�����B
	String(const String& source):string(NULL) {
		set(source.string);
	}
	// ��̕������A������R���X�g���N�^�B
	// ����:
	//	str1 �O�ɂȂ镶����B
	//	str2 ��ɂȂ镶����B
	String(const TCHAR* str1, const TCHAR* str2):string(NULL) {
		set(concat(str1, str2));
	}
	// ��̕������A������R���X�g���N�^�B
	// ����:
	//	str1 �O�ɂȂ镶����B
	//	str2 ��ɂȂ镶����B
	String(const String& str1, const TCHAR* str2):string(NULL) {
		set(*str2 != '\0' ? concat(str1.string, str2) : str1.string);
	}
	// destructor
	// �f�X�g���N�^�B
	// �h�����邱�Ƃ͍l���Ă��Ȃ��̂ŉ��z�֐��ɂ͂��Ȃ��B
	~String() {
		release();
	}
	// public methods
	// ���̕�����̌�Ɏw��̕������A������B
	// ����:
	//	source �A�����镶����B
	// �Ԓl:
	//	�A�����ꂽ������B
	String concat(const TCHAR* source)const {
		return String(*this, source);
	}
	// ������Ƃ̔�r���s���B
	// NULL�Ƃ���r�ł���B
	// ����:
	//	str ��r���镶����B
	// �Ԓl:
	//	���������0�Astr�̕����傫����Ε��A��������ΐ��B
	int compareTo(const TCHAR* str)const {
		if (str == NULL)
			return string == NULL ? 0 : 1;
		else if (string == NULL)
			return -1;
		return _tcscmp(string, str);
	}
	// ������Ƃ̔�r��啶���������̋�ʂȂ��ōs���B
	// NULL�Ƃ���r�ł���B
	// ����:
	//	str ��r���镶����B
	// �Ԓl:
	//	���������0�Astr�̕����傫����Ε��A��������ΐ��B
	int compareToIgnoreCase(const TCHAR* str)const {
		if (str == NULL)
			return string == NULL ? 0 : 1;
		else if (string == NULL)
			return -1;
		return _tcsicmp(string, str);
	}
	// ������Ƃ̔�r���s���B
	// NULL�Ƃ���r�ł���B
	// ����:
	//	str ��r���镶����B
	// �Ԓl:
	//	��������ΐ^�B
	bool equals(const TCHAR* str)const {
		return compareTo(str) == 0;
	}
	// ������Ƃ̔�r��啶���������̋�ʂȂ��ōs���B
	// NULL�Ƃ���r�ł���B
	// ����:
	//	str ��r���镶����B
	// �Ԓl:
	//	��������ΐ^�B
	bool equalsIgnoreCase(const TCHAR* str)const {
		return compareToIgnoreCase(str) == 0;
	}
	// �w�肳�ꂽ������Ŏn�܂��Ă��邩�ǂ����𔻒肷��B
	// ����:
	//	str ��r���镶����B
	// �Ԓl:
	//	�w�肳�ꂽ������Ŏn�܂��Ă���ΐ^�B
	bool startsWith(const TCHAR* str)const {
		return startsWith(str, 0);
	}
	// �w��̈ʒu����w�肳�ꂽ������Ŏn�܂��Ă��邩�ǂ����𔻒肷��B
	// ����:
	//	str ��r���镶����B
	// �Ԓl:
	//	�w�肳�ꂽ������Ŏn�܂��Ă���ΐ^�B
	bool startsWith(const TCHAR* str, int offset)const {
		return _tcsncmp(string, str, _tcslen(str)) == 0;
	}
	// �w�肳�ꂽ������ŏI����Ă��邩�ǂ����𔻒肷��B
	// ����:
	//	str ��r���镶����B
	// �Ԓl:
	//	�w�肳�ꂽ������ŏI����Ă���ΐ^�B
	//
	bool endsWith(const TCHAR* str)const {
		size_t str_length = _tcslen(str);
		size_t string_length = length();
		if (string_length < str_length)
			return false;
		return _tcscmp(string + string_length - str_length, str) == 0;
	}
	// �w��̕������ǂ̈ʒu�ɂ��邩��T���B
	// ����:
	//	chr �T�������B
	// �Ԓl:
	//	�����̌��������C���f�b�N�X�B������Ȃ����-1�B
	int indexOf(TCHAR chr)const {
		return indexOf(chr, 0);
	}
	// �w��̕������ǂ̈ʒu�ɂ��邩���w��̈ʒu����T���B
	// ����:
	//	chr �T�������B
	//	from �T���n�߂�ʒu�B
	// �Ԓl:
	//	�����̌��������C���f�b�N�X�B������Ȃ����-1�B
	int indexOf(TCHAR chr, size_t from)const {
		if (from >= length())
			return -1;
		const TCHAR* found = _tcschr(string + from, chr);
		if (found == NULL)
			return -1;
		return found - string;
	}
	// �w��̕����񂪂ǂ̈ʒu�ɂ��邩��T���B
	// ����:
	//	str �T��������B
	// �Ԓl:
	//	������̌��������C���f�b�N�X�B������Ȃ����-1�B
	int indexOf(const TCHAR* str)const {
		return indexOf(str, 0);
	}
	// �w��̕����񂪂ǂ̈ʒu�ɂ��邩���w��̈ʒu����T���B
	// ����:
	//	str �T��������B
	//	from �T���n�߂�ʒu�B
	// �Ԓl:
	//	������̌��������C���f�b�N�X�B������Ȃ����-1�B
	//
	int indexOf(const TCHAR* str, size_t from)const {
		if (from >= length())
			return -1;
		const TCHAR* found = _tcsstr(string + from, str);
		if (found == NULL)
			return -1;
		return found - string;
	}
	// ������̒�����Ԃ��B
	size_t length()const {
		return _tcslen(string);
	}
	// �w��̕������Ō�Ɍ�����ʒu���擾����B
	// ����:
	//	chr �T�������B
	// �Ԓl:
	//	�����̌��������C���f�b�N�X�B������Ȃ����-1�B
	int lastIndexOf(TCHAR chr)const {
		return lastIndexOf(chr, (size_t) -1);
	}
	// �w��̕������w��̈ʒu�����O�ōŌ�Ɍ�����ʒu���擾����B
	// ����:
	//	chr �T�������B
	//	from �T���n�߂�ʒu�B
	// �Ԓl:
	//	�����̌��������C���f�b�N�X�B������Ȃ����-1�B
	int lastIndexOf(TCHAR chr, size_t from)const {
		size_t len = length();
		if (from > len - 1)
			from = len - 1;
		const TCHAR* s = string;
		const TCHAR* end = string + from;
		const TCHAR* found = NULL;
		while (*s != '0' && s <= end) {
			if (*s == chr)
				found = s;
#if !defined(UNICODE)
			if (isLeadByte(*s))
				s++;
#endif
			s++;
		}
		return found != NULL ? found - string : -1;
	}
	// �w��̕����񂪍Ō�Ɍ�����ʒu���擾����B
	// ����:
	//	str �T��������B
	// �Ԓl:
	//	������̌��������C���f�b�N�X�B������Ȃ����-1�B
	int lastIndexOf(const TCHAR* str)const {
		return lastIndexOf(str, (size_t) -1);
	}
	// �w��̕����񂪎w��̈ʒu�����O�ōŌ�Ɍ�����ʒu���擾����B
	// ����:
	//	str �T��������B
	//	from �T���n�߂�ʒu�B
	// �Ԓl:
	//	������̌��������C���f�b�N�X�B������Ȃ����-1�B
	int lastIndexOf(const TCHAR* str, size_t from)const {
		size_t len = length();
		size_t str_len = _tcslen(str);
		if (from > len - str_len)
			from = len - str_len;
		const TCHAR* s = string + from;
		while (s >= string) {
			if (_tcsncmp(s, str, str_len) == 0)
				return s - string;
			s--;
		}
		return -1;
	}
	// ������̈ꕔ�����o���B
	// ����:
	//	start ���o��������̐擪�̈ʒu�B
	// �Ԓl:
	//	������̈ꕔ�B
	String substring(int start)const {
		return String(string + start);
	}
	// ������̈ꕔ�����o���B
	// ����:
	//	start ���o��������̐擪�̈ʒu�B
	//	end ���o��������̌�̈ʒu�B
	// �Ԓl:
	//	������̈ꕔ�B
	String substring(int start, int end)const {
		return String(string + start, end - start);
	}
	// �w��̈ʒu�ɂ��镶�������o���B
	// ����:
	//	index ���o�������̈ʒu�B
	// �Ԓl:
	//	�w��̈ʒu�ɂ��镶���B
	TCHAR TCHARAt(size_t index)const {
		return index < length() ? string[index] : '\0';
	}
	// �w��̕������w��̕����ɒu�������܂��B
	// ����:
	//	oldChr ���̕����B
	//	newChr �u�������镶���B
	// �Ԓl:
	//	�u����̕�����B
	String replace(TCHAR oldChr, TCHAR newChr)const {
		String result(string);
		TCHAR* s = (TCHAR*) result.string;
		while (*s != '\0'){
#if !defined(UNICODE)
			if (String::isLeadByte(*s))
				s++;
			else
#endif
			if (*s == oldChr)
				*s = newChr;
			s++;
		}
		return result;
	}
	// �����񒆂̑啶�����������ɕϊ�����B
	// �Ԓl:
	//	�ϊ���̕�����B
	String toLowerCase()const {
		String result(string);
		TCHAR* s = (TCHAR*) result.string;
		while (*s != '\0'){
#if !defined(UNICODE)
			if (String::isLeadByte(*s))
				s++;
			else 
#endif
			if ('A' <= *s && *s <= 'Z')
				*s += 'a' - 'A';
			s++;
		}
		return result;
	}
	// �����񒆂̏�������啶���ɕϊ�����B
	// �Ԓl:
	//	�ϊ���̕�����B
	String toUpperCase()const {
		String result(string);
		TCHAR* s = (TCHAR*) result.string;
		while (*s != '\0'){
#if !defined(UNICODE)
			if (String::isLeadByte(*s))
				s++;
			else
#endif
			if (_T('a') <= *s && *s <= _T('z'))
				*s += _T('A') - _T('a');
			s++;
		}
		return result;
	}
	// ������̑O��̋󔒕������폜����B
	// �Ԓl:
	//	�폜��̕�����B
	String trim()const {
		const TCHAR* s = string;
		while (*s != '\0' && (TCHAR) *s <= _T(' '))
			s++;
		const TCHAR* start = s;
		s = string + length();
		while (s > start && (*s != '\0' && (TCHAR) *s <= _T(' ')))
			s--;
		return String(start, s - start);
	}

	// operators

	// const TCHAR*�ւ̃L���X�g���Z�q
	// �Ԓl:
	//	������ւ̃A�h���X�B
	operator const TCHAR*()const {
		return string;
	}
	// TCHAR�z��̂悤�Ɉ������߂�[]���Z�q�B
	// ����:
	//	index �擾���镶���̃C���f�b�N�X�B
	// �Ԓl:
	//	�w��̃C���f�b�N�X�ɂ��镶���B
	TCHAR operator[](size_t index)const {
		return TCHARAt(index);
	}
	// �������A�����邽�߂�+���Z�q�B
	// ����:
	//	source �A�����镶����B
	// �Ԓl:
	//	�A������������B
	String operator+(const TCHAR* source)const {
		return String(string, source);
	}
	// �������A�����邽�߂�+���Z�q�B
	// ����:
	//	source �A�����镶����B
	// �Ԓl:
	//	�A������������B
	String operator+(const String& source)const {
		return *string != '\0' ? String(string, source.string) : source;
	}
	// �������A�����邽�߂�+���Z�q�B
	// ����:
	//	str1 �A�����镶����(�O)�B
	//	str2 �A�����镶����(��)�B
	// �Ԓl:
	//	�A������������B
	friend String operator+(const TCHAR* str1, const String& str2) {
		return *str1 != '\0' ? String(str1, str2.string) : str2;
	}
	// ������Z�q�B
	// ����:
	//	source ������镶����B
	// �Ԓl:
	//	������ʁB
	String& operator=(const TCHAR* source) {
		set(create(source));
		return *this;
	}
	// ������Z�q�B
	// ����:
	//	source ������镶����B
	// �Ԓl:
	//	������ʁB
	String& operator=(const String& source) {
		set(source.string);
		return *this;
	}
	// �A���������ʂ������鉉�Z�q�B
	// ����:
	//	source �A�����镶����B
	// �Ԓl:
	//	�A�����ʁB
	String& operator+=(const TCHAR* source) {
		if (*source != '\0')
			set(concat(string, source));
		return *this;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕�����������ΐ^�B
	bool operator==(const String& str)const {
		return compareTo(str.string) == 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕�����������ΐ^�B
	bool operator==(const TCHAR* str)const {
		return compareTo(str) == 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str1 ��r���镶����B
	//	str2 ��r���镶����B
	// �Ԓl:
	//	str1���str2�̕�����������ΐ^�B
	friend bool operator==(const TCHAR* str1, const String& str2) {
		return str2.compareTo(str1) == 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕����������Ȃ���ΐ^�B
	bool operator!=(const String& str)const {
		return compareTo(str) != 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕����������Ȃ���ΐ^�B
	bool operator!=(const TCHAR* str)const {
		return compareTo(str) != 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str1 ��r���镶����B
	//	str2 ��r���镶����B
	// �Ԓl:
	//	str1���str2�̕����������Ȃ���ΐ^�B
	friend bool operator!=(const TCHAR* str1, const String& str2) {
		return str2.compareTo(str1) != 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕����傫����ΐ^�B
	bool operator<(const String& str)const {
		return compareTo(str) < 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕����傫����ΐ^�B
	bool operator<(const TCHAR* str)const {
		return compareTo(str) < 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str1 ��r���镶����B
	//	str2 ��r���镶����B
	// �Ԓl:
	//	str1���str2�̕����傫����ΐ^�B
	friend bool operator<(const TCHAR* str1, const String& str2) {
		return str2.compareTo(str1) > 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕����傫������������ΐ^�B
	bool operator<=(const String& str)const {
		return compareTo(str) <= 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕����傫������������ΐ^�B
	bool operator<=(const TCHAR* str)const {
		return compareTo(str) <= 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str1 ��r���镶����B
	//	str2 ��r���镶����B
	// �Ԓl:
	//	str1���str2�̕����傫������������ΐ^�B
	friend bool operator<=(const TCHAR* str1, const String& str2) {
		return str2.compareTo(str1) >= 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕�����������ΐ^�B
	bool operator>(const String& str)const {
		return compareTo(str) > 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕�����������ΐ^�B
	bool operator>(const TCHAR* str)const {
		return compareTo(str) > 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str1 ��r���镶����B
	//	str2 ��r���镶����B
	// �Ԓl:
	//	str1���str2�̕�����������ΐ^�B
	friend bool operator>(const TCHAR* str1, const String& str2) {
		return str2.compareTo(str1) < 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕�������������������ΐ^�B
	bool operator>=(const String& str)const {
		return compareTo(str) >= 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕�������������������ΐ^�B
	bool operator>=(const TCHAR* str)const {
		return compareTo(str) >= 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str1 ��r���镶����B
	//	str2 ��r���镶����B
	// �Ԓl:
	//	str1���str2�̕�������������������ΐ^�B
	friend bool operator>=(const TCHAR* str1, const String& str2) {
		return str2.compareTo(str1) <= 0;
	}

	// public utilities

	// 2�o�C�g�����̍ŏ���1�o�C�g���ǂ����𔻒肷��B
	// ����:
	//	���肷��o�C�g�B
	// �Ԓl:
	//	2�o�C�g�����̍ŏ���1�o�C�g�ł���ΐ^�B
#if !defined(UNICODE)
	static bool isLeadByte(TCHAR ch) {
	#ifdef _INC_WINDOWS
		return ::IsDBCSLeadByte(ch) != 0;
	#else
		return (ch & 0x80) != 0;
	#endif
	}
#endif
};

}

#endif//_YCL_STRING_H_
