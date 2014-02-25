require 'rack'
require 'rack/lint'
require 'net/http'

require 'minitest/unit'
require_relative 'lib/rack/handler/sioux'
require_relative '../bayeux/client'
require_relative '../bayeux/network_test_cases'
require_relative '../tests/bayeux_reply'

class RackTestHandler
    attr_reader :environment
    
    def call environment
        raise StopIteration if environment[ 'PATH_INFO' ] == '/stop'
        raise RuntimeError, "raised for testing"  if environment[ 'PATH_INFO' ] == '/throw'
        
        if environment[ 'PATH_INFO' ] == '/errors'
            log = environment[ 'rack.errors' ]
            log.puts 4
            log.write "=2+2"
            log.flush
        end        
        
        @environment = environment
        [ 200, {"Content-Type" => "text/html"}, [ "Hello Rack!" ] ]
    end
end

module SetupRackserver
    @@HOST = '127.0.0.1'
    @@PORT = 8080
    @@URI  = URI( "http://#{@@HOST}:#{@@PORT}" )
    
    def wait_until_started to
        now  = Time.now
        stop = false
        
        while Time.now < now + to and !stop do
            begin
                s = TCPSocket.new @@HOST, @@PORT
                s.close
                stop = true
            rescue
            end
        end
        
        raise "Timeout waiting for sever!" unless stop
    end

    def setup adapter = nil, handler = nil, options = {}
        options = { 'Environment' => 'debug', 'Host' => @@HOST, 'Port' => @@PORT, 'Adapter' => adapter }.merge options
        
        @app = handler || RackTestHandler.new
        @server_loop = Thread.new do
            begin
            puts "####starting server"
                Rack::Handler::Sioux.run( Rack::Lint.new( @app ), options )
            rescue => error
                puts "Error Rack::Handler::Sioux.run() #{error.message}"
                puts error.backtrace.join("\n")        
            end
        end

        wait_until_started 2
    end

    def ignore_errors
        begin
            yield
        rescue
        end
    end

    def teardown
        ignore_errors { Net::HTTP.get( @@URI + '/stop' ) }
        
        @server_loop.join
    end
end

