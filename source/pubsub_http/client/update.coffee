global = `Function('return this')()`
global.PubSub = global.PubSub || {}

update_at_operation = ( update_operations, callbacks, path )->
    [ index, argument, update_operations... ] = update_operations

    callbacks.update path.concat( [ index ] ), argument
    update_operations

delete_at_operation = ( update_operations, callbacks, path )->
    [ index, update_operations... ] = update_operations

    callbacks.delete path.concat( [ index ] )
    update_operations

insert_at_operation = ( update_operations, callbacks, path )->
    [ index, argument, update_operations... ] = update_operations

    callbacks.insert path.concat( [ index ] ), argument
    update_operations

delete_range_operation = ( update_operations, callbacks, path )->
    [ start, end, update_operations... ] = update_operations

    callbacks.delete_range path.concat( [ start ] ), end - start
    update_operations

update_range_operation = ( update_operations, callbacks, path )->
    [ start, end, substitute, update_operations... ] = update_operations

    callbacks.update_range path.concat( [ start ] ), end - start, substitute
    update_operations

edit_at_operation = ( update_operations, callbacks, path )->
    [ index, update, update_operations... ] = update_operations

    perform_updates_impl update, callbacks, path.concat( [ index ] )

    update_operations

operations = [ null, update_at_operation, delete_at_operation, insert_at_operation, delete_range_operation, update_range_operation, edit_at_operation ]

next_operation = ( update_operations, callbacks, path )->
    [ operation, update_operations... ] = update_operations

    operations[ operation ]( update_operations, callbacks, path )

apply_path = ( input, path )->
    [ path..., index ] = path

    for x in path
        throw new RangeError "bad index for edit: #{x}" if typeof input[ x ] == 'undefined'
        input = input[ x ]

    [ input, index ]

default_callbacks = ( input ) -> {
    insert: ( path, argument )->
        [ obj, index ] = apply_path input, path

        if typeof index == 'string'
            throw new RangeError "bad index for insert: #{index}" unless typeof obj[ index ] == 'undefined'
            obj[ index ] = argument
        else
            throw new RangeError "bad index for insert: #{index}" if index < 0
            throw new RangeError "bad index for insert: #{index}" if index > input.length
            obj.splice index, 0, argument

    delete: ( path )->
        [ obj, index ] = apply_path input, path

        throw new RangeError "bad index for delete: #{index}" if typeof obj[ index ] == 'undefined'

        if typeof index == 'string'
            delete obj[ index ]
        else
            obj.splice index, 1

    update: ( path, argument )->
        [ obj, index ] = apply_path input, path

        throw new RangeError "bad index for update: #{index}" if typeof obj[ index ] == 'undefined'

        obj[ index ] = argument

    update_range: ( path, num_elements, update_array )->
        [ obj, index ] = apply_path input, path
        end = index + num_elements

        throw new RangeError "bad range for update: [#{index},#{end}]" if index < 0 || index > end || end > obj.length

        obj.splice index, num_elements, update_array...

    delete_range: ( path, num_elements )->
        [ obj, index ] = apply_path input, path
        end = index + num_elements

        throw new RangeError "bad range for delete: [#{index},#{end}]" if index < 0 || index >= end || end > obj.length

        obj.splice index, num_elements
}

perform_updates_impl = ( update_operations, callbacks, path )->
    update_operations = next_operation( update_operations, callbacks, path ) while update_operations.length != 0

# Performs the callbacks on the given list of update_operations
PubSub.perform_updates = ( update_operations, callbacks )->
    perform_updates_impl( update_operations, callbacks, [] )

# Updates an array or object (input), with the given operations and returns the updated input
PubSub.update = ( input, update_operations )->
    perform_updates_impl( update_operations, default_callbacks( input ), [] )

    input
