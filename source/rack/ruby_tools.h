// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef RACK_RUBY_TOOLS_H_
#define RACK_RUBY_TOOLS_H_

#include <ruby.h>
#include <string>

#include "tools/substring.h"

namespace pubsub
{
    class node_name;
}

namespace json
{
    class value;
    class string;
}

namespace rack
{
    int from_hash( VALUE hash, const char* entry );
    VALUE rb_str_new_sub( const tools::substring& s );
    VALUE rb_str_new_std( const std::string& s );

    /**
     * @attention the returned string will get invalid if the input to function becomes unreferenced or be changed.
     */
    tools::substring rb_str_to_sub( VALUE );
    std::string rb_str_to_std( VALUE );
    json::string rb_str_to_json( VALUE );

    /**
     * @brief converts a node_name into a ruby hash
     */
    VALUE node_to_hash( const pubsub::node_name& node_name );

    /**
     * @brief converts a ruby hash to a node name
     */
    pubsub::node_name hash_to_node( VALUE hash );

    /**
     * @brief converts a json value into a corresponding ruby object
     */
    VALUE json_to_ruby( const json::value& data );

    /**
     * @brief converts a ruby object into a json value
     *
     * In case of an error, the node_name is use to produce a meaningful error message.
     */
    json::value ruby_to_json( VALUE data, const pubsub::node_name& node_name );


    /**
     * @brief assigns the data-ptr of a T_DATA object to the pointer given in the c'tor
     *        and assigns the data-ptr back to 0 in the d'tor
     */
    class local_data_ptr
    {
    public:
        template < class T >
        local_data_ptr( VALUE object, T& data ) : obj_( object )
        {
            Check_Type( object, T_DATA );
            DATA_PTR( object ) = &data;
        }

        ~local_data_ptr()
        {
            DATA_PTR( obj_ ) = 0;
        }
    private:
        local_data_ptr( const local_data_ptr& );
        local_data_ptr& operator=( const local_data_ptr& );

        const VALUE obj_;
    };
}

#endif /* RACK_RUBY_TOOLS_H_ */
