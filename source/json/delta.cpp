// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "json/delta.h"
#include "json/json.h"
#include "tools/asstring.h"
#include <stdexcept>

namespace json {

    namespace {
        enum update_operation_code
        {
            update_at = 1,
            delete_at = 2,
            insert_at = 3,
            delete_range = 4,
            update_range = 5,
            edit_at   =6
        };

        int to_int(const value& val)
        {
            return val.upcast<number>().to_int();
        }

        value update_impl(const array& data, const array& update_operations)
        {
            array result(data);

            for ( std::size_t index = 0; index < update_operations.length(); )
            {
                const int command = to_int(update_operations.at(index++));

                switch ( command )
                {
                case update_at:
                    {
                        const int update_idx = to_int(update_operations.at(index++));
                        result.at(update_idx) = update_operations.at(index++);
                    }
                    break;
                case delete_at:
                    {
                        const int delete_idx = to_int(update_operations.at(index++));
                        result.erase(delete_idx, 1u);
                    }
                    break;
                case insert_at:
                    {
                        const int insert_idx = to_int(update_operations.at(index++));
                        result.insert(insert_idx, update_operations.at(index++));
                    }
                    break;
                case delete_range:
                    {
                        const int start_idx = to_int(update_operations.at(index++));
                        const int end_idx = to_int(update_operations.at(index++));
                        result.erase(start_idx, end_idx-start_idx);
                    }
                    break;
                case update_range:
                    {
                        int start_idx = to_int(update_operations.at(index++));
                        const int end_idx = to_int(update_operations.at(index++));
                        result.erase(start_idx, end_idx-start_idx);

                        const array& fill = update_operations.at(index++).upcast<array>();
                        for ( int i = 0, end = fill.length(); i != end; ++i, ++start_idx )
                            result.insert(start_idx, fill.at(i));
                    }
                    break;
                case edit_at:
                    {
                        const int update_idx = to_int(update_operations.at(index++));
                        const value& update_operation  = update_operations.at(index++);

                        result.at(update_idx) = update(result.at(update_idx), update_operation);
                    }
                    break;
                default:
                    throw std::runtime_error("invalid update operation: " + tools::as_string(command));
                }
            }

            return result;
        }

        value update_impl(const object& data, const array& update_operations)
        {
            object result(data);

            for ( std::size_t index = 0; index < update_operations.length(); )
            {
                const int command = to_int(update_operations.at(index++));

                switch ( command )
                {
                case update_at:
                    {
                        const string update_idx = update_operations.at(index++).upcast<string>();
                        result.at(update_idx) = update_operations.at(index++);
                    }
                    break;
                case delete_at:
                    {
                        const string delete_idx = update_operations.at(index++).upcast<string>();
                        result.erase(delete_idx);
                    }
                    break;
                case insert_at:
                    {
                        const string insert_idx = update_operations.at(index++).upcast<string>();
                        result.add(insert_idx, update_operations.at(index++));
                    }
                    break;
                case edit_at:
                    {
                        const string update_idx = update_operations.at(index++).upcast<string>();
                        const value& update_operation  = update_operations.at(index++);

                        result.at(update_idx) = update(result.at(update_idx), update_operation);
                    }
                    break;
                default:
                    throw std::runtime_error("invalid update operation: " + tools::as_string(command));
                }
            }

            return result;
        }

        value update_impl(const string& data, const array& update_operations)
        {
            string result(data);
            return result;
        }

        value update_impl(const value& a, const array& update_operations)
        {
            struct update_by_type : default_visitor
            {
                update_by_type(const array& update_instructions)
                    : instructions_(update_instructions)
                    , result_(update_instructions)
                {
                }

                void visit(const string& val)
                {
                    result_ = update_impl(val, instructions_);
                }

                void visit(const object& val)
                {
                    result_ = update_impl(val, instructions_);
                }

                void visit(const array& val)
                {
                    result_ = update_impl(val, instructions_);
                }

                const array&    instructions_;
                value           result_;
            } visi(update_operations);

            a.visit(visi);
            return visi.result_;
        }
    }

    value update(const value& a, const value& update_operations)
    {
        struct is_array_visitor : default_visitor
        {
            is_array_visitor(const value& a, const value& update_operations)
                : arg_(a)
                , result_(update_operations)
            {
            }

            void visit(const array& update_operations) 
            {
                result_ = update_impl(arg_, update_operations);
            }

            const value&    arg_;
            value           result_;

        } v(a, update_operations);

        update_operations.visit(v);

        return v.result_;
    }

} // namespace json

