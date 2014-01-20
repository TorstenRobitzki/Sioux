global = `Function('return this')()`
global.PubSub = global.PubSub || {}

# List class to store a node under a name. A node can have every type, a name must be an object with all containing 
# fields beeing strings.
#
# list = new NodeList
#
# # inserting a node:
# list.insert { a: '1', b: 'foo' }, [1,2]
# 
# # updating a node: 
# list.insert { a: '1', b: 'foo' }, [1,2,3]
#
# #retrieving a nodes value:
# list.get { a: '1', b: 'foo' }
#
# # deleting a node from the list:
# list.remove { a: '1', b: 'foo' }
#
class PubSub.NodeList
    # constructs an empty list of nodes
    constructor: ->
        @list = []

    find_node = (keys, found, not_found)->
        low = 0
        high = @list.length - 1
        
        while low <= high
            mid = ( low + high ) >> 1
            val = @list[mid]
            comp = compare_keys keys, val.k
            
            return found.call( @, val, mid ) if comp == 0
            
            if comp < 0 then low = mid + 1 else high = mid - 1
            
        not_found.call @, @list, low                    
            
    # returns an existing node with the given name. If no such node was inserted before, the function will return null.            
    get: ( name ) ->
        keys = keys_from_name name

        find_node.call @, keys, ( node )->
            node.n
        , -> null
        
    check_node_name = ( name )->
        unless typeof name == 'object'
            throw new Error 'node name must be an object'
            
        empty = true
                     
        for key, value of name
            empty = false
            unless typeof( value ) == 'string'  
                throw new Error 'node name must contain only strings' 

        if empty
             throw new Error 'node name must not be empty'
                
    compare_names = (a,b) ->
        if a < b then -1 else 
            if b < a then 1 else 0
                            
    keys_from_name = ( name )->                            
        key_values = ( for key, value of name
            [ key, value ] ).sort( ( a, b )-> compare_names( a[ 0 ], b[ 0 ]) )
            
        key_values            

    compare_keys = (k1,k2)->
        return k1.length - k2.length if k1.length != k2.length
        
        for i in [ 0...k1.length ]
            for j in [ 0..1 ]
                key_compare = compare_names( k1[ i ][ j ], k2[ i ][ j ] )
                
                return key_compare if key_compare != 0 
            
        0
                    
    # inserts the given node with the given name. A next call to get( name ) will yield node. If there is already 
    # a node stored under the given name, the node will be updated.                  
    insert: ( name, node ) ->
        check_node_name.call @, name
        
        keys = keys_from_name.call( @, name )
        find_node.call @, keys, ( n )->
            n.n = node
        , ( list, index )->
            list.splice index, 0, { k: keys, n: node }

    # removes a node stored under the given name. If no such node exists, the function returns null and has no effect. 
    # A next call to get( name ) will result in returning null.
    remove: ( name ) ->
        keys = keys_from_name.call( @, name )
        find_node.call @, keys, ( n, index )->
            @list.splice index, 1
            true
        , -> false
