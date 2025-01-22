/*
 * VariantStream.h - read/write QVariant objects to/from QIODevice
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include <QDataStream>

#include "VeyonCore.h"

class VEYON_CORE_EXPORT VariantStream
{
public:
	static constexpr auto MaxByteArraySize = 16*1024*1024;
	static constexpr auto MaxStringSize = 64*1024;
	static constexpr auto MaxContainerSize = 1024;
	static constexpr auto MaxCheckRecursionDepth = 3;

	explicit VariantStream( QIODevice* ioDevice );

	QVariant read(); // Flawfinder: ignore

	void write( const QVariant& v );

private:
	bool checkBool();
	bool checkByteArray();
	bool checkInt();
	bool checkLong();
	bool checkRect();
	bool checkString();
	bool checkStringList();
	bool checkUuid();
	bool checkVariant(int depth);
	bool checkVariantList(int depth);
	bool checkVariantMap(int depth);

	QDataStream m_dataStream;

} ;
