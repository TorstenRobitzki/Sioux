// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_PUBSUB_TEST_HELPER_H
#define SIOUX_SOURCE_PUBSUB_TEST_HELPER_H

#include "pubsub/pubsub.h"
#include "pubsub/node.h"
#include <map>
#include <set>
#include <boost/thread/mutex.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

namespace json {
    class value;
}

namespace pubsub {
namespace test {


    /**
     * @brief implementation of the subscriber interface that will record all upcalls to 
     *        the implementation.
     *
     * For every function of the ::pubsub::subscriber interface, this class keeps a seperated
     * list to record the arguments used, when calling a functions.
     */
    class subscriber : public ::pubsub::subscriber
    {
    public:
        /**
         * @brief returns true, if on_udate() was called with the given node and value
         *
         * If such an entry exists, it will be deleted.
         * @post a second call to on_udate_called() will return false.
         */
        bool on_update_called(const node_name&, const json::value&);

        /** 
         * @brief returns true, if there are no more stored calls to on_udate().
         */
        bool not_on_update_called() const;

        /**
         * @brief returns true, if on_invalid_node_subscription() was called with the given node_name.
         *
         * If such an entry exists, it will be deleted.
         * @post a second call to on_invalid_node_subscription_called() will return false.
         */
        bool on_invalid_node_subscription_called(const node_name& node);

        /**
         * @brief returns true, if there exists no stored call to on_invalid_node_subscription()
         */
        bool not_on_invalid_node_subscription_called() const;

        /**
         * @brief returns true, if a call to on_unauthorized_node_subscription() with the given 
         *        node was stored. If such a call was stored, it will be deleted.
         *
         * @post a second call to on_unauthorized_node_subscription_called() will return false.
         */
        bool on_unauthorized_node_subscription_called(const node_name& node);

        /**
         * @brief returns true, if no call to on_unauthorized_node_subscription() is stored.
         */
        bool not_on_unauthorized_node_subscription_called() const;

        /**
         * @brief returns true, if a call to on_failed_node_subscription() with the given 
         *        node was stored. If such a call was stored, it will be deleted.
         *
         * @post a second call to on_failed_node_subscription_called() will return false.
         */
        bool on_failed_node_subscription_called(const node_name& node);

        /**
         * @brief returns true, if no call to on_failed_node_subscription() is stored.
         */
        bool not_on_failed_node_subscription_called() const;

        /**
         * @brief returns true, if no more calls are stored.
         */
        bool empty() const;
    private:
        // ::pubsub::subscriber implementation
        virtual void on_update(const node_name& name, const node& data);
        virtual void on_invalid_node_subscription(const node_name& node);
        virtual void on_unauthorized_node_subscription(const node_name& node);
        virtual void on_failed_node_subscription(const node_name& node);

        mutable boost::mutex mutex_;
        typedef std::multiset<boost::tuple<node_name, json::value> > on_udate_list_t;
        on_udate_list_t     on_update_calls_;

        typedef std::multiset<node_name> node_list;
        node_list           on_invalid_node_subscription_calls_;
        node_list           on_unauthorized_node_subscription_calls_;
        node_list           on_failed_node_subscription_calls_;
    };

    /**
     * @brief implementation of the adapter interface for testing.
     * 
     */
    class adapter : public ::pubsub::adapter
    {
    public:
        /**
         * @brief a reference to an io_service is taken and kept over the entire livetime, to perform
         *        defered callback responses
         */
        explicit adapter( boost::asio::io_service& );

        /**
         * @brief if no defered_callback answering is used, no io_service needs to be applied
         */
        adapter();

        /**
         * @brief returns true, if the authorize() function was called at least once 
         *        with the given parameters.
         */
        bool authorization_requested(const boost::shared_ptr< ::pubsub::subscriber>&, const node_name&) const;

        /**
         * @brief answer one authorization request with the given name
         *
         * If no such authorization request is stored up to now, the values are stored
         * and an incomming request that fits to the node_name will be answered with 
         * is_valid and will not be stored.
         */
        void answer_authorization_request(const boost::shared_ptr< ::pubsub::subscriber>&, const node_name&, bool is_authorized);

        /**
         * @brief ignore one authorization request with the given subscriber and node by deleting 
         *        a stored authorization_call_back with that paramters.
         *
         * If no such authorization request is stored up to now, the values are stored
         * and an incomming request that fits to the node_name and subscriber will not be stored.
         */
        void skip_authorization_request(const boost::shared_ptr< ::pubsub::subscriber>&, const node_name&);

        /**
         * @brief returns true, if at least one validation requests with the given name is stored and not jet 
         *        answered or skiped.
         */
        bool validation_requested(const node_name&) const;

        /**
         * @brief answer one validation request with the given name.
         *
         * If no such validation request is stored up to now, the values are stored
         * and an incomming request that fits to the node_name will be answered with 
         * is_valid and will not be stored.
         *
         * @param is_valid if true, validation_call_back::is_valid() will be called, 
         *                 otherwise validation_call_back::not_valid.
         * @param name the name of the node whose request should be answered.
         */
        void answer_validation_request(const node_name& name, bool is_valid);

