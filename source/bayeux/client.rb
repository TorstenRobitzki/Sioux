require 'json'
require 'monitor'

module Bayeux
    @path = '/'

    def self.protocol_path= path
        @path = path
    end
    
    def self.protocol_path
        @path
    end
    
    class Connection
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
                req = Net::HTTP::Post.new( Bayeux.protocol_path )
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

    class Session
        def initialize connection
            @connection = connection || Connection.new
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
            connection ||= Connection.new
            begin
                session = self.new connection
                session.handshake
                yield session
            ensure
                connection.close if close_connection
            end
        end
        
        def subscribe node
            send( { 'channel' => '/meta/subscribe', 'subscription' => node } )
        end
        
        def subscribe_and_wait node, timeout_s = 2
            end_time = Time.new + timeout_s
            
            result = subscribe node
            result = result.concat connect until result.length >= 1 || Time.new > end_time  

            raise RuntimeError, "timeout while waiting for subscription response #{node}" if result.empty?
            raise RuntimeError, "unexpected response while subscribing to #{node}" if result.length != 1
            raise RuntimeError, "failing to subscribe to #{node}, #{result}" if result.shift[ 'successful' ] != true
        end
        
        def unsubscribe node
            send( { 'channel' => '/meta/unsubscribe', 'subscription' => node } )
        end
        
        def publish node, data
            send( { 'channel' => node, 'data' => data } )
        end
        
        # performs a synchronous bayeux 'connect', the result is the payload without the connect response
        def connect
            result = send( { 'channel' => '/meta/connect', 'connectionType' => 'long-polling' } )
            connect_result = result.select { | message | message[ 'channel' ] == '/meta/connect' } 
            
            raise RuntimeError, "no response to connect request" if connect_result.empty?
            raise RuntimeError, "multiple responses to connect request: #{connect_result}" unless connect_result.length == 1
            
            connect_result = connect_result.shift
            raise RuntimeError, "connect error: #{connect_result}" unless connect_result[ 'successful' ] == true
            
            result.reject { | message | message[ 'channel' ] == '/meta/connect' }
        end
    end
end