class RackIntegrationTest < MiniTest::Unit::TestCase
    include SetupRackserver
    
    def test_get_method
        assert_equal "Hello Rack!", Net::HTTP.get( @@URI )

        assert_equal 'GET', @app.environment[ 'REQUEST_METHOD' ]
        assert_equal '', @app.environment[ 'SCRIPT_NAME' ]
        assert_equal '/', @app.environment[ 'PATH_INFO' ]
        assert_equal '', @app.environment[ 'QUERY_STRING' ]
        assert_equal @@HOST, @app.environment[ 'SERVER_NAME' ]
        assert_equal @@PORT.to_s, @app.environment[ 'SERVER_PORT' ]
    end
    
    def test_remote_addr_and_port
        assert_equal "Hello Rack!", Net::HTTP.get( @@URI )

        assert_equal '127.0.0.1', @app.environment[ 'REMOTE_ADDR' ]
        assert @app.environment[ 'REMOTE_ADDR' ]
        refute_equal @@PORT.to_s, @app.environment[ 'REMOTE_PORT' ]
    end
            
    def test_correctly_translated_bindestrich
        assert_equal "Hello Rack!", Net::HTTP.get( @@URI )

        assert @app.environment[ 'HTTP_USER_AGENT' ]
        refute @app.environment[ 'HTTP_USER-AGENT' ]
    end
            
    def test_post_body
        Net::HTTP.post_form @@URI + '/', { 'param1' => 5, 'param2' => 'text' }

        assert_equal 'POST', @app.environment[ 'REQUEST_METHOD' ]
        assert_equal 'param1=5&param2=text', @app.environment[ 'rack.input' ].read
    end   
    
    def test_large_post_body
        large_body = ( 0..5000 ).collect{ ( 65 + rand( 26 ) ).chr }.join
        
        req = Net::HTTP::Post.new( '/' )
        req.body = large_body
        
        Net::HTTP.start(@@HOST, @@PORT) do |http|
            http.request(req)
        end
        
        assert_equal 'POST', @app.environment[ 'REQUEST_METHOD' ]
        assert_equal large_body, @app.environment[ 'rack.input' ].read
    end
    
    def test_rack_version
        Net::HTTP.get @@URI
        assert_equal Rack::VERSION, @app.environment[ 'rack.version' ]
    end     
    
    def test_handler_throws
        ignore_errors { Net::HTTP.get( @@URI + '/throw' ) }
    end

    def test_receive_http_headers
        result = Net::HTTP.start @@HOST, @@PORT do | http |
            request = Net::HTTP::Get.new '/index.html'
            request[ 'request_header' ] = 'FooBar'
            request[ 'OtherHeader' ] = 42
            
            http.request request
        end 

        assert_equal 'FooBar', @app.environment[ 'HTTP_REQUEST_HEADER' ]
        assert_equal '42', @app.environment[ 'HTTP_OTHERHEADER' ]
    end 
    
    # The Net::HTTP libraries does some transformations on the request headers, to circumvent this, a request
    # is directly sended over TCP. issue #12 describes a case, where an all upper case header will not be converted
    def test_receive_ua_cpu_header
        lines = [
            "GET /index.html HTTP/1.1\r\n",
            "Host: google.de\r\n",
            "UA-CPU: AMD64\r\n",
            "Connection: close\r\n",
            "\r\n" ]
            
        require 'socket'            
        socket = TCPSocket.new @@HOST, @@PORT 
        begin
            lines.each do | line |
                socket.write line
            end
            sleep 1 # give the server some time to hand the request to the handler
        ensure
            socket.close
        end            

        assert_equal 'AMD64', @app.environment[ 'HTTP_UA_CPU' ]
    end 

    def test_url_http_schema
        Net::HTTP.get @@URI
        assert_equal 'http', @app.environment[ 'rack.url_scheme' ]
    end
    
    def test_error_stream
        assert_equal "Hello Rack!", Net::HTTP.get( @@URI + '/errors' )
    end

    def test_multithread_value
        assert_equal "Hello Rack!", Net::HTTP.get( @@URI )
        assert_equal false,  @app.environment[ 'rack.multithread' ]
    end 

    def test_multiprocess_value
        assert_equal "Hello Rack!", Net::HTTP.get( @@URI )
        assert_equal false,  @app.environment[ 'rack.multiprocess' ]
    end 

    def test_run_once_value
        assert_equal "Hello Rack!", Net::HTTP.get( @@URI )
        assert_equal false,  @app.environment[ 'rack.run_once' ]
    end
    
    def test_path_info
        assert_equal "Hello Rack!", Net::HTTP.get( @@URI + '/A/B/C' )

        assert_equal '', @app.environment[ 'SCRIPT_NAME' ]
        assert_equal '/A/B/C', @app.environment[ 'PATH_INFO' ]
    end 
    
    def test_query
        assert_equal "Hello Rack!", Net::HTTP.get( @@URI + '/A/B/C' )

        assert_equal '', @app.environment[ 'QUERY_STRING' ]
    end
    
    # just make sure, that the case is handled by the server and does not crash the application
    # currently there is a upper limit for http header length
    def test_very_long_request
        header_length = 10000
        
        result = Net::HTTP.start @@HOST, @@PORT do | http |
            request = Net::HTTP::Get.new '/index.html'
            request[ 'request_header' ] = ( 1..header_length ).collect { 'a' }.reduce(:+)
            
            begin 
                http.request request
            rescue
            end                
        end 
    end

end

