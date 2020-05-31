/*
 *
 *  Copyright ( c ) 2016
 *  name : Francis Banyikwa
 *  email: mhogomchungu@gmail.com
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  ( at your option ) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SECRETS_H
#define SECRETS_H

#include "lxqt_wallet.h"

#include <QIcon>
#include <QWidget>

#include <functional>
#include <type_traits>
#include <utility>

class secrets
{
public:
	class wallet
	{
	public:
		wallet( LXQt::Wallet::Wallet * ) ;
		wallet() ;

		~wallet() ;

		wallet( wallet&& ) ;

		LXQt::Wallet::Wallet& bk() const
		{
			return *m_wallet ;
		}

		LXQt::Wallet::Wallet * operator->()
		{
			return m_wallet ;
		}

		operator bool() const
		{
			return m_wallet ;
		}

		template< typename T >
		struct walletResult
		{
			bool opened ;
			T result ;
		} ;

		template< typename Function,typename Before,typename After >
		walletResult< std::result_of_t< Function() > > openSync( Function&& function,
									 Before&& b = [](){},
									 After&& a = [](){} )
		{
			if( m_wallet->opened() ){

				return { true,function() } ;
			}else{
				m_wallet->setImage( QIcon( ":/sirikali" ) ) ;

				auto s = this->walletInfo() ;

				b() ;

				auto m = m_wallet->open( s.walletName,s.appName ) ;

				a() ;

				if( m ){

					return { true,function() } ;
				}else{
					return { false,{} } ;
				}
			}
		}

		template< typename Function >
		walletResult< std::result_of_t< Function() > > open( Function&& function )
		{
			return this->openSync( std::move( function ),[](){},[](){} ) ;
		}

		bool open()
		{
			return this->openSync( []{ return true ; },[](){},[](){} ).opened ;
		}

		template< typename Opened,typename Before,typename After >
		void open( Opened&& o,Before&& b,After&& a )
		{
			if( m_wallet->opened() ){

				o() ;
			}else{
				b() ;

				auto s = this->walletInfo() ;

				m_wallet->open( s.walletName,s.appName,std::move( a ) ) ;
			}
		}

		template< typename Opened,typename After >
		void open( Opened&& ofunction,After&& afunction )
		{
			this->open( std::move( ofunction ),[](){},std::move( afunction ) ) ;
		}

		struct walletKey
		{
			bool opened ;
			bool notConfigured ;
			QString key ;
		} ;

		walletKey getKey( const QString& keyID,QWidget * widget = nullptr ) ;
	private:		
		struct info{

			QString walletName ;
			QString appName ;
		};

		info walletInfo() ;

		LXQt::Wallet::Wallet * m_wallet = nullptr ;
	};

	secrets::wallet walletBk( LXQt::Wallet::BackEnd ) const ;

	QWidget * parent() const ;

	void changeInternalWalletPassword( const QString&,const QString&,std::function< void( bool ) > ) ;
	void changeWindowsDPAPIWalletPassword( const QString&,const QString&,std::function< void( bool ) > ) ;

	void setParent( QWidget * ) ;
	void close() ;
	secrets( QWidget * parent = nullptr ) ;
	secrets( const secrets& ) = delete ;

	secrets& operator=( const secrets& ) = delete ;

	~secrets() ;
private:
	LXQt::Wallet::Wallet * internalWallet() const ;
	LXQt::Wallet::Wallet * windows_dpapiBackend() const ;
	QWidget * m_parent = nullptr ;
	mutable LXQt::Wallet::Wallet * m_internalWallet = nullptr ;
	mutable LXQt::Wallet::Wallet * m_windows_dpapi = nullptr ;
};

#endif
