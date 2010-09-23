// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_DATA_DOCUMENT_H
#define SIOUX_SOURCE_DATA_DOCUMENT_H

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <list>

namespace data
{

    /**
     * @brief the data of a single node.
     */
    class document
    {
    public:
        /**
         * @brief an empty document
         */
        document();

        /**
         * @brief the size of the serialized document in bytes
         */
        std::size_t size() const;
    };

    class subscriber
    {
    };

    class node_name
    {
    };

    /**
     * @brief describes update policy, node timeout
     */
    class configuration
    {
    public:
        /**
         * @brief the time, that a node without subscriber should stay in the data model
         */
        boost::posix_time::time_duration    node_timeout() const;

        /**
         * @brief The time that have to elapse, before a new version of a document will
         *        be published. 
         *
         * If at the time, where the update was made, the time isn't elapsed, the update
         * will be published, when the time elapses.
         */
        boost::posix_time::time_duration    min_update_period() const;

        /**
         * @brief the ratio of update costs to full document size in %
         */
        unsigned max_update_size() const;
    };

    /**
     * @brief version of a document
     */
    class document_version
    {
    public:
        /**
         * @brief first, initial version of a document
         */
        document_version();

        /**
         * @brief returns true, if this and rhs are different versions
         */
        bool operator==(const document_version& rhs) const;
    }; 

    /**
     * @brief describes, how to bring a document from one version to an higher version
     */
    class update
    {
    public:
        update(const document& first_version, const document& next_version);

        /**
         * @brief the size of the serialized update in bytes
         */
        std::size_t size() const;

    };

    /**
     * @brief a node in the data model
     *
     * Keeps a version history.
     *
     */
    class node
    {
    public:

        /**
         * @brief the content of the node
         */
        const document& content() const;

        /**
         * @brief list of currently subscribed clients
         */
        const std::list<subscriber>& subscribers() const;
    };

} // namespace data

#endif // include guard


