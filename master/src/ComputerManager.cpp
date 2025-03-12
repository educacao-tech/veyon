/*
 * ComputerManager.cpp - maintains and provides a computer object list
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

#include <QCoreApplication>
#include <QFile>
#include <QHostAddress>
#include <QHostInfo>
#include <QMessageBox>
#include <QTime>

#include "ComputerManager.h"
#include "VeyonConfiguration.h"
#include "NetworkObject.h"
#include "NetworkObjectDirectory.h"
#include "NetworkObjectDirectoryManager.h"
#include "NetworkObjectFilterProxyModel.h"
#include "NetworkObjectOverlayDataModel.h"
#include "NetworkObjectTreeModel.h"
#include "PlatformFilesystemFunctions.h"
#include "UserConfig.h"


ComputerManager::ComputerManager( UserConfig& config, QObject* parent ) :
	QObject( parent ),
	m_config( config ),
	m_networkObjectDirectory( VeyonCore::networkObjectDirectoryManager().configuredDirectory() ),
	m_networkObjectModel( new NetworkObjectTreeModel( m_networkObjectDirectory, this ) ),
	m_networkObjectOverlayDataModel(new NetworkObjectOverlayDataModel({tr("User"), tr("Logged in since")}, this)),
	m_computerTreeModel( new CheckableItemProxyModel( NetworkObjectModel::UidRole, this ) ),
	m_networkObjectFilterProxyModel( new NetworkObjectFilterProxyModel( this ) ),
	m_localHostNames( QHostInfo::localHostName().toLower() ),
	m_localHostAddresses(QHostInfo::fromName(QHostInfo::localHostName()).addresses()),
	m_computerNameSource(VeyonCore::config().computerNameSource())
{
	if( m_networkObjectDirectory == nullptr )
	{
		QMessageBox::critical( nullptr,
							   tr( "Missing network object directory plugin" ),
							   tr( "No default network object directory plugin was found. "
								   "Please check your installation or configure a different "
								   "network object directory backend via %1 Configurator." ).
							   arg( VeyonCore::applicationName() ) );
		qFatal( "ComputerManager: missing network object directory plugin!" );
	}

	if( QHostInfo::localDomainName().isEmpty() == false )
	{
		m_localHostNames.append( QHostInfo::localHostName().toLower() +
								 QStringLiteral( "." ) +
								 QHostInfo::localDomainName().toLower() );
	}

	initNetworkObjectLayer();
	initLocations();
	initComputerTreeModel();
}



ComputerManager::~ComputerManager()
{
	m_config.setCheckedNetworkObjects( m_computerTreeModel->saveStates() );
}



void ComputerManager::addLocation( const QString& location )
{
	m_locationFilterList.append( location );

	updateLocationFilterList();
}



void ComputerManager::removeLocation( const QString& location )
{
	if( m_currentLocations.contains( location ) == false )
	{
		m_locationFilterList.removeAll( location );

		updateLocationFilterList();
	}
}



bool ComputerManager::saveComputerAndUsersList( const QString& fileName )
{
	QStringList lines( tr( "Computer name;Hostname;User" ) );

	const auto computers = selectedComputers( QModelIndex() );

	for( const auto& computer : computers )
	{
		const auto networkObjectIndex = findNetworkObject( computer.networkObjectUid() );
		if( networkObjectIndex.isValid() )
		{
			// fetch user
			const auto user = m_networkObjectOverlayDataModel->data( mapToUserNameModelIndex( networkObjectIndex ) ).toString();
			// create new line with computer and user
			lines += computer.displayName() + QLatin1Char(';') + computer.hostAddress() + QLatin1Char(';') + user; // clazy:exclude=reserve-candidates
		}
	}

	// append empty string to generate final newline at end of file
	lines += QString();

	QFile outputFile( fileName );
	if( VeyonCore::platform().filesystemFunctions().openFileSafely(
			&outputFile,
			QFile::WriteOnly | QFile::Truncate,
			QFile::ReadOwner | QFile::WriteOwner ) == false )
	{
		return false;
	}

	outputFile.write( lines.join( QStringLiteral("\r\n") ).toUtf8() );

	return true;
}



void ComputerManager::updateUser(const ComputerControlInterface::Pointer& controlInterface) const
{
	const auto networkObjectIndex = findNetworkObject( controlInterface->computer().networkObjectUid() );

	if( networkObjectIndex.isValid() )
	{
		auto user = controlInterface->userFullName();
		if( user.isEmpty() )
		{
			user = controlInterface->userLoginName();
		}
		m_networkObjectOverlayDataModel->setData(mapToUserNameModelIndex(networkObjectIndex), user);

		QString computerName;
		switch (m_computerNameSource)
		{
		case NetworkObjectDirectory::ComputerNameSource::UserFullName:
			computerName = controlInterface->userFullName();
			break;
		case NetworkObjectDirectory::ComputerNameSource::UserLoginName:
			computerName = controlInterface->userLoginName();
			break;
		default:
			break;
		}

		if (computerName.isEmpty() == false)
		{
			m_networkObjectOverlayDataModel->setData(m_networkObjectOverlayDataModel->mapFromSource(networkObjectIndex),
													 computerName);
		}
	}
}



void ComputerManager::updateSessionInfo(const ComputerControlInterface::Pointer& controlInterface) const
{
	const auto networkObjectIndex = findNetworkObject(controlInterface->computer().networkObjectUid());

	if (networkObjectIndex.isValid())
	{
		const auto uptime24h = controlInterface->sessionInfo().uptime % (60*60*24);
		const auto uptimeDays = controlInterface->sessionInfo().uptime / (60*60*24);
		const QString uptimeString = (uptimeDays > 0 ?
										  ((uptimeDays > 1 ? tr("%1 days").arg(uptimeDays) : tr("1 day")) + QStringLiteral(", ")) :
										  QString()) +
									 QTime::fromMSecsSinceStartOfDay(uptime24h * 1000).toString(QStringLiteral("hh:mm:ss"));
		m_networkObjectOverlayDataModel->setData(mapToSessionUptimeModelIndex(networkObjectIndex),
												  uptimeString,
												  Qt::DisplayRole);
		QString computerName;

		switch (m_computerNameSource)
		{
		case NetworkObjectDirectory::ComputerNameSource::HostAddress:
			computerName = controlInterface->computer().hostAddress();
			break;
		case NetworkObjectDirectory::ComputerNameSource::SessionClientName:
			computerName = controlInterface->sessionInfo().clientName;
			break;
		case NetworkObjectDirectory::ComputerNameSource::SessionClientAddress:
			computerName = controlInterface->sessionInfo().clientAddress;
			break;
		case NetworkObjectDirectory::ComputerNameSource::SessionHostName:
			computerName = controlInterface->sessionInfo().hostName;
			break;
		case NetworkObjectDirectory::ComputerNameSource::SessionMetaData:
			computerName = controlInterface->sessionInfo().metaData;
			break;
		default:
			break;
		}

		if (computerName.isEmpty() == false)
		{
			m_networkObjectOverlayDataModel->setData(m_networkObjectOverlayDataModel->mapFromSource(networkObjectIndex),
													 computerName);
		}
	}
}



void ComputerManager::clearOverlayModelData(const ComputerControlInterface::Pointer& controlInterface) const
{
	const auto networkObjectIndex = findNetworkObject(controlInterface->computer().networkObjectUid());

	if (networkObjectIndex.isValid())
	{
		m_networkObjectOverlayDataModel->setData(mapToUserNameModelIndex(networkObjectIndex), {});
		m_networkObjectOverlayDataModel->setData(mapToSessionUptimeModelIndex(networkObjectIndex), {});
		m_networkObjectOverlayDataModel->setData(m_networkObjectOverlayDataModel->mapFromSource(networkObjectIndex), {});
	}
}



void ComputerManager::checkChangedData( const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles )
{
	Q_UNUSED(topLeft);
	Q_UNUSED(bottomRight);

	if( roles.contains( Qt::CheckStateRole ) )
	{
		Q_EMIT computerSelectionChanged();
	}
}



void ComputerManager::initLocations()
{
	for( const auto& hostName : std::as_const( m_localHostNames ) )
	{
		vDebug() << "initializing locations for hostname" << hostName;
	}

	for( const auto& address : std::as_const( m_localHostAddresses ) )
	{
		vDebug() << "initializing locations for host address" << address.toString();
	}

	const auto currentLocation = findLocationOfComputer(m_localHostNames, m_localHostAddresses, {});
	if (currentLocation.isEmpty() == false)
	{
		m_currentLocations.append(currentLocation);
	}

	vDebug() << "found locations" << m_currentLocations;

	if( VeyonCore::config().showCurrentLocationOnly() )
	{
		if( m_currentLocations.isEmpty() )
		{
			QMessageBox::warning( nullptr,
								  tr( "Location detection failed" ),
								  tr( "Could not determine the location of this computer. "
									  "This indicates a problem with the system configuration. "
									  "All locations will be shown in the computer select panel instead." ) );
			vWarning() << "location detection failed";
		}

		m_locationFilterList = m_currentLocations;
		updateLocationFilterList();
	}
}



void ComputerManager::initNetworkObjectLayer()
{
	m_networkObjectDirectory->update();
	m_networkObjectDirectory->setUpdateInterval( VeyonCore::config().networkObjectDirectoryUpdateInterval() );
	m_networkObjectOverlayDataModel->setSourceModel( m_networkObjectModel );
	m_networkObjectFilterProxyModel->setSourceModel( m_networkObjectOverlayDataModel );
	m_computerTreeModel->setException( NetworkObjectModel::TypeRole, QVariant::fromValue( NetworkObject::Type::Label ) );
	m_computerTreeModel->setSourceModel( m_networkObjectFilterProxyModel );

	const auto hideLocalComputer = VeyonCore::config().hideLocalComputer();
	const auto hideOwnSession = VeyonCore::config().hideOwnSession();

	if( hideLocalComputer || hideOwnSession )
	{
		QStringList localHostNames( {
										QStringLiteral("localhost"),
										QHostAddress( QHostAddress::LocalHost ).toString(),
										QHostAddress( QHostAddress::LocalHostIPv6 ).toString()
									} );

		localHostNames.append( m_localHostNames );

		for( const auto& address : std::as_const( m_localHostAddresses ) )
		{
			localHostNames.append( address.toString() ); // clazy:exclude=reserve-candidates
		}

		QStringList ownSessionNames;
		ownSessionNames.reserve( localHostNames.size() );

		if( hideOwnSession )
		{
			const auto sessionServerPort = QString::number( VeyonCore::config().veyonServerPort() +
															 VeyonCore::sessionId() );

			for( const auto& localHostName : std::as_const(localHostNames) )
			{
				ownSessionNames.append( QStringLiteral("%1:%2").arg( localHostName, sessionServerPort ) );
			}

			vDebug() << "excluding own session via" << ownSessionNames;
		}

		if( hideLocalComputer )
		{
			vDebug() << "excluding local computer via" << localHostNames;
		}
		else
		{
			localHostNames.clear();
		}

		m_networkObjectFilterProxyModel->setComputerExcludeFilter( localHostNames + ownSessionNames );
	}

	m_networkObjectFilterProxyModel->setEmptyGroupsExcluded( VeyonCore::config().hideEmptyLocations() );
}



void ComputerManager::initComputerTreeModel()
{
	QJsonArray checkedNetworkObjects;
	if( VeyonCore::config().autoSelectCurrentLocation() )
	{
		for( const auto& location : std::as_const( m_currentLocations ) )
		{
			const auto computersAtLocation = getComputersAtLocation( location );
			for( const auto& computer : computersAtLocation )
			{
				checkedNetworkObjects += computer.networkObjectUid().toString();
			}
		}
	}
	else
	{
		checkedNetworkObjects = m_config.checkedNetworkObjects();
	}

	m_computerTreeModel->loadStates( checkedNetworkObjects );

	connect( computerTreeModel(), &QAbstractItemModel::modelReset,
			 this, &ComputerManager::computerSelectionReset );
	connect( computerTreeModel(), &QAbstractItemModel::layoutChanged,
			 this, &ComputerManager::computerSelectionReset );

	connect( computerTreeModel(), &QAbstractItemModel::dataChanged,
			 this, &ComputerManager::checkChangedData );
	connect( computerTreeModel(), &QAbstractItemModel::rowsInserted,
			 this, &ComputerManager::computerSelectionChanged );
	connect( computerTreeModel(), &QAbstractItemModel::rowsRemoved,
			 this, &ComputerManager::computerSelectionChanged );
}



void ComputerManager::updateLocationFilterList()
{
	if( VeyonCore::config().showCurrentLocationOnly() )
	{
		m_networkObjectFilterProxyModel->setGroupFilter( m_locationFilterList );
	}
}



QString ComputerManager::findLocationOfComputer(const QStringList& hostNames, const QList<QHostAddress>& hostAddresses,
												const QModelIndex& parent) const
{
	QAbstractItemModel* model = networkObjectModel();

	int rows = model->rowCount( parent );

	for( int i = 0; i < rows; ++i )
	{
		const auto entryIndex = model->index(i, 0, parent);
		const auto objectType = NetworkObject::Type(model->data(entryIndex, NetworkObjectModel::TypeRole).toInt());

		if( NetworkObject::isContainer(objectType) )
		{
			if (model->canFetchMore(entryIndex))
			{
				model->fetchMore(entryIndex);
			}

			const auto location = findLocationOfComputer( hostNames, hostAddresses, entryIndex );
			if( location.isEmpty() == false )
			{
				return location;
			}
		}
		else if( objectType == NetworkObject::Type::Host )
		{
			QString currentHost = model->data( entryIndex, NetworkObjectModel::HostAddressRole ).toString().toLower();
			QHostAddress currentHostAddress;

			if( hostNames.contains( currentHost ) ||
					( currentHostAddress.setAddress( currentHost ) && hostAddresses.contains( currentHostAddress ) ) )
			{
				return model->data( parent, NetworkObjectModel::NameRole ).toString();
			}
		}
	}

	return {};
}



ComputerList ComputerManager::getComputersAtLocation(const QString& locationName, const QModelIndex& parent, bool parentMatches) const
{
	QAbstractItemModel* model = computerTreeModel();

	int rows = model->rowCount( parent );

	ComputerList computers;
	computers.reserve( rows );

	for( int i = 0; i < rows; ++i )
	{
		const auto entryIndex = model->index(i, 0, parent);
		const auto objectType = NetworkObject::Type(model->data(entryIndex, NetworkObjectModel::TypeRole).toInt());
		const auto objectName = model->data(entryIndex, NetworkObjectModel::NameRole).toString();
		if( NetworkObject::isContainer(objectType) )
		{
			const auto currentLocationMatches = objectName == locationName;
			if (parentMatches || currentLocationMatches || hasSubLocations(entryIndex))
			{
				computers += getComputersAtLocation(locationName, entryIndex, parentMatches || currentLocationMatches);
			}
		}
		else if( objectType == NetworkObject::Type::Host )
		{
			if (parentMatches)
			{
				computers += Computer(model->data(entryIndex, NetworkObjectModel::UidRole).toUuid(),
									  objectName,
									  model->data(entryIndex, NetworkObjectModel::HostAddressRole).toString(),
									  model->data(entryIndex, NetworkObjectModel::MacAddressRole).toString());
			}
		}
	}

	return computers;
}



bool ComputerManager::hasSubLocations(const QModelIndex& index) const
{
	const auto model = computerTreeModel();
	const auto rows = model->rowCount(index);

	for (int i = 0; i < rows; ++i)
	{
		const auto objectType = NetworkObject::Type(model->data(model->index(i, 0, index),
																NetworkObjectModel::TypeRole).toInt());

		if (objectType == NetworkObject::Type::Location || objectType == NetworkObject::Type::DesktopGroup)
		{
			return true;
		}
	}

	return false;
}



ComputerList ComputerManager::selectedComputers(const QModelIndex& parent) const
{
	QAbstractItemModel* model = computerTreeModel();

	int rows = model->rowCount( parent );

	ComputerList computers;

	for( int i = 0; i < rows; ++i )
	{
		QModelIndex entryIndex = model->index( i, 0, parent );

		if( model->data( entryIndex, NetworkObjectModel::CheckStateRole ).value<Qt::CheckState>() == Qt::Unchecked )
		{
			continue;
		}

		const auto objectType = NetworkObject::Type( model->data(entryIndex, NetworkObjectModel::TypeRole).toInt() );

		if( NetworkObject::isContainer(objectType) )
		{
			computers += selectedComputers( entryIndex );
		}
		else if( objectType == NetworkObject::Type::Host )
		{
			computers += Computer( model->data( entryIndex, NetworkObjectModel::UidRole ).toUuid(),
								   model->data( entryIndex, NetworkObjectModel::NameRole ).toString(),
								   model->data( entryIndex, NetworkObjectModel::HostAddressRole ).toString(),
								   model->data( entryIndex, NetworkObjectModel::MacAddressRole ).toString(),
								   model->data( parent, NetworkObjectModel::NameRole ).toString() );
		}
	}

	return computers;
}



QModelIndex ComputerManager::findNetworkObject(NetworkObject::Uid networkObjectUid, const QModelIndex& parent) const
{
	QAbstractItemModel* model = networkObjectModel();

	int rows = model->rowCount( parent );

	for( int i = 0; i < rows; ++i )
	{
		QModelIndex entryIndex = model->index( i, 0, parent );

		const auto objectType = NetworkObject::Type( model->data(entryIndex, NetworkObjectModel::TypeRole).toInt() );

		if( NetworkObject::isContainer(objectType) )
		{
			QModelIndex index = findNetworkObject( networkObjectUid, entryIndex );
			if( index.isValid() )
			{
				return index;
			}
		}
		else if( objectType == NetworkObject::Type::Host )
		{
			if( model->data( entryIndex, NetworkObjectModel::UidRole ).toUuid() == networkObjectUid )
			{
				return entryIndex;
			}
		}
	}

	return {};
}



QModelIndex ComputerManager::mapToUserNameModelIndex(const QModelIndex& networkObjectIndex) const
{
	// map arbitrary index from m_networkObjectModel to username column in m_networkObjectOverlayDataModel
	const auto parent = m_networkObjectOverlayDataModel->mapFromSource( networkObjectIndex.parent() );

	return m_networkObjectOverlayDataModel->index( networkObjectIndex.row(), OverlayDataUsernameColumn, parent );
}



QModelIndex ComputerManager::mapToSessionUptimeModelIndex(const QModelIndex& networkObjectIndex) const
{
	// map arbitrary index from m_networkObjectModel to username column in m_networkObjectOverlayDataModel
	const auto parent = m_networkObjectOverlayDataModel->mapFromSource(networkObjectIndex.parent());

	return m_networkObjectOverlayDataModel->index(networkObjectIndex.row(), OverlayDataSessionUptimeColumn, parent);
}
