/*
Copyright (C) 2015 Sebastian Herbord. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "xnginelocalsavegames.h"

#include <QtDebug>
#include <iprofile.h>

#include "gamexngine.h"

XngineLocalSavegames::XngineLocalSavegames(const GameXngine* game,
                                           const QString& iniFileName)
    : m_Game{game}, m_IniFileName(iniFileName)
{}

MappingType XngineLocalSavegames::mappings(const QDir& profileSaveDir) const
{
  if (!m_Game) {
    qDebug() << "[XnGine] LocalSavegames: no game instance available";
    return {};
  }

  const QDir saveDir = m_Game->savesDirectory();
  if (!saveDir.exists()) {
    qDebug() << "[XnGine] LocalSavegames: saves directory does not exist:"
             << saveDir.absolutePath();
    return {};
  }

  return {{profileSaveDir.absolutePath(), saveDir.absolutePath(), true, true}};
}

QString XngineLocalSavegames::localSavesDummy() const
{
  return "__MO_Saves\\";
}

bool XngineLocalSavegames::prepareProfile(MOBase::IProfile* profile)
{
  Q_UNUSED(profile);
  qDebug() << "[XnGine] LocalSavegames: no INI-based local saves for XnGine";
  return false;
}
