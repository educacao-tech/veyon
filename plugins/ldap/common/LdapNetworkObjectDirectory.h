// Copyright (c) 2019-2025 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "LdapDirectory.h"
#include "NetworkObjectDirectory.h"

class LDAP_COMMON_EXPORT LdapNetworkObjectDirectory : public NetworkObjectDirectory
{
	Q_OBJECT
public:
	LdapNetworkObjectDirectory(const LdapConfiguration& ldapConfiguration, QObject* parent);

	NetworkObjectList queryObjects(NetworkObject::Type type,
								   NetworkObject::Attribute attribute, const QVariant& value) override;
	NetworkObjectList queryParents(const NetworkObject& childId) override;

	static NetworkObject computerToObject(LdapDirectory* directory, const QString& computerDn);

private:
	void update() override;
	void fetchObjects(const NetworkObject& parent) override;

	void updateObjects(const NetworkObject& parent);

	NetworkObjectList queryLocations(NetworkObject::Attribute attribute, const QVariant& value);
	NetworkObjectList queryHosts(NetworkObject::Attribute attribute, const QVariant& value);

	void updateLocations(const NetworkObject& parent);
	void updateComputers(const NetworkObject& parent);

	LdapDirectory m_ldapDirectory;

};
