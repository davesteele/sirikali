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

#include "mountinfo.h"

#include <QString>
#include <QStringList>
#include <QDebug>

#include "utility.h"
#include "task.h"
#include "siritask.h"

#include <QCoreApplication>

#include <iostream>

class monitorMountinfo
{
public:
	monitorMountinfo() : m_handle( "/proc/self/mountinfo" )
	{
		m_handle.open( QIODevice::ReadOnly ) ;
		m_monitor.fd     = m_handle.handle() ;
		m_monitor.events = POLLPRI ;
	}
	operator bool()
	{
		return m_monitor.fd != -1 ;
	}
	bool gotEvent()
	{
		poll( &m_monitor,1,-1 ) ;
		return true ;
	}
private:
	QFile m_handle ;
	struct pollfd m_monitor ;
};

QStringList mountinfo::mountedVolumes()
{
	QFile f( "/proc/self/mountinfo" ) ;

	if( f.open( QIODevice::ReadOnly ) ){

		return utility::split( f.readAll() ) ;
	}else{
		return QStringList() ;
	}
}

mountinfo::mountinfo( QObject * parent,bool e,std::function< void() >&& f ) :
	QThread( parent ),m_stop( std::move( f ) ),m_announceEvents( e )
{
	m_babu = parent ;
	m_baba = this ;
	m_main = this ;
}

mountinfo::~mountinfo()
{
}

std::function< void() > mountinfo::stop()
{
	return [ this ](){

		if( m_running ){

			m_mtoto->terminate() ;
		}else{
			this->threadStopped() ;
		}
	} ;
}

void mountinfo::threadStopped()
{
	m_running = false ;
	m_stop() ;
}

void mountinfo::failedToStart()
{
	qDebug() << "failed to monitor /proc/self/mountinfo" ;
	m_running = false ;
}

void mountinfo::announceEvents( bool s )
{
	m_announceEvents = s ;
}

void mountinfo::run()
{
	m_mtoto = this ;

	connect( m_mtoto,SIGNAL( finished() ),m_main,SLOT( threadStopped() ) ) ;
	connect( m_mtoto,SIGNAL( finished() ),m_mtoto,SLOT( deleteLater() ) ) ;

	monitorMountinfo monitor ;

	m_running = monitor ;

	auto oldMountList = mountinfo::mountedVolumes() ;

	decltype( oldMountList ) newMountList ;

	auto _volumeWasMounted = [ & ](){ return oldMountList.size() < newMountList.size() ; } ;

	auto _mountedVolume = [ & ]( const QString& e ){ return !oldMountList.contains( e ) ; } ;

	if( monitor ){

		while( monitor.gotEvent() ){

			newMountList = mountinfo::mountedVolumes() ;

			if( _volumeWasMounted() ){

				for( const auto& it : newMountList ){

					if( _mountedVolume( it ) ){

						const auto e = utility::split( it,' ' ) ;

						if( e.size() > 3 ){

							gotEvent( e.at( 4 ) ) ;
						}
					}
				}
			}

			oldMountList = newMountList ;

			if( m_announceEvents ){

				emit gotEvent() ;
			}
		}
	}else{
		return this->failedToStart() ;
	}
}
