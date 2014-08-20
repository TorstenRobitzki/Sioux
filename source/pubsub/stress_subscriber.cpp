// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "pubsub/stress_subscriber.h"

#include <boost/array.hpp>
#include <boost/lexical_cast.hpp>

namespace pubsub
{
	namespace test
	{
		stress_subscriber::stress_subscriber(
				pubsub::root& root, pubsub::test::stress_adapter& adapter, unsigned number_of_simulated_actions, unsigned seed )
		  : root_( root )
		  , remaining_actions_( number_of_simulated_actions )
		  , random_generator_()
		{
			random_generator_.seed( seed );
		}

		void stress_subscriber::start()
		{
			next_action();
		}

		void stress_subscriber::check() const
		{
			if ( !open_responses_.empty() )
				throw std::logic_error( boost::lexical_cast< std::string >( open_responses_.size() ) + " outstanding responses." );

			if ( !errors_.empty() )
				throw std::logic_error( errors_ );
		}

		void stress_subscriber::on_udate(const pubsub::node_name& name, const pubsub::node& /* data */)
		{
			// this call back will be called when ever a value has been changed or
			next_action();
		}

		void stress_subscriber::on_invalid_node_subscription(const pubsub::node_name& node)
		{
		}

		void stress_subscriber::on_unauthorized_node_subscription(const pubsub::node_name& node)
		{
		}

		void stress_subscriber::on_failed_node_subscription(const pubsub::node_name& node)
		{
		}

		void stress_subscriber::next_action()
		{
	        boost::mutex::scoped_lock lock(mutex_);
	        next_action_impl();
		}

		void stress_subscriber::next_action_impl()
		{
			if ( !remaining_actions_ )
				return;

			--remaining_actions_;
			const boost::int32_t rand = random( 1, 100 );

			if ( rand < 30 )
			{
				subscribe();
			}
			else if ( rand < 60 )
			{
			    unsubscribe();
			}
			else if ( rand < 90 )
			{
				change();
			}
			else
			{
				change_configuration();
			}
		}

		struct stress_subscriber::subject
		{
			node_name	name_;
			bool		needs_authorization_;
		};

		static node_name make_node_name( const char* text )
		{
			return node_name( json::parse( text ).upcast< json::object >() );
		}

		static const boost::array< stress_subscriber::subject, 2u > subjects = { {
			{
				node_name( make_node_name(
						"{ \"valid\": \"valid\", \"node\" : 1 }" ) ),
				false
			},
			{
				node_name( make_node_name(
						"{ \"valid\": \"async_valid\", \"node\" : 2 }" ) ),
				true
			}
		} };

		void stress_subscriber::subscribe()
		{
			const subject* const next_subject = &subjects[ random( 0, subjects.size()-1 ) ];

			if ( subscriptions_.find( next_subject ) != subscriptions_.end() )
			{
				return next_action_impl();
			}

			subscriptions_.insert( next_subject );
			open_responses_.insert( next_subject );
			root_.subscribe( shared_from_this(), next_subject->name_ );
		}

		void stress_subscriber::unsubscribe()
		{
			subjects_t::iterator selected_subject = subscriptions_.begin();
			std::advance( selected_subject, random( 0, subscriptions_.size() -1 ) );

			subscriptions_.erase( *selected_subject );
			root_.unsubscribe( shared_from_this(), ( *selected_subject )->name_ );
		}

		void stress_subscriber::change()
		{
			const subject& changed_subject = subjects[ random( 0, subjects.size()-1 ) ];

			root_.update_node( changed_subject.name_, json::number( random( 1, 100 ) ) );
		}

		void stress_subscriber::change_configuration()
		{
			next_action_impl();
		}

		boost::int32_t stress_subscriber::random( boost::int32_t lower, boost::int32_t upper )
		{
	        rand_distribution_type          destribution( lower, upper );
	        rand_gen_type                   die( random_generator_, destribution );

	        return die();
		}

	}
}

