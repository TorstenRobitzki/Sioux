global = `Function('return this')()`
global.PubSub = global.PubSub || {}

update_at       = 1
delete_at       = 2
insert_at       = 3
delete_range    = 4
update_range    = 5
edit_at         = 6

update_at_operation = ( input, update_operations )->
    index = argument = 0
    [ index, argument, update_operations... ] = update_operations

    throw new RangeError "bad index for update: #{index}" if typeof input[ index ] == 'undefined'

    if typeof index == 'string'
        input[ index ] = argument
    else
        input.splice index, 1, argument

    [ input, update_operations ]

delete_at_operation = ( input, update_operations )->
    index = 0
    [ index, update_operations... ] = update_operations

    throw new RangeError "bad index for delete: #{index}" if typeof input[ index ] == 'undefined'

    if typeof index == 'string'
        delete input[ index ]
    else
        input.splice index, 1

    [ input, update_operations ]

insert_at_operation = ( input, update_operations )->
    index = argument = 0
    [ index, argument, update_operations... ] = update_operations

    if typeof index == 'string'
        throw new RangeError "bad index for insert: #{index}" unless typeof input[ index ] == 'undefined'
        input[ index ] = argument
    else
        throw new RangeError "bad index for insert: #{index}" if index < 0
        throw new RangeError "bad index for insert: #{index}" if index > input.length
        input.splice index, 0, argument

    [ input, update_operations ]

delete_range_operation = ( input, update_operations )->
    start = end = 0
    [ start, end, update_operations... ] = update_operations

    throw new RangeError "bad range for delete: [#{start},#{end}]" if start < 0 || start >= end || end > input.length

    input.splice start, end - start

    [ input, update_operations ]

update_range_operation = ( input, update_operations )->
    start = end = substitute = 0 
    [ start, end, substitute, update_operations... ] = update_operations

    throw new RangeError "bad range for update: [#{start},#{end}]" if start < 0 || start > end || end > input.length

    input.splice start, end - start, substitute...

    [ input, update_operations ]

operations = [ null, update_at_operation, delete_at_operation, insert_at_operation, delete_range_operation, update_range_operation ]

next_operation = ( input, update_operations )->
    operation = 0

    [ operation, update_operations... ] = update_operations

    operations[ operation ]( input, update_operations )

# Updates an array or object (input), with the given operations and returns the updated value 
PubSub.update = ( input, update_operations )->
    [ input, update_operations ] = next_operation( input, update_operations ) while update_operations.length != 0

    input
