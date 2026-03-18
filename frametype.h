/*
	     A useable TYPE buider, interpreter and converter
				Copyright (C) 2026 Thomas A. Early

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstring>
#include <cstdint>

enum class EPayloadType { dataonly, c2_3200, c2_1600, packet };
enum class EEncryptType { none, scram8, scram16, scram24, aes128, aes192, aes256 };
enum class EMetaDatType { none, gnss, ecd, text, aes };
enum class EVersionType { v3, deprecated };

class CFrameType
{
public:
	CFrameType()
		 : m_payload(EPayloadType::c2_3200)
		 , m_encrypt(EEncryptType::none)
		 , m_isSigned(false)
		 , m_metatype(EMetaDatType::none)
		 , m_can(0)
		 , m_version(EVersionType::v3)
		 {}
	CFrameType(uint16_t t) { SetFrameType(t); }
	void SetFrameType(uint16_t t);
	uint16_t GetFrameType(EVersionType vt);
	uint16_t GetOriginType() { return GetFrameType(m_version);  }
	EPayloadType GetPayloadType()  const { return m_payload;  }
	EEncryptType GetEncryptType()  const { return m_encrypt;  }
	EMetaDatType GetMetaDataType() const { return m_metatype; }
	bool GetIsSigned() const { return m_isSigned; }
	EVersionType GetVersion() const { return m_version; }
	uint8_t GetCan() const { return m_can; }
	void SetPayloadType(EPayloadType t)  { m_payload = t; }
	void SetEncryptType(EEncryptType t)  { m_encrypt = t; }
	void SetMetaDataType(EMetaDatType t) { m_metatype = t; }
	void SetSigned(bool issigned) { m_isSigned = issigned; }
	void SetCan(uint8_t can)      { m_can = can; }

private:
	EPayloadType m_payload;
	EEncryptType m_encrypt;
	bool m_isSigned;
	EMetaDatType m_metatype;
	uint8_t m_can;
	EVersionType m_version;

	uint16_t buildDeprecated();
	uint16_t buildV3();
};
