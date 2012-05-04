# Copyright (c) Torrox GmbH & Co KG. All rights reserved.
# Please note that the content of this file is confidential or protected by law.
# Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

require 'net/http/pipeline'
require 'json'
require 'minitest/unit'
require 'thread'

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
    class AsyncResult
        def initialize
            extend(MonitorMixin)
            @result     = nil
            @condition  = new_cond
        end
        
        def signal_data result
            synchronize do
                @result = result
                @condition.signal 
            end            
        end
        
        def wait_data
            synchronize do
                @condition.wait_while { @result.nil? } 
                @result
            end            
        end
    end
    
    def initialize 
        @net        = Net::BufferedIO.new( TCPSocket.new 'localhost', 8080 )
        @mutex      = Mutex.new
        @thread     = Thread.new { receive_loop() }
        @requests   = Array.new
        @close = false
    end
    
    def close
        begin
            @mutex.synchronize { @close = true }
            @net.close
        rescue
        end
    end
    
    def closed?
        @mutex.synchronize { return @close }
    end
    
    def send text
        result = nil

        @mutex.synchronize do
            req = Net::HTTP::Post.new('/')
            req.body = text
            req[ 'host' ] = 'localhost'
            req[ 'Content-Type' ] = 'POST';
            req.exec @net, '1.1', req.path

            result = AsyncResult.new
            @requests.push( result )
        end
        
        result.wait_data
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
protected
    def receive
        result = Net::HTTPResponse.read_new( @net )
        result.reading_body( @net, true ) {}
        result = result.read_body
    
        result
    end
    
    def receive_loop
        begin
            begin
                result = JSON.parse( receive() )
                
                wake = nil
                @mutex.synchronize { wake = @requests.shift }

                wake.signal_data result
            end while true
        rescue Exception => e 
            if !closed? 
                puts "#error: #{e.message}"  
                close
                raise
            end
        end
    end
end

class BayeuxSession
    def initialize connection
        @connection = connection || BayeuxConnection.new
    end
    
    def send hash
        hash[ 'clientId' ] = @session_id unless @session_id.nil?
        text = [ hash ].to_json
        [ @connection.send( text ) ].flatten
    end
    
    def handshake
        result = send( { 'channel' => '/meta/handshake', 'version' => '1.0', 
            'supportedConnectionTypes' => ['long-polling', 'callback-polling'] } )

        raise RuntimeError, "unexpected length of handshake result: #{result.length}" if result.length != 1
        
        result = result.shift

        if !result.has_key?( 'successful' ) || result[ 'successful' ] != true
            raise RuntimeError, ( result[ 'error' ] || "handshake-error" )
        end

        @session_id = result[ 'clientId' ]
    end
    
    def self.start connection = nil
        close_connection = connection.nil?
        connection ||= BayeuxConnection.new
        begin
            session = self.new connection
            session.handshake
            yield session
        ensure
            connection.close if close_connection
        end
    end
    
    def subscribe node
    end
    
    def unsubscribe node
    end
    
    def publish node, data
        send( { 'channel' => node, 'data' => data } )
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
        refute_nil( result, 'response to published data expected' )
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

