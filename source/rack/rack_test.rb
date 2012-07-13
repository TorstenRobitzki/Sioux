# Copyright (c) Torrox GmbH & Co KG. All rights reserved.
# Please note that the content of this file is confidential or protected by law.
# Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

require 'rack'
require 'rack/lint'
require 'net/http'

require 'minitest/unit'
require_relative 'sioux'
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

class RackIntegrationTest < MiniTest::Unit::TestCase
    @@HOST = 'localhost'
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
    
    def setup
        @app = RackTestHandler.new
        @server_loop = Thread.new do
            begin
                Rack::Handler::Sioux.run( Rack::Lint.new( @app ), 'Environment' => 'debug', 'Host' => @@HOST, 'Port' => @@PORT )
            rescue
                @answer = @app.answer
            end
         end

        wait_until_started 1
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
    

    def test_get_method
        assert_equal "Hello Rack!", Net::HTTP.get( @@URI )

        assert_equal 'GET', @app.environment[ 'REQUEST_METHOD' ]
        assert_equal '', @app.environment[ 'SCRIPT_NAME' ]
        assert_equal '/', @app.environment[ 'PATH_INFO' ]
        assert_equal '', @app.environment[ 'QUERY_STRING' ]
        assert_equal @@HOST, @app.environment[ 'SERVER_NAME' ]
        assert_equal @@PORT.to_s, @app.environment[ 'SERVER_PORT' ]
    end
    
    def test_post_body
        Net::HTTP.post_form @@URI + '/', { 'param1' => 5, 'param2' => 'text' }

        assert_equal 'POST', @app.environment[ 'REQUEST_METHOD' ]
        assert_equal 'param1=5&param2=text', @app.environment[ 'rack.input' ].read
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
    
end

