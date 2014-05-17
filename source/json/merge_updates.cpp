#include "json/delta.h"
#include "json/json.h"

using namespace json;

array json::merge_updates( const array& first, const array& second )
{
    json::array result = first + second;
    result.at( 3 ) = json::number(2);

    return result;
}
