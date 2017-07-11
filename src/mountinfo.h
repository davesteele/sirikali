/*
 *
 *  Copyright (c) 2012-2015
 *  name : Francis Banyikwa
 *  email: mhogomchungu@gmail.com
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef MONITOR_MOUNTINFO_H
#define MONITOR_MOUNTINFO_H

#include <QThread>
#include <QString>
#include <QStringList>
#include <QObject>
#include <QProcess>

#include <functional>
#include <memory>

#include "task.h"

class volumeInfo ;

class mountinfo : private QObject
{
	Q_OBJECT
public:
	QStringList mountedVolumes() ;

	mountinfo( QObject * parent,bool,std::function< void() >&& ) ;
	mountinfo() ;

	std::function< void() >& stop() ;

	void announceEvents( bool ) ;

	~mountinfo() ;
private slots:
	void volumeUpdate( void ) ;
private:
	void linuxMonitor( void ) ;
	void osxMonitor( void ) ;
	void updateVolume( void ) ;
	void pbUpdate( void ) ;
	void autoMount( const QString& ) ;

	QObject * m_parent ;
	QProcess m_process ;

	std::function< void() > m_stop ;
	std::function< void() > m_quit ;

	bool m_announceEvents ;
	bool m_linux ;

	QStringList m_oldMountList ;
	QStringList m_newMountList ;
};

#endif // MONITOR_MOUNTINFO_H
