global = `Function('return this')()`
global.PubSub = global.PubSub || {}

set_path = ( path, val )->
    path[ path.length - 1 ] = val

add_update = ( callbacks )->
    ( path, data )->
        callbacks.delete path
        callbacks.insert path, data

add_update_range = ( callbacks )->
    ( path, num_elements, update_array )->
        stored_path = path[ path.length - 1 ]

        min = Math.min.call null, num_elements, update_array.length
        diff = num_elements - update_array.length

        if diff > 0
            set_path path, stored_path + min
            callbacks.delete_range path, diff

        for update, i in update_array.slice 0, min
            set_path path, stored_path + i
            callbacks.update path, update

        if diff < 0
            for insert, i in update_array.slice min
                set_path path, stored_path + min + i
                callbacks.insert path, insert

        path[ path.length - 1 ] = stored_path

add_delete_range = ( callbacks )->
    ( path, num_elements )->
        for i in [ 0...num_elements ]
            callbacks.delete path

PubSub.add_missing_callbacks = ( callbacks )->
    callbacks.update_range = add_update_range( callbacks ) unless callbacks[ 'update_range' ]
    callbacks.delete_range = add_delete_range( callbacks ) unless callbacks[ 'delete_range' ]
    callbacks.update       = add_update( callbacks ) unless callbacks[ 'update' ]

    callbacks
