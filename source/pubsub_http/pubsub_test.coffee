sinon  = require 'sinon'
chai   = require 'chai'
chaiHelper = require './helpers_test'

require './pubsub.coffee'

assert = chai.assert
chai.use chaiHelper
expect = chai.expect

describe "pubsub.http interface", ->

    http_requests = []
    record_http_requests = ( request, cb )->
        http_requests.push 
            request: request
            cb: cb

    simulate_response = ( response )->
        if http_requests.length == 0 
            throw 'no response expected'
            
        { request, cb } = http_requests.shift()
        
        cb( false, response )         
                
    simulate_error = ->
        if http_requests.length == 0 
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
            
            assert.equal http_requests.length, 1, 'one request'
            simulate_error()

            @clock.tick 25000
            assert.equal http_requests.length, 0, 'no request within the timeout period'

            @clock.tick 35000
            assert.equal http_requests.length, 1
            assert.deepEqual http_requests[ 0 ].request, { cmd: [ { subscribe: { a: '1', b: 'Hallo' } } ] }
            
        it "will contain all subscribed subjects, when retrying", ( done )->
            PubSub.subscribe { a: '1', b: 'Hallo' }, ->
            
            assert.equal http_requests.length, 1, 'one request'
            simulate_error()

            # Subscribe during the timeout 
            setTimeout ->
                PubSub.subscribe { a: '1', b: 'Moin' }, ->
            , 25000

            setTimeout ->
                assert.deepEqual http_requests[ 0 ].request, { cmd: [ 
                    { subscribe: { a: '1', b: 'Hallo' } }, { subscribe: { a: '1', b: 'Moin' } } ] }
                    
                done()                    
            , 35000

            @clock.tick 35000

    describe "when beeing subscribed", ->
    
        beforeEach ->
            PubSub.reset()
            http_requests = []
            PubSub.configure_transport record_http_requests
            
            PubSub.subscribe { a: '1', b: 'Hallo' }, ->
            PubSub.subscribe { a: '2', b: 'Hallo' }, ->

            simulate_response { id: 'abc',  "update": [ { "key": { a: '1', b: 'Hallo' }, "data": 12.45, "version": 22345 } ] }

            @clock = sinon.useFakeTimers()
            
        afterEach ->
            @clock.restore()

        it "will resubscribe after an error occured", ->

            simulate_error()

            @clock.tick 25000
            assert.equal http_requests.length, 0, 'no request within the timeout period'

            @clock.tick 35000
            assert.equal http_requests.length, 1, 'there is exactly one request after the reconnect timeout'
            expect( http_requests[ 0 ].request ).to.contain.key 'cmd'
            expect( http_requests[ 0 ].request.cmd ).to.have.deepMembers [
                { subscribe: { a: '1', b: 'Hallo' } },
                { subscribe: { a: '2', b: 'Hallo' } }
            ], 'Contains all subscribed subjects'
            expect( http_requests[ 0 ].request ).to.not.contain.key 'id'
            
        it "a resubscription will contain all requests", ( done )->

            simulate_error()

            # Subscribe during the timeout 
            setTimeout ->
                PubSub.subscribe { a: '1', b: 'Moin' }, ->
            , 25000

            setTimeout ->
                expect( http_requests[ 0 ].request.cmd ).to.have.deepMembers [
                    { subscribe: { a: '1', b: 'Hallo' } },
                    { subscribe: { a: '2', b: 'Hallo' } },
                    { subscribe: { a: '1', b: 'Moin' } }
                ], 'Contains all subscribed subjects'
                    
                done()                    
            , 35000

            @clock.tick 35000
            
        it "will always keep one request open"
        it "calls the callback when updates are comming"
        
    describe "when usubscribing",->                    
            
        it "will resubscribe, when the server answers with an unknown session id"            