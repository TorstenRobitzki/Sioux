# This is the CoffeeScript client library for the Sioux-PubSub-Server: 
# https://github.com/TorstenRobitzki/Sioux

global = `Function('return this')()`
global.PubSub = global.PubSub || {}

create_ajax_transport = ( url = '/pubsub' )->
    ( obj, cb )->
     
class Node 
    constructor: ( @cb )->
        @open = true
        
    is_open:
        @open
                     
class Impl
    constructor: ( @transport = create_ajax_transport() )->
        @pending_subscriptions = []
        @session_id = null
        @current_transports = 0
        @subjects = new PubSub.NodeList

    subscribe: ( node_name, callback )->
        @subjects.insert node_name, new Node callback
        @pending_subscriptions.push node_name 
        
        if @session_id || @current_transports == 0
            issue_request.call @

    issue_request= ->
        cmds = for pending in @pending_subscriptions
            { subscribe: pending }
            
        @pending_subscriptions = []
            
        payload = if @session_id then { id: @session_id, cmd: cmds } else { cmd: cmds }     

        @current_transports++     
        @transport payload, ( error, response ) => 
            receive_call_back.call @, error, response
                                  
    receive_call_back= ( error, response )->
        @current_transports--

        if error 
            disconnect.call @
        else 
            @session_id = response.id
            
            if @pending_subscriptions.length != 0 
                issue_request.call @
        
    disconnect= ->
        @session_id = null
        @current_transports = 0

        @subjects.each ( name ) => @pending_subscriptions.push name
            
        setTimeout => 
            try_reconnect.call @
        , 30000
         
    try_reconnect= ->
        issue_request.call @

impl = new Impl

# resets the internal state of the library. This function is mainly intended for testing.
PubSub.reset = ->
    impl = new Impl

# Subscribes to the node given by node_name. The given callback will be called, when the subscribed data is received,
# updated or if there is an error with the subscription. The callback have to take three arguments:
#  node_name : the name of the subscribed node
#  data      : the initial or updated data of the subscribed node. This is only valid, if the error parameter is undefined
#  error     : If not undefined, this indicates the reason, why the subscription failed. For now, there can be two reasons:
#              'invalid' which indicates, that there is no such node and 'not allowed', which indicates, that you are not
#              allowed (authorized) to subscribe to the given node. If an error was indicated, the callback will not be 
#              called again.
# 
# The behaviour of subscribing more than once to the same subject is undefined and should be avoided.

PubSub.subscribe = ( node_name, callback )->
    if arguments.length != 2
        throw 'wrong number of arguments to PubSub.subscribe'
      
    impl.subscribe node_name, callback      
                 
PubSub.unsubscribe = ( subject )->

# Sets an ajax transport function. That function takes 2 arguments, the first 
# argument is an object to be transported. The second argument is a callback takeing a boolean. That boolean indicates
# an error if set to true. The second parameter to that callback is the answer from the server. The callback will called, 
# when the http request succeded or failed. 
PubSub.configure_transport = ( new_transport )->
    impl = new Impl new_transport
