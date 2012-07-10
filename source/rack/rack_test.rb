# Copyright (c) Torrox GmbH & Co KG. All rights reserved.
# Please note that the content of this file is confidential or protected by law.
# Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

require 'rack'
require 'rack/lint'
require 'net/http'

require 'minitest/unit'
require_relative 'sioux'
require_relative '../tests/bayeux_reply'

class BayeuxRackTest < MiniTest::Unit::TestCase
    def test_default_parameters_for_all
        skip "todo"
        Rack::Handler::Sioux.run( [] )
    end
    
    def test_release_environment
        skip "todo"
        Rack::Handler::Sioux.run( [], 'Environment' => 'release' )
    end
    
    def test_debug_environment
        skip "todo"
        Rack::Handler::Sioux.run( [], 'Environment' => 'debug' )
    end
    
    def test_unrecognized_parameters_are_detected
        assert_raises( RuntimeError ) { Rack::Handler::Sioux.run( [], 'Foo' => 'Bar' ) }
    end
end

class RackTestHandler
    attr_reader :answer
    attr_reader :environment
    
    def initialize 
    end
    
    def call environment
        @environment = environment
        raise StopIteration
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
                stop = true
                s.close
            rescue
            end
        end
        
        raise "Timeout waiting for sever!" unless stop
    end
    
    def setup
        @app = RackTestHandler.new
        @server_loop = Thread.new do
            begin
                Rack::Handler::Sioux.run( @app, 'Environment' => 'debug', 'Host' => @@HOST, 'Port' => @@PORT )
            rescue
                @answer = @app.answer
            end
         end

        wait_until_started 1
    end

    def teardown
        @server_loop.join
    end
    

    def test_get_method
        begin
            Net::HTTP.get( @@URI )
        rescue
        end

        assert_equal( @app.environment[ 'REQUEST_METHOD' ], 'GET' )
        assert_equal( @app.environment[ 'SCRIPT_NAME' ], '' )
        assert_equal( @app.environment[ 'PATH_INFO' ], '/' )
        assert_equal( @app.environment[ 'QUERY_STRING' ], '' )
        assert_equal( @app.environment[ 'SERVER_NAME' ], @@HOST )
        assert_equal( @app.environment[ 'SERVER_PORT' ], @@PORT )
    end    
end