        /**
         * @brief drop one stored validation request with the given name.
         *
         * If no validation request for the given node is stored, the node name is stored
         * and the next validation request is skipped.
         */
        void skip_validation_request(const node_name&);

        /**
         * @brief returns true, if at least one node initialization requested is stored.
         */
        bool initialization_requested(const node_name&) const;

        /** 
         * @brief similar to answer_validation_request()
         * @sa answer_validation_request()
         */
        void answer_initialization_request(const node_name&, const json::value& answer);

        /**
         * @brief defered response to an upcoming initialization request.
         *
         * The passed data is stored until a initialization request is made for the given node.
         * The request isn't directly answered, insteed, a call to the callback is stored in a
         * function that is posted onto the io_service object, that was passed to the c'tor.
         *
         * @pre the c'tor overload that takes a io_service must have been used.
         */
        void answer_initialization_request_defered(const node_name&, const json::value& answer);

        /**
         * @brief similar to skip_validation_request()
         * @sa skip_validation_request()
         */
        void skip_initialization_request(const node_name&);

        /**
         * @brief returns true if invalid_node_subscription() was called with the given parameters
         *
         * If such an entry exist, it will be deleted.
         */
        bool invalid_node_subscription_reported(const node_name& node, const boost::shared_ptr< ::pubsub::subscriber>&);

        /**
         * @brief returns true if unauthorized_subscription() was called with the given parameters
         *
         * If such an entry exist, it will be deleted.
         */
        bool unauthorized_subscription_reported(const node_name& node, const boost::shared_ptr< ::pubsub::subscriber>&);

        /**
         * @brief returns true if initialization_failed() was called with the given parameters
         *
         * If such an entry exist, it will be deleted.
         */
        bool initialization_failed_reported(const node_name& node);

        /**
         * @brief no pending unanswered or unskiped requests and no repored failures
         */
        bool empty() const;

    private:
        virtual void validate_node(const node_name& node_name, const boost::shared_ptr<validation_call_back>&);
        virtual void authorize(const boost::shared_ptr< ::pubsub::subscriber>&, const node_name& node_name, const boost::shared_ptr<authorization_call_back>&);
        virtual void node_init(const node_name& node_name, const boost::shared_ptr<initialization_call_back>&);
        virtual void invalid_node_subscription(const node_name& node, const boost::shared_ptr< ::pubsub::subscriber>&);
        virtual void unauthorized_subscription(const node_name& node, const boost::shared_ptr< ::pubsub::subscriber>&);
        virtual void initialization_failed(const node_name& node);

        typedef std::multimap<std::pair<const boost::shared_ptr< ::pubsub::subscriber>, node_name>, boost::shared_ptr<authorization_call_back> > authorization_request_list;
        typedef std::multimap<std::pair<const boost::shared_ptr< ::pubsub::subscriber>, node_name>, bool>                                        authorization_answer_list;
        typedef std::multiset<std::pair<const boost::shared_ptr< ::pubsub::subscriber>, node_name> >                                             authorization_skip_list;
        typedef std::multimap<node_name, boost::shared_ptr<validation_call_back> >                  validation_request_list;
        typedef std::multimap<node_name, bool>                                                      validation_answer_list;
        typedef std::multiset<node_name>                                                            validation_skip_list;
        typedef std::multimap<node_name, boost::shared_ptr<initialization_call_back> >              initialization_request_list;
        typedef std::multimap<node_name, json::value>                                               initialization_answer_list;     
        typedef std::multiset<node_name>                                                            initialization_skip_list;     
        typedef std::multiset<boost::tuple<node_name, boost::shared_ptr< ::pubsub::subscriber> > >  invalid_node_subscription_reported_list;
        typedef std::multiset<boost::tuple<node_name, boost::shared_ptr< ::pubsub::subscriber> > >  unauthorized_subscription_reported_list;
        typedef std::multiset<node_name>  															initialization_failed_reported_list;


        boost::asio::io_service * const             queue_; // optional reference
        mutable boost::mutex                        mutex_;

        authorization_request_list                  authorization_request_;
        authorization_answer_list                   authorization_answers_;
        authorization_skip_list                     authorizations_to_skip_;

        validation_request_list                     validation_request_;
        validation_answer_list                      validation_answers_;
        validation_skip_list                        validations_to_skip_;

        initialization_request_list                 initialization_request_;
        initialization_answer_list                  initialization_answers_;
        initialization_answer_list                  initialization_answers_defered_;
        initialization_skip_list                    initializations_to_skip_;

        invalid_node_subscription_reported_list     invalid_node_subscription_reports_;
        unauthorized_subscription_reported_list     unauthorized_subscription_reports_;
        initialization_failed_reported_list         initialization_failed_reports_;
    };
}
}
#endif // include guard


