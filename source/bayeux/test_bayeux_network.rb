# Copyright (c) Torrox GmbH & Co KG. All rights reserved.
# Please note that the content of this file is confidential or protected by law.
# Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

require 'net/http/pipeline'
require 'json'
require 'minitest/unit'

class BayeuxServerProxy
    def initialize
        @server_thread = Thread.new do 
            Rake::Task['bayeux_reply'].execute()
        end
        
        while !server_is_running() do
            puts "waiting for reply server...." 
        end
    end

    def stop
        begin
            Net::HTTP.get( URI( 'http://localhost:8080/stop' ) )
        rescue EOFError
        end
        
        @server_thread.join        
    end
    
private
    def server_is_running
        sleep 0.5
        result = nil
        begin
            result = Net::HTTP.get( URI( 'http://localhost:8080/ping' ) )
            puts result
        rescue EOFError
            result = "running"
        rescue
            puts "connecting reply server: #{ $! }"
        end
        
        result
    end
    
end

class BayeuxConnection
    def initialize 
        @net = Net::BufferedIO.new( TCPSocket.new 'localhost', 8080)
#        @net = Net::HTTP.start( 'localhost', 8080 ) 
    end
    
    def close
        begin
            @net.close
#            @net.finish
        rescue
        end
    end
    
    def send text
        req = Net::HTTP::Post.new('/')
        req.body = text
        req[ 'host' ] = 'localhost'
        req[ 'Content-Type' ] = 'POST';
        req.exec @net, '1.1', req.path
        result = Net::HTTPResponse.read_new(@net)
        result.reading_body(@net, req.response_body_permitted?) {}
        
        result
#        @net.post('/', text, { 'Content-Type' => 'POST' } )
    end
    
    def self.connect
        begin
            connection = self.new
            yield connection
        rescue
            connection.close 
            raise
        end
    end
end

class BayeuxSession
    def initialize connection
        @connection = connection || BayeuxConnection.new
    end
    
    def send hash
        hash[ 'clientId' ] = @session_id unless @session_id.nil?
        text = [hash].to_json
        result = @connection.send text
        
        # Raises an HTTP error if the response is not 2xx (success).
        result.value
        JSON.parse result.body
    end
    
    def handshake
        result = send( { 'channel' => '/meta/handshake', 'version' => '1.0', 
            'supportedConnectionTypes' => ['long-polling', 'callback-polling'] } )
            
        if result.class == Array
            raise RuntimeError, "unexpected length of handshake result: #{result.length}" if result.length != 1
            
            result = result[ 0 ]
        end 

        if !result.has_key?( 'successful' ) || result[ 'successful' ] != true
            raise RuntimeError, ( result[ 'error' ] || "handshake-error" )
        end

        @session_id = result[ 'clientId' ]
    end
    
    def self.start connection = nil
        close_connection = !connection.nil?
        connection ||= BayeuxConnection.new
        begin
            session = self.new connection
            session.handshake
            yield session
        ensure
            connection.close
        end
    end
    
    def subscribe node
    end
    
    def unsubscribe node
    end
    
    def publish node, data
    end
    
    def connect
    end
end

class BayeuxProtocolTest < MiniTest::Unit::TestCase
    def setup 
        @server = BayeuxServerProxy.new
    end

    def teardown
        begin
            @server.stop
        rescue
        end
    end
 
    def successful message
        message.class == Hash && message.has_key?( 'successful' ) && message[ 'successful' ]
    end
     
    def single_message message
        return nil if message.class != Array || message.length != 1
        message[ 0 ]
    end
    
    def small_client_server_dialog session
        result = single_message( session.send( { 'channel' => '/foo/bar/chu', 'data' => true } ) )
        refute_nil( result )
        assert( successful( result ) )
    end
    
    def test_simple_subscribe_publish
        BayeuxSession.start do | session |
            small_client_server_dialog session
        end
    end
 
    def test_mulitple_session_over_multiple_connection
        Array.new( 4 ) do
            Thread.new do
                BayeuxSession.start { | session | small_client_server_dialog session }
            end 
        end.each { | t | t.join } 
    end

    def test_mulitple_session_over_single_connection
        BayeuxConnection.connect do | connection |
            Array.new( 4 ) do
                Thread.new do
                    BayeuxSession.start( connection ) { | session | small_client_server_dialog session } 
                end 
            end.each { | t | t.join } 
        end
    end
end

