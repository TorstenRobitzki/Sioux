sinon           = require 'sinon'
chai            = require 'chai'
child_process   = require 'child_process'
http            = require 'http'

require '../pubsub_http/pubsub.js'

assert = chai.assert
expect = chai.expect

run_ruby_script = ( script )->
    rc = child_process.exec script, ( err, stdout, stderr )->
        #console.log "stop: #{stdout}" if stdout
        console.log "stop: #{stderr}" if stderr
        throw err if err 

start_rack_server = ( done )->
    @server = child_process.spawn 'ruby', ['./source/rack/rack_server_for_pubsub_test.rb', 'start']

    #@server.stdout.on 'data', ( data ) -> console.log "start: #{data}"
    @server.stderr.on 'data', ( data ) -> console.log "start: #{data}"
    done()

stop_rack_server = ( done )->
    run_ruby_script "ruby ./source/rack/rack_server_for_pubsub_test.rb stop"
    @server.on 'exit', ( code )->
        done()

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
            console.log "starting request..."

            req = http.request { agent: false, host: '127.0.0.1', port: 8080 }, ( result )->
                expect( result.statusCode ).to.eql 200

                body = ''
                
                result.on 'data', (chunk)-> body += chunk
                result.on 'end', ->
                    expect( body ).to.eql 'Hello Rack!'

                    done() # end of test 
                                
            req.setHeader 'Connection', 'close'                
            req.end()
