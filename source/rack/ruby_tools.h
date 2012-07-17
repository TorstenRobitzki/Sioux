// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef RACK_RUBY_TOOLS_H_
#define RACK_RUBY_TOOLS_H_

#include <ruby.h>
#include <string>

#include "tools/substring.h"

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
