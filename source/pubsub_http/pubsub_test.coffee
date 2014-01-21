assert = require 'assert'
sinon  = require 'sinon'
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
        
        cb( false, response )         
                
    simulate_error = ->
        if record_http_requests.length == 0 
            throw 'no response expected'
                        
        { request, cb } = http_requests.shift()
        
        cb( true )         

    describe "when subscribing to a node", ->
    
        beforeEach ->
            PubSub.reset()
            http_requests = []
            PubSub.configure_transport record_http_requests
            
            @clock = sinon.useFakeTimers()
            
        afterEach ->
            @clock.restore()
                        
        it "should throw, if the wrong number of arguments are given", ->
            
            assert.throws ( -> PubSub.subscribe() ),   
                "wrong number of arguments to PubSub.subscribe"
            
            assert.throws ( -> PubSub.subscribe( 1 ) ),   
                "wrong number of arguments to PubSub.subscribe"

            assert.throws ( -> PubSub.subscribe( 1, 2, 3 ) ),   
                "wrong number of arguments to PubSub.subscribe"

        it "should perform a http request", ->
            PubSub.subscribe { a: '1', b: 'Hallo' }, ->
            assert.equal http_requests.length, 1
            
        it "should generate a valid http request to the server", ->
            PubSub.subscribe { a: '1', b: 'Hallo' }, ->
            assert.deepEqual http_requests[ 0 ].request, { cmd: [ { subscribe: { a: '1', b: 'Hallo' } } ] }
                                
        it "should not perform a second http request, when adding a second subscription", -> 
            PubSub.subscribe { a: '1', b: 'Hallo' }, ->
            PubSub.subscribe { a: '2', b: 'Hallo' }, ->
            assert.equal http_requests.length, 1
        
        it "should transport the second subscription, after the server responded", ->
            PubSub.subscribe { a: '1', b: 'Hallo' }, ->
            PubSub.subscribe { a: '2', b: 'Hallo' }, ->

            simulate_response { id: 'abc' }
            assert.equal http_requests.length, 1
            assert.deepEqual http_requests[ 0 ].request, { id: 'abc', cmd: [ { subscribe: { a: '2', b: 'Hallo' } } ] }
                    
        it "should transport the second and third subscription, after the server responded", ->
            PubSub.subscribe { a: '1', b: 'Hallo' }, ->
            PubSub.subscribe { a: '2', b: 'Hallo' }, ->
            PubSub.subscribe { a: '3', c: 'false' }, ->

            simulate_response { id: 'abc' }
            assert.equal http_requests.length, 1
            assert.deepEqual http_requests[ 0 ].request, 
                { 
                    id: 'abc', 
                    cmd: [ 
                        { subscribe: { a: '2', b: 'Hallo' } },
                        { subscribe: { a: '3', c: 'false' } } ] }
                            
        it "will retry after a timeout, when the server is not reachable", ->
            PubSub.subscribe { a: '1', b: 'Hallo' }, ->
            
            simulate_error()

            @clock.tick 25000
            assert.equal http_requests.length, 0

            @clock.tick 10000
            assert.equal http_requests.length, 1
            