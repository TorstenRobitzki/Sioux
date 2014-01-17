# This is the CoffeeScript client library for the Sioux-PubSub-Server: 
# https://github.com/TorstenRobitzki/Sioux

global = `Function('return this')()`
global.PubSub = global.PubSub || {}

create_ajax_transport = ( url = '/pubsub' )->
    ( obj, cb )->
     
class Impl
    constructor: ( @transport = create_ajax_transport() )->
        @pending_requests = []

    subscribe: ( subject, callback )->
        @pending_requests.push [ subject, callback ]

        if @pending_requests.length == 1
            issue_request.call @

    issue_request= ->
        { s, c } = @pending_requests[ 0 ]
        
        @transport {}, ( error, response ) => 
            receive_call_back.call @, error, response
                                  
    receive_call_back= ( error, response )->
        @pending_requests.shift()

        if @pending_requests.length != 0 
            issue_request.call @
        
impl = new Impl

# resets the internal state of the library. This function is mainly intended for testing.
PubSub.reset = ->
    impl = new Impl
    
PubSub.subscribe = ( subject, callback )->
    if arguments.length != 2
        throw 'wrong number of arguments to PubSub.subscribe'
      
    impl.subscribe subject, callback      
                 
PubSub.unsubscribe = ( subject )->

# Sets an ajax transport function. That function takes 2 arguments, the first 
# argument is an object to be transported. The second argument is a callback takeing a boolean. That boolean indicates
# an error if set to true. The second parameter to that callback is the answer from the server. The callback will called, 
# when the http request succeded or failed. 
PubSub.configure_transport = ( new_transport )->
    impl = new Impl new_transport
