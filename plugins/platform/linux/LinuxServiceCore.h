/*
 * LinuxServiceCore.h - declaration of LinuxServiceCore class
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

#include "LinuxCoreFunctions.h"
#include "PlatformSessionManager.h"
#include "ServiceDataManager.h"

class LinuxServerProcess;

// clazy:excludeall=copyable-polymorphic

class LinuxServiceCore : public QObject
{
	Q_OBJECT
public:
	explicit LinuxServiceCore( QObject* parent = nullptr );
	~LinuxServiceCore() override;

	void run();

private Q_SLOTS:
	void startServer( const QString& login1SessionId, const QDBusObjectPath& sessionObjectPath );
	void stopServer( const QString& login1SessionId, const QDBusObjectPath& sessionObjectPath );

private:
	static constexpr auto LoginManagerReconnectInterval = 3000;
	static constexpr auto SessionEnvironmentProbingInterval = 1000;
	static constexpr auto SessionStateProbingInterval = 1000;
	static constexpr auto ServerRestartInterval = 5000;

	void connectToLoginManager();
	void startServers();
	void startServer( const QString& sessionPath );
	void deferServerStart( const QString& sessionPath, int delay );
	void stopServer( const QString& sessionPath );
	void stopAllServers();

	void checkSessionState( const QString& sessionPath );

	LinuxCoreFunctions::DBusInterfacePointer m_loginManager{LinuxCoreFunctions::systemdLoginManager()};
	QMap<QString, LinuxServerProcess *> m_serverProcesses;
	QStringList m_deferredServerSessions;

	ServiceDataManager m_dataManager{};
	PlatformSessionManager m_sessionManager{};

};
