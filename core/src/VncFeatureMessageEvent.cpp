/*
 * FeatureMessageEvent.cpp - implementation of FeatureMessageEvent
 *
 * Copyright (c) 2018-2025 Tobias Junghans <tobydox@veyon.io>
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

#include "rfb/rfbclient.h"

#include "SocketDevice.h"
#include "VncConnection.h"
#include "VncFeatureMessageEvent.h"


VncFeatureMessageEvent::VncFeatureMessageEvent( const FeatureMessage& featureMessage ) :
	m_featureMessage( featureMessage )
{
}



void VncFeatureMessageEvent::fire( rfbClient* client )
{
	vDebug() << qUtf8Printable(QStringLiteral("%1:%2").arg(QString::fromUtf8(client->serverHost)).arg(client->serverPort))
			 << m_featureMessage;

	SocketDevice socketDevice( VncConnection::libvncClientDispatcher, client );

	m_featureMessage.sendAsRfbMessage(&socketDevice);
}
