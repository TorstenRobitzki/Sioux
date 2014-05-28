global = `Function('return this')()`

global.update_logger = ( logged_updates, without = [] )->
    result = {
        insert:       ( path, data )->                       logged_updates.push [ 'insert', path.slice(), data ],
        delete:       ( path )->                             logged_updates.push [ 'delete', path.slice() ],
        update:       ( path, data )->                       logged_updates.push [ 'update', path.slice(), data ],
        update_range: ( path, num_elements, update_array )-> logged_updates.push [ 'update_range', path.slice(), num_elements, update_array ],
        delete_range: ( path, num_elements )->               logged_updates.push [ 'delete_range', path.slice(), num_elements ]
    }

    delete result[ func ] for func in without

    result
