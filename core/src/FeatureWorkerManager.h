/*
 * FeatureWorkerManager.h - class for managing feature worker instances
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

#include <QPointer>
#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#include <QRecursiveMutex>
#else
#include <QMutex>
#endif

#include "FeatureMessage.h"

class FeatureManager;
class VeyonServerInterface;

class VEYON_CORE_EXPORT FeatureWorkerManager : public QObject
{
	Q_OBJECT
public:
	FeatureWorkerManager( VeyonServerInterface& server, QObject* parent = nullptr );
	~FeatureWorkerManager() override;

	bool startManagedSystemWorker( Feature::Uid featureUid );
	bool startUnmanagedSessionWorker( Feature::Uid featureUid );

	bool stopWorker( Feature::Uid featureUid );

	void sendMessageToManagedSystemWorker( const FeatureMessage& message );
	void sendMessageToUnmanagedSessionWorker( const FeatureMessage& message );

	bool isWorkerRunning( Feature::Uid featureUid );

private:
	void acceptConnection();
	void processConnection( QTcpSocket* socket );
	void closeConnection( QTcpSocket* socket );

	void sendMessage( const FeatureMessage& message );

	void sendPendingMessages();

	static constexpr auto UnmanagedSessionProcessRetryInterval = 5000;

	VeyonServerInterface& m_server;
	QTcpServer m_tcpServer;

	struct Worker
	{
		QPointer<QTcpSocket> socket;
		QPointer<QProcess> process;
		QList<FeatureMessage> pendingMessages;
	};

	using WorkerMap = QMap<Feature::Uid, Worker>;
	WorkerMap m_workers;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	QRecursiveMutex m_workersMutex;
#else
	QMutex m_workersMutex{QMutex::Recursive};
#endif

} ;
