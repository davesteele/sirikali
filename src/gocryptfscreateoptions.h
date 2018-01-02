/*
 *
 *  Copyright (c) 2017
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

#ifndef GOCRYPTFSCREATEOPTIONS_H
#define GOCRYPTFSCREATEOPTIONS_H

#include <functional>
#include <utility>

#include <QCloseEvent>
#include <QString>
#include <QDialog>

namespace Ui {
class gocryptfscreateoptions;
}

class gocryptfscreateoptions : public QDialog
{
	Q_OBJECT
public:
	static void instance( QWidget * parent,std::function< void( const QStringList& ) > function )
	{
                new gocryptfscreateoptions( parent,std::move( function ) ) ;
	}
        gocryptfscreateoptions( QWidget * parent,std::function< void( const QStringList& ) > ) ;
        ~gocryptfscreateoptions() ;
private slots:
	void pbSelectConfigPath() ;
	void pbOK() ;
	void pbCancel() ;
private:
	void HideUI( const QStringList& = QStringList() ) ;
	void closeEvent( QCloseEvent * ) ;
        Ui::gocryptfscreateoptions * m_ui ;
	std::function< void( const QStringList& ) > m_function ;
};

#endif // GOCRYPTFSCREATEOPTIONS_H
