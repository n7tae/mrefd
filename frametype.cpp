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

#include <cassert>
#include "frametype.h"

uint16_t CFrameType::GetFrameType(EVersionType vt)
{
	if (EVersionType::v3 == vt)
		return buildV3();
	else
		return buildDeprecated();
}

void CFrameType::SetFrameType(uint16_t t)
{
	if (0xf000u & t)	// this has to be V#3 TYPE
	{
		// internalize the V#3 TYPE
		m_version = EVersionType::v3;
		switch ((t >> 12) & 0xfu)	// 1. Do the payload
		{
		case 1u:
			m_payload = EPayloadType::dataonly;
			break;
		default:
		case 2u:
			m_payload = EPayloadType::c2_3200;
			break;
		case 3u:
			m_payload = EPayloadType::c2_1600;
			break;
		case 15u:
			m_payload = EPayloadType::packet;
			break;
		}
		switch ((t >> 9) & 0x3u)	// 2. Do the encryption
		{
		default:
		case 0u:
			m_encrypt = EEncryptType::none;
			break;
		case 1u:
			m_encrypt = EEncryptType::scram8;
			break;
		case 2u:
			m_encrypt = EEncryptType::scram16;
			break;
		case 3u:
			m_encrypt = EEncryptType::scram24;
			break;
		case 4u:
			m_encrypt = EEncryptType::aes128;
			break;
		case 5u:
			m_encrypt = EEncryptType::aes192;
			break;
		case 6u:
			m_encrypt = EEncryptType::aes256;
			break;
		}
		m_isSigned = (t & 0x100u) ? true : false;	// 3. The digital signing
		switch ((t >> 4) & 0xfu)					// 4. What's in the META
		{
		default:
		case 0u:
			m_metatype = EMetaDatType::none;
			break;
		case 1u:
			m_metatype = EMetaDatType::gnss;
			break;
		case 2u:
			m_metatype = EMetaDatType::ecd;
			break;
		case 3u:
			m_metatype = EMetaDatType::text;
			break;
		case 4u:
			m_metatype = EMetaDatType::aes;
			break;
		}
		m_can = t & 0xfu;	// 5. Get the CAN
	} else {	// and this had to be a deprecated TYPE
		m_version = EVersionType::deprecated;
		// 1 Get the payload type
		if (t & 1u)
		{
			switch ((t >> 1) & 0x3u)
			{
			case 1u:
				m_payload = EPayloadType::dataonly;
				break;
			default:
			case 2u:
				m_payload = EPayloadType::c2_3200;
				break;
			case 3u:
				m_payload = EPayloadType::c2_1600;
				break;
			}
		} else {
			m_payload = EPayloadType::packet;
		}
		uint8_t subtype = (t >> 5) & 0x3u;	// get the subtype field
		switch ((t >> 3) & 0x3u) // get the encrypt field
		{
		case 0u:	// no encryption
			switch (subtype)
			{
			default:
			case 0u:
				m_metatype = EMetaDatType::none;
				break;
			case 1u:
				m_metatype = EMetaDatType::gnss;
				break;
			case 2u:
				m_metatype = EMetaDatType::ecd;
				break;
			}
			break;
		case 1u: // scrambler
			switch (subtype)
			{
			default:
			case 0u:
				m_encrypt = EEncryptType::scram8;
				break;
			case 1u:
				m_encrypt = EEncryptType::scram16;
				break;
			case 2u:
				m_encrypt = EEncryptType::scram24;
				break;
			}
			break;
		case 2u: // aes
			switch (subtype)
			{
			default:
			case 0u:
				m_encrypt = EEncryptType::aes128;
				break;
			case 1u:
				m_encrypt = EEncryptType::aes192;
				break;
			case 2u:
				m_encrypt = EEncryptType::aes256;
				break;
			}
			break;
		}
		m_isSigned = (t & 0x800u) ? true : false;
		m_can = (t >> 7) & 0xfu;
	}
}

uint16_t CFrameType::buildDeprecated()
{
	uint16_t type { 0 };
	// payload type
	switch (m_payload)
	{
	case EPayloadType::packet:
		type = 0u;
		break;
	case EPayloadType::dataonly:
		type = 3u;
		break;
	case EPayloadType::c2_3200:
		type = 5u;
		break;
	case EPayloadType::c2_1600:
		type = 7u;
		break;
	}

	switch (m_encrypt)
	{
	case EEncryptType::none:
		break;
	case EEncryptType::scram8:
		type |= 0x8u;
		break;
	case EEncryptType::scram16:
		type |= 0x28u;
		break;
	case EEncryptType::scram24:
		type |= 0x48u;
		break;
	case EEncryptType::aes128:
		type |= 0x18u;
		break;
	case EEncryptType::aes192:
		type |= 0x38u;
		break;
	case EEncryptType::aes256:
		type |= 0x58u;
		break;
	}

	switch (m_metatype)
	{
	default:
		break;
	case EMetaDatType::gnss:
		type |= 0x20;
		break;
	case EMetaDatType::ecd:
		type |= 0x40u;
	}

	type |= (m_can << 7);

	if (m_isSigned)
		type |= 0x800u;
	return type;
}

uint16_t CFrameType::buildV3()
{
	uint16_t type { 0 };
	switch (m_payload)
	{
		case EPayloadType::packet:
			type = 0xfu;
			break;
		case EPayloadType::dataonly:
			type = 0x1u;
			break;
		default:
		case EPayloadType::c2_3200:
			type = 0x2u;
			break;
		case EPayloadType::c2_1600:
			type = 0x3u;
	}
	type <<= 3;
	switch (m_encrypt)
	{
	default:
	case EEncryptType::none:
		break;
	case EEncryptType::scram8:
		type |= 1u;
		break;
	case EEncryptType::scram16:
		type |= 2u;
		break;
	case EEncryptType::scram24:
		type |= 3u;
		break;
	case EEncryptType::aes128:
		type |= 4u;
		break;
	case EEncryptType::aes192:
		type |= 5u;
		break;
	case EEncryptType::aes256:
		type |= 6u;
	}
	type <<= 1;
	if (m_isSigned)
		type |= 1u;
	type <<= 4;
	switch (m_metatype)
	{
	default:
	case EMetaDatType::none:
		break;
	case EMetaDatType::gnss:
		type |= 1u;
		break;
	case EMetaDatType::ecd:
		type |= 2u;
		break;
	case EMetaDatType::text:
		type |= 3u;
		break;
	case EMetaDatType::aes:
		type |= 0xf;
		break;
	}
	type <<= 4;
	type |= m_can;
	return type;
}