class RackPublishIntegrationTest < MiniTest::Unit::TestCase

    # adapter implementation    
    def init root
        @root = root
    end
    
    def validate_node node
        @validated_node = node
        true
    end
    
    def authorize user, node
        @authorized_user = user
        @authorized_node = node
        true
    end
    
    def node_init node
        @initialized_node = node
        [{ 'value' => 42 }, true, nil ]
    end
    
    class Init
        include SetupRackserver
    end

    def setup
        @root = nil
        @init = Init.new  
        @init.setup self
    end
    
    def teardown
        @init.teardown
    end
    
    def test_initial_root
        refute_nil @root, "No initial root"
    end

    def test_root_assignable
        @root[ { 'node' => 'name' } ] = 'Foo'; 
    end
    
    def test_validation_requested
        @root.subscribe_for_testing( { 'node' => 'name', 'param' => '42' } )
        sleep 1 
        assert_equal( { 'node' => 'name', 'param' => '42' }, @validated_node )
    end
    
    def test_authorization_requested
        @root.subscribe_for_testing( { 'node' => 'name', 'param' => '42' } )
        sleep 1 
        assert_equal( { 'node' => 'name', 'param' => '42' }, @authorized_node )
    end

    def test_initialization_requested
        @root.subscribe_for_testing( { 'node' => 'name', 'param' => '42' } )
        sleep 1 
        assert_equal( { 'node' => 'name', 'param' => '42' }, @initialized_node )
    end
    
end

class PubsubAuthorizationRequestedParamter < MiniTest::Unit::TestCase

    # adapter implementation    
    def init root
        @root = root
    end
    
    def validate_node node
        true
    end
    
    def authorize user, node
        @authorized_called = true
        true
    end
    
    def node_init node
        [{ 'value' => 42 }, true, nil ]
    end
    
    class Init
        include SetupRackserver
    end

    def setup
        @authorized_called = false
        @root = nil
        @init = Init.new  
    end
    
    def teardown
        @init.teardown
    end
    
    def test_authorization_requested_if_configured
        @init.setup self, nil, { 'Pubsub.authorization_required' => 'yes' }

        @root.subscribe_for_testing( { 'node' => 'name', 'param' => '42' } )
        sleep 1 
        assert @authorized_called 
    end

    def test_authorization_not_requested_if_configured
        @init.setup self, nil, { 'Pubsub.authorization_required' => 'no' }

        @root.subscribe_for_testing( { 'node' => 'name', 'param' => '42' } )
        sleep 1 
        refute @authorized_called 
    end
        
end

class RackServerStartStopTests < MiniTest::Unit::TestCase
    # reuse the start- stop functionsn, but do not apply them by default with every test
    include SetupRackserver

    alias_method :setup_rack_server, :setup
    alias_method :teardown_rack_server, :teardown
    
    def setup() end
    def teardown() end
     
    Bayeux.protocol_path = '/bayeux'  
    
    # Sioux Handler - implementation
    def validate_node node
        true
    end
    
    def authorize user, node
        true
    end
    
    def node_init node
        nil
    end

    def test_fast_and_very_much_start_stops
        1000.times do
            setup_rack_server
            teardown_rack_server
        end
    end
    
    def test_a_lot_of_clients_during_shutdown
        # this relates to https://github.com/TorstenRobitzki/Sioux/issues/8
        
        1.times do
            setup_rack_server self
            subscriber_count = 100
            sync = Queue.new
            
            clients = ( 1..subscriber_count ).collect do | index |
                Thread.new( index ) do | i |
                    Bayeux::Session.start do | session |
                        begin 
                            session.subscribe_and_wait "/foo/bar/#{i}"
                            sync << i
                            session.connect
                        rescue
                        end
                    end
                end
            end
            
            subscriber_count.times { sync.pop }
            
            teardown_rack_server
            
            clients.each do | t |
                t.join
            end
        end        
    end
end
    
class BayeuxProtocolTest < MiniTest::Unit::TestCase
    # the testcases are included from the C++ Component test:
    include BayeuxNetworkTestcases 
    Bayeux.protocol_path = '/bayeux'  
    
    # Sioux Handler - implementation
    def validate_node node
        true
    end
    
    def authorize user, node
        true
    end
    
    def node_init node
        nil
    end
    
    def publish node, data, root
        reply = { 'data' => data }

        root[ node ] = reply
        [ true, '' ]
    end
    
    # Rack Handler - implementation
    def call environment
        raise StopIteration if environment[ 'PATH_INFO' ] == '/stop'
        [ 200, {"Content-Type" => "text/html"}, [ "Hello Rack!" ] ]
    end

    class Init
        include SetupRackserver
    end

    def setup
        @init = Init.new  
        @init.setup self
    end
    
    def teardown
        @init.teardown
    end

end

