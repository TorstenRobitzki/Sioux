// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_PUBSUB_KEY_H
#define SIOUX_SOURCE_PUBSUB_KEY_H

#include <string>

namespace pubsub
{
    /**
     * @brief describes what valid values a key can have
     */
    class key_domain
    {
    public:
        /**
         * @brief an unnamed domain, that compared equal to any other unnamed domain
         */
        key_domain();

        /**
         * @brief a named domain, that compares equal to any other domain, that was constructed with the same name
         */
        key_domain(const std::string& name);

        /**
         * @brief a defined, but unspecified strikt weak order
         */
        bool operator<(const key_domain& rhs) const;

        /**
         * @brief returns true, if both domains have the same name
         */
        bool operator==(const key_domain& rhs) const;

        /**
         * @brief returns !(*this == rhs)
         */
        bool operator!=(const key_domain& rhs) const;

        const std::string& name() const;
    private:
        std::string domain_name_;
    };

    /**
     * @brief key 
     */
    class key
    {
    public:
        /**
         * @brief key, constructed with default domain and value
         */
        key();

        key(const key_domain&, const std::string& value);

        bool operator==(const key& rhs) const;
        bool operator<(const key& rhs) const;

        const key_domain& domain() const;
    private:
        key_domain  domain_;
        std::string value_;
    };

}

#endif // include guard


