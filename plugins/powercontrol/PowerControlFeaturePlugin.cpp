/*
 * PowerControlFeaturePlugin.cpp - implementation of PowerControlFeaturePlugin class
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

#include <QEvent>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QProgressBar>
#include <QProgressDialog>
#include <QUdpSocket>

#include "Computer.h"
#include "ComputerControlInterface.h"
#include "FeatureWorkerManager.h"
#include "NetworkObjectDirectoryManager.h"
#include "PlatformCoreFunctions.h"
#include "PlatformUserFunctions.h"
#include "PowerControlFeaturePlugin.h"
#include "PowerDownTimeInputDialog.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"


PowerControlFeaturePlugin::PowerControlFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_commands( {
{ QStringLiteral("on"), tr( "Power on a computer via Wake-on-LAN (WOL)" ) },
				} ),
	m_powerOnFeature( QStringLiteral( "PowerOn" ),
					  Feature::Flag::Action | Feature::Flag::AllComponents,
					  Feature::Uid( "f483c659-b5e7-4dbc-bd91-2c9403e70ebd" ),
					  Feature::Uid(),
					  tr( "Power on" ), {},
					  tr( "Click this button to power on all computers. "
						  "This way you do not have to power on each computer by hand." ),
					  QStringLiteral(":/powercontrol/preferences-system-power-management.png") ),
	m_rebootFeature( QStringLiteral( "Reboot" ),
					 Feature::Flag::Action | Feature::Flag::AllComponents,
					 Feature::Uid( "4f7d98f0-395a-4fff-b968-e49b8d0f748c" ),
					 Feature::Uid(),
					 tr( "Reboot" ), {},
					 tr( "Click this button to reboot all computers." ),
					 QStringLiteral(":/powercontrol/system-reboot.png") ),
	m_powerDownFeature( QStringLiteral( "PowerDown" ),
						Feature::Flag::Action | Feature::Flag::AllComponents,
						Feature::Uid( "6f5a27a0-0e2f-496e-afcc-7aae62eede10" ),
						Feature::Uid(),
						tr( "Power down" ), {},
						tr( "Click this button to power down all computers. "
							"This way you do not have to power down each computer by hand." ),
						QStringLiteral(":/powercontrol/system-shutdown.png") ),
	m_powerDownNowFeature( QStringLiteral( "PowerDownNow" ),
								   Feature::Flag::Action | Feature::Flag::AllComponents,
								   Feature::Uid( "a88039f2-6716-40d8-b4e1-9f5cd48e91ed" ),
								   m_powerDownFeature.uid(),
								   tr( "Power down now" ), {}, {} ),
	m_installUpdatesAndPowerDownFeature( QStringLiteral( "InstallUpdatesAndPowerDown" ),
								   Feature::Flag::Action | Feature::Flag::AllComponents,
								   Feature::Uid( "09bcb3a1-fc11-4d03-8cf1-efd26be8655b" ),
								   m_powerDownFeature.uid(),
								   tr( "Install updates and power down" ), {}, {} ),
	m_powerDownConfirmedFeature( QStringLiteral( "PowerDownConfirmed" ),
								   Feature::Flag::Action | Feature::Flag::AllComponents,
								   Feature::Uid( "ea2406be-d5c7-42b8-9f04-53469d3cc34c" ),
								   m_powerDownFeature.uid(),
								   tr( "Power down after user confirmation" ), {}, {} ),
	m_powerDownDelayedFeature( QStringLiteral( "PowerDownDelayed" ),
								   Feature::Flag::Action | Feature::Flag::AllComponents,
								   Feature::Uid( "352de795-7fc4-4850-bc57-525bcb7033f5" ),
								   m_powerDownFeature.uid(),
								   tr( "Power down after timeout" ), {}, {} ),
	m_features( { m_powerOnFeature, m_rebootFeature, m_powerDownFeature, m_powerDownNowFeature,
				m_installUpdatesAndPowerDownFeature, m_powerDownConfirmedFeature, m_powerDownDelayedFeature } )
{
}



QStringList PowerControlFeaturePlugin::commands() const
{
	return m_commands.keys();
}



QString PowerControlFeaturePlugin::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



const FeatureList &PowerControlFeaturePlugin::featureList() const
{
	return m_features;
}



bool PowerControlFeaturePlugin::controlFeature( Feature::Uid featureUid,
											   Operation operation,
											   const QVariantMap& arguments,
											   const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( operation != Operation::Start || hasFeature( featureUid ) == false )
	{
		return false;
	}

	if( featureUid == m_powerOnFeature.uid() )
	{
		const auto directory = VeyonCore::networkObjectDirectoryManager().configuredDirectory();

		for( const auto& controlInterface : computerControlInterfaces )
		{
			const auto& host = controlInterface->computer();
			auto macAddress = host.macAddress();
			if (macAddress.isEmpty())
			{
				macAddress = directory->queryObjectProperty(host.networkObjectUid(),
															NetworkObject::Property::MacAddress).toString();
			}

			if (macAddress.isEmpty() == false)
			{
				broadcastWOLPacket(macAddress);
			}
			else
			{
				vWarning() << "no MAC address available for host" << host.hostName() << "with ID" << host.networkObjectUid();
			}
		}
	}
	else if( featureUid == m_powerDownDelayedFeature.uid() )
	{
		const auto shutdownTimeout = arguments.value( argToString(Argument::ShutdownTimeout), 60 ).toInt();

		sendFeatureMessage( FeatureMessage{ featureUid, FeatureMessage::DefaultCommand }
								.addArgument( Argument::ShutdownTimeout, shutdownTimeout ),
							computerControlInterfaces );
	}
	else
	{
		sendFeatureMessage( FeatureMessage{ featureUid, FeatureMessage::DefaultCommand }, computerControlInterfaces );
	}

	return true;
}



bool PowerControlFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
											  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( hasFeature( feature.uid() ) == false )
	{
		return false;
	}

	if( feature == m_powerOnFeature )
	{
		return controlFeature( feature.uid(), Operation::Start, {}, computerControlInterfaces );
	}

	if( feature == m_powerDownDelayedFeature )
	{
		PowerDownTimeInputDialog dialog( master.mainWindow() );

		if( dialog.exec() )
		{
			return controlFeature( feature.uid(), Operation::Start,
								   { { argToString(Argument::ShutdownTimeout), dialog.seconds() } },
								   computerControlInterfaces );
		}

		return true;
	}

	const auto executeOnAllComputers =
		computerControlInterfaces.size() >= master.filteredComputerControlInterfaces().size();

	if (confirmFeatureExecution(feature, executeOnAllComputers, master.mainWindow()) == false)
	{
		return false;
	}

	return controlFeature( feature.uid(), Operation::Start, {}, computerControlInterfaces );
}



bool PowerControlFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
													  const MessageContext& messageContext,
													  const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	auto& featureWorkerManager = server.featureWorkerManager();

	if( message.featureUid() == m_powerDownFeature.uid() ||
		message.featureUid() == m_powerDownNowFeature.uid() ||
		message.featureUid() == m_installUpdatesAndPowerDownFeature.uid() )
	{
		const auto installUpdates = message.featureUid() == m_installUpdatesAndPowerDownFeature.uid();
		VeyonCore::platform().coreFunctions().powerDown( installUpdates );
	}
	else if( message.featureUid() == m_powerDownConfirmedFeature.uid() )
	{
		if( VeyonCore::platform().userFunctions().isAnyUserLoggedInLocally() == false &&
			VeyonCore::platform().userFunctions().isAnyUserLoggedInRemotely() == false )
		{
			VeyonCore::platform().coreFunctions().powerDown( false );
		}
		else
		{
			featureWorkerManager.sendMessageToManagedSystemWorker( message );
		}
	}
	else if( message.featureUid() == m_powerDownDelayedFeature.uid() )
	{
		featureWorkerManager.sendMessageToManagedSystemWorker( message );
	}
	else if( message.featureUid() == m_rebootFeature.uid() )
	{
		VeyonCore::platform().coreFunctions().reboot();
	}
	else
	{
		return false;
	}

	return true;
}



bool PowerControlFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker)

	if( message.featureUid() == m_powerDownConfirmedFeature.uid() )
	{
		confirmShutdown();
		return true;
	}

	if( message.featureUid() == m_powerDownDelayedFeature.uid() )
	{
		displayShutdownTimeout( message.argument( Argument::ShutdownTimeout ).toInt() );
		return true;
	}

	return false;
}



CommandLinePluginInterface::RunResult PowerControlFeaturePlugin::handle_help( const QStringList& arguments )
{
	const auto command = arguments.value( 0 );

	const QMap<QString, QStringList> commands = {
		{ QStringLiteral("on"),
		  QStringList( { QStringLiteral("<%1>").arg( tr("MAC ADDRESS") ),
						 tr( "This command broadcasts a Wake-on-LAN (WOL) packet to the network in order to power on the computer with the given MAC address." ) } ) },
	};

	if( commands.contains( command ) )
	{
		const auto& helpString = commands[command];
		print( QStringLiteral("\n%1 %2 %3\n\n%4\n\n").arg( commandLineModuleName(), command, helpString[0], helpString[1] ) );

		return NoResult;
	}

	print( tr("Please specify the command to display help for!") );

	return Unknown;
}



CommandLinePluginInterface::RunResult PowerControlFeaturePlugin::handle_on( const QStringList& arguments )
{
	if( arguments.isEmpty() )
	{
		return NotEnoughArguments;
	}

	return broadcastWOLPacket( arguments.first() ) ? Successful : Failed;
}



bool PowerControlFeaturePlugin::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::Close &&
		qobject_cast<QProgressDialog *>(watched))
	{
		event->ignore();
		return true;
	}

	return QObject::eventFilter(watched, event);
}



bool PowerControlFeaturePlugin::confirmFeatureExecution( const Feature& feature, bool all, QWidget* parent )
{
	if( VeyonCore::config().confirmUnsafeActions() == false )
	{
		return true;
	}

	if( feature == m_rebootFeature )
	{
		return QMessageBox::question( parent, tr( "Confirm reboot" ),
									  all ? tr( "Do you really want to reboot <b>ALL</b> computers?" )
										  : tr( "Do you really want to reboot the selected computers?" ) ) ==
				QMessageBox::Yes;
	}

	if( feature == m_powerDownFeature ||
			 feature == m_powerDownNowFeature ||
			 feature == m_installUpdatesAndPowerDownFeature ||
			 feature == m_powerDownConfirmedFeature ||
			 feature == m_powerDownDelayedFeature )
	{
		return QMessageBox::question( parent, tr( "Confirm power down" ),
									  all ? tr( "Do you really want to power down <b>ALL</b> computers?" )
										  : tr( "Do you really want to power down the selected computers?" ) ) ==
				QMessageBox::Yes;
	}

	return true;
}



bool PowerControlFeaturePlugin::broadcastWOLPacket( QString macAddress )
{
	if (macAddress.isEmpty())
	{
		return false;
	}

	const auto originalMacAddress = macAddress;

	// remove all possible delimiters
	macAddress.replace( QLatin1Char(':'), QString() );
	macAddress.replace( QLatin1Char('-'), QString() );
	macAddress.replace( QLatin1Char('.'), QString() );

	const auto macAddressBytes = QByteArray::fromHex(macAddress.toUtf8());
	static constexpr auto MacAddressSize = 6;

	if (macAddressBytes.size() != MacAddressSize)
	{
		CommandLineIO::error( tr( "Invalid MAC address specified!" ) );
		vWarning() << "invalid MAC address" << originalMacAddress;
		return false;
	}

	static constexpr auto MagicPacketFieldCount = 17;

	QByteArray datagram(MacAddressSize * MagicPacketFieldCount, char(0xff));

	for (int i = 1; i < MagicPacketFieldCount; ++i)
	{
		datagram.replace(i * MacAddressSize, MacAddressSize, macAddressBytes);
	}

	QUdpSocket udpSocket;

	vDebug() << "broadcasting WOL packet for" << originalMacAddress;
	bool success = ( udpSocket.writeDatagram( datagram, QHostAddress::Broadcast, 9 ) == datagram.size() );

	const auto networkInterfaces = QNetworkInterface::allInterfaces();
	for( const auto& networkInterface : networkInterfaces )
	{
		const auto addressEntries = networkInterface.addressEntries();
		for( const auto& addressEntry : addressEntries )
		{
			if( addressEntry.broadcast().isNull() == false )
			{
				vDebug() << "broadcasting WOL packet for" << originalMacAddress << "via" << addressEntry.broadcast().toString();
				success &= ( udpSocket.writeDatagram( datagram, addressEntry.broadcast(), 9 ) == datagram.size() );
			}
		}
	}

	return success;
}



void PowerControlFeaturePlugin::confirmShutdown()
{
	QMessageBox m( QMessageBox::Question, tr( "Confirm power down" ),
				   tr( "The computer was remotely requested to power down. Do you want to power down the computer now?" ),
				   QMessageBox::Yes | QMessageBox::No );
	m.show();
	VeyonCore::platform().coreFunctions().raiseWindow( &m, true );

	if( m.exec() == QMessageBox::Yes )
	{
		VeyonCore::platform().coreFunctions().powerDown( false );
	}
}



static void updateDialog( QProgressDialog* dialog, int newValue )
{
	dialog->setValue( newValue );

	const auto remainingSeconds = dialog->maximum() - newValue;

	dialog->setLabelText( PowerControlFeaturePlugin::tr(
							  "The computer will be powered down in %1 minutes, %2 seconds.\n\n"
							  "Please save your work and close all programs.").
						 arg( remainingSeconds / 60, 2, 10, QLatin1Char('0') ).
						 arg( remainingSeconds % 60, 2, 10, QLatin1Char('0') ) );

	if( remainingSeconds <= 0 )
	{
		dialog->accept();
	}

}

void PowerControlFeaturePlugin::displayShutdownTimeout( int shutdownTimeout )
{
	QProgressDialog dialog;
	dialog.setAutoReset( false );
	dialog.setMinimum( 0 );
	dialog.setMaximum( shutdownTimeout );
	dialog.setCancelButton( nullptr );
	dialog.setWindowFlags( Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint );
	dialog.installEventFilter(this);

	auto progressBar = dialog.findChild<QProgressBar *>();
	if( progressBar )
	{
		progressBar->setTextVisible( false );
	}

	updateDialog( &dialog, 0 );

	dialog.show();
	VeyonCore::platform().coreFunctions().raiseWindow( &dialog, true );

	QTimer powerdownTimer;
	powerdownTimer.start( 1000 );

	connect( &powerdownTimer, &QTimer::timeout, this, [&dialog]() {
		updateDialog( &dialog, dialog.value()+1 );
	} );

	dialog.exec();

	VeyonCore::platform().coreFunctions().powerDown(false);
}
