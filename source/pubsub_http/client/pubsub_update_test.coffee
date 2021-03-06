assert = require( 'chai' ).assert

require './update.coffee'
require './pubsub.coffee'

update_at       = 1
delete_at       = 2
insert_at       = 3
delete_range    = 4
update_range    = 5
edit_at         = 6

describe "pubsub.http interface", ->

    describe "given the client is subscribed to a node", ->

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

        updates = []

        beforeEach ->
            updates = []
            PubSub.reset()
            http_requests = []
            PubSub.configure_transport record_http_requests
            
            PubSub.subscribe { a: '1', b: 'Hallo' }, ( node, data, error )->
                assert.deepEqual node, { a: '1', b: 'Hallo' }
                assert.isUndefined error
                updates.push JSON.parse( JSON.stringify data )

            simulate_response { id: 'abc',  update: [ { key: { a: '1', b: 'Hallo' }, data: [ 1, 2, 3, 4 ], version: 22345 } ] }

        describe "and an update contains delta updates", ->

            describe "and the update comes from the the old version", ->

                beforeEach ->
                    simulate_response { 
                        id: 'abc', 
                        update:[ { key: { a: '1', b: 'Hallo' }, update: [ delete_range, 1, 3 ], from: 22345, version: 22346 } ] }

                it "will update the given node", ->
                    assert.deepEqual updates, [ [ 1, 2, 3, 4 ], [ 1, 4 ] ] 

                it "will update the given node and that node after a second update", ->
                    simulate_response { 
                        id: 'abc', 
                        update:[ { key: { a: '1', b: 'Hallo' }, update: [ update_at, 1, 2 ], from: 22346, version: 22347 } ] }

                    assert.deepEqual updates, [ [ 1, 2, 3, 4 ], [ 1, 4 ], [ 1, 2 ] ] 

            describe "and the update is for an unknown version", ->

                beforeEach ->            
                    simulate_response { 
                        id: 'abc', 
                        update:[ { key: { a: '1', b: 'Hallo' }, update: [ delete_range, 1, 3 ], from: 22344, version: 22346 } ] }

                it "will resubscribe, if the given version is unknown", ->
                    assert.deepEqual(
                        http_requests.reverse()[ 0 ].request, 
                        { id: 'abc', cmd: [ { unsubscribe: { a: '1', b: 'Hallo' } }, { subscribe: { a: '1', b: 'Hallo' } } ] } )

                it "will not call the callback, if the given version is unknown", ->
                    assert.deepEqual updates, [ [ 1, 2, 3, 4 ] ] 



