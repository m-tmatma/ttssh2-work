/*
 * $Id: StringBuffer.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_STRINGBUFFER_H_
#define _YCL_STRINGBUFFER_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/common.h>

#include <YCL/String.h>

#include <malloc.h>
#include <tchar.h>

namespace yebisuya {

// �ϒ��̕�������������߂̃N���X�B
class StringBuffer {
private:
	// ��������i�[����o�b�t�@�B
	TCHAR* buffer;
	// ���ݗL���ȕ�����̒����B
	size_t validLength;
	// �o�b�t�@�̑傫���B
	size_t bufferSize;
	enum {
		// �o�b�t�@���L����ۂɎg�p����T�C�Y�B
		INIT_CAPACITY = 16,
	};
	// �o�b�t�@������������B
	// ����:
	//	source	����������B
	//	length	����������̒����B
	//	capacity	�o�b�t�@�̏����T�C�Y�B
	void init(const TCHAR* source, size_t length, size_t capacity) {
		if ((capacity != 0 || length != 0) && capacity < length + INIT_CAPACITY)
			capacity = length + INIT_CAPACITY;
		validLength = length;
		bufferSize = capacity;
		if (bufferSize == 0) {
			buffer = NULL;
		}else{
			buffer = new TCHAR[bufferSize];
		}
		memcpy(buffer, source, sizeof(TCHAR) * validLength);
		memset(buffer + validLength, '\0', sizeof(TCHAR) * (bufferSize - validLength));
	}
public:
	// �f�t�H���g�R���X�g���N�^�B
	StringBuffer() {
		init(NULL, 0, 0);
	}
	// �o�b�t�@�̏����T�C�Y���w�肷��R���X�g���N�^�B
	// ����:
	//	capacity �o�b�t�@�̏����T�C�Y�B
	StringBuffer(size_t capacity) {
		init(NULL, 0, capacity);
	}
	// �o�b�t�@�̏�����������w�肷��R���X�g���N�^�B
	// ����:
	//	source	����������B
	StringBuffer(const TCHAR* source) {
		init(source, _tcslen(source), 0);
	}
	// �R�s�[�R���X�g���N�^�B
	// ����:
	//	source	����������B
	StringBuffer(const StringBuffer& source) {
		init(source.buffer, source.validLength, source.bufferSize);
	}
	// �f�X�g���N�^�B
	~StringBuffer() {
		delete[] buffer;
	}

	// ���ݗL���ȕ�����̒������擾����B
	// �Ԓl:
	//	�L���ȕ�����̒����B
	size_t length()const {
		return validLength;
	}
	// �o�b�t�@�̃T�C�Y���擾����B
	// �Ԓl:
	//	�o�b�t�@�̃T�C�Y�B
	size_t capacity()const {
		return bufferSize;
	}
	// �o�b�t�@�̃T�C�Y���w��̒��������܂�悤�ɒ��߂���B
	// ����:
	//	newLength	���߂��钷���B
	void ensureCapacity(size_t newLength) {
		if (bufferSize < newLength) {
			TCHAR* oldBuffer = buffer;
			init(oldBuffer, validLength, newLength + INIT_CAPACITY);
			delete[] oldBuffer;
		}
	}
	// �L���ȕ����񒷂�ύX����B
	// ����:
	//	newLength	�V���������񒷁B
	void setLength(size_t newLength) {
		if (validLength < newLength)
			ensureCapacity(newLength);
		validLength = newLength;
	}
	// �w��̈ʒu�̕������擾����B
	// ����:
	//	index	�����̈ʒu�B
	// �Ԓl:
	//	�w��̈ʒu�̕����B
	TCHAR charAt(size_t index)const {
		return index < validLength ? buffer[index] : '\0';
	}
	// �w��̈ʒu�̕������擾����B
	// ����:
	//	index	�����̈ʒu�B
	// �Ԓl:
	//	�w��̈ʒu�̕����̎Q�ƁB
	TCHAR& charAt(size_t index) {
		if (index >= validLength) {
			ensureCapacity(validLength + 1);
			index = validLength++;
		}
		return buffer[index];
	}
	// �w��̈ʒu�̕�����ύX����B
	// ����:
	//	index	�ύX���镶���̈ʒu�B
	//	chr	�ύX���镶���B
	void setCharAt(int index, TCHAR chr) {
		charAt(index) = chr;
	}
	// ������ǉ�����B
	// ����:
	//	chr	�ǉ����镶���B
	// �Ԓl:
	//	�ǉ����ʁB
	StringBuffer& append(TCHAR chr) {
		charAt(validLength) = chr;
		return *this;
	}
	// �������ǉ�����B
	// ����:
	//	source	�ǉ����镶����B
	// �Ԓl:
	//	�ǉ����ʁB
	StringBuffer& append(const TCHAR* source) {
		return append(source, _tcslen(source));
	}
	// �������ǉ�����B
	// ����:
	//	source	�ǉ����镶����B
	//	length	�ǉ����镶����̒����B
	// �Ԓl:
	//	�ǉ����ʁB
	StringBuffer& append(const TCHAR* source, size_t length) {
		size_t oldLength = validLength;
		ensureCapacity(validLength + length);
		memcpy(buffer + oldLength, source, sizeof(TCHAR) * length);
		validLength += length;
		return *this;
	}
	// �w��̈ʒu�̕������폜����B
	// ����:
	//	start	�폜����ʒu�B
	// �Ԓl:
	//	�폜���ʁB
	StringBuffer& remove(size_t index) {
		return remove(index, index + 1);
	}
	// �w��̈ʒu�̕�������폜����B
	// ����:
	//	start	�폜����擪�ʒu�B
	//	end	�폜����I���̈ʒu�B
	// �Ԓl:
	//	�폜���ʁB
	StringBuffer& remove(size_t start, size_t end) {
		if (start <= 0)
			start = 0;
		if (start < end) {
			if (end < validLength){
				memcpy(buffer + start, buffer + end, sizeof(TCHAR) * (validLength - end));
				validLength -= end - start;
			}else{
				validLength = start;
			}
		}
		return *this;
	}
	// �w��̈ʒu�̕������u������B
	// ����:
	//	start	�u������擪�ʒu�B
	//	end	�u������I���̈ʒu�B
	//	source	�u�����镶����B
	// �Ԓl:
	//	�u�����ʁB
	StringBuffer& replace(size_t start, size_t end, const TCHAR* source) {
		if (end > validLength)
			end = validLength;
		if (start < end) {
			size_t length = _tcslen(source);
			size_t oldLength = validLength;
			ensureCapacity(validLength += length - (end - start));
			memcpy(buffer + start + length, buffer + end, sizeof(TCHAR) * (oldLength - end));
			memcpy(buffer + start, source, sizeof(TCHAR) * length);
		}
		return *this;
	}
	// �w��̈ʒu�̕�������擾����B
	// ����:
	//	start	�擾���镶����̐擪�ʒu�B
	// �Ԓl:
	//	�w��̈ʒu�̕�����B
	String substring(size_t index)const {
		return String(buffer + index, validLength - index);
	}
	// �w��̈ʒu�̕�������擾����B
	// ����:
	//	start	�擾���镶����̐擪�ʒu�B
	//	end	�擾���镶����̏I���̈ʒu�B
	// �Ԓl:
	//	�w��̈ʒu�̕�����B
	String substring(size_t start, size_t end)const {
		if (end > validLength)
			end = validLength;
		return String(buffer + start, end - start);
	}
	// �w��̈ʒu�ɕ�����}������B
	// ����:
	//	index	�}������ʒu�B
	//	source	�}�����镶���B
	// �Ԓl:
	//	�}�����ʁB
	StringBuffer& insert(size_t index, TCHAR chr) {
		return insert(index, &chr, 1);
	}
	// �w��̈ʒu�ɕ������}������B
	// ����:
	//	index	�}������ʒu�B
	//	source	�}�����镶����B
	// �Ԓl:
	//	�}�����ʁB
	StringBuffer& insert(size_t index, const TCHAR* source) {
		return insert(index, source, _tcslen(source));
	}
	// �w��̈ʒu�ɕ������}������B
	// ����:
	//	index	�}������ʒu�B
	//	source	�}�����镶����B
	//	length	������̒����B
	// �Ԓl:
	//	�}�����ʁB
	StringBuffer& insert(size_t index, const TCHAR* source, size_t length) {
		if (index >= validLength)
			index = validLength;
		size_t oldLength = validLength;
		ensureCapacity(validLength + length);
		TCHAR* temp = (TCHAR*) alloca(sizeof(TCHAR) * (oldLength - index));
		memcpy(temp, buffer + index, sizeof(TCHAR) * (oldLength - index));
		memcpy(buffer + index, source, sizeof(TCHAR) * length);
		memcpy(buffer + index + length, temp, sizeof(TCHAR) * (oldLength - index));
		validLength += length;
		return *this;
	}
	// ������𔽓]����B
	// �Ԓl:
	//	���]���ʁB
	StringBuffer& reverse() {
		TCHAR* temporary = (TCHAR*) alloca(sizeof (TCHAR) * validLength);
		TCHAR* dst = temporary + validLength;
		TCHAR* src = buffer;
		while (temporary < dst) {
#if !defined(UNICODE)
			if (String::isLeadByte(*src)) {
				TCHAR pre = *src++;
				*--dst = *src++;
				*--dst = pre;
			}else
#endif
			{
				*--dst = *src++;
			}
		}
		memcpy(buffer, temporary, sizeof(TCHAR) * validLength);
		return *this;
	}
	// ��������擾����B
	// �Ԓl:
	//	���ݐݒ肳��Ă��镶����B
	String toString()const {
		return String(buffer, validLength);
	}

	// �ꕶ�������̕�����ɕύX����B
	// ����:
	//	�ύX����ꕶ���B
	// �Ԓl:
	//	�ύX���ʁB
	StringBuffer& set(TCHAR chr) {
		ensureCapacity(1);
		buffer[0] = chr;
		validLength = 1;
		return *this;
	}
	// �w��̕�����ɕύX����B
	// ����:
	//	source	�ύX���镶����B
	// �Ԓl:
	//	�ύX���ʁB
	StringBuffer& set(const TCHAR* source) {
		size_t length = _tcslen(source);
		ensureCapacity(validLength = length);
		memcpy(buffer, source, sizeof(TCHAR) * length);
		return *this;
	}

	// TCHAR*�ɕϊ�����L���X�g���Z�q�B
	// �o�b�t�@�̃A�h���X���擾����B
	// �Ԓl:
	//	�o�b�t�@�̃A�h���X�B
	operator TCHAR*() {
		return buffer;
	}
	// String�ɕϊ�����L���X�g���Z�q�B
	// ��������擾����B
	// �Ԓl:
	//	���ݐݒ肳��Ă��镶����B
	operator String()const {
		return toString();
	}
	// ������Z�q�B
	// �ꕶ�������̕�����ɕύX����B
	// ����:
	//	ch	�ύX����ꕶ���B
	// �Ԓl:
	//	������ʁB
	StringBuffer& operator=(TCHAR ch) {
		return set(ch);
	}
	// ������Z�q�B
	// �w��̕�����ɕύX����B
	// ����:
	//	source	�ύX���镶����B
	// �Ԓl:
	//	������ʁB
	StringBuffer& operator=(const TCHAR* source) {
		return set(source);
	}
	// �A��������Z�q�B
	// ������ǉ�����B
	// ����:
	//	ch	�ǉ����镶���B
	// �Ԓl:
	//	������ʁB
	StringBuffer& operator+=(TCHAR ch) {
		return append(ch);
	}
	// �A��������Z�q�B
	// �������ǉ�����B
	// ����:
	//	source	�ǉ����镶����B
	// �Ԓl:
	//	������ʁB
	StringBuffer& operator+=(const TCHAR* source) {
		return append(source);
	}
};

}

#endif//_YCL_STRINGBUFFER_H_
