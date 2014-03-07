chai            = require 'chai'
child_process   = require 'child_process'
http            = require 'http'

require '../pubsub_http/pubsub.js'

assert = chai.assert
expect = chai.expect

host = '127.0.0.1'
port = 8080 

start_rack_server = ( done )->
    @server = child_process.spawn 'ruby', ['./source/rack/rack_server_for_pubsub_test.rb', 'start']

    #@server.stdout.on 'data', ( data ) -> console.log "start: #{data}"
    @server.stderr.on 'data', ( data ) -> console.log "start: #{data}"
    done()

stop_rack_server = ( done )->
    @stop = child_process.spawn 'ruby', ['./source/rack/rack_server_for_pubsub_test.rb', 'stop']

    #@stop.stdout.on 'data', ( data ) -> console.log "stop: #{data}"
    @stop.stderr.on 'data', ( data ) -> console.log "stop: #{data}"

    @server.on 'exit', ( code )-> done()

describe "pubsub.http rack", ->

    describe "used as a rack server", -> 
        
        beforeEach ( done ) ->
            @timeout 5000
            start_rack_server ->
                # give the server some time to start
                setTimeout done, 2000                
            
        afterEach ( done )->
            @timeout 5000
            stop_rack_server done

        it 'it response to a GET', ( done )->

            req = http.request { agent: false, host: host, port: port }, ( result )->
                expect( result.statusCode ).to.eql 200

                body = ''
                
                result.on 'data', (chunk)-> body += chunk
                result.on 'end', ->
                    expect( body ).to.eql 'Hello Rack!'

                    done() # end of test 
                
                result.on 'error', -> done() 
                                                                        
            req.setHeader 'Connection', 'close'                
            req.end()
            
    describe "using the rack server as a pubsub server",->   
        closing = false

        before ( done ) ->
            @timeout 5000
            closing = false

            start_rack_server ->
                # give the server some time to start
                setTimeout done, 2000                

        beforeEach ->
            PubSub.configure_transport ( obj, cb, url = '/pubsub' )->

                unless closing
                    try
                          
                        req = http.request { host: host, port: port, method: 'POST', path: url }, ( result )->
                            
                            body = ''
                            
                            result.on 'data', (chunk)-> body += chunk
                            result.on 'end', ->
                                cb( false, JSON.parse( body ) )                                                                 
        
                        req.write JSON.stringify obj
                        req.end()
                        
                        req.on 'error', -> 
                    catch error
                        cb true                    

        after ( done )->           
            closing = true

            @timeout 60000
            try
                stop_rack_server done
            catch error            

        afterEach ->
            PubSub.reset()

        describe "the rack library provides an easy way to call adapter.publish", ->            
            it 'is possible to get a simple echo back', ( done )->
                PubSub.publish { echo: 'Hallo Echo' }, ( error, resp )->
                    expect( error ).to.be.false
                    expect( resp ).to.eql 'Hallo Echo'
                    done()
                     
        describe "used as a pubsub server", -> 
                        
            it 'responds to a subscription', ( done )->
                PubSub.subscribe { a: 'a', b: 'b' }, ( node_name, data, error )->
                    expect( node_name ).to.eql { a: 'a', b: 'b' }
                    expect( data ).to.eql "Test-Data"
                    expect( error ).to.satisfy ( v ) -> !v      
                    
                    done()        
    
            it 'can subscribe to more than one nodes', ( done )->
                received = 0
                do_done = ->
                    received++
                    done() if received == 2
                    
                PubSub.subscribe { a: '1', b: 'b' }, ( node_name, data, error )->
                    do_done()        

                PubSub.subscribe { a: '2', b: 'b' }, ( node_name, data, error )->
                    do_done()        

            it 'a node can be updated', ( done )->
                PubSub.subscribe { a: 'update_test', b: 'b' }, ( node_name, data, error )->
                    expect( error ).to.satisfy ( v ) -> !v
                    
                    unless data == 'foo'
                        PubSub.publish { update: { a: 'update_test', b: 'b' }, value: 'foo' }, ( error, resp )->
                            expect( error ).to.be.false
                            expect( resp ).to.eql 'ok'
                    else
                        done()                    
                                                             