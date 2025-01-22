/*
 * VeyonConnection.h - declaration of class VeyonConnection
 *
 * Copyright (c) 2008-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <QPointer>

#include "RfbVeyonAuth.h"
#include "VncConnection.h"

class AuthenticationProxy;
class FeatureMessage;

class VEYON_CORE_EXPORT VeyonConnection : public QObject
{
	Q_OBJECT
public:
	VeyonConnection();

	void stopAndDeleteLater();

	VncConnection* vncConnection()
	{
		return m_vncConnection;
	}

	VncConnection::State state() const
	{
		return m_vncConnection->state();
	}

	bool isConnected() const
	{
		return m_vncConnection && m_vncConnection->isConnected();
	}

	void setVeyonAuthType( RfbVeyonAuth::Type authType )
	{
		m_veyonAuthType = authType;
	}

	RfbVeyonAuth::Type veyonAuthType() const
	{
		return m_veyonAuthType;
	}

	void setAuthenticationProxy( AuthenticationProxy* authenticationProxy )
	{
		m_authenticationProxy = authenticationProxy;
	}

	void sendFeatureMessage(const FeatureMessage& featureMessage);

	bool handleServerMessage( rfbClient* client, uint8_t msg );

	static constexpr auto VeyonConnectionTag = 0xFE14A11;


Q_SIGNALS:
	void featureMessageReceived( const FeatureMessage& );

private:
	~VeyonConnection() override;

	void registerConnection();
	void unregisterConnection();

	// authentication
	static int8_t handleSecTypeVeyon( rfbClient* client, uint32_t authScheme );
	static void hookPrepareAuthentication( rfbClient* client );

	AuthenticationCredentials authenticationCredentials() const;

	VncConnection* m_vncConnection{new VncConnection};

	RfbVeyonAuth::Type m_veyonAuthType;

	AuthenticationProxy* m_authenticationProxy{nullptr};

} ;
