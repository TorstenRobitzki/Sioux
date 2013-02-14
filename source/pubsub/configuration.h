// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_PUBSUB_CONFIGURATION_H
#define SIOUX_SOURCE_PUBSUB_CONFIGURATION_H

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <iosfwd>

namespace pubsub
{
    /**
     * @brief describes update policy, node timeout etc.
     */
    class configuration
    {
    public:
        configuration();

        /**
         * @brief the time, that a node without subscriber should stay in the data model
         */
        boost::posix_time::time_duration    node_timeout() const;

        /**
         * @brief sets the node timeout to a new value
         * @post node_timeout() returns new_timeout
         */
        void node_timeout(const boost::posix_time::time_duration& new_timeout);

        /**
         * @brief The time that have to elapse, before a new version of a document will
         *        be published. 
         *
         * If at the time, where the update was made, the time isn't elapsed, the update
         * will be published, when the time elapses.
         */
        boost::posix_time::time_duration    min_update_period() const;

        /**
         * @brief the ratio of update costs to full nodes data size in %
         */
        unsigned max_update_size() const;

        /**
         * @brief sets the ratio of update costs to full nodes data size in %
         */
        void max_update_size( unsigned );

        /**
         * @brief returns true, if the configured nodes require authorization to be accessed
         */
        bool authorization_required() const;

        /**
         * @brief set the authorization_required flag to the given value
         * @post authorization_required() will return new_value
         */
        void authorization_required(bool new_value);

        /**
         * @brief prints the content of this object onto the given stream in a human readable manner
         */
        void print( std::ostream& out ) const;

    private:
        boost::posix_time::time_duration    node_timeout_;
        boost::posix_time::time_duration    min_update_period_;
        unsigned                            max_update_size_;
        bool                                authorization_required_;
    };

    /**
     * @brief prints the given configuration onto the given stream in a human readable manner
     * @relates configuration
     */
    std::ostream& operator<<( std::ostream& out, const configuration& config );

    /**
     * @brief a configuration builder for nicer configuration syntax
     */
    class configurator
    {
    public:
        const configurator& node_timeout(const boost::posix_time::time_duration&) const;
        const configurator& authorization_required() const;
        const configurator& authorization_not_required() const;

        operator const configuration() const;
    private:
        mutable configuration config_;
    };

} // namespace pubsub

#endif // include guard



