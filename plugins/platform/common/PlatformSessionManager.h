/*
 * PlatformSessionManager.h - declaration of PlatformSessionManager class
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

#pragma once

#include <QMutex>
#include <QThread>

#include "PlatformSessionFunctions.h"

class QLocalSocket;

// clazy:excludeall=copyable-polymorphic

class PlatformSessionManager : public QThread
{
	Q_OBJECT
public:
	enum class Mode
	{
		Local,
		Active,
		Multi
	};
	Q_ENUM(Mode)

	using SessionId = PlatformSessionFunctions::SessionId;
	using PlatformSessionId = QString;

	PlatformSessionManager( QObject* parent = nullptr );
	~PlatformSessionManager() override;

	SessionId openSession( const PlatformSessionId& platformSessionId );
	void closeSession( const PlatformSessionId& platformSessionId );

	static SessionId resolveSessionId( const PlatformSessionId& platformSessionId );

	Mode mode() const
	{
		return m_mode;
	}

	int maximumSessionCount() const
	{
		return m_maximumSessionCount;
	}

	using SessionMap = QMap<PlatformSessionId, SessionId>;

protected:
	void run() override;

private:
	static constexpr auto ServerConnectTimeout = 5000;
	static constexpr auto SocketWaitTimeout = 1000;
	static constexpr auto MessageReadTimeout = 10000;

	static QString serverName()
	{
		return QStringLiteral("VeyonSessionManager");
	}

	static bool waitForMessage( QLocalSocket* socket );

	const Mode m_mode;
	const int m_maximumSessionCount;

	QMutex m_mutex;
	QVariantMap m_sessions;

};
