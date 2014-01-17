assert = require "assert"
require( './pubsub.coffee' )

describe "pubsub.http interface", ->

    http_requests = []
    record_http_requests = ( request, cb )->
        http_requests.push 
            request: request
            cb: cb

    simulate_response = ( response )->
        if record_http_requests.length == 0 
            throw 'no response expected'
            
        { request, cb } = http_requests.shift()
        
        cb( true, response )         
                
    describe "when subscribing to a node", ->
    
        beforeEach () ->
            PubSub.reset()
            http_requests = []
            PubSub.configure_transport record_http_requests
            
        it "should throw, if the wrong number of arguments are given", ->
            
            assert.throws ( -> PubSub.subscribe() ),   
                "wrong number of arguments to PubSub.subscribe"
            
            assert.throws ( -> PubSub.subscribe( 1 ) ),   
                "wrong number of arguments to PubSub.subscribe"

            assert.throws ( -> PubSub.subscribe( 1, 2, 3 ) ),   
                "wrong number of arguments to PubSub.subscribe"

        it "should perform a http request", ->
            PubSub.subscribe { a: 1, b: 'Hallo' }, ->
            assert.equal http_requests.length, 1
            
        it "should not perform a second http request, when adding a second subscription", -> 
            PubSub.subscribe { a: 1, b: 'Hallo' }, ->
            PubSub.subscribe { a: 2, b: 'Hallo' }, ->
            assert.equal http_requests.length, 1
        
        it "should transport the second subscription, after the server responded", ->
            PubSub.subscribe { a: 1, b: 'Hallo' }, ->
            PubSub.subscribe { a: 2, b: 'Hallo' }, ->

            simulate_response 1
            assert.equal http_requests.length, 1
                    
        it "the second transport must contain the server assigned session id"