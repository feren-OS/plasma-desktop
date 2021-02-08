/* This file is part of the KDE project
   Copyright (C) 2021 Cyril Rossi <cyril.rossi@enioka.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#ifndef KDEDCONFIGDATA_H
#define KDEDCONFIGDATA_H

#include <KCModuleData>
#include <QObject>

class KDEDConfigData : public KCModuleData
{
    Q_OBJECT

public:
    KDEDConfigData(QObject *parent = nullptr, const QVariantList &args = QVariantList());

    bool isDefaults() const override;
};

#endif // KDEDCONFIGDATA_H